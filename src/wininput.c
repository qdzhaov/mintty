// wininput.c (part of mintty)
// Copyright 2008-12 Andy Koppe, 2015-2018 Thomas Wolff
// Licensed under the terms of the GNU General Public License v3 or later.

#include "winpriv.h"
#include "winsearch.h"

#include "charset.h"
#include "child.h"
#include "tek.h"

#include <math.h>
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM
#include <winnls.h>
#include <termios.h>

int tabctrling=0;
bool kb_input = false;
uint kb_trace = 0;

static HMENU ctxmenu = NULL;
static HMENU sysmenu;
static int sysmenulen;
//static uint kb_select_key = 0;
static uint super_key = 0;
static uint hyper_key = 0;
static uint newwin_key = 0;
static bool newwin_pending = false;
static bool newwin_shifted = false;
static bool newwin_home = false;
static int newwin_monix = 0, newwin_moniy = 0;
static int transparency_pending = 0;

static int fundef_stat(char*cmd);
static int fundef_run(char*cmd,uint key, mod_keys mods);

static inline void
show_last_error()
{
  int err = GetLastError();
  if (err) {
    static wchar winmsg[1024];  // constant and < 1273 or 1705 => issue #530
    FormatMessageW(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
      0, err, 0, winmsg, lengthof(winmsg), 0
    );
    printf("Error %d: %ls\n", err, winmsg);
  }
}

static inline void send_syscommand2(WPARAM cmd, LPARAM p) { SendMessage(wnd, WM_SYSCOMMAND, cmd, p); }
static inline void send_syscommand(WPARAM cmd) { SendMessage(wnd, WM_SYSCOMMAND, cmd, ' '); }

/* Icon conversion */

// https://www.nanoant.com/programming/themed-menus-icons-a-complete-vista-xp-solution
// last attempt among lots of googled solution proposals, 
// and the only one that actually works, except that it uses white background
static HBITMAP
icon_bitmap(HICON hIcon)
{
  RECT rect;
  rect.left = rect.top = 0;
  // retrieve needed size of menu icons; but what about per-monitor DPI?
  rect.right = GetSystemMetrics(SM_CXMENUCHECK);
  rect.bottom = GetSystemMetrics(SM_CYMENUCHECK);

  //HWND desktop = GetDesktopWindow();
  //HWND desktop = 0;
  HWND desktop = wnd;

  HDC screen_dev = GetDC(desktop);
  if (screen_dev == NULL)
    return NULL;

  // Create a compatible DC
  HDC dst_hdc = CreateCompatibleDC(screen_dev);
  if (dst_hdc == NULL) {
    ReleaseDC(desktop, screen_dev);
    return NULL;
  }

  // Create a new bitmap of icon size
  HBITMAP bmp = CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
  if (bmp == NULL) {
    DeleteDC(dst_hdc);
    ReleaseDC(desktop, screen_dev);
    return NULL;
  }

  // Select it into the compatible DC
  HBITMAP old_dst_bmp = (HBITMAP)SelectObject(dst_hdc, bmp);
  if (old_dst_bmp == NULL)
    return NULL;

  // Fill the background of the compatible DC with the given colour
  SetBkColor(dst_hdc, win_get_sys_colour(COLOR_MENU));
  ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

  // Draw the icon into the compatible DC
  DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);

  // Restore settings
  SelectObject(dst_hdc, old_dst_bmp);
  DeleteDC(dst_hdc);
  ReleaseDC(desktop, screen_dev);
  return bmp;
}


/* Menu handling */

static inline void
show_menu_info(HMENU menu)
{
  MENUINFO mi;
  mi.cbSize = sizeof(MENUINFO);
  mi.fMask = MIM_STYLE | MIM_BACKGROUND;
  GetMenuInfo(menu, &mi);
  printf("menuinfo style %04X brush %p\n", (uint)mi.dwStyle, mi.hbrBack);
}

static void
append_commands(HMENU menu, wstring commands, UINT_PTR idm_cmd, bool add_icons, bool sysmenu)
{
  char * cmds = cs__wcstoutf(commands);
  char * cmdp = cmds;
  int n = 0;
  char sepch = ';';
  if ((uchar)*cmdp <= (uchar)' ')
    sepch = *cmdp++;

  char * paramp;
  while ((paramp = strchr(cmdp, ':'))) {
    *paramp++ = '\0';
    if (sysmenu)
      InsertMenuW(menu, SC_CLOSE, MF_ENABLED, idm_cmd + n, _W(cmdp));
    else
      AppendMenuW(menu, MF_ENABLED, idm_cmd + n, _W(cmdp));
    cmdp = strchr(paramp, sepch);
    if (cmdp)
      *cmdp++ = '\0';

    if (add_icons) {
      MENUITEMINFOW mi;
      mi.cbSize = sizeof(MENUITEMINFOW);
      mi.fMask = MIIM_BITMAP;
      wchar * params = cs__utftowcs(paramp);
      wstring iconfile = wslicon(params);  // default: 0 (no icon)
      free(params);
      HICON icon;
      if (iconfile)
        icon = (HICON) LoadImageW(0, iconfile,
                                  IMAGE_ICON, 0, 0,
                                  LR_DEFAULTSIZE | LR_LOADFROMFILE
                                  | LR_LOADTRANSPARENT);
      else
        icon = LoadIcon(inst, MAKEINTRESOURCE(IDI_MAINICON));
      HBITMAP bitmap = icon_bitmap(icon);
      mi.hbmpItem = bitmap;
      SetMenuItemInfoW(menu, idm_cmd + n, 0, &mi);
      if (icon)
        DestroyIcon(icon);
    }

    n++;
    if (!cmdp)
      break;
    // check for multi-line separation
    if (*cmdp == '\\' && cmdp[1] == '\n') {
      cmdp += 2;
      while (iswspace(*cmdp))
        cmdp++;
    }
  }
  free(cmds);
}

struct data_add_switcher {
  int tabi;
  bool use_win_icons;
  HMENU menu;
};

static void
add_switcher(HMENU menu, bool vsep, bool hsep, bool use_win_icons)
{
  (void)use_win_icons;
  //printf("add_switcher vsep %d hsep %d\n", vsep, hsep);
  uint bar = vsep ? MF_MENUBARBREAK : 0;
  if (hsep)
    AppendMenuW(menu, MF_SEPARATOR, 0, 0);
  //__ Context menu, session switcher ("virtual tabs") menu label
  AppendMenuW(menu, MF_DISABLED | bar, 0, _W("Session switcher"));
  AppendMenuW(menu, MF_SEPARATOR, 0, 0);
}

static bool
add_launcher(HMENU menu, bool vsep, bool hsep)
{
  //printf("add_launcher vsep %d hsep %d\n", vsep, hsep);
  if (*cfg.session_commands) {
    uint bar = vsep ? MF_MENUBARBREAK : 0;
    if (hsep)
      AppendMenuW(menu, MF_SEPARATOR, 0, 0);
    AppendMenuW(menu, MF_DISABLED | bar, 0, _W("Session launcher"));
    AppendMenuW(menu, MF_SEPARATOR, 0, 0);
    append_commands(menu, cfg.session_commands, IDM_SESSIONCOMMAND, true, false);
    return true;
  }
  else
    return false;
}

#define dont_debug_modify_menu

void
win_update_menus(bool callback)
{
  if (callback) {
    // invoked after WM_INITMENU
  }
  else
    return;

  bool shorts = !cterm->shortcut_override;
  bool clip = shorts && cfg.clip_shortcuts;
  bool alt_fn = shorts && cfg.alt_fn_shortcuts;
  bool ct_sh = shorts && cfg.ctrl_shift_shortcuts;

#ifdef debug_modify_menu
  printf("win_update_menus\n");
#endif

  void
  modify_menu(HMENU menu, UINT item, UINT state, wchar * label, wchar * key)
  // if item is sysentry: ignore state
  // state: MF_ENABLED, MF_GRAYED, MF_CHECKED, MF_UNCHECKED
  // label: if null, use current label
  // key: shortcut description; localize "Ctrl+Alt+Shift+"
  {
    bool sysentry = item >= 0xF000;
#ifdef debug_modify_menu
    if (sysentry)
      printf("mm %04X <%ls> <%ls>\n", item, label, key);
#endif

    MENUITEMINFOW mi;
    mi.cbSize = sizeof(MENUITEMINFOW);
#define dont_debug_menuitem
#ifdef debug_menuitem
    mi.fMask = MIIM_BITMAP | MIIM_STATE | MIIM_STRING | MIIM_DATA;
    mi.dwTypeData = NULL;
    GetMenuItemInfoW(menu, item, 0, &mi);
    mi.cch++;
    mi.dwTypeData = newn(wchar, mi.cch);
    int ok = GetMenuItemInfoW(menu, item, 0, &mi);
    printf("%d %X %d<%ls> <%ls>\n", ok, mi.fState, mi.cch, mi.dwTypeData, (wstring)mi.dwItemData);
    mi.fState &= ~MFS_DEFAULT;  // does not work if used 
                                // in SetMenuItemInfoW with MIIM_STATE
#endif
    mi.fMask = MIIM_STRING;
    if (!label || sysentry) {
      mi.dwTypeData = NULL;
      GetMenuItemInfoW(menu, item, 0, &mi);
      mi.cch++;
      mi.dwTypeData = newn(wchar, mi.cch);
      if (sysentry)
        mi.fMask |= MIIM_DATA;
      GetMenuItemInfoW(menu, item, 0, &mi);
    }

    // prepare info to write
    mi.fMask = MIIM_STRING;
    if (sysentry) {
      if (label) {
        // backup system (localized) label to application data
        if (!mi.dwItemData) {
          mi.dwItemData = (ULONG_PTR)wcsdup(mi.dwTypeData);
          mi.fMask |= MIIM_DATA;  // make sure it's stored
        }
      }
      else if (mi.dwItemData) {
        // restore system (localized) label from backup
        mi.dwTypeData = wcsdup((wstring)mi.dwItemData);
      }
    }
    //don't mi.fMask |= MIIM_ID; mi.wID = ...; would override item ID
    if (label)
      mi.dwTypeData = wcsdup(label);
    if (!sysentry) {
      mi.fMask |= MIIM_STATE | MIIM_FTYPE;
      mi.fState = state;
      mi.fType = MFT_STRING;
    }
    wchar * tab = wcschr(mi.dwTypeData, '\t');
    if (tab)
      *tab = '\0';
    if (key) {
      // append TAB and shortcut to label; localize "Ctrl+Alt+Shift+"
      mod_keys mod = 0;
      if (0 == wcsncmp(key, W("Ctrl+"), 5)) {
        mod |= MDK_CTRL;
        key += 5;
      }
      if (0 == wcsncmp(key, W("Alt+"), 4)) {
        mod |= MDK_ALT;
        key += 4;
      }
      if (0 == wcsncmp(key, W("Shift+"), 6)) {
        mod |= MDK_SHIFT;
        key += 6;
      }
      int len1 = wcslen(mi.dwTypeData) + 1
                 + (mod & MDK_CTRL ? wcslen(_W("Ctrl+")) : 0)
                 + (mod & MDK_ALT ? wcslen(_W("Alt+")) : 0)
                 + (mod & MDK_SHIFT ? wcslen(_W("Shift+")) : 0)
                 + wcslen(key) + 1;
      mi.dwTypeData = renewn(mi.dwTypeData, len1);
      wcscat(mi.dwTypeData, W("\t"));
      if (mod & MDK_CTRL) wcscat(mi.dwTypeData, _W("Ctrl+"));
      if (mod & MDK_ALT) wcscat(mi.dwTypeData, _W("Alt+"));
      if (mod & MDK_SHIFT) wcscat(mi.dwTypeData, _W("Shift+"));
      wcscat(mi.dwTypeData, key);
    }
#ifdef debug_modify_menu
    if (sysentry)
      printf("-> %04X [%04X] %04X <%ls>\n", item, mi.fMask, mi.fState, mi.dwTypeData);
#endif

    SetMenuItemInfoW(menu, item, 0, &mi);

    free(mi.dwTypeData);
  }

  wchar *
  itemlabel(char * label)
  {
    char * loc = _(label);
    if (loc == label)
      // no localization entry
      return null;  // indicate to use system localization
    else
      return _W(label);  // use our localization
  }
  if(1){
    DeleteMenu(sysmenu,SC_RESTORE ,0);
    DeleteMenu(sysmenu,SC_MOVE    ,0);
    DeleteMenu(sysmenu,SC_SIZE    ,0);
    DeleteMenu(sysmenu,SC_MINIMIZE,0);
    DeleteMenu(sysmenu,SC_MAXIMIZE,0);
  }else{
    modify_menu(sysmenu, SC_RESTORE , 0, itemlabel(__("&Restore")), null);
    modify_menu(sysmenu, SC_MOVE    , 0, itemlabel(__("&Move")), null);
    modify_menu(sysmenu, SC_SIZE    , 0, itemlabel(__("&Size")), null);
    modify_menu(sysmenu, SC_MINIMIZE, 0, itemlabel(__("Mi&nimize")), null);
    modify_menu(sysmenu, SC_MAXIMIZE, 0, itemlabel(__("Ma&ximize")), null);
  }
  modify_menu(sysmenu, SC_CLOSE, 0, itemlabel(__("&Close")),
    alt_fn ? W("Alt+F4") : ct_sh ? W("Ctrl+Shift+W") : null
  );
  uint switch_move_enabled = win_tab_count() == 1;
  EnableMenuItem(sysmenu, IDM_PREVTAB, switch_move_enabled);
  EnableMenuItem(sysmenu, IDM_NEXTTAB, switch_move_enabled);
  EnableMenuItem(sysmenu, IDM_MOVELEFT, switch_move_enabled);
  EnableMenuItem(sysmenu, IDM_MOVERIGHT, switch_move_enabled);

  //__ System menu:
  modify_menu(sysmenu, IDM_NEW, 0, _W("Ne&w"),
    alt_fn ? W("Alt+F2") : ct_sh ? W("Ctrl+Shift+N") : null
  );

  uint sel_enabled = cterm->selected ? MF_ENABLED : MF_GRAYED;
  EnableMenuItem(ctxmenu, IDM_OPEN, sel_enabled);
  //__ Context menu:
  modify_menu(ctxmenu, IDM_COPY, sel_enabled, _W("&Copy"),
    clip ? W("Ctrl+Ins") : ct_sh ? W("Ctrl+Shift+C") : null
  );
  // enable/disable predefined extended context menu entries
  // (user-definable ones are handled via fct_status())
  EnableMenuItem(ctxmenu, IDM_COPY_TEXT, sel_enabled);
  EnableMenuItem(ctxmenu, IDM_COPY_TABS, sel_enabled);
  EnableMenuItem(ctxmenu, IDM_COPY_TXT, sel_enabled);
  EnableMenuItem(ctxmenu, IDM_COPY_RTF, sel_enabled);
  EnableMenuItem(ctxmenu, IDM_COPY_HTXT, sel_enabled);
  EnableMenuItem(ctxmenu, IDM_COPY_HFMT, sel_enabled);
  EnableMenuItem(ctxmenu, IDM_COPY_HTML, sel_enabled);

  uint paste_enabled =
    IsClipboardFormatAvailable(CF_TEXT) ||
    IsClipboardFormatAvailable(CF_UNICODETEXT) ||
    IsClipboardFormatAvailable(CF_HDROP)
    ? MF_ENABLED : MF_GRAYED;
  //__ Context menu:
  modify_menu(ctxmenu, IDM_PASTE, paste_enabled, _W("&Paste "),
    clip ? W("Shift+Ins") : ct_sh ? W("Ctrl+Shift+V") : null
  );

  //__ Context menu:
  modify_menu(ctxmenu, IDM_COPASTE, sel_enabled, _W("Copy → Paste"),
    clip ? W("Ctrl+Shift+Ins") : null
  );

  //__ Context menu:
  modify_menu(ctxmenu, IDM_SEARCH, 0, _W("S&earch"),
    alt_fn ? W("Alt+F3") : ct_sh ? W("Ctrl+Shift+H") : null
  );

  uint logging_enabled = (logging || *cfg.log) ? MF_ENABLED : MF_GRAYED;
  uint logging_checked = logging ? MF_CHECKED : MF_UNCHECKED;
  //__ Context menu:
  modify_menu(ctxmenu, IDM_TOGLOG, logging_enabled | logging_checked, _W("&Log to File"),
    null
  );

  uint charinfo = show_charinfo ? MF_CHECKED : MF_UNCHECKED;
  //__ Context menu:
  modify_menu(ctxmenu, IDM_TOGCHARINFO, charinfo, _W("Character &Info"),
    null
  );

  uint vt220kb = cterm->vt220_keys ? MF_CHECKED : MF_UNCHECKED;
  //__ Context menu:
  modify_menu(ctxmenu, IDM_TOGVT220KB, vt220kb, _W("VT220 Keyboard"),
    null
  );

  //__ Context menu:
  modify_menu(ctxmenu, IDM_RESET, 0, _W("&Reset"),
    alt_fn ? W("Alt+F8") : ct_sh ? W("Ctrl+Shift+R") : null
  );

  uint defsize_enabled =
    IsZoomed(wnd) || cterm->cols != cfg.cols || cterm->rows != cfg.rows
    ? MF_ENABLED : MF_GRAYED;
  //__ Context menu:
  modify_menu(ctxmenu, IDM_DEFSIZE_ZOOM, defsize_enabled, _W("&Default Size"),
    alt_fn ? W("Alt+F10") : ct_sh ? W("Ctrl+Shift+D") : null
  );

#define CKED(v) ((v)?MF_CHECKED : MF_UNCHECKED)  
  uint scrollbar_checked = CKED(cterm->show_scrollbar );
#ifdef allow_disabling_scrollbar
  if (!cfg.scrollbar)
    scrollbar_checked |= MF_GRAYED;
#endif
  //__ Context menu:
  modify_menu(ctxmenu, IDM_TABBAR   , CKED(cfg.tab_bar_show)  , _W("Tabbar(&H)"),    null);
  //__ Context menu:
  modify_menu(ctxmenu, IDM_SCROLLBAR, scrollbar_checked       , _W("Scrollbar(&O)"), null);
  //__ Context menu:
  modify_menu(ctxmenu, IDM_PARTLINE , CKED(cterm->usepartline), _W("PartLine(&K)"),  null);
  //__ Context menu:
  modify_menu(ctxmenu, IDM_INDICATOR, CKED(cfg.indicator)     , _W("Indicator(&I)"), null);

  uint fullscreen_checked = win_is_fullscreen ? MF_CHECKED : MF_UNCHECKED;
  //__ Context menu:
  modify_menu(ctxmenu, IDM_FULLSCREEN_ZOOM, fullscreen_checked, _W("&Full Screen"),
    alt_fn ? W("Alt+F11") : ct_sh ? W("Ctrl+Shift+F") : null
  );

  uint otherscreen_checked = cterm->show_other_screen ? MF_CHECKED : MF_UNCHECKED;
  //__ Context menu:
  modify_menu(ctxmenu, IDM_FLIPSCREEN, otherscreen_checked, _W("Flip &Screen"),
    alt_fn ? W("Alt+F12") : ct_sh ? W("Ctrl+Shift+S") : null
  );

  uint options_enabled = config_wnd ? MF_GRAYED : MF_ENABLED;
  EnableMenuItem(ctxmenu, IDM_OPTIONS, options_enabled);
  EnableMenuItem(sysmenu, IDM_OPTIONS, options_enabled);

  // refresh remaining labels to facilitate (changed) localization
  //__ Context menu:
  modify_menu(sysmenu, IDM_COPYTITLE, 0, _W("Copy T&itle"), null);
  //__ Context menu:
  modify_menu(sysmenu, IDM_OPTIONS, 0, _W("&Options..."), null);

  // update user-defined menu functions (checked/enabled)
  void
  check_commands(HMENU menu, wstring commands, UINT_PTR idm_cmd)
  {
    char * cmds = cs__wcstoutf(commands);
    char * cmdp = cmds;
    int n = 0;
    char sepch = ';';
    if ((uchar)*cmdp <= (uchar)' ')
      sepch = *cmdp++;

    char * paramp;
    while ((paramp = strchr(cmdp, ':'))) {
      *paramp++ = '\0';
      char * newcmdp = strchr(paramp, sepch);
      if (newcmdp)
        *newcmdp++ = '\0';

      // localize
      wchar * label = _W(cmdp);
      uint status = fundef_stat(paramp);
      modify_menu(menu, idm_cmd + n, status, label, null);

      cmdp = newcmdp;
      n++;
      if (!cmdp)
        break;
      // check for multi-line separation
      if (*cmdp == '\\' && cmdp[1] == '\n') {
        cmdp += 2;
        while (iswspace(*cmdp))
          cmdp++;
      }
    }
    free(cmds);
  }
  if (*cfg.ctx_user_commands)
    check_commands(ctxmenu, cfg.ctx_user_commands, IDM_CTXMENUFUNCTION);
  if (*cfg.sys_user_commands)
    check_commands(sysmenu, cfg.sys_user_commands, IDM_SYSMENUFUNCTION);

#ifdef vary_sysmenu
  static bool switcher_in_sysmenu = false;
  if (!switcher_in_sysmenu) {
    add_switcher(sysmenu, true, false, true);
    switcher_in_sysmenu = true;
  }
#endif
  (void)sysmenulen;
}

static bool
add_user_commands(HMENU menu, bool vsep, bool hsep, wstring title, wstring commands, UINT_PTR idm_cmd)
{
  //printf("add_user_commands vsep %d hsep %d\n", vsep, hsep);
  if (*commands) {
    uint bar = vsep ? MF_MENUBARBREAK : 0;
    if (hsep)
      AppendMenuW(menu, MF_SEPARATOR, 0, 0);
    if (title) {
      AppendMenuW(menu, MF_DISABLED | bar, 0, title);
      AppendMenuW(menu, MF_SEPARATOR, 0, 0);
    }

    append_commands(menu, commands, idm_cmd, false, false);
    return true;
  }
  else
    return false;
}

static void
win_init_ctxmenu(bool extended_menu, bool with_user_commands)
{
#ifdef debug_modify_menu
  printf("win_init_ctxmenu\n");
#endif
  //__ Context menu:
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_OPEN, _W("Ope&n"));
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_NEWTAB, _W("New tab\tCtrl+Shift+T"));
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_KILLTAB, _W("Kill tab"));
  AppendMenuW(ctxmenu, MF_SEPARATOR, 0, 0);
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_PREVTAB, _W("Previous tab\tShift+<-"));
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_NEXTTAB, _W("Next tab\tShift+->"));
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_MOVELEFT, _W("Move to left\tCtrl+Shift+<-"));
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_MOVERIGHT, _W("Next to right\tCtrl+Shift+->"));
  AppendMenuW(ctxmenu, MF_SEPARATOR, 0, 0);
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_COPY, 0);
  if (extended_menu) {
    //__ Context menu:
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_COPY_TEXT, _W("Copy as text"));
    //__ Context menu:
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_COPY_TABS, _W("Copy with TABs"));
    //__ Context menu:
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_COPY_RTF, _W("Copy as RTF"));
    //__ Context menu:
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_COPY_HTXT, _W("Copy as HTML text"));
    //__ Context menu:
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_COPY_HFMT, _W("Copy as HTML"));
    //__ Context menu:
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_COPY_HTML, _W("Copy as HTML full"));
  }
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_PASTE, 0);
  if (extended_menu) {
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_COPASTE, 0);
  }
  //__ Context menu:
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_SELALL, _W("Select &All"));
  //__ Context menu:
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_SAVEIMG, _W("Save as &Image"));
  if (tek_mode) {
    AppendMenuW(ctxmenu, MF_SEPARATOR, 0, 0);
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_TEKRESET, W("Tektronix RESET"));
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_TEKPAGE, W("Tektronix PAGE"));
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_TEKCOPY, W("Tektronix COPY"));

  }
  AppendMenuW(ctxmenu, MF_SEPARATOR, 0, 0);
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_SEARCH, 0);
  if (extended_menu) {
    //__ Context menu: write terminal window contents as HTML file
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_HTML, _W("HTML Screen Dump"));
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_TOGLOG, 0);
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_TOGCHARINFO, 0);
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_TOGVT220KB, 0);
  }
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_RESET, 0);
  if (extended_menu) {
    //__ Context menu: clear scrollback buffer (lines scrolled off the window)
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_CLRSCRLBCK, _W("Clear Scrollback"));
  }
  AppendMenuW(ctxmenu, MF_SEPARATOR, 0, 0);
  AppendMenuW(ctxmenu, MF_ENABLED | MF_UNCHECKED, IDM_DEFSIZE_ZOOM, 0);
  AppendMenuW(ctxmenu, MF_ENABLED | MF_UNCHECKED, IDM_SCROLLBAR, 0);
  AppendMenuW(ctxmenu, MF_ENABLED | MF_CHECKED  , IDM_TABBAR   , 0);
  AppendMenuW(ctxmenu, MF_ENABLED | MF_UNCHECKED, IDM_PARTLINE , 0);
  AppendMenuW(ctxmenu, MF_ENABLED | MF_UNCHECKED, IDM_INDICATOR, 0);
  AppendMenuW(ctxmenu, MF_ENABLED | MF_UNCHECKED, IDM_FULLSCREEN_ZOOM, 0);
  AppendMenuW(ctxmenu, MF_ENABLED | MF_UNCHECKED, IDM_FLIPSCREEN, 0);
  AppendMenuW(ctxmenu, MF_SEPARATOR, 0, 0);
  if (extended_menu) {
    //__ Context menu: generate a TTY BRK condition (tty line interrupt)
    AppendMenuW(ctxmenu, MF_ENABLED, IDM_BREAK, _W("Send Break"));
    AppendMenuW(ctxmenu, MF_SEPARATOR, 0, 0);
  }

  if (with_user_commands && *cfg.ctx_user_commands) {
    append_commands(ctxmenu, cfg.ctx_user_commands, IDM_CTXMENUFUNCTION, false, false);
    AppendMenuW(ctxmenu, MF_SEPARATOR, 0, 0);
  }
  else if (with_user_commands && *cfg.user_commands) {
    append_commands(ctxmenu, cfg.user_commands, IDM_USERCOMMAND, false, false);
    AppendMenuW(ctxmenu, MF_SEPARATOR, 0, 0);
  }

  //__ Context menu:
  AppendMenuW(ctxmenu, MF_ENABLED, IDM_OPTIONS, _W("&Options..."));
}

void
win_init_menus(void)
{
#ifdef debug_modify_menu
  printf("win_init_menus\n");
#endif

  HMENU smenu;
  sysmenu = GetSystemMenu(wnd, false);

  if (*cfg.sys_user_commands)
    append_commands(sysmenu, cfg.sys_user_commands, IDM_SYSMENUFUNCTION, false, true);
  else {
    InsertMenuW(sysmenu, SC_CLOSE, MF_ENABLED, IDM_COPYTITLE, _W("&Copy Title"));
    InsertMenuW(sysmenu, SC_CLOSE, MF_ENABLED, IDM_OPTIONS, _W("&Options..."));
  }
  InsertMenuW(sysmenu, SC_CLOSE, MF_SEPARATOR, 0, 0);
  smenu = CreatePopupMenu();
  AppendMenuW(smenu,  MF_ENABLED, IDM_NEWWSLT, _W("WSL"         ));
  AppendMenuW(smenu,  MF_ENABLED, IDM_NEWCYGT, _W("Cygwin"      ));
  AppendMenuW(smenu,  MF_ENABLED, IDM_NEWCMDT, _W("CMD"         ));
  AppendMenuW(smenu,  MF_ENABLED, IDM_NEWPSHT, _W("PowerShell"  ));
  AppendMenuW(smenu,  MF_ENABLED, IDM_NEWUSRT, _W("faststart"   ));
  InsertMenuW(sysmenu,0,MF_BYPOSITION|MF_POPUP,   (UINT_PTR)(smenu), L"New &Tab");
  smenu = CreatePopupMenu();
  AppendMenuW(smenu,  MF_ENABLED, IDM_NEWWSLW, _W("WSL"         ));
  AppendMenuW(smenu,  MF_ENABLED, IDM_NEWCYGW, _W("Cygwin"      ));
  AppendMenuW(smenu,  MF_ENABLED, IDM_NEWCMDW, _W("CMD"         ));
  AppendMenuW(smenu,  MF_ENABLED, IDM_NEWPSHW, _W("PowerShell"  ));
  AppendMenuW(smenu,  MF_ENABLED, IDM_NEWUSRW, _W("faststart"   ));
  InsertMenuW(sysmenu,1,MF_BYPOSITION|MF_POPUP,   (UINT_PTR)(smenu), L"New &Win");
  InsertMenuW(sysmenu,2,MF_BYPOSITION|MF_ENABLED, IDM_NEW, 0);
  InsertMenuW(sysmenu, 3,MF_BYPOSITION|MF_ENABLED, IDM_NEWTAB, _W("New tab\tCtrl+Shift+T"));
  InsertMenuW(sysmenu, 4,MF_BYPOSITION|MF_ENABLED, IDM_KILLTAB, _W("Kill tab"));
  InsertMenuW(sysmenu, 5,MF_BYPOSITION|MF_SEPARATOR, 0, 0);
  InsertMenuW(sysmenu, 6,MF_BYPOSITION|MF_ENABLED, IDM_PREVTAB, _W("Previous tab\tWin+<-"));
  InsertMenuW(sysmenu, 7,MF_BYPOSITION|MF_ENABLED, IDM_NEXTTAB, _W("Next tab\tWin+->"));
  InsertMenuW(sysmenu, 8,MF_BYPOSITION|MF_ENABLED, IDM_MOVELEFT, _W("Move to left\tWin+Shift+<-"));
  InsertMenuW(sysmenu, 9,MF_BYPOSITION|MF_ENABLED, IDM_MOVERIGHT, _W("Next to right\tWin+Shift+->"));
  sysmenulen = GetMenuItemCount(sysmenu);
}

static void
open_popup_menu(bool use_text_cursor, string menucfg, mod_keys mods)
{
  //printf("open_popup_menu txtcur %d <%s> %X\n", use_text_cursor, menucfg, mods);
  /* Create a new context menu structure every time the menu is opened.
     This was a fruitless attempt to achieve its proper DPI scaling.
     It also supports opening different menus (Ctrl+ for extended menu).
     if (mods & MDK_CTRL) open extended menu...
   */
  if (ctxmenu)
    DestroyMenu(ctxmenu);

  ctxmenu = CreatePopupMenu();
  //show_menu_info(ctxmenu);

  if (!menucfg) {
    if (mods & MDK_ALT)
      menucfg = *cfg.menu_altmouse ? cfg.menu_altmouse : "ls";
    else if (mods & MDK_CTRL)
      menucfg = *cfg.menu_ctrlmouse ? cfg.menu_ctrlmouse : "e|ls";
    else
      menucfg = *cfg.menu_mouse ? cfg.menu_mouse : "b";
  }

  bool vsep = false;
  bool hsep = false;
  bool init = false;
  bool wicons = strchr(menucfg, 'W');
  while (*menucfg) {
    if (*menucfg == '|')
      // Windows mangles the menu style if the flag MF_MENUBARBREAK is used 
      // as triggered by vsep...
      vsep = true;
    else if (!strchr(menucfg + 1, *menucfg)) {
      // suppress duplicates except separators
      bool ok = true;
      switch (*menucfg) {
        when 'b': if (!init) {
                    win_init_ctxmenu(false, false);
                    init = true;
                  }
        when 'x': if (!init) {
                    win_init_ctxmenu(true, false);
                    init = true;
                  }
        when 'e': if (!init) {
                    win_init_ctxmenu(true, true);
                    init = true;
                  }
        when 'u': ok = add_user_commands(ctxmenu, vsep, hsep & !vsep,
                                         null,
                                         cfg.ctx_user_commands, IDM_CTXMENUFUNCTION
                                         )
                       ||
                       add_user_commands(ctxmenu, vsep, hsep & !vsep,
                                         //__ Context menu, user commands
                                         _W("User commands"),
                                         cfg.user_commands, IDM_USERCOMMAND
                                         );
        when 'W': wicons = true;
        when 's': add_switcher(ctxmenu, vsep, hsep & !vsep, wicons);
        when 'l': ok = add_launcher(ctxmenu, vsep, hsep & !vsep);
        when 'T': use_text_cursor = true;
        when 'P': use_text_cursor = false;
      }
      if (ok) {
        vsep = false;
        hsep = true;
      }
    }
    menucfg++;
  }
  win_update_menus(false);  // dispensable; also called via WM_INITMENU
  //show_menu_info(ctxmenu);

  POINT p;
  if (use_text_cursor) {
    GetCaretPos(&p);
    ClientToScreen(wnd, &p);
  }
  else
    GetCursorPos(&p);

  TrackPopupMenu
  (
    ctxmenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
    p.x, p.y, 0, wnd, null
  );
}
void 
win_popup_menu(uint key,mod_keys mods)
{
	(void)key;
  open_popup_menu(false, null, mods);
}

static int win_key_menu(uint key,mod_keys mods)
{
	(void)key;
  if (*cfg.key_menu)return 0; 
  if (mods&MDK_SHIFT)
    send_syscommand(SC_KEYMENU);
  else {
    win_show_mouse();
    open_popup_menu(false, null, mods);
  }
  return true;
}

bool
win_title_menu(bool leftbut)
{
  string title_menu = leftbut ? cfg.menu_title_ctrl_l : cfg.menu_title_ctrl_r;
  if (*title_menu) {
    open_popup_menu(false, title_menu, 0);
    return true;
  }
  else
    return false;
}


/* Mouse and Keyboard modifiers */

typedef enum {
  ALT_CANCELLED = -1, ALT_NONE = 0, ALT_ALONE = 1,
  ALT_OCT = 8, ALT_DEC = 10, ALT_HEX = 16
} alt_state_t;
static alt_state_t alt_state;
static uint alt_code;

static bool lctrl;  // Is left Ctrl pressed?
static int lctrl_time;

mod_keys
get_mods(void)
{
  inline bool is_key_down(uchar vk) { return GetKeyState(vk) & 0x80; }
  lctrl_time = 0;
  lctrl = is_key_down(VK_LCONTROL) && (lctrl || !is_key_down(VK_RMENU));
  return
    is_key_down(VK_SHIFT) * MDK_SHIFT
    | is_key_down(VK_MENU) * MDK_ALT
    | (lctrl | is_key_down(VK_RCONTROL)) * MDK_CTRL
    | (is_key_down(VK_LWIN) | is_key_down(VK_RWIN)) * MDK_WIN
    ;
}


/* Mouse handling */

static void
update_mouse(mod_keys mods)
{
  static bool app_mouse;
  bool new_app_mouse =
    cterm->mouse_mode && !cterm->show_other_screen &&
    cfg.clicks_target_app ^ ((mods & cfg.click_target_mod) != 0);
  if (new_app_mouse != app_mouse) {
    HCURSOR cursor = LoadCursor(null, new_app_mouse ? IDC_ARROW : IDC_IBEAM);
    SetClassLongPtr(wnd, GCLP_HCURSOR, (LONG_PTR)cursor);
    SetCursor(cursor);
    app_mouse = new_app_mouse;
  }
}

void
win_update_mouse(void)
{ update_mouse(get_mods()); }

void
win_capture_mouse(void)
{ SetCapture(wnd); }

static bool mouse_showing = true;

void
win_show_mouse(void)
{
  if (!mouse_showing) {
    ShowCursor(true);
    mouse_showing = true;
  }
}

static void
hide_mouse(void)
{
  POINT p;
  if (cterm->hide_mouse && mouse_showing && GetCursorPos(&p) && WindowFromPoint(p) == wnd) {
    ShowCursor(false);
    mouse_showing = false;
  }
}

static pos
translate_pos(int x, int y)
{
  return (pos){
    .x = floorf((x - PADDING) / (float)cell_width),
    .y = floorf((y - PADDING - OFFSET) / (float)cell_height),
    .pix = min(max(0, x - PADDING), cterm->rows * cell_height - 1),
    .piy = min(max(0, y - PADDING - OFFSET), cterm->cols * cell_width - 1),
    .r = (cfg.elastic_mouse && !cterm->mouse_mode)
         ? (x - PADDING) % cell_width > cell_width / 2
         : 0
  };
}

pos last_pos = {-1, -1, -1, -1, false};
static LPARAM last_lp = -1;
static int button_state = 0;

bool click_focus_token = false;
static mouse_button last_button = -1;
static mod_keys last_mods;
static pos last_click_pos;
static bool last_skipped = false;
static mouse_button skip_release_token = -1;
static uint last_skipped_time;
static bool mouse_state = false;

static pos
get_mouse_pos(LPARAM lp)
{
  last_lp = lp;
  return translate_pos(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
}

static bool tab_bar_click(LPARAM lp) {
  int y = GET_Y_LPARAM(lp);
  if (y >= PADDING && y < PADDING + OFFSET) {
    win_tab_mouse_click(GET_X_LPARAM(lp));
    return true;
  }
  return false;
}
bool
win_mouse_click(mouse_button b, LPARAM lp)
{
  mouse_state = true;
  bool click_focus = click_focus_token;
  click_focus_token = false;

  static uint last_time, count;

  win_show_mouse();
  if (tab_bar_click(lp)) return 1;
  mod_keys mods = get_mods();
  pos p = get_mouse_pos(lp);

  uint t = GetMessageTime();
  bool dblclick = b == last_button
                  && p.x == last_click_pos.x && p.y == last_click_pos.y
                  && t - last_time <= GetDoubleClickTime();
  if (!dblclick || ++count > 3)
    count = 1;
  //printf("mouse %d (focus %d skipped %d) ×%d\n", b, click_focus, last_skipped, count);

  SetFocus(wnd);  // in case focus was in search bar

  bool res = false;

  if (click_focus && b == MBT_LEFT && count == 1
      && // not in application mouse mode
         !(cterm->mouse_mode && cterm->report_focus &&
           cfg.clicks_target_app ^ ((mods & cfg.click_target_mod) != 0)
          )
     ) {
    //printf("suppressing focus-click selection, t %d\n", t);
    // prevent accidental selection when focus-clicking into the window (#717)
    last_skipped = true;
    last_skipped_time = t;
    skip_release_token = b;
    res = true;
  }
  else {
    if (last_skipped && dblclick) {
      // recognize double click also in application mouse modes
      term_mouse_click(b, mods, p, 1);
    }
    res = term_mouse_click(b, mods, p, count);
    last_skipped = false;
  }
  last_pos = (pos){INT_MIN, INT_MIN, INT_MIN, INT_MIN, false};
  last_click_pos = p;
  last_time = t;
  last_button = b;
  last_mods = mods;

  if (alt_state > ALT_NONE)
    alt_state = ALT_CANCELLED;
  switch (b) {
    when MBT_RIGHT:
      button_state |= 1;
    when MBT_MIDDLE:
      button_state |= 2;
    when MBT_LEFT:
      button_state |= 4;
    when MBT_4:
      button_state |= 8;
    otherwise:;
  }

  return res;
}

void
win_mouse_release(mouse_button b, LPARAM lp)
{
  mouse_state = false;

  if (b == skip_release_token) {
    skip_release_token = -1;
    return;
  }

  term_mouse_release(b, get_mods(), get_mouse_pos(lp));
  ReleaseCapture();
  switch (b) {
    when MBT_RIGHT:
      button_state &= ~1;
    when MBT_MIDDLE:
      button_state &= ~2;
    when MBT_LEFT:
      button_state &= ~4;
    when MBT_4:
      button_state &= ~8;
    otherwise:;
  }
}

void
win_mouse_move(bool nc, LPARAM lp)
{
  if (tek_mode == TEKMODE_GIN) {
    int y = GET_Y_LPARAM(lp) - PADDING - OFFSET;
    int x = GET_X_LPARAM(lp) - PADDING;
    tek_move_to(y, x);
    return;
  }

  if (lp == last_lp)
    return;

  win_show_mouse();

  pos p = get_mouse_pos(lp);
  if (nc || (p.x == last_pos.x && p.y == last_pos.y && p.r == last_pos.r))
    return;
  if (last_skipped && last_button == MBT_LEFT && mouse_state) {
    // allow focus-selection if distance spanned 
    // is large enough or with sufficient delay (#717)
    uint dist = sqrt(sqr(p.x - last_click_pos.x) + sqr(p.y - last_click_pos.y));
    uint diff = GetMessageTime() - last_skipped_time;
    //printf("focus move %d %d\n", dist, diff);
    if (dist * diff > 999) {
      term_mouse_click(last_button, last_mods, last_click_pos, 1);
      last_skipped = false;
      skip_release_token = -1;
    }
  }

  last_pos = p;
  term_mouse_move(get_mods(), p);
}

void
win_mouse_wheel(POINT wpos, bool horizontal, int delta)
{
  pos tpos = translate_pos(wpos.x, wpos.y);

  int lines_per_notch;
  SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines_per_notch, 0);

  term_mouse_wheel(horizontal, delta, lines_per_notch, get_mods(), tpos);
}

void
win_get_locator_info(int *x, int *y, int *buttons, bool by_pixels)
{
  POINT p = {-1, -1};

  if (GetCursorPos(&p)) {
    if (ScreenToClient(wnd, &p)) {
      if (p.x < PADDING)
        p.x = 0;
      else
        p.x -= PADDING;
      if (p.x >= cterm->cols * cell_width)
        p.x = cterm->cols * cell_width - 1;
      if (p.y < OFFSET + PADDING)
        p.y = 0;
      else
        p.y -= OFFSET + PADDING;
      if (p.y >= cterm->rows * cell_height)
        p.y = cterm->rows * cell_height - 1;

      if (by_pixels) {
        *x = p.x;
        *y = p.y;
      } else {
        *x = floorf(p.x / (float)cell_width);
        *y = floorf(p.y / (float)cell_height);
      }
    }
  }

  *buttons = button_state;
}
#include "sckdef.h"


typedef enum {
  COMP_CLEAR = -1,
  COMP_NONE = 0,
  COMP_PENDING = 1, COMP_ACTIVE = 2
} comp_state_t;
static comp_state_t comp_state = COMP_NONE;
static uint last_key_down = 0;
static uint last_key_up = 0;

static struct {
  wchar kc[4];
  char * s;
} composed[] = {
#include "composed.t"
};
static wchar compose_buf[lengthof(composed->kc) + 4];
static int compose_buflen = 0;

void
compose_clear(void)
{
  comp_state = COMP_CLEAR;
  compose_buflen = 0;
  last_key_down = 0;
  last_key_up = 0;
}

void
win_key_reset(void)
{
  alt_state = ALT_NONE;
  compose_clear();
}

// notify margin bell ring enabled
void
provide_input(wchar c1)
{
  if (cterm->margin_bell && c1 != '\e')
    cterm->ring_enabled = true;
}

#define dont_debug_virtual_key_codes
#define dont_debug_key
#define dont_debug_alt
#define dont_debug_compose

#ifdef debug_virtual_key_codes
static struct {
  uint vk_;
  char * vk_name;
} vk_names[] = {
#include "_vk.t"
};

static string
vk_name(uint key)
{
  for (uint i = 0; i < lengthof(vk_names); i++)
    if (key == vk_names[i].vk_)
      return vk_names[i].vk_name;
  static char vk_name[3];
  sprintf(vk_name, "%02X", key & 0xFF);
  return vk_name;
}
#endif

#ifdef debug_compose
# define debug_key
#endif

#ifdef debug_key
#define trace_key(tag)	printf(" key(%s)\n", tag)
#else
#define trace_key(tag)	(void)0
#endif

#ifdef debug_alt
#define trace_alt	printf
#else
#define trace_alt(...)	
#endif

// key names for user-definable functions
static int
win_key_nullify(uchar vk)
{
  INPUT ki[2];
  ki[0].type = INPUT_KEYBOARD;
  ki[1].type = INPUT_KEYBOARD;
  ki[0].ki.dwFlags = KEYEVENTF_KEYUP;
  ki[1].ki.dwFlags = 0;
  ki[0].ki.wVk = vk;
  ki[1].ki.wVk = vk;
  ki[0].ki.wScan = 0;
  ki[1].ki.wScan = 0;
  ki[0].ki.time = 0;
  ki[1].ki.time = 0;
  ki[0].ki.dwExtraInfo = 0;
  ki[1].ki.dwExtraInfo = 0;
  return SendInput(2, ki, sizeof(INPUT));
}

#define dont_debug_def_keys 1

static int
pick_key_function(wstring key_commands, char * tag, int n, uint key, mod_keys mods, mod_keys mod0, uint scancode)
{
  char * ukey_commands = cs__wcstoutf(key_commands);
  char * cmdp = ukey_commands;
  char sepch = ';';
  if ((uchar)*cmdp <= (uchar)' ')
    sepch = *cmdp++;
#ifdef debug_def_keys
  printf("pick_key_function (%s) <%s> %d\n", ukey_commands, tag, n);
#endif

  // derive modifiers from specification prefix, module their order;
  // in order to abstract from the order and support flexible configuration,
  // the modifiers could have been collected separately already instead of 
  // prefixing them to the tag (before calling pick_key_function) but 
  // that would have been more substantial redesign; or the prefix could 
  // be normalized here by sorting; better solution: collect info here
  mod_keys tagmods(char * k)
  {
    mod_keys m = mod0;
    char * sep = strrchr(k, '+');
    if (sep)
      for (; *k && k < sep; k++)
        switch (*k) {
          when 'S': m |= MDK_SHIFT;
          when 'A': m |= MDK_ALT;
          when 'C': m |= MDK_CTRL;
          when 'W': m |= MDK_WIN;
          when 'U': m |= MDK_SUPER;
          when 'Y': m |= MDK_HYPER;
        }
    return m;
  }

  mod_keys mod_tag = tagmods(tag ?: "");
  char * tag0 = tag ? strchr(tag, '+') : 0;
  if (tag0)
    tag0++;
  else
    tag0 = tag;

#if defined(debug_def_keys) && debug_def_keys > 0
  printf("key_fun tag <%s> tag0 <%s> mod %X\n", tag ?: "(null)", tag0 ?: "(null)", mod_tag);
#endif

  char * paramp;
  while ((tag || n >= 0) && (paramp = strchr(cmdp, ':'))) {
    *paramp = '\0';
    paramp++;
    char * sepp = strchr(paramp, sepch);
    if (sepp)
      *sepp = '\0';

    mod_keys mod_cmd = tagmods(cmdp);
    char * cmd0 = strrchr(cmdp, '+');
    if (cmd0)
      cmd0++;
    else
      cmd0 = cmdp;

    if (*cmdp == '*') {
      mod_cmd = mod_tag;
      cmd0 = cmdp;
      cmd0++;
      if (*cmd0 == '+')
        cmd0++;
    }

#if defined(debug_def_keys) && debug_def_keys > 1
    printf("tag <%s>: cmd <%s> cmd0 <%s> mod %X fct <%s>\n", tag, cmdp, cmd0, mod_cmd, paramp);
#endif

    if (tag ? (mod_cmd == mod_tag && !strcmp(cmd0, tag0)) : n == 0) {
#if defined(debug_def_keys) && debug_def_keys == 1
      printf("tag <%s>: cmd <%s> fct <%s>\n", tag, cmdp, paramp);
#endif
      int ret = false;
      wchar * fct = cs__utftowcs(paramp);

      if (key == VK_CAPITAL || key == VK_SCROLL || key == VK_NUMLOCK) {
        // nullify the keyboard state effect implied by the Lock key; 
        // use fake keyboard events, but avoid the recursion, 
        // fake events have scancode 0, ignore them also in win_key_up;
        // alternatively, we could hook the keyboard (low-level) and 
        // swallow the Lock key, but then it's not handled anymore so 
        // we'd need to fake its keyboard state effect 
        // (SetKeyboardState, and handle the off transition...) 
        // or consider it otherwise, all getting very tricky...
        if (!scancode) {
          ret = true;
        }
        else
          win_key_nullify(key);
      }

      uint code;
      if (!ret) {
        int len=wcslen(fct);
        switch(*fct){
          when '"' or '\'' :
              if(fct[len-1]==*fct)
              {//raww string "xxx" or 'xxx'
                int len = wcslen(fct) - 2;
                if (len > 0) {
                  provide_input(fct[1]);
                  child_sendw(cterm,&fct[1], wcslen(fct) - 2);
                  ret = true;
                }
              }
          when '^':
              if(len==2){
                char cc[2];
                cc[1] = ' ';
                if (fct[1] == '?')
                  cc[1] = '\177';
                else if (fct[1] == ' ' || (fct[1] >= '@' && fct[1] <= '_')) {
                  cc[1] = fct[1] & 0x1F;
                }
                if (cc[1] != ' ') {
                  if (mods & MDK_ALT) {
                    cc[0] = '\e';
                    child_send(cterm,cc, 2);
                  }
                  else {
                    provide_input(cc[1]);
                    child_send(cterm,&cc[1], 1);
                  }
                  ret = true;
                }
              }
          when '`':
              if (fct[len - 1] == '`') {//`shell cmd`
                fct[len - 1] = 0;
                char * cmd = cs__wcstombs(&fct[1]);
                if (*cmd) {
                  term_cmd(cmd);
                  ret = true;
                }
                free(cmd);
              }
          when '0' ... '9':
              if (sscanf (paramp, "%u%c", & code, &(char){0}) == 1) {//key escape
                char buf[33];
                int len = sprintf(buf, mods ? "\e[%i;%u~" : "\e[%i~", code, mods + 1);
                child_send(cterm,buf, len);
                ret = true;
              }
          when 0:
          // empty definition (e.g. "A+Enter:;"), shall disable 
          // further shortcut handling for the input key but 
          // trigger fall-back to "normal" key handling (with mods)
          ret = -1;
          otherwise : 
            ret=fundef_run(paramp,key,mods);
        }
        free(fct);
        free(ukey_commands);
        return ret;
      }
    }

    n--;
    if (sepp) {
      cmdp = sepp + 1;
      // check for multi-line separation
      if (*cmdp == '\\' && cmdp[1] == '\n') {
        cmdp += 2;
        while (iswspace(*cmdp))
          cmdp++;
      }
    }
    else
      break;
  }
  free(ukey_commands);
  return false;
}

void
user_function(wstring commands, int n)
{
  pick_key_function(commands, 0, n, 0, 0, 0, 0);
}
int lwinkey=0,rwinkey=0;
bool 
win_whotkey(WPARAM wp, LPARAM lp){
  (void)lp;
  uint key = wp;
  last_key_down = key;
  last_key_up = 0;


  uchar kbd[256];
  GetKeyboardState(kbd);
  inline bool is_key_down(uchar vk) { return kbd[vk] & 0x80; }

  bool lwin=(is_key_down(VK_LWIN) && key != VK_LWIN);
  bool rwin=(is_key_down(VK_RWIN) && key != VK_RWIN);
  bool win =lwin|rwin; 
  bool shift = is_key_down(VK_SHIFT);
  // bool lshift = is_key_down(VK_RSHIFT);
  // bool rshift = is_key_down(VK_RSHIFT);
  bool lalt = is_key_down(VK_LMENU);
  bool ralt = is_key_down(VK_RMENU);
  bool alt = lalt | ralt;
  trace_alt("alt %d lalt %d ralt %d\n", alt, lalt, ralt);
  bool rctrl = is_key_down(VK_RCONTROL);
  bool lctrl = is_key_down(VK_LCONTROL); 
  bool ctrl = lctrl | rctrl;
  mod_keys mods = shift * MDK_SHIFT
                | alt * MDK_ALT
                | ctrl * MDK_CTRL
                | win * MDK_WIN
                ;
  bool super = super_key && is_key_down(super_key);
  bool hyper = hyper_key && is_key_down(hyper_key);
  mods |= super * MDK_SUPER | hyper * MDK_HYPER;
  if(!win)return 0;
  int kmask=sckwmask[key];//assume winkey not to here 
  int modt=packmod(mods);
  if(kmask&(1<<modt)){
    bool lshift = is_key_down(VK_RSHIFT);
    bool rshift = is_key_down(VK_RSHIFT);
    mod_keys modl = lshift* MDK_SHIFT
        | lalt  * MDK_ALT
        | lctrl * MDK_CTRL
        | lwin *  MDK_WIN
        ;
    mod_keys modr = rshift* MDK_SHIFT
        | ralt  * MDK_ALT
        | rctrl * MDK_CTRL
        | rwin *  MDK_WIN
        ;
    int moda=mods|(modl<<8)|(modr<<16);
    if(sckw_run(key,moda))return 1;
  }
  return 0;
}
bool
win_key_down(WPARAM wp, LPARAM lp)
{
  uint scancode = HIWORD(lp) & (KF_EXTENDED | 0xFF);
  bool extended = HIWORD(lp) & KF_EXTENDED;
  bool repeat = HIWORD(lp) & KF_REPEAT;
  uint count = LOWORD(lp);

  uint key = wp;
  last_key_down = key;
  last_key_up = 0;

  if (comp_state == COMP_ACTIVE)
    comp_state = COMP_PENDING;
  else if (comp_state == COMP_CLEAR && !repeat)
    comp_state = COMP_NONE;

#ifdef debug_virtual_key_codes
  printf("win_key_down %02X %s scan %d ext %d rpt %d/%d other %02X\n", key, vk_name(key), scancode, extended, repeat, count, HIWORD(lp) >> 8);
#endif

  static LONG last_key_time = 0;

  LONG message_time = GetMessageTime();
  if (repeat) {
    if (!cterm->auto_repeat)
      return true;
    if (cterm->repeat_rate &&
        message_time - last_key_time < 1000 / cterm->repeat_rate)
      return true;
  }
  if (repeat && cterm->repeat_rate &&
      message_time - last_key_time < 2000 / cterm->repeat_rate)
    /* Key repeat seems to be continued. */
    last_key_time += 1000 / cterm->repeat_rate;
  else
    last_key_time = message_time;

  if (key == VK_PROCESSKEY) {
    MSG msg={.hwnd = wnd, .message = WM_KEYDOWN, .wParam = wp, .lParam = lp};
    TranslateMessage( &msg);
    return true;
  }

  uchar kbd[256];
  GetKeyboardState(kbd);
  inline bool is_key_down(uchar vk) { return kbd[vk] & 0x80; }
#ifdef debug_virtual_key_codes
  printf("-- [%u %c%u] Shift %d:%d/%d Ctrl %d:%d/%d Alt %d:%d/%d\n",
         (int)message_time , lctrl_time ? '+' : '=', (int)message_time - lctrl_time,
         is_key_down(VK_SHIFT), is_key_down(VK_LSHIFT), is_key_down(VK_RSHIFT),
         is_key_down(VK_CONTROL), is_key_down(VK_LCONTROL), is_key_down(VK_RCONTROL),
         is_key_down(VK_MENU), is_key_down(VK_LMENU), is_key_down(VK_RMENU));
#endif

  // Fix AltGr detection;
  // workaround for broken Windows on-screen keyboard (#692)
  if (!cfg.old_altgr_detection) {
    static bool lmenu_tweak = false;
    if (key == VK_MENU && !scancode) {
      extended = true;
      scancode = 312;
      kbd[VK_LMENU] = 0x00;
      kbd[VK_RMENU] = 0x80;
      lmenu_tweak = true;
    }
    else if (lmenu_tweak) {
      kbd[VK_LMENU] = 0x00;
      kbd[VK_RMENU] = 0x80;
      lmenu_tweak = false;
    }
  }

  // Distinguish real LCONTROL keypresses from fake messages sent for AltGr.
  // It's a fake if the next message is an RMENU with the same timestamp.
  // Or, as of buggy TeamViewer, if the RMENU comes soon after (#783).
  if (key == VK_CONTROL && !extended) {
    lctrl = true;
    lctrl_time = message_time;
    //printf("lctrl (true) %d (%d)\n", lctrl, is_key_down(VK_LCONTROL));
  }
  else if (lctrl_time) {
    lctrl = !(key == VK_MENU && extended 
              && message_time - lctrl_time <= cfg.ctrl_alt_delay_altgr);
    lctrl_time = 0;
    //printf("lctrl (time) %d (%d)\n", lctrl, is_key_down(VK_LCONTROL));
  }
  else {
    lctrl = is_key_down(VK_LCONTROL) && (lctrl || !is_key_down(VK_RMENU));
    //printf("lctrl (else) %d (%d)\n", lctrl, is_key_down(VK_LCONTROL));
  }
  bool numlock = kbd[VK_NUMLOCK] & 1;
  bool shift = is_key_down(VK_SHIFT);
  bool lalt = is_key_down(VK_LMENU);
  bool ralt = is_key_down(VK_RMENU);
  bool alt = lalt | ralt;
  trace_alt("alt %d lalt %d ralt %d\n", alt, lalt, ralt);
  bool rctrl = is_key_down(VK_RCONTROL);
  bool ctrl = lctrl | rctrl;
  //win key not to here,and win key not used
  //bool lwin=(is_key_down(VK_LWIN) && key != VK_LWIN);
  //bool rwin=(is_key_down(VK_RWIN) && key != VK_RWIN);
  //bool win =lwin|rwin; 
  bool ctrl_lalt_altgr = cfg.ctrl_alt_is_altgr & ctrl & lalt & !ralt;
  //bool altgr0 = ralt | ctrl_lalt_altgr;
  // Alt/AltGr detection and handling could do with a complete revision 
  // from scratch; on the other hand, no unnecessary risk should be taken, 
  // so another hack is added.
  bool lctrl0 = is_key_down(VK_LCONTROL);
  bool altgr0 = (ralt & lctrl0) | ctrl_lalt_altgr;
  bool external_hotkey ;
  if (ralt && !scancode && cfg.external_hotkeys) {
    // Support external hot key injection by overriding disabled Alt+Fn
    // and fix buggy StrokeIt (#833).
    trace_alt("ralt = false\n");
    ralt = false;
    if (cfg.external_hotkeys > 1) external_hotkey = true;
  }

  bool altgr = ralt | ctrl_lalt_altgr;
  // While this should more properly reflect the AltGr modifier state, 
  // with the current implementation it has the opposite effect;
  // it spoils Ctrl+AltGr with modify_other_keys mode.
  //altgr = (ralt & lctrl0) | ctrl_lalt_altgr;

  trace_alt("alt %d lalt %d ralt %d altgr %d\n", alt, lalt, ralt, altgr);

  mod_keys mods = shift * MDK_SHIFT
                | alt   * MDK_ALT
                | ctrl  * MDK_CTRL
                //| win   * MDK_WIN
                ;
  bool super = super_key && is_key_down(super_key);
  bool hyper = hyper_key && is_key_down(hyper_key);
  mods |= super * MDK_SUPER | hyper * MDK_HYPER;

  update_mouse(mods);

  // Workaround for Windows clipboard history pasting simply injecting Ctrl+V
  // (mintty/wsltty#139)
  if (key == 'V' && mods == MDK_CTRL && !scancode) {
    win_paste(); 
    return true;
  }

  if (key == VK_MENU) {
    if (!repeat && mods == MDK_ALT && alt_state == ALT_NONE)
      alt_state = ALT_ALONE;
    return true;
  }

  alt_state_t old_alt_state = alt_state;
  if (alt_state > ALT_NONE)
    alt_state = ALT_CANCELLED;

  static LONG last_tabk_time = 0;
  switch(tabctrling){
      when 0:
        if(key==VK_CONTROL){
          tabctrling=1;
          last_tabk_time=message_time ;
        }
      when 1:
        if(key==VK_CONTROL){
          if( message_time - last_tabk_time < 500 ){
            tabctrling=2;
            last_tabk_time=message_time ;
            return 1;
          }else{
            tabctrling=1;
            last_tabk_time=message_time ;
          }
        }else{
          tabctrling=0;
        } 
      when 2:
        if(key=='A'){
          if( message_time - last_tabk_time < 500 ){
            tabctrling=3;
            last_tabk_time=message_time ;
            return 1;
          }else{
            tabctrling=0;
            last_tabk_time=message_time ;
          }
        }else{
          tabctrling=0;
        } 
      when 3:
        if(message_time - last_tabk_time< 5000 ){
          int res=1;
          int zoom=-10000;
          switch (key) {
            when VK_LEFT or 'H':  
                if (shift) win_tab_move(-1);
                else win_tab_change(-1);
            when VK_RIGHT or 'L': 
                if (shift) win_tab_move(1);
                else win_tab_change(1);
            when '1' ... '9':
                win_tab_go(key-'1');
            when ' ': kb_select(0,mods); tabctrling=0;
            when 'A': term_select_all();
            when 'C': term_copy();
            when 'D': send_syscommand(IDM_DEFSIZE);
            when 'F': send_syscommand(cfg.zoom_font_with_window ? IDM_FULLSCREEN_ZOOM : IDM_FULLSCREEN);
            when 'G': win_tab_show();
            when 'I': win_tab_indicator();
            when 'K': win_tog_partline();
            when 'O': win_tog_scrollbar();
            when 'M': open_popup_menu(true, "Wb|l|s", mods);
            when 'N': send_syscommand(IDM_NEW);
            when 'P': cycle_pointer_style();
            when 'R': send_syscommand(IDM_RESET);
            when 'S': send_syscommand(IDM_SEARCH);
            when 'T': new_tab_def();
            when 'V': win_paste();
            when 'W': win_close();
            when VK_SUBTRACT:  zoom = -1;
            when VK_ADD:       zoom = 1;
            when VK_NUMPAD0:   zoom = 0;
            when VK_OEM_MINUS: zoom = -1; mods &= ~MDK_SHIFT;
            when VK_OEM_PLUS:  zoom = 1; mods &= ~MDK_SHIFT;
            when '0':          zoom = 0;
            when VK_SHIFT :    res=-1;
            when VK_CONTROL or 
                VK_ESCAPE: res=-1;tabctrling=0;
            otherwise: res=0;
          }
          if(zoom>=-1){
            win_zoom_font(zoom, mods & MDK_SHIFT);
            return true;
          }
          last_tabk_time=message_time ;
          if(res>0)return 1;
        }else tabctrling=0;
  }
  // Handling special shifted key functions
  if (newwin_pending) {
    if (!extended) {  // only accept numeric keypad
      switch (key) {
        when VK_HOME : newwin_monix--; newwin_moniy--;
        when VK_UP   : newwin_moniy--;
        when VK_PRIOR: newwin_monix++; newwin_moniy--;
        when VK_LEFT : newwin_monix--;
        when VK_CLEAR: newwin_monix = 0; newwin_moniy = 0; newwin_home = true;
        when VK_RIGHT: newwin_monix++;
        when VK_END  : newwin_monix--; newwin_moniy++;
        when VK_DOWN : newwin_moniy++;
        when VK_NEXT : newwin_monix++; newwin_moniy++;
        when VK_INSERT or VK_DELETE:
                       newwin_monix = 0; newwin_moniy = 0; newwin_home = false;
      }
    }
    return true;
  }
  if (transparency_pending) {
    transparency_pending = 2;
    switch (key) {
      when VK_HOME  : set_transparency(previous_transparency);
      when VK_CLEAR : if (win_is_glass_available()) {
                        cfg.transparency = TR_GLASS;
                        win_update_transparency(false);
                      }
      when VK_DELETE: set_transparency(0);
      when VK_INSERT: set_transparency(127);
      when VK_END   : set_transparency(TR_HIGH);
      when VK_UP    : set_transparency(cfg.transparency + 1);
      when VK_PRIOR : set_transparency(cfg.transparency + 16);
      when VK_LEFT  : set_transparency(cfg.transparency - 1);
      when VK_RIGHT : set_transparency(cfg.transparency + 1);
      when VK_DOWN  : set_transparency(cfg.transparency - 1);
      when VK_NEXT  : set_transparency(cfg.transparency - 16);
      otherwise: transparency_pending = 0;
    }
#ifdef debug_transparency
    printf("==%d\n", transparency_pending);
#endif
    if (transparency_pending) {
      transparency_tuned = true;
      return true;
    }
  }
  if (cterm->selection_pending) {
    bool sel_adjust = false;
    //WPARAM scroll = 0;
    int sbtop = -sblines();
    int sbbot = term_last_nonempty_line();
    int oldisptop = cterm->disptop;
    //printf("y %d disptop %d sb %d..%d\n", cterm->sel_pos.y, cterm->disptop, sbtop, sbbot);
    switch (key) {
      when VK_CLEAR:
        // re-anchor keyboard selection
        cterm->sel_anchor = cterm->sel_pos;
        cterm->sel_start = cterm->sel_pos;
        cterm->sel_end = cterm->sel_pos;
        cterm->sel_rect = mods & MDK_ALT;
        sel_adjust = true;
      when VK_LEFT:
        if (cterm->sel_pos.x > 0)
          cterm->sel_pos.x--;
        sel_adjust = true;
      when VK_RIGHT:
        if (cterm->sel_pos.x < cterm->cols)
          cterm->sel_pos.x++;
        sel_adjust = true;
      when VK_UP:
        if (cterm->sel_pos.y > sbtop) {
          if (cterm->sel_pos.y <= cterm->disptop)
            term_scroll(0, -1);
          cterm->sel_pos.y--;
          sel_adjust = true;
        }
      when VK_DOWN:
        if (cterm->sel_pos.y < sbbot) {
          if (cterm->sel_pos.y + 1 >= cterm->disptop + cterm->rows)
            term_scroll(0, +1);
          cterm->sel_pos.y++;
          sel_adjust = true;
        }
      when VK_PRIOR:
        //scroll = SB_PAGEUP;
        term_scroll(0, -max(1, cterm->rows - 1));
        cterm->sel_pos.y += cterm->disptop - oldisptop;
        sel_adjust = true;
      when VK_NEXT:
        //scroll = SB_PAGEDOWN;
        term_scroll(0, +max(1, cterm->rows - 1));
        cterm->sel_pos.y += cterm->disptop - oldisptop;
        sel_adjust = true;
      when VK_HOME:
        //scroll = SB_TOP;
        term_scroll(+1, 0);
        cterm->sel_pos.y += cterm->disptop - oldisptop;
        cterm->sel_pos.y = sbtop;
        cterm->sel_pos.x = 0;
        sel_adjust = true;
      when VK_END:
        //scroll = SB_BOTTOM;
        term_scroll(-1, 0);
        cterm->sel_pos.y += cterm->disptop - oldisptop;
        cterm->sel_pos.y = sbbot;
        if (sbbot < cterm->rows) {
          termline *line = cterm->lines[sbbot];
          if (line)
            for (int j = line->cols - 1; j > 0; j--) {
              cterm->sel_pos.x = j + 1;
              if (!termchars_equal(&line->chars[j], &cterm->erase_char))
                break;
            }
        }
        sel_adjust = true;
      when VK_INSERT or VK_RETURN:  // copy
        term_copy();
        cterm->selection_pending = false;
      when VK_DELETE or VK_ESCAPE:  // abort
        cterm->selection_pending = false;
      otherwise:
        //cterm->selection_pending = false;
        win_bell(&cfg);
    }
    //if (scroll) {
    //  if (!cterm->app_scrollbar)
    //    SendMessage(wnd, WM_VSCROLL, scroll, 0);
    //  sel_adjust = true;
    //}
    if (sel_adjust) {
      if (cterm->sel_rect) {
        cterm->sel_start.y = min(cterm->sel_anchor.y, cterm->sel_pos.y);
        cterm->sel_start.x = min(cterm->sel_anchor.x, cterm->sel_pos.x);
        cterm->sel_end.y = max(cterm->sel_anchor.y, cterm->sel_pos.y);
        cterm->sel_end.x = max(cterm->sel_anchor.x, cterm->sel_pos.x);
      }
      else if (posle(cterm->sel_anchor, cterm->sel_pos)) {
        cterm->sel_start = cterm->sel_anchor;
        cterm->sel_end = cterm->sel_pos;
      }
      else {
        cterm->sel_start = cterm->sel_pos;
        cterm->sel_end = cterm->sel_anchor;
      }
      //printf("->sel %d:%d .. %d:%d\n", cterm->sel_start.y, cterm->sel_start.x, cterm->sel_end.y, cterm->sel_end.x);
      cterm->selected = true;
      win_update(true);
    }
    if (cterm->selection_pending)
      return true;
    else
      cterm->selected = false;
    return true;
  }
  if (tek_mode == TEKMODE_GIN) {
    int step = (mods & MDK_SHIFT) ? 40 : (mods & MDK_CTRL) ? 1 : 4;
    switch (key) {
      when VK_HOME : tek_move_by(step, -step);
      when VK_UP   : tek_move_by(step, 0);
      when VK_PRIOR: tek_move_by(step, step);
      when VK_LEFT : tek_move_by(0, -step);
      when VK_CLEAR: tek_move_by(0, 0);
      when VK_RIGHT: tek_move_by(0, step);
      when VK_END  : tek_move_by(-step, -step);
      when VK_DOWN : tek_move_by(-step, 0);
      when VK_NEXT : tek_move_by(-step, step);
      otherwise: step = 0;
    }
    if (step)
      return true;
  }
  if (!cterm->shortcut_override) {
    int kmask=sckmask[key];//assume winkey not to here 
    int modt=packmod(mods);
    if(kmask&(1<<modt)){
      bool lshift = is_key_down(VK_RSHIFT);
      bool rshift = is_key_down(VK_RSHIFT);
      (void)external_hotkey ;// todo:external_hotkey is not process now
      //assume winkey not to here 
      mod_keys modl = lshift* MDK_SHIFT
          | lalt  * MDK_ALT
          | lctrl * MDK_CTRL
          ;
      mod_keys modr = rshift* MDK_SHIFT
          | ralt  * MDK_ALT
          | rctrl * MDK_CTRL
          ;
      int moda=mods|(modl<<8)|(modr<<16);
      if(sck_run(key,moda))return 1;
    }
  }
  //==========================================
  // process key ,do not check shortcut here
  // Keycode buffers
  char buf[32];
  int len = 0;

  inline void ch(char c) { buf[len++] = c; }
  inline void esc_if(bool b) { if (b) ch('\e'); }
  void ss3(char c) { ch('\e'); ch('O'); ch(c); }
  void csi(char c) { ch('\e'); ch('['); ch(c); }
  void mod_csi(char c) { len = sprintf(buf, "\e[1;%u%c", mods + 1, c); }
  void mod_ss3(char c) { mods ? mod_csi(c) : ss3(c); }
  void tilde_code(uchar code) {
    trace_key("tilde");
    len = sprintf(buf, mods ? "\e[%i;%u~" : "\e[%i~", code, mods + 1);
  }
  void other_code(wchar c) {
    trace_key("other");
    len = sprintf(buf, "\e[%u;%uu", c, mods + 1);
  }
  void app_pad_code(char c) {
    void mod_appl_xterm(char c) {len = sprintf(buf, "\eO%u%c", mods + 1, c);}
    if (mods && cterm->app_keypad) switch (key) {
      when VK_DIVIDE or VK_MULTIPLY or VK_SUBTRACT or VK_ADD or VK_RETURN:
        mod_appl_xterm(c - '0' + 'p');
        return;
    }
    if (cterm->vt220_keys && mods && cterm->app_keypad) switch (key) {
      when VK_CLEAR or VK_PRIOR ... VK_DOWN or VK_INSERT or VK_DELETE:
        mod_appl_xterm(c - '0' + 'p');
        return;
    }
    mod_ss3(c - '0' + 'p');
  }
  void strcode(string s) {
    unsigned int code;
    if (sscanf (s, "%u", & code) == 1)
      tilde_code(code);
    else
      len = sprintf(buf, "%s", s);
  }

  bool alt_code_key(char digit) {
    if (old_alt_state > ALT_ALONE && digit < old_alt_state) {
      alt_state = old_alt_state;
      alt_code = alt_code * alt_state + digit;
      return true;
    }
    return false;
  }

  bool alt_code_numpad_key(char digit) {
    if (old_alt_state == ALT_ALONE) {
      alt_code = digit;
      alt_state = digit ? ALT_DEC : ALT_OCT;
      return true;
    }
    return alt_code_key(digit);
  }

  bool app_pad_key(char symbol) {
    if (extended)
      return false;
    // Mintty-specific: produce app_pad codes not only when vt220 mode is on,
    // but also in PC-style mode when app_cursor_keys is off, to allow the
    // numpad keys to be distinguished from the cursor/editing keys.
    if (cterm->app_keypad && (!cterm->app_cursor_keys || cterm->vt220_keys)) {
      // If NumLock is on, Shift must have been pressed to override it and
      // get a VK code for an editing or cursor key code.
      if (numlock)
        mods |= MDK_SHIFT;
      app_pad_code(symbol);
      return true;
    }
    return symbol != '.' && alt_code_numpad_key(symbol - '0');
  }

  void edit_key(uchar code, char symbol) {
    if (!app_pad_key(symbol)) {
      if (code != 3 || ctrl || alt || shift || !cterm->delete_sends_del)
        tilde_code(code);
      else
        ch(CDEL);
    }
  }

  void cursor_key(char code, char symbol) {
    if (cterm->vt52_mode)
      len = sprintf(buf, "\e%c", code);
    else if (!app_pad_key(symbol))
      mods ? mod_csi(code) : cterm->app_cursor_keys ? ss3(code) : csi(code);
  }

static struct {
  unsigned int combined;
  unsigned int base;
  unsigned int spacing;
} comb_subst[] = {
#include "combined.t"
};

  // Keyboard layout
  bool layout(void) {
    // ToUnicode returns up to 4 wchars according to
    // http://blogs.msdn.com/b/michkap/archive/2006/03/24/559169.aspx
    // https://web.archive.org/web/20120103012712/http://blogs.msdn.com/b/michkap/archive/2006/03/24/559169.aspx
    wchar wbuf[4];
    int wlen = ToUnicode(key, scancode, kbd, wbuf, lengthof(wbuf), 0);
    trace_alt("layout %d alt %d altgr %d\n", wlen, alt, altgr);
    if (!wlen)     // Unassigned.
      return false;
    if (wlen < 0)  // Dead key.
      return true;

    esc_if(alt);

    // Substitute accent compositions not supported by Windows
    if (wlen == 2)
      for (unsigned int i = 0; i < lengthof(comb_subst); i++)
        if (comb_subst[i].spacing == wbuf[0] && comb_subst[i].base == wbuf[1]
            && comb_subst[i].combined < 0xFFFF  // -> wchar/UTF-16: BMP only
           ) {
          wchar wtmp = comb_subst[i].combined;
          short mblen = cs_wcntombn(buf + len, &wtmp, lengthof(buf) - len, 1);
          // short to recognise 0xFFFD as negative (WideCharToMultiByte...?)
          if (mblen > 0) {
            wbuf[0] = comb_subst[i].combined;
            wlen = 1;
          }
          break;
        }

    // Compose characters
    if (comp_state > 0) {
#ifdef debug_compose
      printf("comp (%d)", wlen);
      for (int i = 0; i < compose_buflen; i++) printf(" %04X", compose_buf[i]);
      printf(" +");
      for (int i = 0; i < wlen; i++) printf(" %04X", wbuf[i]);
      printf("\n");
#endif
      for (int i = 0; i < wlen; i++)
        compose_buf[compose_buflen++] = wbuf[i];
      uint comp_len = min((uint)compose_buflen, lengthof(composed->kc));
      bool found = false;
      for (uint k = 0; k < lengthof(composed); k++)
        if (0 == wcsncmp(compose_buf, composed[k].kc, comp_len)) {
          if (comp_len < lengthof(composed->kc) && composed[k].kc[comp_len]) {
            // partial match
            comp_state = COMP_ACTIVE;
            return true;
          }
          else {
            // match
            ///can there be an uncomposed rest in wbuf? should we consider it?
#ifdef utf8_only
            ///alpha, UTF-8 only, unchecked...
            strcpy(buf + len, composed[k].s);
            len += strlen(composed[k].s);
            compose_buflen = 0;
            return true;
#else
            wchar * wc = cs__utftowcs(composed[k].s);
            wlen = 0;
            while (wc[wlen] && wlen < (int)lengthof(wbuf)) {
              wbuf[wlen] = wc[wlen];
              wlen++;
            }
            free(wc);
            found = true;  // fall through, but skip error handling
#endif
          }
        }
      ///should we deliver compose_buf[] first...?
      compose_buflen = 0;
      if (!found) {
        // unknown compose sequence
        win_bell(&cfg);
        // continue without composition
      }
    }
    else
      compose_buflen = 0;

    // Check that the keycode can be converted to the current charset
    // before returning success.
    int mblen = cs_wcntombn(buf + len, wbuf, lengthof(buf) - len, wlen);
#ifdef debug_ToUnicode
    printf("wlen %d:", wlen);
    for (int i = 0; i < wlen; i ++) printf(" %04X", wbuf[i] & 0xFFFF);
    printf("\n");
    printf("mblen %d:", mblen);
    for (int i = 0; i < mblen; i ++) printf(" %02X", buf[i] & 0xFF);
    printf("\n");
#endif
    bool ok = mblen > 0;
    len = ok ? len + mblen : 0;
    return ok;
  }

  wchar undead_keycode(void) {
    wchar wc;
    int len = ToUnicode(key, scancode, kbd, &wc, 1, 0);
#ifdef debug_key
    printf("undead %02X scn %d -> %d %04X\n", key, scancode, len, wc);
#endif
    if (len < 0) {
      // Ugly hack to clear dead key state, a la Michael Kaplan.
      uchar empty_kbd[256];
      memset(empty_kbd, 0, sizeof empty_kbd);
      uint scancode = MapVirtualKey(VK_DECIMAL, 0);
      wchar dummy;
      while (ToUnicode(VK_DECIMAL, scancode, empty_kbd, &dummy, 1, 0) < 0);
      return wc;
    }
    return len == 1 ? wc : 0;
  }

  void modify_other_key(void) {
    wchar wc = undead_keycode();
    if (!wc) {
#ifdef debug_key
      printf("modf !wc mods %X shft %d\n", mods, mods & MDK_SHIFT);
#endif
      if (mods & MDK_SHIFT) {
        kbd[VK_SHIFT] = 0;
        wc = undead_keycode();
      }
    }
#ifdef debug_key
    printf("modf wc %04X (ctrl %d key %02X)\n", wc, ctrl, key);
#endif
    if (wc) {
      if (altgr && !is_key_down(VK_LMENU))
        mods &= ~ MDK_ALT;
      if (!altgr && (mods == MDK_CTRL) && wc > '~' && key <= 'Z') {
        // report control char on non-latin keyboard layout
        other_code(key);
      }
      else
        other_code(wc);
    }
  }

  bool char_key(void) {
    alt = lalt & !ctrl_lalt_altgr;
    trace_alt("char_key alt %d (l %d r %d altgr %d)\n", alt, lalt, ralt, altgr);

    // Sync keyboard layout with our idea of AltGr.
    kbd[VK_CONTROL] = altgr ? 0x80 : 0;

    // Don't handle Ctrl combinations here.
    // Need to check there's a Ctrl that isn't part of Ctrl+LeftAlt==AltGr.
    if ((ctrl & !ctrl_lalt_altgr) | (lctrl & rctrl))
      return false;

    // Try the layout.
    if (layout())
      return true;

    // This prevents AltGr from behaving like Alt in modify_other_keys mode.
    if (!cfg.altgr_is_alt && altgr0)
      return false;

    if (ralt) {
      // Try with RightAlt/AltGr key treated as Alt.
      kbd[VK_CONTROL] = 0;
      trace_alt("char_key ralt; alt = true\n");
      alt = true;
      layout();
      return true;
    }
    return !ctrl;
  }

  bool altgr_key(void) {
    if (!altgr)
      return false;

    trace_alt("altgr_key alt %d -> %d\n", alt, lalt & !ctrl_lalt_altgr);
    alt = lalt & !ctrl_lalt_altgr;

    // Sync keyboard layout with our idea of AltGr.
    kbd[VK_CONTROL] = altgr ? 0x80 : 0;

    // Don't handle Ctrl combinations here.
    // Need to check there's a Ctrl that isn't part of Ctrl+LeftAlt==AltGr.
    if ((ctrl & !ctrl_lalt_altgr) | (lctrl & rctrl))
      return false;

    // Try the layout.
    return layout();
  }

  void ctrl_ch(uchar c) {
    esc_if(alt);
    if (shift && !cfg.ctrl_exchange_shift) {
      // Send C1 control char if the charset supports it.
      // Otherwise prefix the C0 char with ESC.
      if (c < 0x20) {
        wchar wc = c | 0x80;
        int l = cs_wcntombn(buf + len, &wc, cs_cur_max, 1);
        if (l > 0 && buf[len] != '?') {
          len += l;
          return;
        }
      };
      esc_if(!alt);
    }
    ch(c);
  }

  bool ctrl_key(void) {
    bool try_appctrl(wchar wc) {
      switch (wc) {
        when '@' or '[' ... '_' or 'a' ... 'z':
          if (cterm->app_control & (1 << (wc & 0x1F))) {
            mods = ctrl * MDK_CTRL;
            other_code((wc & 0x1F) + '@');
            return true;
          }
      }
      return false;
    }

    bool try_key(void) {
      wchar wc = undead_keycode();  // should we fold that out into ctrl_key?

      if (try_appctrl(wc))
        return true;

      char c;
      switch (wc) {
        when '@' or '[' ... '_' or 'a' ... 'z': c = CTRL(wc);
        when '/': c = CTRL('_');
        when '?': c = CDEL;
        otherwise: return false;
      }
      ctrl_ch(c);
      return true;
    }

    bool try_shifts(void) {
      shift = is_key_down(VK_LSHIFT) & is_key_down(VK_RSHIFT);
      if (try_key())
        return true;
      shift = is_key_down(VK_SHIFT);
      if (shift || (key >= '0' && key <= '9' && !cterm->modify_other_keys)) {
        kbd[VK_SHIFT] ^= 0x80;
        if (try_key())
          return true;
        kbd[VK_SHIFT] ^= 0x80;
      }
      return false;
    }

    if (try_shifts())
      return true;
    if (altgr) {
      // Try with AltGr treated as Alt.
      kbd[VK_CONTROL] = 0;
      trace_alt("ctrl_key altgr alt = true\n");
      alt = true;
      return try_shifts();
    }
    return false;
  }

  bool vk_special(string key_mapped) {
    if (!* key_mapped) {
      if (!layout())
        return false;
    }
    else if ((key_mapped[0] & ~037) == 0 && key_mapped[1] == 0)
      ctrl_ch(key_mapped[0]);
    else
      strcode(key_mapped);
    return true;
  }

  switch (key) {
    when VK_RETURN:
      if (extended && cterm->vt52_mode && cterm->app_keypad)
        len = sprintf(buf, "\e?M");
      else if (extended && !numlock && cterm->app_keypad)
        app_pad_code('M' - '@');
      else if (!extended && cterm->modify_other_keys && (shift || ctrl))
        other_code('\r');
#ifdef support_special_key_Enter
      else if (ctrl)
        ctrl_ch(CTRL('^'));
#endif
      else
        esc_if(alt),
        cterm->newline_mode ? ch('\r'), ch('\n') : ch(shift ? '\n' : '\r');
    when VK_BACK:
      if (!ctrl)
        esc_if(alt), ch(cterm->backspace_sends_bs ? '\b' : CDEL);
      else if (cterm->modify_other_keys)
        other_code(cterm->backspace_sends_bs ? '\b' : CDEL);
      else
        ctrl_ch(cterm->backspace_sends_bs ? CDEL : CTRL('_'));
    when VK_TAB:
      if (!ctrl) shift ? csi('Z') : ch('\t');
      else
        cterm->modify_other_keys ? other_code('\t') : mod_csi('I');
    when VK_ESCAPE:
      cterm->app_escape_key
      ? ss3('[')
      : ctrl_ch(cterm->escape_sends_fs ? CTRL('\\') : CTRL('['));
    when VK_PAUSE:
      if (!vk_special(ctrl & !extended ? cfg.key_break : cfg.key_pause))
        // default cfg.key_pause is CTRL(']')
        return false;
    when VK_CANCEL:
      if (!strcmp(cfg.key_break, "_BRK_")) {
        child_break(cterm);
        return false;
      }
      if (!vk_special(cfg.key_break))
        // default cfg.key_break is CTRL('\\')
        return false;
    when VK_SNAPSHOT:
      if (!vk_special(cfg.key_prtscreen))
        return false;
    when VK_APPS:
      if (!vk_special(cfg.key_menu))
        return false;
    when VK_SCROLL:
      if (!vk_special(cfg.key_scrlock))
        return false;
    when VK_F1 ... VK_F24:
      if (key <= VK_F4 && cterm->vt52_mode) {
        len = sprintf(buf, "\e%c", key - VK_F1 + 'P');
        break;
      }
      if (cterm->vt220_keys && ctrl && VK_F3 <= key && key <= VK_F10)
        key += 10, mods &= ~MDK_CTRL;
      if (key <= VK_F4)
        mod_ss3(key - VK_F1 + 'P');
      else {
        tilde_code(
          (uchar[]){
            15, 17, 18, 19, 20, 21, 23, 24, 25, 26,
            28, 29, 31, 32, 33, 34, 42, 43, 44, 45
          }[key - VK_F5]
        );
      }
    when VK_INSERT: edit_key(2, '0');
    when VK_DELETE: edit_key(3, '.');
    when VK_PRIOR:  edit_key(5, '9');
    when VK_NEXT:   edit_key(6, '3');
    when VK_HOME:   cterm->vt220_keys ? edit_key(1, '7') : cursor_key('H', '7');
    when VK_END:    cterm->vt220_keys ? edit_key(4, '1') : cursor_key('F', '1');
    when VK_UP:     cursor_key('A', '8');
    when VK_DOWN:   cursor_key('B', '2');
    when VK_LEFT:   cursor_key('D', '4');
    when VK_RIGHT:  cursor_key('C', '6');
    when VK_CLEAR:  cursor_key('E', '5');
    when VK_MULTIPLY ... VK_DIVIDE:
      if (cterm->vt52_mode && cterm->app_keypad)
        len = sprintf(buf, "\e?%c", key - VK_MULTIPLY + 'j');
      else if (key == VK_ADD && old_alt_state == ALT_ALONE)
        alt_state = ALT_HEX, alt_code = 0;
      else if (mods || (cterm->app_keypad && !numlock) || !layout())
        app_pad_code(key - VK_MULTIPLY + '*');
    when VK_NUMPAD0 ... VK_NUMPAD9:
      if (cterm->vt52_mode && cterm->app_keypad)
        len = sprintf(buf, "\e?%c", key - VK_NUMPAD0 + 'p');
      else if ((cterm->app_cursor_keys || !cterm->app_keypad) &&
          alt_code_numpad_key(key - VK_NUMPAD0));
      else if (layout())
        ;
      else
        app_pad_code(key - VK_NUMPAD0 + '0');
    when 'A' ... 'Z' or ' ': {
      //// support Ctrl+Shift+AltGr combinations (esp. Ctrl+Shift+@)
      //bool modaltgr = (mods & ~MDK_ALT) == (cfg.ctrl_exchange_shift ? MDK_CTRL : (MDK_CTRL | MDK_SHIFT));
      // support Ctrl+AltGr combinations (esp. Ctrl+@ and Ctrl+Shift+@)
      bool modaltgr = ctrl;
#ifdef debug_key
      printf("-- mods %X alt %d altgr %d/%d ctrl %d lctrl %d/%d (modf %d comp %d)\n", mods, alt, altgr, altgr0, ctrl, lctrl, lctrl0, cterm->modify_other_keys, comp_state);
#endif
      if (altgr_key())
        trace_key("altgr");
      else if (!modaltgr && !cfg.altgr_is_alt && altgr0 && !cterm->modify_other_keys)
        // prevent AltGr from behaving like Alt
        trace_key("!altgr");
      else if (key != ' ' && alt_code_key(key - 'A' + 0xA))
        trace_key("alt");
      else if (cterm->modify_other_keys > 1 && mods == MDK_SHIFT && !comp_state)
        // catch Shift+space (not losing Alt+ combinations if handled here)
        // only in modify-other-keys mode 2
        modify_other_key();
      else if (char_key())
        trace_key("char");
      else if (cterm->modify_other_keys > 1 || (cterm->modify_other_keys && altgr))
        // handle Alt+space after char_key, avoiding undead_ glitch;
        // also handle combinations like Ctrl+AltGr+e
        trace_key("modf"),
        modify_other_key();
      else if (ctrl_key())
        trace_key("ctrl");
      else
        ctrl_ch(CTRL(key));
    }
    when '0' ... '9' or VK_OEM_1 ... VK_OEM_102:
      if (key <= '9' && alt_code_key(key - '0'))
        ;
      else if (char_key())
        trace_key("0... char_key");
      else if (cterm->modify_other_keys <= 1 && ctrl_key())
        trace_key("0... ctrl_key");
      else if (cterm->modify_other_keys)
        modify_other_key();
      else if (!cfg.ctrl_controls && (mods & MDK_CTRL))
        return false;
      else if (key <= '9')
        app_pad_code(key);
      else if (VK_OEM_PLUS <= key && key <= VK_OEM_PERIOD)
        app_pad_code(key - VK_OEM_PLUS + '+');
    when VK_PACKET:
      trace_alt("VK_PACKET alt %d lalt %d ralt %d altgr %d altgr0 %d\n", alt, lalt, ralt, altgr, altgr0);
      if (altgr0)
        alt = lalt;
      if (!layout())
        return false;
    otherwise:
      if (!layout())
        return false;
  }

  hide_mouse();
  term_cancel_paste();

  if (len) {
    //printf("[%ld] win_key_down %02X\n", mtime(), key); kb_trace = key;
    provide_input(*buf);
    while (count--)
      child_send(cterm,buf, len);
    compose_clear();
    // set token to enforce immediate display of keyboard echo;
    // we cannot win_update_now here; need to wait for the echo (child_proc)
    kb_input = true;
    //printf("[%ld] win_key sent %02X\n", mtime(), key); kb_trace = key;
    if (tek_mode == TEKMODE_GIN)
      tek_send_address();
  }
  else if (comp_state == COMP_PENDING)
    comp_state = COMP_ACTIVE;

  return true;
}

void
win_csi_seq(char * pre, char * suf)
{
  mod_keys mods = get_mods();
  inline bool is_key_down(uchar vk) { return GetKeyState(vk) & 0x80; }
  bool super = super_key && is_key_down(super_key);
  bool hyper = hyper_key && is_key_down(hyper_key);
  mods |= super * MDK_SUPER | hyper * MDK_HYPER;

  if (mods)
    child_printf(cterm,"\e[%s;%u%s", pre, mods + 1, suf);
  else
    child_printf(cterm,"\e[%s%s", pre, suf);
}

bool
win_key_up(WPARAM wp, LPARAM lp)
{
  inline bool is_key_down(uchar vk) { return GetKeyState(vk) & 0x80; }

  uint key = wp;
#ifdef debug_virtual_key_codes
  printf("  win_key_up %02X %s\n", key, vk_name(key));
#endif

  if (key == VK_CANCEL) {
    // in combination with Control, this may be the KEYUP event 
    // for VK_PAUSE or VK_SCROLL, so there actual state cannot be 
    // detected properly for use as a modifier; let's try to fix this
    super_key = 0;
    hyper_key = 0;
  }

  win_update_mouse();

  uint scancode = HIWORD(lp) & (KF_EXTENDED | 0xFF);
  // avoid impact of fake keyboard events (nullifying implicit Lock states)
  if (!scancode) {
    last_key_up = key;
    return false;
  }

  if (key == last_key_down
      // guard against cases of hotkey injection (#877)
      && (!last_key_up || key == last_key_up)
     )
  {
    if (
        (cfg.compose_key == MDK_CTRL && key == VK_CONTROL) ||
        (cfg.compose_key == MDK_SHIFT && key == VK_SHIFT) ||
        (cfg.compose_key == MDK_ALT && key == VK_MENU)
       )
    {
      if (comp_state >= 0)
        comp_state = COMP_ACTIVE;
    }
  }
  else
    comp_state = COMP_NONE;

  last_key_up = key;

  if (newwin_pending) {
    if (key == newwin_key) {
      if (is_key_down(VK_SHIFT))
        newwin_shifted = true;
#ifdef control_AltF2_size_via_token
      if (newwin_shifted /*|| win_is_fullscreen*/)
        clone_size_token = false;
#endif

      newwin_pending = false;

      // Calculate heuristic approximation of selected monitor position
      int x, y;
      MONITORINFO mi;
      search_monitors(&x, &y, 0, newwin_home, &mi);
      RECT r = mi.rcMonitor;
      int refx, refy;
      if (newwin_monix < 0)
        refx = r.left + 10;
      else if (newwin_monix > 0)
        refx = r.right - 10;
      else
        refx = (r.left + r.right) / 2;
      if (newwin_moniy < 0)
        refy = r.top + 10;
      else if (newwin_monix > 0)
        refy = r.bottom - 10;
      else
        refy = (r.top + r.bottom) / 2;
      POINT pt;
      pt.x = refx + newwin_monix * x;
      pt.y = refy + newwin_moniy * y;
      // Find monitor over or nearest to point
      HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
      int moni = search_monitors(&x, &y, mon, true, 0);

#ifdef debug_multi_monitors
      printf("NEW @ %d,%d @ monitor %d\n", pt.x, pt.y, moni);
#endif
      send_syscommand2(IDM_NEW_MONI, moni);
    }
  }
  if (transparency_pending) {
    transparency_pending--;
#ifdef debug_transparency
    printf("--%d\n", transparency_pending);
#endif
    if (!transparency_tuned)
      cycle_transparency();
    if (!transparency_pending && cfg.opaque_when_focused)
      win_update_transparency(true);
  }

  if (key == VK_CONTROL && cterm->hovering) {
    cterm->hovering = false;
    win_update(false);
  }

  if (key != VK_MENU)
    return false;

  if (alt_state > ALT_ALONE && alt_code) {
    if (cs_cur_max < 4) {
      char buf[4];
      int pos = sizeof buf;
      do
        buf[--pos] = alt_code;
      while (alt_code >>= 8);
      provide_input(buf[pos]);
      child_send(cterm,buf + pos, sizeof buf - pos);
      compose_clear();
    }
    else if (alt_code < 0x10000) {
      wchar wc = alt_code;
      if (wc < 0x20)
        MultiByteToWideChar(CP_OEMCP, MB_USEGLYPHCHARS,
                            (char[]){wc}, 1, &wc, 1);
      provide_input(wc);
      child_sendw(cterm,&wc, 1);
      compose_clear();
    }
    else {
      xchar xc = alt_code;
      provide_input(' ');
      child_sendw(cterm,(wchar[]){high_surrogate(xc), low_surrogate(xc)}, 2);
      compose_clear();
    }
  }

  alt_state = ALT_NONE;
  return true;
}

static int
win_key_fake(int vk)
{
  //printf("-> win_key_fake %02X\n", vk);
  INPUT ki[2];
  ki[0].type = INPUT_KEYBOARD;
  ki[1].type = INPUT_KEYBOARD;
  ki[0].ki.dwFlags = 0;
  ki[1].ki.dwFlags = KEYEVENTF_KEYUP;
  ki[0].ki.wVk = vk;
  ki[1].ki.wVk = vk;
  ki[0].ki.wScan = 0;
  ki[1].ki.wScan = 0;
  ki[0].ki.time = 0;
  ki[1].ki.time = 0;
  ki[0].ki.dwExtraInfo = 0;
  ki[1].ki.dwExtraInfo = 0;
  return SendInput(2, ki, sizeof(INPUT));
}

void
do_win_key_toggle(int vk, bool on)
{
  // this crap does not work
  return;

  // use some heuristic combination to detect the toggle state
  int delay = 33333;
  usleep(delay);
  int st = GetKeyState(vk);  // volatile; save in case of debugging
  int ast = GetAsyncKeyState(vk);  // volatile; save in case of debugging
  //uchar kbd[256];
  //GetKeyboardState(kbd);
  //printf("do_win_key_toggle %02X %d (st %02X as %02X kb %02X)\n", vk, on, st, ast, kbd[vk]);
  if (((st | ast) & 1) != on) {
    win_key_fake(vk);
    usleep(delay);
  }
  /* It is possible to switch the LED only and revert the actual 
     virtual input state of the current thread as it was by using 
     SetKeyboardState in win_key_down, but this "fix" would only 
     apply to the current window while the effective state remains 
     switched for other windows which makes no sense.
   */
}

static void
win_key_toggle(int vk, bool on)
{
  //printf("send IDM_KEY_DOWN_UP %02X\n", vk | (on ? 0x10000 : 0));
  send_syscommand2(IDM_KEY_DOWN_UP, vk | (on ? 0x10000 : 0));
}

void
win_led(int led, bool set)
{
  //printf("\n[%ld] win_led %d %d\n", mtime(), led, set);
  int led_keys[] = {VK_NUMLOCK, VK_CAPITAL, VK_SCROLL};
  if (led <= 0)
    for (uint i = 0; i < lengthof(led_keys); i++)
      win_key_toggle(led_keys[i], set);
  else if (led <= (int)lengthof(led_keys))
    win_key_toggle(led_keys[led - 1], set);
}

