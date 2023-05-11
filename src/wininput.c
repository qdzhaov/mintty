// wininput.c (part of mintty)
// Copyright 2008-22 Andy Koppe, 2015-2022 Thomas Wolff
// Licensed under the terms of the GNU General Public License v3 or later.

#include "winpriv.h"
#include "winsearch.h"

#include "charset.h"
#include "child.h"
#include "tek.h"

#include <math.h>
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM
#include <winnls.h>
#include "term.h"

uint kb_trace = 0;

static HMENU ctxmenu = NULL;
static HMENU sysmenu;
static int sysmenulen;
//static uint kb_select_key = 0;
static uint super_key = 0;
static uint hyper_key = 0;
// Compose support
static uint compose_key = 0;
static uint last_key_down = 0;
static uint last_key_up = 0;
static uint key_special=0;
enum Key_Special{KS_NEWWIN=1,KS_TRANP=2,KS_TERMSEL=4};
static uint newwin_key = 0;
static bool newwin_pending = false;
static bool newwin_shifted = false;
static bool newwin_home = false;
static int newwin_monix = 0, newwin_moniy = 0;
static int transparency_pending = 0;

static int fundef_stat(const char*cmd);
static int fundef_run(const char*cmd,uint key, mod_keys mods);
extern void LoadConfig();
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

static inline void send_syscommand2(WPARAM cmd, LPARAM p) { SendMessage(wv.wnd, WM_SYSCOMMAND, cmd, p); }
static inline void send_syscommand(WPARAM cmd) { SendMessage(wv.wnd, WM_SYSCOMMAND, cmd, ' '); }

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
  HWND desktop = wv.wnd;

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
append_commands(HMENU menu, wstring commands, UINT_PTR idm_cmd, bool add_icons, bool is_sysmenu)
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
    if (is_sysmenu)
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
        icon = LoadIcon(wv.inst, MAKEINTRESOURCE(IDI_MAINICON));
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

  bool shorts = !term.shortcut_override;
  bool clip = shorts && cfg.clip_shortcuts;
  bool alt_fn = shorts && cfg.alt_fn_shortcuts;
  bool ct_sh = shorts && cfg.ctrl_shift_shortcuts;
  uint mflags;
  wstring cap;

#ifdef debug_modify_menu
  printf("win_update_menus\n");
#endif

  void
  modify_menu(HMENU menu, UINT item, UINT state,const wchar * label,const wchar * key)
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
    wchar * tabp = wcschr(mi.dwTypeData, '\t');
    if (tabp)
      *tabp = '\0';
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
                 + (mod & MDK_CTRL ? 5 : 0)
                 + (mod & MDK_ALT ? 4 : 0)
                 + (mod & MDK_SHIFT ? 6 : 0)
                 + wcslen(key) + 1;
      mi.dwTypeData = renewn(mi.dwTypeData, len1);
      wcscat(mi.dwTypeData, W("\t"));
      if (mod & MDK_CTRL) wcscat(mi.dwTypeData, W("Ctrl+"));
      if (mod & MDK_ALT) wcscat(mi.dwTypeData, W("Alt+"));
      if (mod & MDK_SHIFT) wcscat(mi.dwTypeData, W("Shift+"));
      wcscat(mi.dwTypeData, key);
    }
#ifdef debug_modify_menu
    if (sysentry)
      printf("-> %04X [%04X] %04X <%ls>\n", item, mi.fMask, mi.fState, mi.dwTypeData);
#endif

    SetMenuItemInfoW(menu, item, 0, &mi);

    free(mi.dwTypeData);
  }

  const wchar *
  itemlabel(const char * label)
  {
    const char * loc = _(label);
    if (loc == label)
      // no localization entry
      return null;  // indicate to use system localization
    else
      return _W(label);  // use our localization
  }
#define ENSM(id,f)   EnableMenuItem(sysmenu, id, f)
#define ENCM(id,f)   EnableMenuItem(ctxmenu, id, f)
#define MFSM(id,f,lbl,key)   modify_menu(sysmenu, id, f,lbl,key)
#define MFCM(id,f,lbl,key)   modify_menu(ctxmenu , id, f,lbl,key)
  MFSM( SC_CLOSE, 0, itemlabel(__("&Close")), alt_fn ? W("Alt+F4") : ct_sh ? W("Ctrl+Shift+W") : null);
  mflags= win_tab_count() == 1;
  ENSM( IDM_PREVTAB   , mflags);
  ENSM( IDM_NEXTTAB   , mflags);
  ENSM( IDM_MOVELEFT  , mflags);
  ENSM( IDM_MOVERIGHT , mflags);

#define CKED(v) ((v)?MF_CHECKED : MF_UNCHECKED)  
#define GYED(v) ((v)?MF_GRAYED : MF_ENABLED)
  //__ System menu:
  MFSM( IDM_NEW, 0, _W("New &Win"),  W("Alt+F2") );
  ENCM( IDM_OPEN, GYED(term.selected ));
  //__ Context menu:
  MFCM( IDM_COPY, GYED(term.selected ), _W("&Copy"), clip ? W("Ctrl+Ins") : ct_sh ? W("Ctrl+Shift+C") : null);
  // enable/disable predefined extended context menu entries
  // (user-definable ones are handled via fct_status())
  ENCM( IDM_COPY_TEXT, GYED(term.selected ));
  ENCM( IDM_COPY_TABS, GYED(term.selected ));
  ENCM( IDM_COPY_TXT , GYED(term.selected ));
  ENCM( IDM_COPY_RTF , GYED(term.selected ));
  ENCM( IDM_COPY_HTXT, GYED(term.selected ));
  ENCM( IDM_COPY_HFMT, GYED(term.selected ));
  ENCM( IDM_COPY_HTML, GYED(term.selected ));
  MFCM( IDM_COPASTE, GYED(term.selected ), _W("Copy → Paste"), clip ? W("Ctrl+Shift+Ins") : null);
  mflags =
    IsClipboardFormatAvailable(CF_TEXT) ||
    IsClipboardFormatAvailable(CF_UNICODETEXT) ||
    IsClipboardFormatAvailable(CF_HDROP)
    ? MF_ENABLED : MF_GRAYED;
  MFCM( IDM_PASTE, mflags, _W("&Paste "),
    clip ? W("Shift+Ins") : ct_sh ? W("Ctrl+Shift+V") : null
  );


  MFCM( IDM_SEARCH, 0, _W("S&earch"),
    alt_fn ? W("Alt+F3") : ct_sh ? W("Ctrl+Shift+H") : null
  );
  switch(wv.logging){    
    when 1:cap=_W("&Log to File[I]");
    when 2:cap=_W("&Log to File[O]");
    when 3:cap=_W("&Log to File[IO]");
    otherwise:
    cap=_W("&Log to File");
    mflags=MF_UNCHECKED;
  }
  MFCM( IDM_TOGLOG, mflags, cap, null);
  MFCM( IDM_TOGCHARINFO, CKED(show_charinfo ), _W("Character &Info"), null);

  MFCM( IDM_TOGVT220KB, CKED(term.vt220_keys), _W("VT220 Keyboard"), null);
  MFCM( IDM_RESET, 0, _W("&Reset"), 
        alt_fn ? W("Alt+F8") : ct_sh ? W("Ctrl+Shift+R") : null);

  mflags = IsZoomed(wv.wnd) || term.cols != cfg.cols || term.rows != cfg.rows ? MF_ENABLED : MF_GRAYED;
  MFCM( IDM_DEFSIZE_ZOOM, mflags, _W("&Default Size"), 
        alt_fn ? W("Alt+F10") : ct_sh ? W("Ctrl+Shift+D") : null);
  MFCM( IDM_TABBAR   , CKED(cfg.tab_bar_show)  , _W("Tabbar(&H)"),    null);

  mflags = CKED(term.show_scrollbar );
#ifdef allow_disabling_scrollbar
  if (!cfg.scrollbar)
    mflags |= MF_GRAYED;
#endif
  MFCM( IDM_SCROLLBAR, mflags       , _W("Scrollbar(&S)"), null);

  mflags=MF_CHECKED;
  switch(wv.border_style){    
    when 1:cap=_W("&Border[T]");
    when 2:cap=_W("&Border[N]");
    otherwise: cap=_W("&Border"); mflags=MF_UNCHECKED;
  }
  MFCM( IDM_BORDERS  , mflags, cap, null);
  MFCM( IDM_PARTLINE , CKED(term.usepartline), _W("PartLine(&K)"),  null);
  MFCM( IDM_INDICATOR, CKED(cfg.indicator)     , _W("Indicator(&J)"), null);
  MFCM( IDM_FULLSCREEN_ZOOM, CKED(wv.win_is_fullscreen), _W("&Full Screen"), 
        alt_fn ? W("Alt+F11") : ct_sh ? W("Ctrl+Shift+F") : null);

  MFCM( IDM_FLIPSCREEN, CKED(term.show_other_screen), _W("Flip Screen(&W)"), 
        alt_fn ? W("Alt+F12") : ct_sh ? W("Ctrl+Shift+S") : null);
  uint status_line = term.st_type == 1 ? MF_CHECKED
                   : term.st_type == 0 ? MF_UNCHECKED
                   : MF_GRAYED;
  //__ Context menu:
  MFCM( IDM_STATUSLINE, status_line,_W("Status Line"),null  );

  ENCM( IDM_OPTIONS, GYED(wv.config_wnd) );
  ENSM( IDM_OPTIONS, GYED(wv.config_wnd));

  // refresh remaining labels to facilitate (changed) localization
  MFSM( IDM_COPYTITLE, 0, _W("Copy T&Itle"), null);
  MFSM( IDM_OPTIONS, 0, _W("&Options..."), null);
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
      const wchar * label = _W(cmdp);
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
  void apcm(int id,int checked,wstring cap){
    if(id) {
      if(checked) AppendMenuW(ctxmenu, MF_ENABLED|MF_CHECKED  , id, cap);
      else AppendMenuW(ctxmenu, MF_ENABLED|MF_UNCHECKED, id, cap);
    } else AppendMenuW(ctxmenu, MF_SEPARATOR, 0, 0);
  }
  //__ Context menu:
  apcm( IDM_OPEN     ,0,_W("Ope&n"));
  apcm( IDM_NEWTAB   ,0, _W("New &Tab\twin+T"));
  apcm( IDM_KILLTAB  ,0, _W("&Kill tab\twin+w"));
  apcm( 0,0, 0);
  apcm( IDM_PREVTAB  ,0, _W("&Previous tab\twin+<-"));
  apcm( IDM_NEXTTAB  ,0, _W("&Next tab\twin+->"));
  apcm( IDM_MOVELEFT ,0, _W("Move to &Left\twin+<-"));
  apcm( IDM_MOVERIGHT,0, _W("Move to &Right\twin+->"));
  apcm( 0,0, 0);
  apcm( IDM_COPY,0, 0);
  if (extended_menu) {
    apcm( IDM_COPY_TEXT,0, _W("Copy as text"));
    apcm( IDM_COPY_TABS,0, _W("Copy with TABs"));
    apcm( IDM_COPY_RTF ,0, _W("Copy as RTF"));
    apcm( IDM_COPY_HTXT,0, _W("Copy as HTML text"));
    apcm( IDM_COPY_HFMT,0, _W("Copy as HTML"));
    apcm( IDM_COPY_HTML,0, _W("Copy as HTML full"));
  }
  apcm( IDM_PASTE, 0,0);
  if (extended_menu) {
    apcm( IDM_COPASTE, 0,0);
  }
  apcm( IDM_SELALL, 0,_W("Select &All"));
  apcm( IDM_SAVEIMG,0, _W("Save as &Image"));
  if (tek_mode) {
    apcm( 0, 0,0);
    apcm( IDM_TEKRESET,0, W("Tektronix RESET"));
    apcm( IDM_TEKPAGE,0, W("Tektronix PAGE"));
    apcm( IDM_TEKCOPY,0, W("Tektronix COPY"));

  }
  apcm( 0,0, 0);
  apcm( IDM_SEARCH,0, 0);
  if (extended_menu) {
    //__ Context menu: write terminal window contents as HTML file
    apcm( IDM_HTML,0, _W("HTML Screen Dump"));
    apcm( IDM_TOGLOG,0, 0);
    apcm( IDM_TOGCHARINFO,0, 0);
    apcm( IDM_TOGVT220KB,0, 0);
  }
  apcm( IDM_RESET_NOASK,0, 0);
  if (extended_menu) {
    //__ Context menu: clear scrollback buffer (lines scrolled off the window)
    apcm( IDM_CLRSCRLBCK,0, _W("Clear Scrollback"));
  }
  apcm( 0,0, 0);
  apcm( IDM_DEFSIZE_ZOOM   ,0,  0);
  apcm( IDM_BORDERS        ,0,  0);
  apcm( IDM_SCROLLBAR      ,0,  0);
  apcm( IDM_TABBAR         ,1,  0);
  apcm( IDM_PARTLINE       ,0,  0);
  apcm( IDM_INDICATOR      ,0,  0);
  apcm( IDM_FULLSCREEN_ZOOM,0,  0);
  apcm( IDM_FLIPSCREEN     ,0,  0);
  apcm( IDM_STATUSLINE     ,0,  0);
  apcm( 0,0, 0);
  if (extended_menu) {
    //__ Context menu: generate a TTY BRK condition (tty line interrupt)
    apcm( IDM_BREAK,0, _W("Send Break"));
    apcm( 0,0, 0);
  }

  if (with_user_commands && *cfg.ctx_user_commands) {
    append_commands(ctxmenu, cfg.ctx_user_commands, IDM_CTXMENUFUNCTION, false, false);
    apcm( 0,0, 0);
  }
  else if (with_user_commands && *cfg.user_commands) {
    append_commands(ctxmenu, cfg.user_commands, IDM_USERCOMMAND, false, false);
    apcm( 0,0, 0);
  }

  //__ Context menu:
  apcm( IDM_OPTIONS,0, _W("&Options..."));
}

void
win_init_menus(void)
{
#ifdef debug_modify_menu
  printf("win_init_menus\n");
#endif
  HMENU smenu;
  sysmenu = GetSystemMenu(wv.wnd, false);
  void insm(int id,wstring cap){
    if(id){
      if(id>0xF000) InsertMenuW(sysmenu, 0,MF_BYPOSITION|MF_POPUP,   id, cap);
      else          InsertMenuW(sysmenu, 0,MF_BYPOSITION|MF_ENABLED, id, cap);
    }else{
      InsertMenuW(sysmenu, 0,MF_BYPOSITION|MF_SEPARATOR, 0, 0);
    }
  }
  void apsm(int id,wstring cap){
    AppendMenuW(smenu,  MF_ENABLED, id, cap);
  }

  insm( 0, 0);

  insm( IDM_MOVERIGHT , _W("Move to &Left\twin+<-")); 
  insm( IDM_MOVELEFT  , _W("Move to &Right\twin+->"));
  insm( IDM_NEXTTAB   , _W("&Next tab\twin+->"));
  insm( IDM_PREVTAB   , _W("&Previous tab\twin+<-"));

  insm( IDM_KILLTAB   ,_W("&Kill tab\twin+w"));
  insm( IDM_NEWTAB    , _W("New &Tab\twin+T"));
  insm( IDM_NEW       , 0);
  if (*cfg.sys_user_commands)
    append_commands(sysmenu, cfg.sys_user_commands, IDM_SYSMENUFUNCTION, false, true);
  else {
    insm( IDM_COPYTITLE , _W("Copy T&Itle"));
    insm( IDM_OPTIONS   , _W("&Options..."));
  }
  insm( 0, 0);
  smenu = CreatePopupMenu();
  apsm( IDM_NEWWSLW, _W("WS&L"         ));
  apsm( IDM_NEWCYGW, _W("&Cygwin"      ));
  apsm( IDM_NEWCMDW, _W("C&MD"         ));
  apsm( IDM_NEWPSHW, _W("&PowerShell"  ));
  apsm( IDM_NEWUSRW, _W("&Faststart"   ));
  insm((UINT_PTR)(smenu), _W("N&Ew Win"));
  smenu = CreatePopupMenu();
  apsm( IDM_NEWWSLT, _W("WS&L"         ));
  apsm( IDM_NEWCYGT, _W("&Cygwin"      ));
  apsm( IDM_NEWCMDT, _W("C&MD"         ));
  apsm( IDM_NEWPSHT, _W("&PowerShell"  ));
  apsm( IDM_NEWUSRT, _W("&Faststart"   ));
  insm( (UINT_PTR)(smenu), _W("New T&Ab"));
  if(0){
    DeleteMenu(sysmenu,SC_RESTORE ,0);
    DeleteMenu(sysmenu,SC_MOVE    ,0);
    DeleteMenu(sysmenu,SC_SIZE    ,0);
    DeleteMenu(sysmenu,SC_MINIMIZE,0);
    DeleteMenu(sysmenu,SC_MAXIMIZE,0);
  }

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
    ClientToScreen(wv.wnd, &p);
  }
  else
    GetCursorPos(&p);

  TrackPopupMenu
  (
    ctxmenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
    p.x, p.y, 0, wv.wnd, null
  );
}
void 
win_popup_menu(uint key,mod_keys mods)
{
	(void)key;
  open_popup_menu(false, null, mods);
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
static alt_state_t old_alt_state;
static uint alt_code;
static bool alt_uni;

static bool lctrl;  // Is left Ctrl pressed?
static int lctrl_time;

static int win_key_menu(uint key,mod_keys mods)
{
	(void)key;
  if (*cfg.key_menu)return 0; 
  if (mods&MDK_SHIFT)
    send_syscommand(SC_KEYMENU);
  else {
    win_show_mouse();
    open_popup_menu(true, null, mods);
  }
  alt_state = ALT_NONE;
  return true;
}


mod_keys
get_mods(void)
{
  inline bool is_key_down(uchar vk) { return GetKeyState(vk) & 0x80; }
  lctrl_time = 0;
  lctrl = is_key_down(VK_LCONTROL) && (lctrl || !is_key_down(VK_RMENU));
  bool super = super_key && is_key_down(super_key);
  bool hyper = hyper_key && is_key_down(hyper_key);
  return
    is_key_down(VK_SHIFT) * MDK_SHIFT
    | is_key_down(VK_MENU) * MDK_ALT
    | (lctrl | is_key_down(VK_RCONTROL)) * MDK_CTRL
    | (is_key_down(VK_LWIN) | is_key_down(VK_RWIN)) * MDK_WIN
    | super * MDK_SUPER
    | hyper * MDK_HYPER
    ;
}


/* Mouse handling */

static void
update_mouse(mod_keys mods)
{
  static bool last_app_mouse = false;

  // unhover (end hovering) if hover modifiers are withdrawn
  if (term.hovering && (char)(mods & ~cfg.click_target_mod) != cfg.opening_mod) {
    term.hovering = false;
    win_update(false);
  }

  bool new_app_mouse =
    (term.mouse_mode || term.locator_1_enabled)
    // disable app mouse pointer while showing "other" screen (flipped)
    && !term.show_other_screen
    // disable app mouse pointer while not targetting app
    && (cfg.clicks_target_app ^ ((mods & cfg.click_target_mod) != 0));

  if (new_app_mouse != last_app_mouse) {
    //HCURSOR cursor = LoadCursor(null, new_app_mouse ? IDC_ARROW : IDC_IBEAM);
    HCURSOR cursor = win_get_cursor(new_app_mouse);
    SetClassLongPtr(wv.wnd, GCLP_HCURSOR, (LONG_PTR)cursor);
    SetCursor(cursor);
    last_app_mouse = new_app_mouse;
  }
}

void
win_update_mouse(void)
{ update_mouse(get_mods()); }

void
win_capture_mouse(void)
{ SetCapture(wv.wnd); }

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
  if (term.hide_mouse && mouse_showing && GetCursorPos(&p) && WindowFromPoint(p) == wv.wnd) {
    ShowCursor(false);
    mouse_showing = false;
  }
}

static pos
translate_pos(int x, int y)
{
  int rows = term.rows;
  if (term.st_active) {
    rows = term.st_rows;
    y = max(0, y - term.rows * wv.cell_height);
  }
  return (pos){
    .x = floorf((x - PADDING) / (float)wv.cell_width),
    .y = floorf((y - PADDING - OFFSET) / (float)wv.cell_height),
    .pix = min(max(0, x - PADDING), term.cols * wv.cell_width - 1),
    .piy = min(max(0, y - PADDING - OFFSET), rows * wv.cell_height - 1),
    .r = (cfg.elastic_mouse && !term.mouse_mode)
         ? (x - PADDING) % wv.cell_width > wv.cell_width / 2
         : 0
  };
}

pos last_pos = {-1, -1, -1, -1, false};
static LPARAM last_lp = -1;
static int button_state = 0;

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
  bool click_focus = wv.click_focus_token;
  wv.click_focus_token = false;

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

  SetFocus(wv.wnd);  // in case focus was in search bar

  bool res = false;

  if (click_focus && b == MBT_LEFT && count == 1
      && // not in application mouse mode
         !(term.mouse_mode && term.report_focus &&
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
    if (ScreenToClient(wv.wnd, &p)) {
      if (p.x < PADDING)
        p.x = 0;
      else
        p.x -= PADDING;
      if (p.x >= term.cols * wv.cell_width)
        p.x = term.cols * wv.cell_width - 1;
      if (p.y < OFFSET + PADDING)
        p.y = 0;
      else
        p.y -= OFFSET + PADDING;
      if (term.st_active) {
        p.y = max(0, p.y - term.rows * wv.cell_height);
        if (p.y >= term.st_rows * wv.cell_height)
          p.y = term.st_rows * wv.cell_height - 1;
      }
      else if (p.y >= term.rows * wv.cell_height)
        p.y = term.rows * wv.cell_height - 1;

      if (by_pixels) {
        *x = p.x;
        *y = p.y;
      } else {
        *x = floorf(p.x / (float)wv.cell_width);
        *y = floorf(p.y / (float)wv.cell_height);
      }
    }
  }

  *buttons = button_state;
}
struct KNVDef{
  uint key;
  char*name;
}knvdef[]={
#define DKNU(n,k) {VK_##k,#n}
#define DKND(n) {VK_##n,#n}
  DKND(LBUTTON            ),
  DKND(RBUTTON            ),
  DKND(CANCEL             ),
  DKND(MBUTTON            ),
  DKND(XBUTTON1           ),
  DKND(XBUTTON2           ),
  DKND(BACK               ),
  DKND(TAB                ),
  DKND(CLEAR              ),
  DKND(RETURN             ),
  DKND(SHIFT              ),
  DKND(CONTROL            ),
  DKND(MENU               ),
  DKND(PAUSE              ),
  DKND(CAPITAL            ),
  DKND(KANA               ),
  DKND(HANGEUL            ),
  DKND(HANGUL             ),
  DKND(IME_ON             ),
  DKND(JUNJA              ),
  DKND(FINAL              ),
  DKND(HANJA              ),
  DKND(KANJI              ),
  DKND(IME_OFF            ),
  DKND(ESCAPE             ),
  DKND(CONVERT            ),
  DKND(NONCONVERT         ),
  DKND(ACCEPT             ),
  DKND(MODECHANGE         ),
  DKND(SPACE              ),
  DKND(PRIOR              ),
  DKND(NEXT               ),
  DKND(END                ),
  DKND(HOME               ),
  DKND(LEFT               ),
  DKND(UP                 ),
  DKND(RIGHT              ),
  DKND(DOWN               ),
  DKND(SELECT             ),
  DKND(PRINT              ),
  DKND(EXECUTE            ),
  DKND(SNAPSHOT           ),
  DKND(INSERT             ),
  DKND(DELETE             ),
  DKND(HELP               ),
  DKND(LWIN               ),
  DKND(RWIN               ),
  DKND(APPS               ),
  DKND(SLEEP              ),
  DKND(NUMPAD0            ),
  DKND(NUMPAD1            ),
  DKND(NUMPAD2            ),
  DKND(NUMPAD3            ),
  DKND(NUMPAD4            ),
  DKND(NUMPAD5            ),
  DKND(NUMPAD6            ),
  DKND(NUMPAD7            ),
  DKND(NUMPAD8            ),
  DKND(NUMPAD9            ),
  DKND(MULTIPLY           ),
  DKND(ADD                ),
  DKND(SEPARATOR          ),
  DKND(SUBTRACT           ),
  DKND(DECIMAL            ),
  DKND(DIVIDE             ),
  DKND(F1                 ),
  DKND(F2                 ),
  DKND(F3                 ),
  DKND(F4                 ),
  DKND(F5                 ),
  DKND(F6                 ),
  DKND(F7                 ),
  DKND(F8                 ),
  DKND(F9                 ),
  DKND(F10                ),
  DKND(F11                ),
  DKND(F12                ),
  DKND(F13                ),
  DKND(F14                ),
  DKND(F15                ),
  DKND(F16                ),
  DKND(F17                ),
  DKND(F18                ),
  DKND(F19                ),
  DKND(F20                ),
  DKND(F21                ),
  DKND(F22                ),
  DKND(F23                ),
  DKND(F24                ),
  DKND(NUMLOCK            ),
  DKND(SCROLL             ),
  DKND(OEM_NEC_EQUAL      ),
  DKND(OEM_FJ_JISHO       ),
  DKND(OEM_FJ_MASSHOU     ),
  DKND(OEM_FJ_TOUROKU     ),
  DKND(OEM_FJ_LOYA        ),
  DKND(OEM_FJ_ROYA        ),
  DKND(LSHIFT             ),
  DKND(RSHIFT             ),
  DKND(LCONTROL           ),
  DKND(RCONTROL           ),
  DKND(LMENU              ),
  DKND(RMENU              ),
  DKND(BROWSER_BACK       ),
  DKND(BROWSER_FORWARD    ),
  DKND(BROWSER_REFRESH    ),
  DKND(BROWSER_STOP       ),
  DKND(BROWSER_SEARCH     ),
  DKND(BROWSER_FAVORITES  ),
  DKND(BROWSER_HOME       ),
  DKND(VOLUME_MUTE        ),
  DKND(VOLUME_DOWN        ),
  DKND(VOLUME_UP          ),
  DKND(MEDIA_NEXT_TRACK   ),
  DKND(MEDIA_PREV_TRACK   ),
  DKND(MEDIA_STOP         ),
  DKND(MEDIA_PLAY_PAUSE   ),
  DKND(LAUNCH_MAIL        ),
  DKND(LAUNCH_MEDIA_SELECT),
  DKND(LAUNCH_APP1        ),
  DKND(LAUNCH_APP2        ),
  DKND(OEM_1              ),
  DKND(OEM_PLUS           ),
  DKND(OEM_COMMA          ),
  DKND(OEM_MINUS          ),
  DKND(OEM_PERIOD         ),
  DKND(OEM_2              ),
  DKND(OEM_3              ),
  DKND(OEM_4              ),
  DKND(OEM_5              ),
  DKND(OEM_6              ),
  DKND(OEM_7              ),
  DKND(OEM_8              ),
  DKND(OEM_AX             ),
  DKND(OEM_102            ),
  DKND(ICO_HELP           ),
  DKND(ICO_00             ),
  DKND(PROCESSKEY         ),
  DKND(ICO_CLEAR          ),
  DKND(PACKET             ),
  DKND(OEM_RESET          ),
  DKND(OEM_JUMP           ),
  DKND(OEM_PA1            ),
  DKND(OEM_PA2            ),
  DKND(OEM_PA3            ),
  DKND(OEM_WSCTRL         ),
  DKND(OEM_CUSEL          ),
  DKND(OEM_ATTN           ),
  DKND(OEM_FINISH         ),
  DKND(OEM_COPY           ),
  DKND(OEM_AUTO           ),
  DKND(OEM_ENLW           ),
  DKND(OEM_BACKTAB        ),
  DKND(ATTN               ),
  DKND(CRSEL              ),
  DKND(EXSEL              ),
  DKND(EREOF              ),
  DKND(PLAY               ),
  DKND(ZOOM               ),
  DKND(NONAME             ),
  DKND(PA1                ),
  DKND(OEM_CLEAR          ),
  DKNU(BREAK      ,CANCEL ),
  DKNU(ENTER      ,RETURN ),
  DKNU(ESC        ,ESCAPE ),
  DKNU(PRINTSCREEN,SNAPSHOT),
  DKNU(MENU       ,APPS   ),
  DKNU(CAPSLOCK   ,CAPITAL),
  DKNU(SCROLLLOCK ,SCROLL ),
  DKNU(EXEC       ,EXECUTE),
  DKNU(BEGIN      ,CLEAR  ),
#if _WIN32_WINNT >= 0x0604
  //  DKND(NAVIGATION_VIEW    ),
  //  DKND(NAVIGATION_MENU    ),
  //  DKND(NAVIGATION_UP      ),
  //  DKND(NAVIGATION_DOWN    ),
  //  DKND(NAVIGATION_LEFT    ),
  //  DKND(NAVIGATION_RIGHT   ),
  //  DKND(NAVIGATION_ACCEPT  ),
  //  DKND(NAVIGATION_CANCEL  ),
#endif
#if _WIN32_WINNT >= 0x0604
  //  DKND(GAMEPAD_A                         ),
  //  DKND(GAMEPAD_B                         ),
  //  DKND(GAMEPAD_X                         ),
  //  DKND(GAMEPAD_Y                         ),
  //  DKND(GAMEPAD_RIGHT_SHOULDER            ),
  //  DKND(GAMEPAD_LEFT_SHOULDER             ),
  //  DKND(GAMEPAD_LEFT_TRIGGER              ),
  //  DKND(GAMEPAD_RIGHT_TRIGGER             ),
  //  DKND(GAMEPAD_DPAD_UP                   ),
  //  DKND(GAMEPAD_DPAD_DOWN                 ),
  //  DKND(GAMEPAD_DPAD_LEFT                 ),
  //  DKND(GAMEPAD_DPAD_RIGHT                ),
  //  DKND(GAMEPAD_MENU                      ),
  //  DKND(GAMEPAD_VIEW                      ),
  //  DKND(GAMEPAD_LEFT_THUMBSTICK_BUTTON    ),
  //  DKND(GAMEPAD_RIGHT_THUMBSTICK_BUTTON   ),
  //  DKND(GAMEPAD_LEFT_THUMBSTICK_UP        ),
  //  DKND(GAMEPAD_LEFT_THUMBSTICK_DOWN      ),
  //  DKND(GAMEPAD_LEFT_THUMBSTICK_RIGHT     ),
  //  DKND(GAMEPAD_LEFT_THUMBSTICK_LEFT      ),
  //  DKND(GAMEPAD_RIGHT_THUMBSTICK_UP       ),
  //  DKND(GAMEPAD_RIGHT_THUMBSTICK_DOWN     ),
  //  DKND(GAMEPAD_RIGHT_THUMBSTICK_RIGHT    ),
  //  DKND(GAMEPAD_RIGHT_THUMBSTICK_LEFT     ),
#endif 
  {0,0}
#undef DKNU 
#undef DKND 
};
static string
vk_name(uint key){
  struct KNVDef*p;
  for(p=knvdef;p->name;p++){
    if(p->key==key)return p->name;
  }
  static char vk_name[3];
  sprintf(vk_name, "%02X", key & 0xFF);
  return vk_name;
}
static int getvk(const char *n){
  struct KNVDef*p;
  if(n[1]==0){
    return (int)(unsigned char)n[0];
  }
  if(n[0]=='\\'){
    int iv;
    if((n[1]|0x20)=='x'){
      sscanf(n+2,"%x",&iv);
    }else iv=atoi(n+2);
    return iv;
  } 
  for(p=knvdef;p->name;p++){
    if(strcasecmp(p->name,n)==0)return p->key;
  }
  return -1;
}

typedef enum {
  COMP_CLEAR = -1,
  COMP_NONE = 0,
  COMP_PENDING = 1, COMP_ACTIVE = 2
} comp_state_t;
static comp_state_t comp_state = COMP_NONE;

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

wchar *
char_code_indication(uint * what)
{
static wchar cci_buf[13];

  if (alt_state > ALT_ALONE) {
    int ac = alt_code;
    int i = lengthof(cci_buf);
    cci_buf[--i] = 0;
    do {
      int digit = ac % alt_state;
      cci_buf[--i] = digit > 9 ? digit - 10 + 'A' : digit + '0';
      ac /= alt_state;
    } while (ac && i);
    if (alt_state == ALT_HEX && alt_uni && i > 1) {
      cci_buf[--i] = '+';
      cci_buf[--i] = 'U';
    }
    *what = alt_state;
    return &cci_buf[i];
  }
  else if (alt_state == ALT_ALONE) {
    *what = 4;
    //return W(" ");
    return 0;  // don't obscure text when just pressing Alt
  }
  else if (comp_state > COMP_NONE) {
    int i;
    for (i = 0; i < compose_buflen; i++)
      cci_buf[i] = compose_buf[i];
    cci_buf[i++] = ' ';
    cci_buf[i] = 0;
    *what = 2;
    return cci_buf;
  }
  else
    return 0;
}

// notify margin bell ring enabled
void
provide_input(wchar c1)
{
  if (term.margin_bell && c1 != '\e')
    term.ring_enabled = true;
}

#define no_debug_virtual_key_codes
#define no_debug_key
#define no_debug_alt
#define no_debug_compose

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
  if (!cfg.manage_leds || (cfg.manage_leds < 4 && vk == VK_SCROLL))
    return 0;

#ifdef heuristic_detection_of_ScrollLock_auto_repeat_glitch
  if (vk == VK_SCROLL) {
    int st = GetKeyState(VK_SCROLL);
    //printf("win_key_nullify st %d key %d\n", term.no_scroll || term.scroll_mode, st);
    // heuristic detection of race condition with auto-repeat
    // without setting KeyFunctions=ScrollLock:toggle-no-scroll;
    // handled in common with heuristic compensation in win_key_up
    if ((st & 1) == (term.no_scroll || term.scroll_mode)) {
      return 0;  // nothing sent
    }
  }
#endif

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
pick_key_function(wstring key_commands,const char * tag, int n, uint key, mod_keys mods, mod_keys mod0, uint scancode)
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
  mod_keys tagmods(const char * k)
  {
    mod_keys m = mod0;
    const char * sep = strrchr(k, '+');
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
  const char * tag0 = tag ? strchr(tag, '+') : 0;
  if (tag0)
    tag0++;
  else
    tag0 = tag;

#if defined(debug_def_keys) && debug_def_keys > 0
  printf("key_fun tag <%s> tag0 <%s> mod %X\n", tag ?: "(null)", tag0 ?: "(null)", mod_tag);
#endif

  int ret = false;

  char * paramp;
  while ((tag || n >= 0) && (paramp = strchr(cmdp, ':'))) {
    ret = false;

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
        else {
          if (key == VK_SCROLL) {
#ifdef debug_vk_scroll
            printf("pick VK_SCROLL\n");
#endif
            sync_scroll_lock(term.no_scroll || term.scroll_mode);
          }
          else
            win_key_nullify(key);
        }
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
                  child_sendw(&term,&fct[1], wcslen(fct) - 2);
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
                    child_send(&term,cc, 2);
                  }
                  else {
                    provide_input(cc[1]);
                    child_send(&term,&cc[1], 1);
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
                child_send(&term,buf, len);
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
#ifdef common_return_handling
#warning produces bad behaviour; appends "~" input
        break;
#endif
        free(ukey_commands);

        if (key == VK_SCROLL) {
#ifdef debug_vk_scroll
          printf("pick VK_SCROLL break scn %d ret %d\n", scancode, ret);
#endif
          if (scancode && ret == true /*sic!*/)
            // don't call this if ret == -1
            sync_scroll_lock(term.no_scroll || term.scroll_mode);
        }

        return ret;
      }
    }
    else if (key == VK_CAPITAL && cfg.compose_key == MDK_CAPSLOCK) {
      // support config ComposeKey=capslock:
      // nullify the keyboard state lock effect, see above,
      // so we can support config ComposeKey=capslock;
      // avoid the recursion with fake events that have scancode 0
      if (scancode)
        win_key_nullify(key);
      else
        return false;
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

#ifdef debug_vk_scroll
  if (key == VK_SCROLL)
    printf("pick VK_SCROLL return\n");
#endif
#ifdef common_return_handling
  // try to set ScrollLock keyboard LED consistently
#warning interferes with key functions (see above); does not work anyway
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
      if (ret != true)
        win_key_nullify(key);
    }
#endif

  return false;
}

void
user_function(wstring commands, int n)
{
  pick_key_function(commands, 0, n, 0, 0, 0, 0);
}

static void
insert_alt_code(void)
{
  if (cs_cur_max < 4 && !alt_uni) {
    char buf[4];
    int pos = sizeof buf;
    do
      buf[--pos] = alt_code;
    while (alt_code >>= 8);
    provide_input(buf[pos]);
    child_send(&term,buf + pos, sizeof buf - pos);
  }
  else if (alt_code < 0x10000) {
    wchar wc = alt_code;
    if (wc < 0x20)
      MultiByteToWideChar(CP_OEMCP, MB_USEGLYPHCHARS,
                          (char[]){wc}, 1, &wc, 1);
    provide_input(wc);
    child_sendw(&term,&wc, 1);
  }
  else {
    xchar xc = alt_code;
    provide_input(' ');
    child_sendw(&term,(wchar[]){high_surrogate(xc), low_surrogate(xc)}, 2);
  }
  compose_clear();
}

// The ToUnicode function for converting keyboard states to characters may
// return multiple wchars due to dead keys and ligatures defined in the
// keyboard layout. The latter aren't limited to actual ligatures but can be any
// sequence of wchars.
//
// Unfortunately MSDN doesn't define a maximum length.
//
// The semi-official limit is four:
// http://www.siao2.com/2015/08/07/8770668856267196989.aspx
//
// However, KbdEdit supports up to nine:
// http://www.kbdedit.com/manual/high_level_ligatures.html
//
// And in this ill-tempered thread on unicode.org, it was found that ligatures
// can be up to sixteen wchars long:
// https://www.unicode.org/mail-arch/unicode-ml/y2015-m08/0023.html
//
// So let's go with the biggest number.
#define TO_UNICODE_MAX 16

void
win_csi_seq(const char * pre, const char * suf)
{
  mod_keys mods = get_mods();
  inline bool is_key_down(uchar vk) { return GetKeyState(vk) & 0x80; }
  bool super = super_key && is_key_down(super_key);
  bool hyper = hyper_key && is_key_down(hyper_key);
  mods |= super * MDK_SUPER | hyper * MDK_HYPER;

  if (mods)
    child_printf(&term,"\e[%s;%u%s", pre, mods + 1, suf);
  else
    child_printf(&term,"\e[%s%s", pre, suf);
}

// simulate a key press/release sequence
static int
win_key_fake(int vk)
{
  if (!cfg.manage_leds || (cfg.manage_leds < 4 && vk == VK_SCROLL))
    return 0;

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
#define dont_debug_key_state
#ifdef debug_key_state
  uchar kbd[256];
  GetKeyboardState(kbd);
  printf("do_win_key_toggle %02X %d (st %02X as %02X kb %02X)\n", vk, on, st, ast, kbd[vk]);
#endif
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


bool
get_scroll_lock(void)
{
  return GetKeyState(VK_SCROLL);
}

void
sync_scroll_lock(bool locked)
{
  //win_led(3, term.no_scroll);
  //do_win_key_toggle(VK_SCROLL, locked);
  int st = GetKeyState(VK_SCROLL);
  //printf("sync_scroll_lock %d key %d\n", locked, st);
  if (st ^ locked)
    win_key_fake(VK_SCROLL);
}
static void clear_scroll_lock();
static void cycle_transparency(void);
static void set_transparency(int t);
static int packmod(int mods);
static int kb_select(uint key, mod_keys mods);
static void cycle_pointer_style();
static int  sckmask[256]={0};
static int  sckwmask[256]={0};
int sckw_run(uint key,int moda);
int sck_run(uint key,int moda);

static int previous_transparency;
static bool transparency_tuned;

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

  (void)vk_name;
#ifdef debug_virtual_key_codes
  printf("win_key_down %02X %s scan %d ext %d rpt %d/%d other %02X\n", key, vk_name(key), scancode, extended, repeat, count, HIWORD(lp) >> 8);
#endif

  static LONG last_key_time = 0;

  LONG message_time = GetMessageTime();
  if (repeat) {
    if (!term.auto_repeat)
      return true;
    if (term.repeat_rate &&
        message_time - last_key_time < 1000 / term.repeat_rate)
      return true;
  }
  if (repeat && term.repeat_rate &&
      message_time - last_key_time < 2000 / term.repeat_rate)
    /* Key repeat seems to be continued. */
    last_key_time += 1000 / term.repeat_rate;
  else
    last_key_time = message_time;

  if (key == VK_PROCESSKEY) {
    MSG msg={.hwnd = wv.wnd, .message = WM_KEYDOWN, .wParam = wp, .lParam = lp};
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
  if (key == VK_MENU) {
    if (!repeat && mods == MDK_ALT && alt_state == ALT_NONE)
      alt_state = ALT_ALONE;
    return true;
  }
  old_alt_state = alt_state;
  if (alt_state > ALT_NONE)
    alt_state = ALT_CANCELLED;

  // Workaround for Windows clipboard history pasting simply injecting Ctrl+V
  // (mintty/wsltty#139)
  switch(wv.tabctrling){
      when 0:
        if(key==VK_CONTROL){
          wv.tabctrling=1;
          wv.last_tabk_time=message_time ;
        }
      when 1:
        if(key==VK_CONTROL){
          if( message_time - wv.last_tabk_time < 500 ){
            wv.tabctrling=2;
            wv.last_tabk_time=message_time ;
            return 1;
          }else{
            wv.tabctrling=1;
            wv.last_tabk_time=message_time ;
          }
        }else{
          wv.tabctrling=0;
        } 
      when 2:
        if(key=='A'){
          if( message_time - wv.last_tabk_time < 500 ){
            wv.tabctrling=3;
            wv.last_tabk_time=message_time ;
            return 1;
          }else{
            wv.tabctrling=0;
            wv.last_tabk_time=message_time ;
          }
        }else{
          wv.tabctrling=0;
        } 
      when 3:
        if(message_time - wv.last_tabk_time< 5000 ){
          int res=1;
          int zoom=-10000;
          switch (key) {
            when VK_LEFT :  
                if (shift) win_tab_move(-1);
                else win_tab_change(-1);
            when VK_RIGHT:  
                if (shift) win_tab_move(1);
                else win_tab_change(1);
            when '1' ... '9': win_tab_go(key-'1');
            
            when ' ': kb_select(0,mods); wv.tabctrling=0;
            when 'A': term_select_all();
            when 'C': term_copy();
            when 'V': win_paste();
            when 'I': term_save_image(0);
            
            when 'D': send_syscommand(IDM_DEFSIZE);
            when 'F': send_syscommand(cfg.zoom_font_with_window ? IDM_FULLSCREEN_ZOOM : IDM_FULLSCREEN);
            when 'R': send_syscommand(IDM_RESET_NOASK);
            when 'S': send_syscommand(IDM_SEARCH);
            when 'T': new_tab_def();
            when 'W': win_close();
            when 'N': send_syscommand(IDM_NEW);
            when 'O': win_open_config();
            when 'M': open_popup_menu(true, "Wb|l|s", mods);

            when 'B': win_tog_scrollbar();
            when 'H': win_tab_show();
            when 'J': win_tab_indicator();
            when 'K': win_tog_partline();

            when 'L': LoadConfig();
            when 'P': cycle_pointer_style();
            //when 'E':
            //when 'G':
            //when 'Q':
            //when 'U':
            //when 'X':
            //when 'Y':
            //when 'Z':
            when VK_SUBTRACT:  zoom = -1;
            when VK_ADD:       zoom = 1;
            when VK_NUMPAD0:   zoom = 0;
            when VK_OEM_MINUS: zoom = -1; mods &= ~MDK_SHIFT;
            when VK_OEM_PLUS:  zoom = 1; mods &= ~MDK_SHIFT;
            when '0':          zoom = 0;
            when VK_SHIFT :    res=-1;
            when VK_ESCAPE: res=-1;wv.tabctrling=0;
            otherwise: res=0;
          }
          if(zoom>=-1){
            win_zoom_font(zoom, mods & MDK_SHIFT);
            return true;
          }
          wv.last_tabk_time=message_time ;
          if(res>0)return 1;
        }else wv.tabctrling=0;
  }
  if(!scancode){
    if (key == 'V' && mods == MDK_CTRL ) {
      win_paste(); 
      return true;
    }
  }
  if(key_special|(tek_mode -TEKMODE_GIN)){
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
                          win_update_transparency(TR_GLASS, false);
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
        otherwise: transparency_pending = 0;key_special&=~KS_TRANP;
      }
#ifdef debug_transparency
      printf("==%d\n", transparency_pending);
#endif
      if (transparency_pending) {
        transparency_tuned = true;
        return true;
      }
    }
    if (term.selection_pending) {
      bool sel_adjust = false;
      //WPARAM scroll = 0;
      int sbtop = -sblines();
      int sbbot = term_last_nonempty_line();
      int oldisptop = term.disptop;
      //printf("y %d disptop %d sb %d..%d\n", term.sel_pos.y, term.disptop, sbtop, sbbot);
      switch (key) {
        when VK_CLEAR:
          // re-anchor keyboard selection
          term.sel_anchor = term.sel_pos;
          term.sel_start = term.sel_pos;
          term.sel_end = term.sel_pos;
          term.sel_rect = mods & MDK_ALT;
          sel_adjust = true;
        when VK_LEFT:
          if (term.sel_pos.x > 0)
            term.sel_pos.x--;
          sel_adjust = true;
        when VK_RIGHT:
          if (term.sel_pos.x < term.cols)
            term.sel_pos.x++;
          sel_adjust = true;
        when VK_UP:
          if (term.sel_pos.y > sbtop) {
            if (term.sel_pos.y <= term.disptop)
              term_scroll(0, -1);
            term.sel_pos.y--;
            sel_adjust = true;
          }
        when VK_DOWN:
          if (term.sel_pos.y < sbbot) {
            if (term.sel_pos.y + 1 >= term.disptop + term.rows)
              term_scroll(0, +1);
            term.sel_pos.y++;
            sel_adjust = true;
          }
        when VK_PRIOR:
          //scroll = SB_PAGEUP;
          term_scroll(0, -max(1, term.rows - 1));
          term.sel_pos.y += term.disptop - oldisptop;
          sel_adjust = true;
        when VK_NEXT:
          //scroll = SB_PAGEDOWN;
          term_scroll(0, +max(1, term.rows - 1));
          term.sel_pos.y += term.disptop - oldisptop;
          sel_adjust = true;
        when VK_HOME:
          //scroll = SB_TOP;
          term_scroll(+1, 0);
          term.sel_pos.y += term.disptop - oldisptop;
          term.sel_pos.y = sbtop;
          term.sel_pos.x = 0;
          sel_adjust = true;
        when VK_END:
          //scroll = SB_BOTTOM;
          term_scroll(-1, 0);
          term.sel_pos.y += term.disptop - oldisptop;
          term.sel_pos.y = sbbot;
          if (sbbot < term.rows) {
            termline *line = term.lines[sbbot];
            if (line)
              for (int j = line->cols - 1; j > 0; j--) {
                term.sel_pos.x = j + 1;
                if (!termchars_equal(&line->chars[j], &term.erase_char))
                  break;
              }
          }
          sel_adjust = true;
        when VK_INSERT or VK_RETURN:  // copy
          term_copy();
          term.selection_pending = false;
          key_special &= ~KS_TERMSEL;
        when VK_DELETE or VK_ESCAPE:  // abort
          term.selection_pending = false;
          key_special &= ~KS_TERMSEL;
        otherwise:
          //term.selection_pending = false;
          //key_special &= ~KS_TERMSEL;
          win_bell(&cfg);
      }
      //if (scroll) {
      //  if (!term.app_scrollbar)
      //    SendMessage(wv.wnd, WM_VSCROLL, scroll, 0);
      //  sel_adjust = true;
      //}
      if (sel_adjust) {
        if (term.sel_rect) {
          term.sel_start.y = min(term.sel_anchor.y, term.sel_pos.y);
          term.sel_start.x = min(term.sel_anchor.x, term.sel_pos.x);
          term.sel_end.y = max(term.sel_anchor.y, term.sel_pos.y);
          term.sel_end.x = max(term.sel_anchor.x, term.sel_pos.x);
        }
        else if (posle(term.sel_anchor, term.sel_pos)) {
          term.sel_start = term.sel_anchor;
          term.sel_end = term.sel_pos;
        }
        else {
          term.sel_start = term.sel_pos;
          term.sel_end = term.sel_anchor;
        }
        //printf("->sel %d:%d .. %d:%d\n", term.sel_start.y, term.sel_start.x, term.sel_end.y, term.sel_end.x);
        term.selected = true;
        win_update(true);
      }
      if (term.selection_pending)
        return true;
      else
        term.selected = false;
      return true;
    }
  }
  bool allow_shortcut = true;
  if (!term.shortcut_override && old_alt_state <= ALT_ALONE) {
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
    if (cfg.format_other_keys)
      // xterm "formatOtherKeys: 1": CSI 64 ; 2 u
      len = sprintf(buf, "\e[%u;%uu", c, mods + 1);
    else
      // xterm "formatOtherKeys: 0": CSI 2 7 ; 2 ; 64 ~
      len = sprintf(buf, "\e[27;%u;%u~", mods + 1, c);
  }
  void app_pad_code(char c) {
    void mod_appl_xterm(char c) {len = sprintf(buf, "\eO%u%c", mods + 1, c);}
    if (mods && term.app_keypad) switch (key) {
      when VK_DIVIDE or VK_MULTIPLY or VK_SUBTRACT or VK_ADD or VK_RETURN:
        mod_appl_xterm(c - '0' + 'p');
        return;
    }
    if (term.vt220_keys && mods && term.app_keypad) switch (key) {
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
    if (old_alt_state > ALT_ALONE) {
      alt_state = old_alt_state;  // stay in alt_state, process key
      if (digit >= 0 && digit < alt_state) {
        alt_code = alt_code * alt_state + digit;
        if ((signed int)alt_code < 0 || alt_code > 0x10FFFF) {
          win_bell(&cfg);
          alt_state = ALT_NONE;
        }
        else
          win_update(false);
      }
      else
        win_bell(&cfg);
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

  bool alt_code_ignore(void) {
    if (old_alt_state > ALT_ALONE) {
      alt_state = old_alt_state;  // keep alt_state, ignore key
      win_bell(&cfg);
      return true;
    }
    else
      return false;
  }

  bool app_pad_key(char symbol) {
    if (extended)
      return false;
    // Mintty-specific: produce app_pad codes not only when vt220 mode is on,
    // but also in PC-style mode when app_cursor_keys is off, to allow the
    // numpad keys to be distinguished from the cursor/editing keys.
    if (term.app_keypad && (!term.app_cursor_keys || term.vt220_keys)) {
      // If NumLock is on, Shift must have been pressed to override it and
      // get a VK code for an editing or cursor key code.
      if (numlock)
        mods |= MDK_SHIFT;
      app_pad_code(symbol);
      return true;
    }
    if (symbol == '.')
      return alt_code_ignore();
    else
      return alt_code_numpad_key(symbol - '0');
  }

  void edit_key(uchar code, char symbol) {
    if (!app_pad_key(symbol)) {
      if (code != 3 || ctrl || alt || shift || !term.delete_sends_del)
        tilde_code(code);
      else
        ch(CDEL);
    }
  }

  void cursor_key(char code, char symbol) {
    if (term.vt52_mode)
      len = sprintf(buf, "\e%c", code);
    else if (!app_pad_key(symbol))
      mods ? mod_csi(code) : term.app_cursor_keys ? ss3(code) : csi(code);
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
    wchar wbuf[TO_UNICODE_MAX];
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
      win_update(false);

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
          if (term.app_control & (1 << (wc & 0x1F))) {
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
      if (shift || (key >= '0' && key <= '9' && !term.modify_other_keys)) {
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
      if (old_alt_state > ALT_ALONE) {
        insert_alt_code();
        alt_state = ALT_NONE;
      } else if (extended && term.vt52_mode && term.app_keypad)
        len = sprintf(buf, "\e?M");
      else if (extended && !numlock && term.app_keypad)
        app_pad_code('M' - '@');
      else if (!extended && term.modify_other_keys && (shift || ctrl))
        other_code('\r');
      else if (ctrl && (cfg.old_modify_keys & 32))
        ctrl_ch(CTRL('^'));
      else
        esc_if(alt),
        term.newline_mode ? ch('\r'), ch('\n') : ch(shift ? '\n' : '\r');
    when VK_BACK:
      if (old_alt_state > ALT_ALONE) {
        alt_state = old_alt_state;  // keep alt_state, process key
        alt_code = alt_code / alt_state;
        win_update(false);
      } else if (cfg.old_modify_keys & 1) {
        if (!ctrl)
          esc_if(alt), ch(term.backspace_sends_bs ? '\b' : CDEL);
        else if (term.modify_other_keys)
          other_code(term.backspace_sends_bs ? '\b' : CDEL);
        else
          ctrl_ch(term.backspace_sends_bs ? CDEL : CTRL('_'));
      }
      else {
        if (term.modify_other_keys > 1 && mods)
          // perhaps also partially if:
          // term.modify_other_keys == 1 && (mods & ~(MDK_CTRL | MDK_ALT)) ?
          other_code(term.backspace_sends_bs ? '\b' : CDEL);
        else {
          esc_if(alt);
          ch(term.backspace_sends_bs ^ ctrl ? '\b' : CDEL);
        }
      }
    when VK_TAB:
      if (alt_code_ignore()) {
      } else if (!(cfg.old_modify_keys & 2) && term.modify_other_keys > 1 && mods) {
        // perhaps also partially if:
        // term.modify_other_keys == 1 && (mods & ~(MDK_SHIFT | MDK_ALT)) ?
        other_code('\t');
      }
#ifdef handle_alt_tab
      else if (alt) {
        if (cfg.switch_shortcuts) {
          // does not work as Alt+TAB is not passed here anyway;
          // could try something with KeyboardHook:
          // http://www.codeproject.com/Articles/14485/Low-level-Windows-API-hooks-from-C-to-stop-unwante
          win_switch(shift, true);
          return true;
        }
        else
          return false;
      }
#endif
      else if (!ctrl) {
        esc_if(alt);
        shift ? csi('Z') : ch('\t');
      }
      else if (allow_shortcut && cfg.switch_shortcuts) {
        win_switch(shift, lctrl & rctrl);
        return true;
      }
      //else term.modify_other_keys ? other_code('\t') : mod_csi('I');
      else if ((cfg.old_modify_keys & 4) && term.modify_other_keys)
        other_code('\t');
      else {
        esc_if(alt);
        mod_csi('I');
      }

    when VK_ESCAPE:
      if (old_alt_state > ALT_ALONE) {
        alt_state = ALT_CANCELLED;
      } else if (comp_state > COMP_NONE) {
        compose_clear();
      } else if (!(cfg.old_modify_keys & 8) && term.modify_other_keys > 1 && mods)
        other_code('\033');
      else
        term.app_escape_key
        ? ss3('[')
        : ctrl_ch(term.escape_sends_fs ? CTRL('\\') : CTRL('['));
    when VK_PAUSE:
      if (!vk_special(ctrl & !extended ? cfg.key_break : cfg.key_pause))
        // default cfg.key_pause is CTRL(']')
        return false;
    when VK_CANCEL:
      if (!strcmp(cfg.key_break, "_BRK_")) {
        child_break(&term);
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
#ifdef debug_vk_scroll
      printf("when VK_SCROLL scn %d\n", scancode);
#endif
      if (scancode)  // prevent recursion...
        // sync_scroll_lock() does not work in this case 
        // if ScrollLock is not defined in KeyFunctions
        win_key_nullify(VK_SCROLL);
      if (!vk_special(cfg.key_scrlock))
        return false;
    when VK_F1 ... VK_F24:
      if (alt_code_ignore()) {
        return true;
      }

      if (key <= VK_F4 && term.vt52_mode) {
        len = sprintf(buf, "\e%c", key - VK_F1 + 'P');
        break;
      }

      if (term.vt220_keys && ctrl && VK_F3 <= key && key <= VK_F10)
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
    when VK_HOME:   term.vt220_keys ? edit_key(1, '7') : cursor_key('H', '7');
    when VK_END:    term.vt220_keys ? edit_key(4, '1') : cursor_key('F', '1');
    when VK_UP:     cursor_key('A', '8');
    when VK_DOWN:   cursor_key('B', '2');
    when VK_LEFT:   cursor_key('D', '4');
        printf("VK_LEFT %x\n",mods);
    when VK_RIGHT:  cursor_key('C', '6');
    when VK_CLEAR:  cursor_key('E', '5');
    when VK_MULTIPLY ... VK_DIVIDE:
      if (term.vt52_mode && term.app_keypad)
        len = sprintf(buf, "\e?%c", key - VK_MULTIPLY + 'j');
      // initiate hex numeric input
      else if (key == VK_ADD && old_alt_state == ALT_ALONE)
        alt_state = ALT_HEX, alt_code = 0, alt_uni = false;
      // initiate decimal numeric input; override user-assigned functions
      else if (key == VK_SUBTRACT && old_alt_state == ALT_ALONE)
        alt_state = ALT_DEC, alt_code = 0, alt_uni = false;
      else if (alt_code_ignore()) {
      }
      else if (mods || (term.app_keypad && !numlock) || !layout())
        app_pad_code(key - VK_MULTIPLY + '*');
    when VK_NUMPAD0 ... VK_NUMPAD9:
      if (term.vt52_mode && term.app_keypad)
        len = sprintf(buf, "\e?%c", key - VK_NUMPAD0 + 'p');
      else if ((term.app_cursor_keys || !term.app_keypad) &&
          alt_code_numpad_key(key - VK_NUMPAD0))
        ;
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
      printf("-- mods %X alt %d altgr %d/%d ctrl %d lctrl %d/%d (modf %d comp %d)\n", mods, alt, altgr, altgr0, ctrl, lctrl, lctrl0, term.modify_other_keys, comp_state);
#endif
      if (key == ' ' && old_alt_state > ALT_ALONE) {
        insert_alt_code();
        alt_state = ALT_NONE;
      } else if (key > 'F' && alt_code_ignore()) {
      } else if (altgr_key())
        trace_key("altgr");
      else if (!modaltgr && !cfg.altgr_is_alt && altgr0 && !term.modify_other_keys)
        // prevent AltGr from behaving like Alt
        trace_key("!altgr");
      else if (key != ' ' && alt_code_key(key - 'A' + 0xA))
        trace_key("alt");
      else if (term.modify_other_keys > 1 && mods == MDK_SHIFT && !comp_state)
        // catch Shift+space (not losing Alt+ combinations if handled here)
        // only in modify-other-keys mode 2
        modify_other_key();
      else if (!(cfg.old_modify_keys & 16) && term.modify_other_keys > 1 && mods == (MDK_ALT | MDK_SHIFT))
        // catch this case before char_key
        trace_key("alt+shift"),
        modify_other_key();
      else if (char_key())
        trace_key("char");
      else if (term.modify_other_keys > 1 || (term.modify_other_keys && altgr))
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
      if (key > '9' && alt_code_ignore()) {
      } else if (key <= '9' && alt_code_key(key - '0'))
        ;
      else if (char_key())
        trace_key("0... char_key");
      else if (term.modify_other_keys <= 1 && ctrl_key())
        trace_key("0... ctrl_key");
      else if (term.modify_other_keys)
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
    clear_scroll_lock();
    provide_input(*buf);
    while (count--)
      child_send(&term,buf, len);
    compose_clear();
    // set token to enforce immediate display of keyboard echo;
    // we cannot win_update_now here; need to wait for the echo (child_proc)
    wv.kb_input = true;
    //printf("[%ld] win_key sent %02X\n", mtime(), key); kb_trace = key;
    if (tek_mode == TEKMODE_GIN)
      tek_send_address();
  }
  else if (comp_state == COMP_PENDING)
    comp_state = COMP_ACTIVE;

  return true;
}

bool
win_key_up(WPARAM wp, LPARAM lp)
{
  inline bool is_key_down(uchar vk) { return GetKeyState(vk) & 0x80; }

  uint key = wp;
#ifdef debug_virtual_key_codes
  printf("  win_key_up %02X (down %02X) %s\n", key, last_key_down, vk_name(key));
#endif

  if (key == VK_CANCEL) {
    // in combination with Control, this may be the KEYUP event 
    // for VK_PAUSE or VK_SCROLL, so their actual state cannot be 
    // detected properly for use as a modifier; let's try to fix this
    super_key = 0;
    hyper_key = 0;
    compose_key = 0;
  }
  else if (key == VK_SCROLL) {
    // heuristic compensation of race condition with auto-repeat
    sync_scroll_lock(term.no_scroll || term.scroll_mode);
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
        || (cfg.compose_key == MDK_SUPER && key == super_key)
        || (cfg.compose_key == MDK_HYPER && key == hyper_key)
        // support KeyFunctions=CapsLock:compose (or other key)
        || key == compose_key
        // support config ComposeKey=capslock
        // needs support by nullifying capslock state (see above)
        || (cfg.compose_key == MDK_CAPSLOCK && key == VK_CAPITAL)
       )
    {
      if (comp_state >= 0){
        comp_state = COMP_ACTIVE;
        win_update(false);
      }
    }
  }
  //zz else comp_state = COMP_NONE;

  last_key_up = key;

  if (newwin_pending) {
    if (key == newwin_key) {
      if (is_key_down(VK_SHIFT))
        newwin_shifted = true;
#ifdef control_AltF2_size_via_token
      if (newwin_shifted /*|| wv.win_is_fullscreen*/)
        clone_size_token = false;
#endif

      newwin_pending = false;
      key_special&= ~KS_NEWWIN;

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
    if (!transparency_pending && cfg.opaque_when_focused){
      win_update_transparency(cfg.transparency, false);
      key_special&=~KS_TRANP;
    }
  }

#if 0
  // "unhovering" is now handled in update_mouse, based on configured mods
  if (key == VK_CONTROL && term.hovering) {
    term.hovering = false;
    win_update(false);
  }
#endif

  if (key == VK_MENU) {
    if (alt_state > ALT_ALONE && alt_code) {
      insert_alt_code();
    }
    alt_state = ALT_NONE;
    return true;
  }
  return false;
}
bool 
win_whotkey(WPARAM wp, LPARAM lp){
  (void)lp;
  uint key = wp;
  //last_key_down = key; last_key_up = 0;

  if(!wv.winkey)return 0;

  uchar kbd[256];
  GetKeyboardState(kbd);
  inline bool is_key_down(uchar vk) { return kbd[vk] & 0x80; }

  bool lwin=wv.lwinkey;//(is_key_down(VK_LWIN) && key != VK_LWIN);
  bool rwin=wv.rwinkey;//(is_key_down(VK_RWIN) && key != VK_RWIN);
  bool win =wv.winkey;
  bool shift = is_key_down(VK_SHIFT);
  bool lalt = is_key_down(VK_LMENU);
  bool ralt = is_key_down(VK_RMENU);
  bool alt = lalt | ralt;
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


#define dont_debug_transparency
enum funct_type{
  FT_NULL=0,FT_CMD,FT_NORM,FT_NORV,FT_KEY,FT_KEYV,FT_PAR1,FT_PAR2,
  FT_RAWS,FT_ESCC,FT_SHEL
};
struct function_def {
  string name;
  int type;
  union {
    WPARAM cmd;
    void *f;
    void (*fct)(void);
    int  (*fctv)(void);
    void (*fct_key)(uint key, mod_keys mods);
    int  (*fct_keyv)(uint key, mod_keys mods);
    void (*fct_par1)(int p0);
    void (*fct_par2)(int p0,int p1);
  };
  uint (*fct_status)(void);
  int p0,p1;
  /*type :
   * 0: cmd;
   * 1: fct();
   * 2: fct_key(...)
   * 3: fct_par(p0,...)
   */
};

static void nop() { }
static void
cycle_transparency(void)
{
  cfg.transparency = ((cfg.transparency + 16) / 16 * 16) % 128;
  win_update_transparency(cfg.transparency, false);
}

static void
set_transparency(int t)
{
  if (t >= 128)
    t = 127;
  else if (t < 0)
    t = 0;
  cfg.transparency = t;
  win_update_transparency(t, false);
}

static void
cycle_pointer_style()
{
  cfg.cursor_type = (cfg.cursor_type + 1) % 3;
  term.cursor_invalid = true;
  term_schedule_cblink();
  win_update(false);
}


/*
   Some auxiliary functions for user-defined key assignments.
 */

static void
unicode_char()
{
  alt_state = ALT_HEX;
  old_alt_state = ALT_ALONE;
  alt_code = 0;
  alt_uni = true;
}

static void
transparency_level()
{
  if (!transparency_pending) {
    previous_transparency = cfg.transparency;
    transparency_pending = 1;
    key_special |= KS_TRANP;
    transparency_tuned = false;
  }
  if (cfg.opaque_when_focused)
    win_update_transparency(cfg.transparency, false);
}

static void
toggle_opaque()
{
  wv.force_opaque = !wv.force_opaque;
  win_update_transparency(cfg.transparency, wv.force_opaque);
}

static void
newwin(uint key, mod_keys mods)
{
	// defer send_syscommand(IDM_NEW) until key released
	// monitor cursor keys to collect parameters meanwhile
  newwin_pending = true;
  key_special|=KS_NEWWIN;
  newwin_home = false; newwin_monix = 0; newwin_moniy = 0;

  newwin_key = key;
  if (mods & MDK_SHIFT)
    newwin_shifted = true;
  else
    newwin_shifted = false;
}


static uint mflags_paste() {
  return
    IsClipboardFormatAvailable(CF_TEXT) ||
    IsClipboardFormatAvailable(CF_UNICODETEXT) ||
    IsClipboardFormatAvailable(CF_HDROP)
    ? MF_ENABLED : MF_GRAYED;
}
static void
compose_down(uint key, mod_keys mods)
{
  compose_key = key;
  last_key_down = key;
  (void)mods;
}
static int
kb_select(uint key, mod_keys mods)
{
  (void)key;
  // note kb_select_key for re-anchor handling?
  //kb_select_key = key;
  // start and anchor keyboard selection
  term.sel_pos = (pos){.y = term.curs.y, .x = term.curs.x, .r = 0};
  term.sel_anchor = term.sel_pos;
  term.sel_start = term.sel_pos;
  term.sel_end = term.sel_pos;
  term.sel_rect = mods & MDK_ALT;
  term.selection_pending = true;
  key_special |= KS_TERMSEL;
  return 1;
}
static uint mflags_defsize() { return (IsZoomed(wv.wnd) || term.cols != cfg.cols || term.rows != cfg.rows) ? MF_ENABLED : MF_GRAYED; }
#ifdef allow_disabling_scrollbar
static uint mflags_scrollbar_outer() { return term.show_scrollbar ? MF_CHECKED : MF_UNCHECKED | cfg.scrollbar ? 0 : MF_GRAYED ; }
#else
static uint mflags_scrollbar_outer() { return term.show_scrollbar ? MF_CHECKED : MF_UNCHECKED ; }
#endif
static uint mflags_scrollbar_inner() { return cfg.scrollbar?(  term.show_scrollbar ? MF_CHECKED : MF_UNCHECKED): MF_GRAYED; }
static uint mflags_opaque() { return wv.force_opaque ? MF_CHECKED : MF_UNCHECKED; }
static uint mflags_logging() { return (wv.logging ? MF_CHECKED : MF_UNCHECKED) ; }
static uint mflags_bidi() { return (cfg.bidi == 0 || (cfg.bidi == 1 && (term.on_alt_screen ^ term.show_other_screen))) ? MF_GRAYED : term.disable_bidi ? MF_UNCHECKED : MF_CHECKED; }
static uint mflags_dim_margins() { return term.dim_margins ? MF_CHECKED : MF_UNCHECKED; }
static uint mflags_status_line() { return term.st_type == 1 ? MF_CHECKED : term.st_type == 0 ? MF_UNCHECKED : MF_GRAYED; }
static void zoom_font_out   (){ win_zoom_font(-1, 0);}
static void zoom_font_in    (){ win_zoom_font( 1, 0);}
static void zoom_font_reset (){ win_zoom_font( 0, 0);}
static void zoom_font_sout  (){ win_zoom_font(-1, 1);}
static void zoom_font_sin   (){ win_zoom_font( 1, 1);}
static void zoom_font_sreset(){ win_zoom_font( 0, 1);}
static void menu_text() { open_popup_menu(true, null, get_mods()); }
static void menu_pointer() { open_popup_menu(false, null, get_mods()); }
static void window_full() { win_maximise(2); }
static void window_max() { win_maximise(1); }
static void window_toggle_max() { win_maximise(!IsZoomed(wv.wnd)); }
static void window_restore() { win_maximise(0); }
static void window_min() { win_set_iconic(true); }
static void switch_next() { win_switch(false, true); }
static void switch_prev() { win_switch(true, true); }
static void switch_visible_next() { win_switch(false, false); }
static void switch_visible_prev() { win_switch(true, false); }
static void lock_title() { cfg.title_settable = false; }
static void clear_title() { win_set_title(W("")); }
static void refresh() { win_invalidate_all(false); }
//static void scroll_key(int key) { SendMessage(wv.wnd, WM_VSCROLL, key, 0); }
//static int  vtabclose    (){if(!child_is_alive(&term)) {win_tab_clean();return 1;}return 0;}
static void scroll_top	 (){SendMessage(wv.wnd, WM_VSCROLL,SB_TOP      ,0);}      
static void scroll_end	 (){SendMessage(wv.wnd, WM_VSCROLL,SB_BOTTOM   ,0);}   
static void scroll_pgup	 (){SendMessage(wv.wnd, WM_VSCROLL,SB_PAGEUP   ,0);}   
static void scroll_pgdn	 (){SendMessage(wv.wnd, WM_VSCROLL,SB_PAGEDOWN ,0);} 
static void scroll_lnup	 (){SendMessage(wv.wnd, WM_VSCROLL,SB_LINEUP   ,0);}   
static void scroll_lndn	 (){SendMessage(wv.wnd, WM_VSCROLL,SB_LINEDOWN ,0);} 
static void scroll_prev	 (){SendMessage(wv.wnd, WM_VSCROLL,SB_PRIOR    ,0);}    
static void scroll_next	 (){SendMessage(wv.wnd, WM_VSCROLL,SB_NEXT     ,0);}     
void toggle_vt220() { term.vt220_keys = !term.vt220_keys; }
void toggle_auto_repeat() { term.auto_repeat = !term.auto_repeat; }
void toggle_bidi() { term.disable_bidi = !term.disable_bidi; }
void toggle_dim_margins() { term.dim_margins = !term.dim_margins; }
void toggle_status_line() {
  if (term.st_type == 1) term_set_status_type(0, 0);
  else if (term.st_type == 0) term_set_status_type(1, 0);
}
void tab_prev	    (){win_tab_change(-1);}
void tab_next	    (){win_tab_change( 1);}
void tab_move_prev(){win_tab_move  (-1);}
void tab_move_next(){win_tab_move  ( 1);}
void new_tab_def(){new_tab(IDSS_CUR);}
void new_tab_wsl(){new_tab(IDSS_WSL);}
void new_tab_cyg(){new_tab(IDSS_CYG);}
void new_tab_cmd(){new_tab(IDSS_CMD);}
void new_tab_psh(){new_tab(IDSS_PSH);}
void new_tab_usr(){new_tab(IDSS_USR);}
void new_win_wsl(){new_win(IDSS_WSL,0);}
void new_win_cyg(){new_win(IDSS_CYG,0);}
void new_win_cmd(){new_win(IDSS_CMD,0);}
void new_win_psh(){new_win(IDSS_PSH,0);}
void new_win_usr(){new_win(IDSS_USR,0);}
void new_win_def(){new_win(IDSS_DEF,0);}
static void win_hide() { ShowWindow(wv.wnd, IsIconic(wv.wnd) ?SW_RESTORE: SW_MINIMIZE ); }
static void super_down(uint key, mod_keys mods) { super_key = key; (void)mods; }
static void hyper_down(uint key, mod_keys mods) { hyper_key = key; (void)mods; }
static void win_ctrlmode(){
  if(wv.tabctrling!=3)wv.tabctrling=3;else wv.tabctrling=0;
  wv.last_tabk_time=GetMessageTime();
}
static uint mflags_lock_title() { return cfg.title_settable ? MF_ENABLED : MF_GRAYED; }
static uint mflags_copy() { return term.selected ? MF_ENABLED : MF_GRAYED; }
static uint mflags_kb_select() { return term.selection_pending; }

static void
refresh_scroll_title()
{
  win_unprefix_title(_W("[NO SCROLL] "));
  win_unprefix_title(_W("[SCROLL MODE] "));
  win_unprefix_title(_W("[NO SCROLL] "));
  if (term.no_scroll)
    win_prefix_title(_W("[NO SCROLL] "));
  if (term.scroll_mode)
    win_prefix_title(_W("[SCROLL MODE] "));
}

static void
clear_scroll_lock()
{
  bool scrlock0 = term.no_scroll || term.scroll_mode;
  if (term.no_scroll < 0) {
    term.no_scroll = 0;
  }
  if (term.scroll_mode < 0) {
    term.scroll_mode = 0;
  }
  bool scrlock = term.no_scroll || term.scroll_mode;
  if (scrlock != scrlock0) {
    sync_scroll_lock(term.no_scroll || term.scroll_mode);
    refresh_scroll_title();
  }
}
#define Funtion_def_OldName
#ifdef Funtion_def_OldName
static void win_toggle_screen_on() { win_keep_screen_on(!wv.keep_screen_on); }
static int no_scroll(uint key, mod_keys mods) {
  (void)mods;
  (void)key;
  if (!term.no_scroll) {
    term.no_scroll = -1;
    sync_scroll_lock(true);
    win_prefix_title(_W("[NO SCROLL] "));
    term_flush();
  }
  return 0;
}
static int scroll_mode(uint key, mod_keys mods) {
  (void)mods;
  (void)key;
  if (!term.scroll_mode) {
    term.scroll_mode = -1;
    sync_scroll_lock(true);
    win_prefix_title(_W("[SCROLL MODE] "));
    term_flush();
  }
  return 0;
}
static int toggle_no_scroll(uint key, mod_keys mods) {
  (void)mods;
  (void)key;
  term.no_scroll = !term.no_scroll;
  sync_scroll_lock(term.no_scroll || term.scroll_mode);
  if (!term.no_scroll) {
    refresh_scroll_title();
    term_flush();
  }
  else
    win_prefix_title(_W("[NO SCROLL] "));
  return 0;
}
static int toggle_scroll_mode(uint key, mod_keys mods) {
  (void)mods;
  (void)key;
  term.scroll_mode = !term.scroll_mode;
  sync_scroll_lock(term.no_scroll || term.scroll_mode);
  if (!term.scroll_mode) {
    refresh_scroll_title();
    term_flush();
  }
  else
    win_prefix_title(_W("[SCROLL MODE] "));
  return 0;
}
static uint mflags_no_scroll() { return term.no_scroll ? MF_CHECKED : MF_UNCHECKED; }
static uint mflags_scroll_mode() { return term.scroll_mode ? MF_CHECKED : MF_UNCHECKED; }
static uint mflags_always_top() { return wv.win_is_always_on_top ? MF_CHECKED: MF_UNCHECKED; }
static uint mflags_screen_on() { return wv.keep_screen_on ? MF_CHECKED: MF_UNCHECKED; } 
#endif
static uint mflags_fullscreen() { return wv.win_is_fullscreen ? MF_CHECKED : MF_UNCHECKED; }
static uint mflags_zoomed() { return IsZoomed(wv.wnd) ? MF_CHECKED: MF_UNCHECKED; }
static uint mflags_flipscreen() { return term.show_other_screen ? MF_CHECKED : MF_UNCHECKED; }
static uint mflags_open() { return term.selected ? MF_ENABLED : MF_GRAYED; }
static uint mflags_char_info() { return show_charinfo ? MF_CHECKED : MF_UNCHECKED; }
static uint mflags_vt220() { return term.vt220_keys ? MF_CHECKED : MF_UNCHECKED; }
static uint mflags_auto_repeat() { return term.auto_repeat ? MF_CHECKED : MF_UNCHECKED; }
static uint mflags_options() { return wv.config_wnd ? MF_GRAYED : MF_ENABLED; }
static uint mflags_tek_mode() { return tek_mode ? MF_ENABLED : MF_GRAYED; }

static void hor_left_1() { horscroll(-1); }
static void hor_right_1() { horscroll(1); }
static void hor_out_1() { horsizing(1, false); }
static void hor_in_1() { horsizing(-1, false); }
static void hor_narrow_1() { horsizing(-1, true); }
static void hor_wide_1() { horsizing(1, true); }
static void hor_left_mult() { horscroll(-term.cols / 10); }
static void hor_right_mult() { horscroll(term.cols / 10); }
static void hor_out_mult() { horsizing(term.cols / 10, false); }
static void hor_in_mult() { horsizing(-term.cols / 10, false); }
static void hor_narrow_mult() { horsizing(-term.cols / 10, true); }
static void hor_wide_mult() { horsizing(term.cols / 10, true); }

void win_close();
typedef struct pstr{ short len; char s[1]; }pstr;
typedef struct pwstr{ short len; wchar s[1]; }pwstr;
pwstr *pwsdup(const wchar*s){
  int len=wcslen(s);
  pwstr *d=(pwstr*)malloc(len+2);
  wcsncpy(d->s,s,len);
  d->len=len;
  return d;
}
pstr *psdup(const char*s){
  int len=strlen(s);
  pstr *d=(pstr*)malloc(len+4);
  strncpy(d->s,s,len);
  d->len=len;
  return d;
}

static void wpwstr(pwstr*s){//FT_RAWS
  provide_input(s->s[0]);
  child_sendw(&term,s->s, s->len );
}
static void wpesccode(int code,int mods){//FT_ESCC
  char buf[33];
  int len = sprintf(buf, mods ? "\e[%i;%u~" : "\e[%i~", code, mods + 1);
  child_send(&term,buf, len);
}
static void shellcmd(const char*cmd){//FT_SHEL
  term_cmd(cmd);
}
/* Keyboard handling */
typedef void(*QFT)(int,int);
static struct function_def cmd_defs[] = {
#ifdef Funtion_def_OldName
  //compatible 
#define DFDC(n,f,s)     {#n,FT_CMD ,{.cmd     =f},s,0,0}
#define DFDN(n,f,s)     {#n,FT_NORM,{.fct     =f},s,0,0}
#define DFDK(n,f,s)     {#n,FT_KEY ,{.fct_key =f},s,0,0}
#define DFDV(n,f,s)     {#n,FT_KEYV,{.fct_keyv=f},s,0,0}
#define DFDP(n,f,s,p)   {#n,FT_PAR1,{.fct_par1=f},s,p,0}
#define DFDQ(n,f,s,p,q) {#n,FT_PAR2,{.fct_par2=f},s,p,q}
#ifdef support_sc_defs
#warning these do not work, they crash
  DFDC(restore	    ,SC_RESTORE , 0),
  DFDC(move		      ,SC_MOVE    , 0),
  DFDC(resize		    ,SC_SIZE    , 0),
  DFDC(minimize	    ,SC_MINIMIZE, 0),
  DFDC(maximize	    ,SC_MAXIMIZE, 0),
  DFDC(menu		      ,SC_KEYMENU , 0),
  DFDC(close		    ,SC_CLOSE   , 0),
#endif
  DFDC(new-window         ,IDM_NEW              , 0),
  //DFDC(new-monitor      ,IDM_NEW_MONI         , 0),
  //DFDC(default-size     ,IDM_DEFSIZE          , 0),
  DFDN(hor-left-1			    ,hor_left_1				    , 0),
  DFDN(hor-right-1			  ,hor_right_1				  , 0),
  DFDN(hor-out-1			    ,hor_out_1				    , 0),
  DFDN(hor-in-1			      ,hor_in_1				      , 0),
  DFDN(hor-narrow-1			  ,hor_narrow_1				  , 0),
  DFDN(hor-wide-1			    ,hor_wide_1				    , 0),
  DFDN(hor-left-mult			,hor_left_mult				, 0),
  DFDN(hor-right-mult			,hor_right_mult				, 0),
  DFDN(hor-out-mult			  ,hor_out_mult				  , 0),
  DFDN(hor-in-mult			  ,hor_in_mult				  , 0),
  DFDN(hor-narrow-mult		,hor_narrow_mult			, 0),
  DFDN(hor-wide-mult			,hor_wide_mult				, 0),

  DFDC(default-size       ,IDM_DEFSIZE_ZOOM     , mflags_defsize),
  DFDC(toggle-fullscreen  ,IDM_FULLSCREEN_ZOOM  , mflags_fullscreen),
  DFDN(fullscreen         ,window_full          , mflags_fullscreen),
  DFDN(win-max            ,window_max           , mflags_zoomed),
  DFDN(win-toggle-max     ,window_toggle_max    , mflags_zoomed),
  DFDN(win-restore        ,window_restore       , 0),
  DFDN(win-icon           ,window_min           , 0),
  DFDN(close              ,win_close            , 0),
  DFDN(win-toggle-always-on-top ,win_toggle_on_top   , mflags_always_top),
  DFDN(win-toggle-keep-screen-on,win_toggle_screen_on, mflags_screen_on ),

  DFDN(unicode-char       ,unicode_char         , 0),

  DFDK(new                ,newwin               , 0),  // deprecated
  DFDK(new-key            ,newwin               , 0),
  DFDC(options            ,IDM_OPTIONS          , mflags_options),
  DFDN(menu-text          ,menu_text            , 0),
  DFDN(menu-pointer       ,menu_pointer         , 0),

  DFDC(search             ,IDM_SEARCH           , 0),
  DFDC(scrollbar-outer    ,IDM_SCROLLBAR        , mflags_scrollbar_outer),
  DFDN(scrollbar-inner    ,win_tog_scrollbar    , mflags_scrollbar_inner),
  DFDN(cycle-pointer-style,cycle_pointer_style  , 0),
  DFDN(cycle-transparency-level ,transparency_level, 0),
  DFDN(toggle-opaque      ,toggle_opaque        , mflags_opaque),

  DFDC(copy               ,IDM_COPY             , mflags_copy),
  DFDC(copy-text          ,IDM_COPY_TEXT        , mflags_copy),
  DFDC(copy-tabs          ,IDM_COPY_TABS        , mflags_copy),
  DFDC(copy-plain         ,IDM_COPY_TXT         , mflags_copy),
  DFDC(copy-rtf           ,IDM_COPY_RTF         , mflags_copy),
  DFDC(copy-html-text     ,IDM_COPY_HTXT        , mflags_copy),
  DFDC(copy-html-format   ,IDM_COPY_HFMT        , mflags_copy),
  DFDC(copy-html-full     ,IDM_COPY_HTML        , mflags_copy),
  DFDC(paste              ,IDM_PASTE            , mflags_paste),
  DFDN(paste-path         ,win_paste_path       , mflags_paste),
  DFDC(copy-paste         ,IDM_COPASTE          , mflags_copy),
  DFDC(select-all         ,IDM_SELALL           , 0),
  DFDC(clear-scrollback   ,IDM_CLRSCRLBCK       , 0),
  DFDC(copy-title         ,IDM_COPYTITLE        , 0),
  DFDN(lock-title         ,lock_title           , mflags_lock_title),
  DFDN(clear-title        ,clear_title          , 0),
  DFDC(reset              ,IDM_RESET            , 0),
  DFDC(reset-noask        ,IDM_RESET_NOASK      , 0),
  DFDC(tek-reset          ,IDM_TEKRESET         , mflags_tek_mode),
  DFDC(tek-page           ,IDM_TEKPAGE          , mflags_tek_mode),
  DFDC(tek-copy           ,IDM_TEKCOPY          , mflags_tek_mode),
  DFDC(save-image         ,IDM_SAVEIMG          , 0),
  DFDC(break              ,IDM_BREAK            , 0),
  DFDC(flipscreen         ,IDM_FLIPSCREEN       , mflags_flipscreen),
  DFDC(open               ,IDM_OPEN             , mflags_open),
  DFDC(toggle-logging     ,IDM_TOGLOG           , mflags_logging),
  DFDC(toggle-char-info   ,IDM_TOGCHARINFO      , mflags_char_info),
  DFDC(export-html        ,IDM_HTML             , 0),
  DFDN(print-screen       ,print_screen         , 0),
  DFDN(toggle-vt220       ,toggle_vt220         , mflags_vt220),
  DFDN(toggle-auto-repeat ,toggle_auto_repeat   , mflags_auto_repeat),
  DFDN(toggle-bidi        ,toggle_bidi          , mflags_bidi),
  DFDN(refresh            ,refresh              , 0),
  DFDN(toggle-dim-margins ,toggle_dim_margins   , mflags_dim_margins),
  DFDN(toggle-status-line ,toggle_status_line   , mflags_status_line),

  DFDK(super              ,super_down           , 0),
  DFDK(hyper              ,hyper_down           , 0),
  DFDK(compose            ,compose_down         , 0),
  DFDV(kb-select          ,kb_select            ,mflags_kb_select     ),
  DFDV(no-scroll          ,no_scroll            , mflags_no_scroll),
  DFDV(toggle-no-scroll   ,toggle_no_scroll     , mflags_no_scroll),
  DFDV(scroll-mode        ,scroll_mode          , mflags_scroll_mode),
  DFDV(toggle-scroll-mode ,toggle_scroll_mode   , mflags_scroll_mode),

  DFDN(switch-prev		    ,switch_prev          , 0),
  DFDN(switch-next		    ,switch_next          , 0),
  DFDN(switch-visible-prev,switch_visible_prev  , 0),
  DFDN(switch-visible-next,switch_visible_next  , 0),
#endif

#undef DFDC
#undef DFDN
#undef DFDK
#undef DFDP
#undef DFDQ
#define DFSC(f,s)     {#f,FT_CMD ,{.cmd     =IDM_##f},s,0,0}
#define DFSN(f,s)     {#f,FT_NORM,{.fct     =f},s,0,0}
#define DFSK(f,s)     {#f,FT_KEY ,{.fct_key =f},s,0,0}
#define DFSV(f,s)     {#f,FT_KEYV,{.fct_keyv=f},s,0,0}
#define DFSP(f,s)     {#f,FT_PAR1,{.fct_par1=f},s,0,0}
#define DFSQ(f,s)     {#f,FT_PAR2,{.fct_par2=(QFT)(void*)f},s,0,0}
  DFSC(DEFSIZE_ZOOM       , mflags_defsize),
  DFSC(FULLSCREEN_ZOOM    , mflags_fullscreen),
  DFSC(FULLSCREEN         , mflags_fullscreen),
  DFSN(window_full        , mflags_fullscreen),
  DFSN(window_max         , mflags_zoomed),
  DFSN(window_toggle_max  , mflags_zoomed),
  DFSC(OPTIONS            , mflags_options),
  DFSC(BORDERS            , 0),
  DFSC(SCROLLBAR          , mflags_scrollbar_outer),
  DFSN(win_tog_scrollbar  , mflags_scrollbar_inner),
  DFSC(COPY               , mflags_copy),
  DFSC(COPY_TEXT          , mflags_copy),
  DFSC(COPY_TABS          , mflags_copy),
  DFSC(COPY_TXT           , mflags_copy),
  DFSC(COPY_RTF           , mflags_copy),
  DFSC(COPY_HTXT          , mflags_copy),
  DFSC(COPY_HFMT          , mflags_copy),
  DFSC(COPY_HTML          , mflags_copy),
  DFSC(PASTE              , mflags_paste),
  DFSN(win_paste_path     , mflags_paste),
  DFSC(COPASTE            , mflags_copy),
  DFSN(lock_title         , mflags_lock_title),
  DFSC(TEKRESET           , mflags_tek_mode),
  DFSC(TEKPAGE            , mflags_tek_mode),
  DFSC(TEKCOPY            , mflags_tek_mode),
  DFSC(FLIPSCREEN         , mflags_flipscreen),
  DFSC(OPEN               , mflags_open),
  DFSC(TOGLOG             , mflags_logging),
  DFSC(TOGCHARINFO        , mflags_char_info),
  DFSN(toggle_vt220       , mflags_vt220),
  DFSN(toggle_auto_repeat , mflags_auto_repeat),
  DFSN(toggle_bidi        , mflags_bidi),
  DFSV(kb_select          , mflags_kb_select),
  DFSC(NEW                , 0),
  DFSC(DEFSIZE            , 0),
  DFSC(DEFSIZE_ZOOM       , 0),
  DFSN(window_restore     , 0),
  DFSN(window_min         , 0),
  DFSN(win_close          , 0),
  DFSN(app_close          , 0),
  DFSK(newwin             , 0),
  DFSN(menu_text          , 0),
  DFSN(menu_pointer       , 0),
  DFSC(SEARCH             , 0),
  DFSN(cycle_pointer_style, 0),
  DFSN(transparency_level , 0),
  DFSC(SELALL             , 0),
  DFSC(CLRSCRLBCK         , 0),
  DFSC(COPYTITLE          , 0),
  DFSN(clear_title        , 0),
  DFSC(RESET              , 0),
  DFSC(SAVEIMG            , 0),
  DFSC(BREAK              , 0),
  DFSC(HTML               , 0),
  DFSN(print_screen       , 0),
  DFSN(refresh            , 0),
  DFSK(super_down         , 0),
  DFSK(hyper_down         , 0),
  DFSN(scroll_top		      , 0),      
  DFSN(scroll_end		      , 0),   
  DFSN(scroll_pgup	      , 0),   
  DFSN(scroll_pgdn	      , 0), 
  DFSN(scroll_lnup	      , 0),   
  DFSN(scroll_lndn	      , 0), 
  DFSN(scroll_prev	      , 0),    
  DFSN(scroll_next	      , 0),     
  DFSN(tab_prev	          , 0),    
  DFSN(tab_next	          , 0),    
  DFSN(tab_move_prev	    , 0),    
  DFSN(tab_move_next	    , 0),    
  DFSN(switch_prev        , 0),
  DFSN(switch_next        , 0),
  DFSN(switch_visible_prev, 0),
  DFSN(switch_visible_next, 0),

  DFSN(term_copy          , 0),
  DFSN(win_paste          , 0),
  DFSC(RESET              , 0),
  DFSN(term_select_all    , 0),
  DFSK(win_popup_menu     , 0),
  DFSV(win_key_menu       , 0),
  DFSN(win_tab_show       , 0),  
  DFSN(win_tab_indicator  , 0),
  DFSN(win_tog_partline   , 0),
  DFSN(win_tog_scrollbar  , 0),
  DFSN(zoom_font_out      , 0),
  DFSN(zoom_font_in       , 0),
  DFSN(zoom_font_reset    , 0),
  DFSN(zoom_font_sout     , 0),
  DFSN(zoom_font_sin      , 0),
  DFSN(zoom_font_sreset   , 0),

  DFSP(win_tab_change     , 0),    
  DFSP(win_tab_go         , 0),    
  DFSP(win_tab_move       , 0),    
  DFSQ(win_zoom_font      , 0),
#undef DFSC
#undef DFSN
#undef DFSK
#undef DFSP
#undef DFSQ
  {0,0,{.fct=nop},0,0,0}
};
static struct function_def *
function_def(const char * cmd)
{
  for (uint i = 0; i < lengthof(cmd_defs); i++)
    if (!strcasecmp(cmd, cmd_defs[i].name))
      return &cmd_defs[i];
  return 0;
}
static int fundef_run(const char*cmd,uint key, mod_keys mods){
  struct function_def * fundef = function_def(cmd);
  if (fundef) {
    switch(fundef->type){
      when FT_CMD :PostMessage(wv.wnd, WM_SYSCOMMAND, fundef->cmd, 0); 
      when FT_NORM:fundef->fct();
      when FT_KEY :fundef->fct_key(key,mods);
      when FT_NORV:return fundef->fctv();
      when FT_KEYV:return fundef->fct_keyv(key,mods);
      when FT_PAR1:fundef->fct_par1(fundef->p0);
      when FT_PAR2:fundef->fct_par2(fundef->p0,fundef->p1);
    }
    return true;
    // should we trigger ret = false if (fudef->fct_key == kb_select)
    // so the case can be handled further in win_key_down ?
  }
  else {
    // invalid definition (e.g. "A+Enter:foo;"), shall 
    // not cause any action (return true) but provide a feedback
    win_bell(&cfg);
    return true;
  }
}
static int fundef_stat(const char*cmd){
  struct function_def * fudef = function_def(cmd);
  if (fudef && fudef->fct_status) {
    return fudef->fct_status();
    //EnableMenuItem(menu, idm_cmd + n, status);  // done by modify_menu
  }
  return 0;
} 
struct SCKDef{
  int moda,mods;
  union {
    WPARAM cmd;
    void *f;
    void (*fct)(void);
    int  (*fctv)(void);
    void (*fct_key)(uint key, mod_keys mods);
    int  (*fct_keyv)(uint key, mod_keys mods);
    void (*fct_par1)(int p0);
    void (*fct_par2)(int p0,int p1);
  };
  short key,type;
  int p0,p1,p2;
  struct SCKDef*next;
};
//static int  sckmask[256]={0};
//static int  sckwmask[256]={0};
static struct SCKDef *sckdef[256]={0};
static struct SCKDef *sckwdef[256]={0};
//modkey < 7,
static int modr2s(int moda){
  int r=(moda&0xFF)|((moda>>8)&0xFF)|((moda>>16)&0xFF);
  return r;
}
//asume MKD_WIN=8,depend on MDK_KEYS
static int packmod(int mods){
  if(mods&0x70)return 0x8|((mods>>4)&0x7);
  return mods&7;
}
static void desck(struct SCKDef *sckd){
  switch(sckd->type){
    when FT_RAWS or FT_SHEL :
        free(sckd->f);
  }
}
static void setsck(int moda,uint key,int ft,void*func){
  struct SCKDef **pp,*n;
  int *pm;
  int mods=modr2s(moda);;
  moda|=mods;
  if(ft==FT_NULL){
    int nmask=0;
    if(mods&MDK_WIN){
      pm=&sckwmask[key];
      pp=&sckwdef[key];
    }else{
      pm=&sckmask[key];
      pp=&sckdef[key];
    }
    for(n=*pp;n;pp=&n->next,n=n->next){
      int m=0;
      if(n->moda&~0xF){
        m=(moda==n->moda);
      }else {
        m=(n->moda==(moda&0xf));
      }
      if(m){
        *pp=n->next;
        desck(n);  
      }else{
        nmask|=(1<<packmod(n->mods));
      }
    }
    *pm=nmask;
  } else{
    if(mods&MDK_WIN){
      pm=&sckwmask[key];
      pp=&sckwdef[key];
    }else{
      pm=&sckmask[key];
      pp=&sckdef[key];
    }
    for(n=*pp;n;pp=&n->next,n=n->next){
      int m=0;
      if(n->moda&~0xF){
        m=(moda==n->moda);
      }else {
        m=(n->moda==(moda&0xf));
      }
      if(m)break;
    }
    if(!n){
      n=new(struct SCKDef);
      n->next=*pp;
      *pp=n;
    }
    n->f=func;
    n->type=ft;
    n->key=key;
    n->mods=mods;
    n->moda=moda;
    *pm|=(1<<packmod(mods));
  }
}
void desckk(int key,int mods,int moda){
  sckmask[key]&=~(1<<mods);
  struct SCKDef **pp=&sckdef[key],*p;
  for(p=*pp;p;p=*pp){
    if(p->moda==moda){
      desck(p);
      *pp=p->next;
      free(p);
      break;
    }
    pp=&p->next;
  }
}
int sck_runr(struct SCKDef*pd,uint key,int moda){
  int res=1;
  switch(pd->type){
    when FT_CMD : send_syscommand(pd->cmd);
    when FT_NORM: pd->fct();
    when FT_KEY : pd->fct_key(key,moda);
    when FT_NORV: res=pd->fctv();
    when FT_KEYV: res=pd->fct_keyv(key,moda);
    when FT_PAR1: pd->fct_par1(pd->p0);
    when FT_PAR2: pd->fct_par2(pd->p0,pd->p1);
    when FT_RAWS: wpwstr(pd->f);
    when FT_ESCC: wpesccode(pd->cmd,moda);
    when FT_SHEL: shellcmd(pd->f);
  }
  return res;
}

int sckw_run(uint key,int moda){
  struct SCKDef*pd;
  for(pd=sckwdef[key];pd;pd=pd->next){
    int m=0;
    if(pd->moda&~0xF){
      m=(moda==pd->moda);
    }else {
      m=(pd->moda==(moda&0xf));
    }
    if(m){
      return sck_runr(pd,key,moda);
    }
  }
  return 0;
}
int sck_run(uint key,int moda){
  struct SCKDef*pd;
  for(pd=sckdef[key];pd;pd=pd->next){
    int m=0;
    if(pd->moda&~0xF){
      m=(moda==pd->moda);
    }else {
      m=(pd->moda==(moda&0xf));
    }
    if(m){
      return sck_runr(pd,key,moda);
    }
  }
  return 0;
}
void pstrsck(char*ssck){
  char * cmdp = ssck;
  char sepch = ';';
  if ((uchar)*cmdp <= (uchar)' ') sepch = *cmdp++;
    // check for multi-line separation
  char*p,*pd;
  for(pd=p=cmdp;*p;pd++,p++){
    if (*p == '\\' && p[1] == '\n') {
      p += 2;
    }else{
      *pd++=*p++;
    }
  }


  // derive modifiers from specification prefix, module their order;
  // in order to abstract from the order and support flexible configuration,
  // the modifiers could have been collected separately already instead of 
  // prefixing them to the tag (before calling pick_key_function) but 
  // that would have been more substantial redesign; or the prefix could 
  // be normalized here by sorting; better solution: collect info here
  mod_keys tagmods(char * k,char**pn)
  {
    mod_keys m = 0;//mod0
    *pn=k;
    if(!k)return m;
    char*p= strchr(k, '+');
    if(!p)return m;
    *pn=p+1;
    int sc=1;
    for (; k<p; k++){
      switch (*k) {
        when 'S': m |= (MDK_SHIFT<<sc);sc=0;
        when 'A': m |= (MDK_ALT  <<sc);sc=0;
        when 'C': m |= (MDK_CTRL <<sc);sc=0;
        when 'W': m |= (MDK_WIN  <<sc);sc=0;
        when 'U': m |= (MDK_SUPER<<sc);sc=0;
        when 'Y': m |= (MDK_HYPER<<sc);sc=0;
        when 'L': sc=8;
        when 'R': sc=16;
      }
    } 
    return m;
  }
  char*strsep(char*s,char sep){
    char*p= strchr(s, sep);
    if(!p)return p;
    *p=0;
    return p+1;
  } 

  //test: "-:'foo';A+F3:;A+F5:flipscreen;"
  //"A+F9:\"f9\";C+F10:\"f10\";p:paste;"
  //"d:`date`;o:\"oo\";ö:\"öö\";€:\"euro\";"
  //"~:'tilde';[:'[[';µ:'µµ'")
  char * paramp;
  while ((paramp = strsep(cmdp, ':'))) {
    char * sepp = strsep(paramp, sepch);
    char * cmd0;
    mod_keys mod_cmd = tagmods(cmdp,&cmd0);
    int key=getvk(cmd0);
#if defined(debug_def_keys) && debug_def_keys > 1
    printf("tag <%s>: cmd <%s> cmd0 <%s> mod %X fct <%s>\n", tag, cmdp, cmd0, mod_cmd, paramp);
#endif
    for(;*paramp==' ';paramp++);
    wchar * fct = cs__utftowcs(paramp);
    uint code;
    int len=wcslen(fct);
    switch(*fct){
      when 0:
          // empty definition (e.g. "A+Enter:;"), shall disable 
          // further shortcut handling for the input key but 
          // trigger fall-back to "normal" key handling (with mods)
            setsck(mod_cmd,key,FT_NULL,0);
      when '"' or '\'' :
          if(fct[len-1]==*fct) {//raww string "xxx" or 'xxx'
            fct[len-1]=0;
            pwstr *s=(pwstr*)malloc(len+2);
            s->len=len-2;
            wcsncpy(s->s,fct+1,len-2);
            setsck(mod_cmd,key,FT_RAWS,s);
          }
      when '^':
          if(len==2){
            char cc[2];
            char c=fct[1];
            cc[1] = ' ';
            if (c == '?')
              cc[1] = '\177';
            else if (c == ' ' || (c >= '@' && c <= '_')) {
              cc[1] = fct[1] & 0x1F;
            }
            pwstr *s=(pwstr*)malloc(3+2);
            s->len=1;
            s->s[0]=cc[1];
            s->s[1]=0;
            setsck(mod_cmd,key,FT_RAWS,s);
          }
      when '`':
          if (fct[len - 1] == '`') {//`shell cmd`
            fct[len - 1] = 0;
            char * cmd = cs__wcstombs(&fct[1]);
            if (*cmd) {
              setsck(mod_cmd,key,FT_SHEL,cmd);
            }
          }
      when '0' ... '9':
          if (sscanf (paramp, "%u%c", & code, &(char){0}) == 1) {//key escape
            setsck(mod_cmd,key,FT_ESCC,(void*)(long)code);
          }
      otherwise: 
      {
        struct function_def * fd = function_def(paramp);
          if(fd)setsck(mod_cmd,key,fd->type,fd->f);
      }
    }
    free(fct);
    if (!sepp)break; 
    cmdp = sepp ;
    // check for multi-line separation
    if (*cmdp == '\\' && cmdp[1] == '\n') {
      cmdp += 2;
      while (iswspace(*cmdp)) cmdp++;
    }
  }

}

void win_update_shortcuts(){
  int i;
  memset (sckmask,0,sizeof(sckmask));
  for(i=0;i<256;i++){
    struct SCKDef *p=sckdef[i],*n;
    for(;p;p=n){
      desck(p); n=p->next; free(p);
    }
    sckdef[i]=NULL;
  };
  /*
  typedef enum { MDK_SHIFT = 1, MDK_ALT = 2, MDK_CTRL = 4, 
    MDK_WIN = 8, MDK_SUPER = 16, MDK_HYPER = 32 } mod_keys;
    */
#undef W
#define S MDK_SHIFT
#define A MDK_ALT
#define C MDK_CTRL
#define W MDK_WIN
#define AS MDK_ALT+MDK_SHIFT
#define CS MDK_CTRL+MDK_SHIFT
#define WS MDK_WIN+MDK_SHIFT
#define SETSCK(m,k,t,f) setsck(m,k,t,(void*)f)
  SETSCK(AS,VK_RETURN    ,FT_CMD ,IDM_FULLSCREEN_ZOOM);
  SETSCK(A,VK_RETURN     ,FT_CMD ,IDM_FULLSCREEN );
//SETSCK(0,VK_RETURN     ,FT_NORV,vtabclose);
//SETSCK(0,VK_ESCAPE     ,FT_NORV,vtabclose);
  SETSCK(S,VK_APPS       ,FT_CMD ,SC_KEYMENU);
  SETSCK(C,VK_APPS       ,FT_KEYV,win_key_menu);
  SETSCK(0,VK_APPS       ,FT_KEYV,win_key_menu);
  if(cfg.window_shortcuts){
    SETSCK(A,VK_SPACE      ,FT_CMD ,SC_KEYMENU);
  }

  if (cfg.switch_shortcuts) {
    SETSCK(CS,VK_TAB    ,FT_NORM,tab_prev	    );
    SETSCK(C ,VK_TAB    ,FT_NORM,tab_next	    );
  }
  if(cfg.win_shortcuts ){
    SETSCK(W ,'T'        ,FT_CMD ,IDM_NEWTAB   );
    SETSCK(W ,'Q'        ,FT_NORM,app_close    );
    SETSCK(W ,'W'        ,FT_NORM,win_close    );
    SETSCK(W ,'X'        ,FT_NORM,win_ctrlmode );
    SETSCK(W ,'Z'        ,FT_NORM,win_hide     );
    SETSCK(W ,VK_LEFT    ,FT_NORM,tab_prev	   );
    SETSCK(W ,VK_RIGHT   ,FT_NORM,tab_next	   );
    SETSCK(WS,VK_LEFT    ,FT_NORM,tab_move_prev);
    SETSCK(WS,VK_RIGHT   ,FT_NORM,tab_move_next);
  }
  if(cfg.clip_shortcuts ){
    SETSCK(C,VK_INSERT  ,FT_NORM,term_copy);
    SETSCK(S,VK_INSERT  ,FT_NORM,win_paste);
    SETSCK(C,VK_F12     ,FT_NORM,term_copy);
    SETSCK(S,VK_F12     ,FT_NORM,win_paste);
  }
  if(cfg.alt_fn_shortcuts){
    SETSCK(A,VK_F2   ,FT_NORM,newwin);
    SETSCK(A,VK_F3   ,FT_CMD ,IDM_SEARCH);
    SETSCK(A,VK_F4   ,FT_CMD ,SC_CLOSE);
    SETSCK(A,VK_F8   ,FT_CMD ,IDM_RESET);
    SETSCK(A,VK_F10  ,FT_CMD ,IDM_DEFSIZE_ZOOM);
    SETSCK(A,VK_F11  ,FT_CMD ,IDM_FULLSCREEN);
    SETSCK(A,VK_F12  ,FT_CMD ,IDM_FLIPSCREEN);
  }
  if(cfg.ctrl_shift_shortcuts  ){
    SETSCK(CS,'A'     ,FT_NORM,term_select_all);
    SETSCK(CS,'C'     ,FT_NORM,term_copy);
    SETSCK(CS,'V'     ,FT_NORM,win_paste);
    SETSCK(CS,'I'     ,FT_NORM,win_popup_menu);
    SETSCK(CS,'N'     ,FT_NORM,new_tab_def);
    SETSCK(CS,'R'     ,FT_CMD ,IDM_RESET_NOASK);
    SETSCK(CS,'D'     ,FT_CMD ,IDM_DEFSIZE);
    SETSCK(CS,'F'     ,FT_CMD ,IDM_FULLSCREEN_ZOOM );
    SETSCK(CS,'S'     ,FT_CMD ,IDM_FLIPSCREEN);
    SETSCK(CS,'H'     ,FT_CMD ,IDM_SEARCH);
    SETSCK(CS,'T'     ,FT_NORM,transparency_level); 
    SETSCK(CS,'W'     ,FT_NORM,win_close);
    SETSCK(CS,'P'     ,FT_NORM,cycle_pointer_style);
    SETSCK(CS,'O'     ,FT_NORM,win_tog_scrollbar);
    SETSCK(CS,'U'     ,FT_NORM,unicode_char);
  }
  if( cfg.zoom_shortcuts ){
    SETSCK(C,VK_SUBTRACT   ,FT_NORM,zoom_font_out   );
    SETSCK(C,VK_ADD        ,FT_NORM,zoom_font_in    );
    SETSCK(C,VK_NUMPAD0    ,FT_NORM,zoom_font_reset );
    SETSCK(C,VK_OEM_MINUS  ,FT_NORM,zoom_font_out   );
    SETSCK(C,VK_OEM_PLUS   ,FT_NORM,zoom_font_in    );
    SETSCK(C,'0'				   ,FT_NORM,zoom_font_reset );
    SETSCK(CS,VK_SUBTRACT  ,FT_NORM,zoom_font_sout  );
    SETSCK(CS,VK_ADD       ,FT_NORM,zoom_font_sin   );
    SETSCK(CS,VK_NUMPAD0   ,FT_NORM,zoom_font_sreset);
  }
#undef S 
#undef A 
#undef C 
#undef W 
#undef AS
#undef CS
#undef WS
#undef SETSCK
  if(cfg.key_commands){
    char * s= cs__wcstoutf(cfg.key_commands);
    pstrsck(s);
    free(s);
  }

}

