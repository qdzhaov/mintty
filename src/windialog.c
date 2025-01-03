// windialog.c (part of mintty)
// Copyright 2008-11 Andy Koppe, -2024 Thomas Wolff
// Based on code from PuTTY-0.60 by Simon Tatham and team.
// Licensed under the terms of the GNU General Public License v3 or later.

//G #include "winpriv.h"

#include "ctrls.h"
#include "winctrls.h"
#include "winids.h"
#include "res.h"
#include "appinfo.h"

//G #include "charset.h"  // nonascii, cs__utftowcs
extern void setup_config_box(controlbox *);

#include <commctrl.h>

# define nodebug_dialog_crash

#ifdef debug_dialog_crash
#include <signal.h>
#endif

#ifdef __CYGWIN__
//G #include <sys/cygwin.h>  // cygwin_internal
#endif
#include <sys/stat.h>  // chmod



#ifdef debug_handler
static control *
trace_ctrl(int line, int ev, control * ctrl)
{
static char * debugopt = 0;
  if (!debugopt) {
    debugopt = getenv("MINTTY_DEBUG");
    if (!debugopt)
      debugopt = "";
  }

  if (strchr(debugopt, 'o')) {
    printf("[%d ev %d] type %d (%d %d %d) label %s col %d\n", line, ev,
           ctrl->type, !!ctrl->handler, !!ctrl->widget, !!ctrl->context,
           ctrl->label, ctrl->column);
  }
  return ctrl;
}
// inject trace invocation into calls like ctrl->handler
#define handler(ctrl, ev)	handler(trace_ctrl(__LINE__, ev, ctrl), ev)
#endif

/*
 * windlg.c - Dialogs, including the configuration dialog.

   To make the TreeView widget work with Unicode support, 
   it is particularly essential to use message TVM_INSERTITEMW 
   to insert a TVINSERTSTRUCTW.

   To document a minimum set of Unicode-enabled API usage as could be 
   identified, some calls below are explicitly maintained in "ANSI" mode:
     RegisterClassA			would work for the TreeView
     RegisterClassW			needs "W" for title if UNICODE
     RegisterClassW with DefDlgProcW	needed for proper window title
     CreateDialogW			must be "W" if UNICODE is defined
     CreateWindowExA			works
   The TreeView_ macros are implicitly mapped to either "A" or "W", 
   so to use TreeView_InsertItem in either mode, it needs to be expanded 
   to SendMessageA/SendMessageW.
 */

/*
 * These are the various bits of data required to handle the
 * portable-dialog stuff in the config box.
 */
#define SWND_CLASS "SWND"
static controlbox *ctrlbox;
/*
 * ctrls_base holds the OK and Cancel buttons: the controls which
 * are present in all dialog panels. ctrls_panel holds the ones
 * which change from panel to panel.
 */
static winctrls ctrls_base, ctrls_panel;
static int heightsc=1;
//#define SC(a) (heightsc*(a)/100)
#define SC(a) win_dialog_sc(a)
windlg dlg;
wstring dragndrop;
static int dialog_height,dialog_width,ldpi=72;  
#define USECTLWND
#define DLGH 28
#define DLGW 35
typedef struct {
  HWND treeview;
  HTREEITEM lastat[4];
} treeview_faff;
int win_dialog_sc(int iv){
  return (heightsc*(iv)/100);
}
static HTREEITEM
treeview_insert(treeview_faff * faff, int level, const wchar *text, const wchar *path)
{
// text will be the label of an Options dialog treeview item;
// it is passed in here as the basename of path

  HTREEITEM newitem;

  TVINSERTSTRUCTW ins;
  ins.hParent = (level > 0 ? faff->lastat[level - 1] : TVI_ROOT);
  ins.hInsertAfter = faff->lastat[level];
  ins.item.mask = TVIF_TEXT | TVIF_PARAM;
  ins.item.pszText = (wchar*)text;
  //ins.item.cchTextMax = wcslen(utext) + 1;  // ignored when setting
  ins.item.lParam = (LPARAM) path;
  // It is essential to also use TVM_INSERTITEMW here!
  newitem = (HTREEITEM)SendMessageW(faff->treeview, TVM_INSERTITEMW, 0, (LPARAM)&ins);
  //TreeView_SetUnicodeFormat((HWND)newitem, TRUE);  // does not work

  if (level > 0)
    (void)TreeView_Expand(faff->treeview, faff->lastat[level - 1],
                          (level > 1 ? TVE_COLLAPSE : TVE_EXPAND));
  faff->lastat[level] = newitem;
  for (int i = level + 1; i < 4; i++)
    faff->lastat[i] = null;
  return newitem;
}

/*
 * Create the panelfuls of controls in the configuration box.
 */
static void
create_controls(HWND wnd, const wchar *path)
{
  ctrlpos cp;
  int index;
  int base_id;
  winctrls *wc;

  if (!path[0]) {
   /*
    * Here we must create the basic standard controls.
    */
    ctrlposinit(&cp, wnd, SC(3), SC(3), SC(dialog_height- 24));
    wc = &ctrls_base;
    base_id = IDCX_STDBASE;
  }
  else {
   /*
    * Otherwise, we're creating the controls for a particular panel.
    */
#ifdef USECTLWND
    ctrlposinit(&cp, wnd, SC(3), SC(3), SC(2));//for ctlwnd
#else
    ctrlposinit(&cp, wnd, SC(69), SC(3), SC(3));//
#endif
    wc = &ctrls_panel;
    base_id = IDCX_PANELBASE;
  }

#ifdef debug_layout
  printf("create_controls (%ls)\n", path);
#endif
  for (index = -1; (index = ctrl_find_path(ctrlbox, path, index)) >= 0;) {
    controlset *s = ctrlbox->ctrlsets[index];
    winctrl_layout(wc, &cp, s, &base_id);
  }
#ifdef USECTLWND
  SCROLLINFO si = {
    .cbSize = sizeof si,
    .fMask = SIF_ALL,
    .nMin = 0,
    .nMax = cp.ypos,
    .nPage = SC(dialog_height - 30),
    .nPos = 0
  };
  SetScrollInfo(dlg.ctlwnd,SB_VERT, &si,1);
#endif
}
#ifdef debug_dialog_crash
static char * debugopt = 0;
static char * debugtag = "none";

static void
sigsegv(int sig)
{
  signal(sig, SIG_DFL);
  printf("catch %d: %s\n", sig, debugtag);
  fflush(stdout);
  MessageBoxA(0, debugtag, "Critical Error", MB_ICONSTOP);
}

inline static void
crashtest()
{
  char * x0 = 0;
  *x0 = 'x';
}

static void
debug(const char *tag)
{
  if (!debugopt) {
    debugopt = getenv("MINTTY_DEBUG");
    if (!debugopt)
      debugopt = "";
  }

  debugtag = tag;

  if (debugopt && strchr(debugopt, 'o'))
    printf("%s\n", tag);
}

#else
# define debug(tag)	
#endif


#define dont_debug_version_check 1

#ifdef WSLTTY_VERSION
char * mtv = "https://raw.githubusercontent.com/mintty/wsltty/master/VERSION";
#define CHECK_APP "wsltty"
#define CHECK_VERSION STRINGIFY(WSLTTY_VERSION)
#else
char * mtv = "https://raw.githubusercontent.com/mintty/mintty/master/VERSION";
#define CHECK_APP APPNAME
#define CHECK_VERSION VERSION
#endif

static char * version_available = 0;
static bool version_retrieving = false;

static void
display_update(const char * new)
{
  if (!wv.config_wnd)
    return;

  //__ Options: dialog title
  const char * opt = _("Options");
  //__ Options: dialog title: "mintty <release> available (for download)"
  const char * avl = _("available");
  const char * pat = "%s            ▶ %s %s %s ◀";
  int len = strlen(opt) + strlen(CHECK_APP) + strlen(new) + strlen(avl) + strlen(pat) - 7;
  char * msg = newn(char, len);
  sprintf(msg, pat, opt, CHECK_APP, new, avl);
#ifdef debug_version_check
  printf("new version <%s> -> '%s'\n", new, msg);
#endif
  wchar * wmsg = cs__utftowcs(msg);
  free(msg);
  SendMessageW(wv.config_wnd, WM_SETTEXT, 0, (LPARAM)wmsg);
  free(wmsg);
}

static char * vfn = 0;
static void
getvfn()
{
  if (!vfn)
    vfn = asform("%s/.mintty-version", tmpdir());
}

void
update_available_version(bool ok)
{
  version_retrieving = false;
  if (!ok)
    return;
  getvfn();

  char vers[99];
  char * new = 0;
  FILE * vfd = fopen(vfn, "r");
  if (vfd) {
    if (fgets(vers, sizeof vers, vfd)) {
      vers[strcspn(vers, "\n")] = 0;
      new = vers;
#ifdef debug_version_check
      printf("update_available_version read <%s>\n", vers);
#endif
    }
    fclose(vfd);
  }
#if defined(debug_version_check) && debug_version_check > 1
  new = "9.9.9";  // test value
#endif
#ifdef debug_version_check
  printf("update_available_version: <%s>\n", new);
#endif

  /* V av new
     x  0  x    =
     x  0  y  ! =
     x  x  x
     x  x  y  ! =
     x  y  y  %
     x  y  z  ! =
  */
  if (new && strcmp(new, CHECK_VERSION))
    display_update(new);
  if (new && (!version_available || strcmp(new, version_available))) {
    if (version_available)
      free(version_available);
    version_available = strdup(new);
  }
#ifdef debug_version_check
  printf("update_available_version -> available <%s>\n", version_available);
#endif
}

static void
deliver_available_version()
{
  if (version_retrieving || !cfg.check_version_update)
    return;
  getvfn();

#if CYGWIN_VERSION_API_MINOR >= 74
  static time_t version_retrieved = 0;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (version_retrieved && ts.tv_sec - version_retrieved < cfg.check_version_update)
    return;
  version_retrieved = ts.tv_sec;
#endif

  version_retrieving = true;

  if (fork())
    return;  // do nothing in parent (or on failure)
  //setsid();  // failed attempt to avoid busy hourglass
  // proceed asynchronously, in child process

  // determine available version
  char * wfn = path_posix_to_win_a(vfn);
  bool ok = true;
#ifdef debug_version_check
  printf("deliver_available_version downloading to <%s>...\n", wfn);
#endif
#ifdef use_powershell
#warning on Windows 7, this hangs the mintty parent process!!!
  char * cmdpat = "powershell.exe -command '(new-object System.Net.WebClient).DownloadFile(\"%s\", \"%s\")'";
  char * cmd = newn(char, strlen(cmdpat) + strlen(mtv) + strlen(wfn) - 3);
  sprintf(cmd, cmdpat, mtv, wfn);
  system(cmd);
  free(cmd);
#else
  HRESULT (WINAPI * pURLDownloadToFile)(void *, LPCSTR, LPCSTR, DWORD, void *) = 0;
  pURLDownloadToFile = load_library_func("urlmon.dll", "URLDownloadToFileA");
  if (pURLDownloadToFile) {
#ifdef __CYGWIN__
    /* Need to sync the Windows environment */
    cygwin_internal(CW_SYNC_WINENV);
#endif
    if (S_OK != pURLDownloadToFile(NULL, mtv, wfn, 0, NULL))
      ok = false;
#ifdef debug_version_check
      printf("downloading %s -> %d to compare with <%s>\n", mtv, ok, CHECK_VERSION);
#endif
    chmod(vfn, 0666);
  }
  else
    ok = false;
#endif
  free(wfn);

  // notify terminal window to display the new available version
  SendMessageA(wv.wnd, WM_APP, ok, 0);  // -> parent -> update_available_version
#ifdef debug_version_check
  printf("deliver_available_version notified %d\n", ok);
#endif
  exit(0);
}


/*
   adapted from messageboxmanager.zip
   @ https://www.codeproject.com/articles/18399/localizing-system-messagebox
 */
static HHOOK windows_hook = 0;
static bool hooked_window_activated = false;

static void
hook_windows(HOOKPROC hookproc)
{
  windows_hook = SetWindowsHookExW(WH_CBT, hookproc, 0, GetCurrentThreadId());
}

static void
unhook_windows()
{
  UnhookWindowsHookEx(windows_hook);
  hooked_window_activated = false;
}

static void OnVScroll(HWND wnd,UINT nSBCode, int nPos) 
{
  SCROLLINFO si;
  memset(&si,0,sizeof(si));
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_ALL;
  if(!GetScrollInfo(wnd,SB_VERT,&si)) return;
  int sy;
  switch (nSBCode)
  {
    when SB_BOTTOM  : nPos = si.nMax;
    when SB_TOP     : nPos = si.nMin;
    when SB_LINEUP  : nPos =si.nPos -10; if (nPos<si.nMin) nPos = si.nMin;
    when SB_LINEDOWN: nPos =si.nPos +10; if (nPos>si.nMax) nPos = si.nMax;
    when SB_PAGEUP  : nPos =si.nPos -50; if (nPos<si.nMin) nPos = si.nMin;
    when SB_PAGEDOWN: nPos =si.nPos +50; if (nPos>si.nMax) nPos = si.nMax;
    when SB_ENDSCROLL: nPos =si.nPos ;
    when SB_THUMBPOSITION: sy=si.nPos-nPos;
    when SB_THUMBTRACK   : sy=si.nPos-nPos;
    otherwise: nPos=si.nPos;
  }
  sy=si.nPos-nPos; si.nPos=nPos;
  RECT r;
  ScrollWindowEx(wnd,0,sy,0,0,0,&r,SW_SCROLLCHILDREN);//SW_INVALIDATE|SW_ERASE);
  InvalidateRect(wnd,&r,1);
  SetScrollInfo(wnd,SB_VERT,&si,SIF_POS);
}
static LRESULT CALLBACK
swnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uid, DWORD_PTR data)
{
  (void)uid; (void)data;
  switch (msg) {
		when WM_VSCROLL: OnVScroll(hwnd,LOWORD(wParam),HIWORD(wParam));
    when WM_COMMAND or WM_DRAWITEM: {
      debug("WM_COMMAND");
      int ret = winctrl_handle_command(hwnd,msg, wParam, lParam);
      debug("WM_COMMAND: handle");
      if (dlg.ended) {
        PostMessage(hwnd, WM_CLOSE,0 , 0);
        //DestroyWindow(hwnd);
        debug("WM_COMMAND: Destroy");
      }
      debug("WM_COMMAND: end");
      return ret;
    }
    when WM_NOTIFY: {
      winctrl_handle_notify(hwnd,msg, wParam, lParam);
        //int id=LOWORD(wParam);
    }
  }
  return DefSubclassProc(hwnd, msg, wParam, lParam);
}

#define dont_darken_dialog_elements

#ifdef darken_dialog_elements
static LRESULT CALLBACK
tree_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR uid, DWORD_PTR data)
{
  (void)uid; (void)data;
  bool support_dark_mode = true;
  /// ... determine support_dark_mode as in win_dark_mode
  colour bg = RGB(22, 22, 22);
  /// ... retrieve bg from DarkMode_Explorer theme
  switch (msg) {
    when WM_ERASEBKGND:      // darken treeview background
      if (support_dark_mode) {
        HDC hdc = (HDC)wp;
        RECT rc;
        GetClientRect(wv.wnd, &rc);
        HBRUSH br = CreateSolidBrush(bg);
        int res = FillRect(hdc, &rc, br);
        DeleteObject(br);
        return res;
      }
  }
  return DefSubclassProc(hwnd, msg, wp, lp);
}
#endif

#define dont_debug_messages

/*
 * This function is the configuration box.
 * (Being a dialog procedure, in general it returns 0 if the default
 * dialog processing should be performed, and 1 if it should not.)
 */
extern void win_global_keyboard_hook(bool on,bool autooff);
static HFONT cfdfont=0;
char *wm_names[WM_USER]={
#  include "_wm.t"
};
char*get_wmname(UINT msg){
  char *n;
  static char buf[64];
  if(msg == WM_SETCURSOR || msg == WM_NCHITTEST || msg == WM_MOUSEMOVE ||
    msg == WM_ERASEBKGND || msg == WM_CTLCOLORDLG || msg == WM_PRINTCLIENT ||
    msg == WM_CTLCOLORBTN || msg == WM_ENTERIDLE 
    ){
    return NULL;
  }
  if(msg<WM_USER){
    n=wm_names[msg];
    if(n)return n;
    int i;
    for(i=msg;i;i--){
      if((n=wm_names[i]))break;
    }
    sprintf(buf,"%x %s+%x",i,n,msg-i);
    return buf;
  }
  sprintf(buf,"WM_USER+%x",msg-WM_USER);
  return buf;
}
static INT_PTR CALLBACK
config_dialog_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef debug_messages
//G #include <time.h>
  if(msg != WM_NOTIFY || (LOWORD(wParam) == IDCX_TREEVIEW && ((LPNMHDR) lParam)->code == TVN_SELCHANGED)){
    char * wm_name = get_wmname(msg);
    if(wm_name)printf("[%d] dialog_proc %04X %s (%04X %08X)\n", (int)time(0), msg, wm_name, (unsigned)wParam, (unsigned)lParam);
  }
#endif

#ifdef darken_dialog_elements
  bool support_dark_mode = true;
  /// ... determine support_dark_mode as in win_dark_mode
  colour fg = RGB(222, 22, 22); // test value
  colour bg = RGB(22, 22, 22);  // test value
  /// ... retrieve fg, bg from DarkMode_Explorer theme
#endif

  switch (msg) {
    when WM_ACTIVATE:{
      if ((wParam & 0xF) == WA_INACTIVE) {
        win_global_keyboard_hook(1,1);
      } else {
        win_global_keyboard_hook(1,0);
      }
    }
		when WM_VSCROLL: OnVScroll(dlg.ctlwnd,LOWORD(wParam),HIWORD(wParam));
    when WM_SETFONT: cfdfont=(HFONT)wParam;
    when WM_GETFONT: return (WPARAM)cfdfont;
    when WM_INITDIALOG: {
      win_set_font(hwnd);
      RECT r,r2;
      GetClientRect(hwnd,&r);
      GetWindowRect(hwnd,&r2);
      int x,y,w,h;
      x=0;w=SC(dialog_width)+r2.right-r2.left-r.right;
      y=0;h=SC(dialog_height)+r2.bottom-r2.top-r.bottom; 
      SetWindowPos(hwnd,0,0,0,w,h , SWP_NOMOVE|SWP_NOZORDER);
      ctrlbox = ctrl_new_box();
      setup_config_box(ctrlbox);
      windlg_init();
      winctrl_init(&ctrls_base);
      winctrl_init(&ctrls_panel);
      windlg_add_tree(&ctrls_base);
      windlg_add_tree(&ctrls_panel);

      /*
       * Create the actual GUI widgets.
       */
      // here we need the correct DIALOG_HEIGHT already
      create_controls(hwnd, W(""));        /* Open and Cancel buttons etc */

      SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG,
                  (LPARAM) LoadIcon(wv.inst, MAKEINTRESOURCE(IDI_MAINICON)));

      dlg.wnd = hwnd;

      x = SC( 70); w = SC(dialog_width -70)-1;
      y = SC(  3); h = SC(dialog_height - 30);
#ifdef USECTLWND
      dlg.ctlwnd = CreateWindowExA(WS_EX_CLIENTEDGE, SWND_CLASS , "",
                                   WS_CHILD | WS_VISIBLE|WS_VSCROLL ,x,y,w,h,hwnd, 0, wv.inst, null);
      SCROLLINFO si = {
        .cbSize = sizeof si,
        .fMask = SIF_ALL ,
        .nMin = 0,
        .nMax = SC(dialog_height - 30)*2,
        //滚动块自身的长短，通常有如下关系：其长度/滚动条长度（含两个箭头）=nPage/(nMax+2)，
        //另外nPage取值-1时，滚动条会不见了。
        .nPage = SC(dialog_height - 30),
        .nPos = 0
      };
      SetScrollInfo(dlg.ctlwnd,SB_VERT, &si,1);
      SetWindowSubclass(dlg.ctlwnd , swnd_proc, 0, 0); 
#else
      (void)swnd_proc;  
      dlg.ctlwnd=hwnd;
#endif

      /*
       * Create the tree view.
       */
      x = SC( 3); w = SC(64); 
      y = SC( 3); h = SC(dialog_height - 30);
      HWND treeview =
          CreateWindowExA(WS_EX_CLIENTEDGE, WC_TREEVIEWA, "",
                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASLINES |
                          TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_LINESATROOT
                          | TVS_SHOWSELALWAYS, x,y,w,h , hwnd, (HMENU) IDCX_TREEVIEW, wv.inst,
                          null);
      win_set_font(treeview);
      treeview_faff tvfaff;
      tvfaff.treeview = treeview;
      memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));

#ifdef darken_dialog_elements
      /*
       * Apply dark mode to tree menu background and active item
       */
      win_dark_mode(treeview); // active item
                               /// ... passive items?
      SetWindowSubclass(treeview, tree_proc, 0, 0); // background
#endif

      /*
       * Set up the tree view contents.
       */
      HTREEITEM hfirst = null;
      const wchar *path = null;
      wstring defpane=_W("HotKeys");
      for (int i = 0; i < ctrlbox->nctrlsets; i++) {
        controlset *s = ctrlbox->ctrlsets[i];
        HTREEITEM item;
        int j=0;
        const wchar *c;

        if (!s->pathname[0])
          continue;
        if (path&&(j=ctrl_path_compare(s->pathname, path)) == -1)
          continue;   /* same path, nothing to add to tree */

        /*
         * We expect never to find an implicit path component. 
         For example, we expect never to see A/B/C followed by A/D/E, 
         because that would _implicitly_ create A/D. 
         All our path prefixes are expected to contain actual controls 
         and be selectable in the treeview; so we would expect 
         to see A/D _explicitly_ before encountering A/D/E.
         */

        c = wcsrchr(s->pathname, '/');
        if (!c)
          c = s->pathname;
        else
          c++;

        item = treeview_insert(&tvfaff, j, c, s->pathname);
        if (!hfirst) hfirst = item;
        if(defpane &&ctrl_path_compare(s->pathname,defpane )==-1){
          hfirst = item;
        }
        path = s->pathname;
      }

      /*
       * Put the treeview selection on to the Session panel.
       * This should also cause creation of the relevant controls.
       */
      (void)TreeView_SelectItem(treeview, hfirst);

      /*
       * Set focus into the first available control.
       */
      for (winctrl *c = ctrls_panel.first; c; c = c->next) {
        if (c->ctrl) {
          dlg_set_focus(c->ctrl);
          break;
        }
      }
    }

#ifdef darken_dialog_elements
    when WM_CTLCOLOREDIT     // popup items
      or WM_CTLCOLORLISTBOX  // popup menu
      or WM_CTLCOLORBTN      // button borders; for buttons, see doctl
      or WM_CTLCOLORDLG      // dialog background
      or WM_CTLCOLORSTATIC   // labels
      // or WM_CTLCOLORMSGBOX or WM_CTLCOLORSCROLLBAR ?
      :
        // setting fg fails for WM_CTLCOLORSTATIC
        if (support_dark_mode) {
          HDC hdc = (HDC)wParam;
          SetTextColor(hdc, fg);
          SetBkColor(hdc, bg);
          return (INT_PTR)CreateSolidBrush(bg);
        }
#ifdef draw_dialog_bg
    when WM_ERASEBKGND:      // handled via WM_CTLCOLORDLG above
      if (support_dark_mode) {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH br = CreateSolidBrush(bg);
        int res = FillRect(hdc, &rc, br);
        DeleteObject(br);
        return res;
      }
#endif
#endif

    when WM_DPICHANGED:
      if (*cfg.opt_font.name || cfg.opt_font.size != DIALOG_FONTSIZE)
        // rescaling does not work, so better drop the Options dialog
        DestroyWindow(hwnd);

    when WM_CLOSE:
      DestroyWindow(hwnd);

    when WM_DESTROY:
      winctrl_cleanup(&ctrls_base);
      winctrl_cleanup(&ctrls_panel);
      ctrl_free_box(ctrlbox);
      wv.config_wnd = 0;

#ifdef debug_dialog_crash
      signal(SIGSEGV, SIG_DFL);
#endif

    when WM_USER: {
      debug("WM_USER");
      HWND target = (HWND)wParam;
      // could delegate this to winctrls.c, like winctrl_handle_command;
      // but then we'd have to fiddle with the location of dragndrop
     /*
      * Look up the window handle in our data; find the control.
        (Hmm, apparently it works without looking for the widget entry 
        that was particularly introduced for this purpose...)
      */
      control * ctrl = null;
      int cid=-1;
      for (winctrl *c = ctrls_panel.first; c && !ctrl; c = c->next) {
        if (c->ctrl)
          for (int k = 0; k < c->num_ids; k++) {
#ifdef debug_dragndrop
            printf(" [->%8p] %8p\n", target, GetDlgItem(hwnd, c->base_id + k));
#endif
            if (target == GetDlgItem(hwnd, c->base_id + k)) {
              cid=c->base_id + k;
              ctrl = c->ctrl;
              break;
            }
        }
      }
      debug("WM_USER: lookup");
      if (ctrl) {
        //dlg_editbox_setW(ctrl, L"Test");  // may hit unrelated items...
        // drop the drag-and-drop contents here
        dragndrop = (wstring)lParam;
        if(ctrl->handler)ctrl->handler(ctrl, cid,EVENT_DROP,0);
        debug("WM_USER: handler");
      }
      debug("WM_USER: end");
    }

    when WM_NOTIFY: {
      if (LOWORD(wParam) == IDCX_TREEVIEW && ((LPNMHDR) lParam)->code == TVN_SELCHANGED) {
        debug("WM_NOTIFY");
        HTREEITEM i = TreeView_GetSelection(((LPNMHDR) lParam)->hwndFrom);
        debug("WM_NOTIFY: GetSelection");

        TVITEM item;
        item.hItem = i;
        item.mask = TVIF_PARAM;
        (void)TreeView_GetItem(((LPNMHDR) lParam)->hwndFrom, &item);
        /* Destroy all controls in the currently visible panel. */
        for (winctrl *c = ctrls_panel.first; c; c = c->next) {
          for (int k = 0; k < c->num_ids; k++) {
            HWND item = GetDlgItem(dlg.ctlwnd, c->base_id + k);
            if (item)
              DestroyWindow(item);
          }
        }
        debug("WM_NOTIFY: Destroy");
        winctrl_cleanup(&ctrls_panel);
        debug("WM_NOTIFY: cleanup");
        OnVScroll(dlg.ctlwnd,SB_TOP,0);
        // here we need the correct DIALOG_HEIGHT already
        create_controls(dlg.ctlwnd, (wchar *) item.lParam);
        debug("WM_NOTIFY: create");
        dlg_refresh(null); /* set up control values */
        debug("WM_NOTIFY: refresh");
      }else winctrl_handle_notify(hwnd,msg, wParam, lParam);
    }

    when WM_COMMAND or WM_DRAWITEM: {
      debug("WM_COMMAND");
      int ret = winctrl_handle_command(hwnd,msg, wParam, lParam);
      debug("WM_COMMAND: handle");
      if (dlg.ended) {
        PostMessage(hwnd, WM_CLOSE,0 , 0);
        //DestroyWindow(hwnd);
        debug("WM_COMMAND: Destroy");
      }
      debug("WM_COMMAND: end");
      return ret;
    }
  }
#ifdef debug_messages
//  catch return above
//  printf(" end dialog_proc %04X %s (%04X %08X)\n", msg, wm_name, (unsigned)wParam, (unsigned)lParam);
#endif
  return 0;
}

static LRESULT CALLBACK
scale_options(int nCode, WPARAM wParam, LPARAM lParam)
{
  (void)wParam;

#define dont_debug_scale_options

#ifdef debug_scale_options
  char * hcbt[] = {
    "HCBT_MOVESIZE",
    "HCBT_MINMAX",
    "HCBT_QS",
    "HCBT_CREATEWND",
    "HCBT_DESTROYWND",
    "HCBT_ACTIVATE",
    "HCBT_CLICKSKIPPED",
    "HCBT_KEYSKIPPED",
    "HCBT_SYSCOMMAND",
    "HCBT_SETFOCUS",
  };
  char * sCode = "?";
  if (nCode >= 0 && nCode < (int)lengthof(hcbt))
    sCode = hcbt[nCode];
  printf("hook %d %s", nCode, sCode);
  if (nCode == HCBT_CREATEWND) {
    CREATESTRUCTW * cs = ((CBT_CREATEWNDW *)lParam)->lpcs;
    printf(" x %3d y %3d w %3d h %3d (%08X %07X) <%ls>\n", cs->x, cs->y, cs->cx, cs->cy, (uint)cs->style, (uint)cs->dwExStyle, cs->lpszName);
  }
  else if (nCode == HCBT_ACTIVATE) {
    bool from_mouse = ((CBTACTIVATESTRUCT *)lParam)->fMouse;
    printf(" mou %d\n", from_mouse);
  }
  else
    printf("\n");
#endif

  if (nCode == HCBT_CREATEWND) {
    // dialog item geometry calculations and adjustments
    CREATESTRUCTW * cs = ((CBT_CREATEWNDW *)lParam)->lpcs;
    if (!(cs->style & WS_CHILD)) {
      //__ Options: dialog width scale factor (80...200)
      if (cfg.scale_options_width >= 80 && cfg.scale_options_width <= 200)
        cs->cx = cs->cx * cfg.scale_options_width / 100;
      // scale Options dialog with custom font
      cs->cx = scale_dialog(cs->cx);
      cs->cy = scale_dialog(cs->cy);
    }
  }

  //return CallNextHookEx(0, nCode, wParam, lParam);
  return 0;  // 0: let default dialog box procedure process the message
}
void
win_open_config(void)
{
  if (wv.config_wnd)
    return;

#ifdef debug_dialog_crash
  signal(SIGSEGV, sigsegv);
#endif

  set_dpi_auto_scaling(true);

  static bool initialised = false;
  if (!initialised) {
    InitCommonControls();
    RegisterClassW(&(WNDCLASSW){
      .lpszClassName = W(DIALOG_CLASS),
      .lpfnWndProc = DefDlgProcW,
      .style = CS_DBLCLKS,
      .cbClsExtra = 0,
      .cbWndExtra = DLGWINDOWEXTRA + 2 * sizeof(LONG_PTR),
      .hInstance = wv.inst,
      .hIcon = null,
      .hCursor = LoadCursor(null, IDC_ARROW),
      .hbrBackground = (HBRUSH)(COLOR_WINDOW),
      .lpszMenuName = null
    });
    RegisterClassW(&(WNDCLASSW){
      .lpszClassName = W(SWND_CLASS),
      //.lpfnWndProc = DefWindowProcA ,
      .lpfnWndProc = DefDlgProcW,
      .style = CS_DBLCLKS,
      .cbClsExtra = 0,
      .cbWndExtra = DLGWINDOWEXTRA + 2 * sizeof(LONG_PTR),
      .hInstance = wv.inst,
      .hIcon = null,
      .hCursor = LoadCursor(null, IDC_ARROW),
      .hbrBackground = (HBRUSH)(COLOR_WINDOW),
      .lpszMenuName = null
    });
    initialised = true;
  }
  HDC dc = GetDC(wv.wnd);
  ldpi= GetDeviceCaps(dc, LOGPIXELSY) ;
  ReleaseDC(wv.wnd, dc);
  cfg.opt_font.size = cfg.opt_font.size ?: DIALOG_FONTSIZE;
  heightsc=(cfg.opt_font.size ?: DIALOG_FONTSIZE)*100*ldpi/(720);
  dialog_width  = DLGW*10;
  dialog_height = DLGH*12;

  hook_windows(scale_options);
  wv.config_wnd = CreateDialog(wv.inst, MAKEINTRESOURCE(IDD_MAINBOX), wv.wnd, config_dialog_proc);
  unhook_windows();
  // At this point, we could actually calculate the size of the 
  // dialog box used for the Options menu; however, the resulting 
  // value(s) (here DIALOG_HEIGHT) is already needed before this point, 
  // as the callback config_dialog_proc sets up dialog box controls.
  // How insane is that resource concept! Shouldn't I know my own geometry?


  // Set title of Options dialog explicitly to facilitate I18N
  //__ Options: dialog title
  SendMessageW(wv.config_wnd, WM_SETTEXT, 0, (LPARAM)_W("Options"));
  if (version_available && strcmp(CHECK_VERSION, version_available))
    display_update(version_available);
  deliver_available_version();

  // Apply dark mode to dialog title
  win_dark_mode(wv.config_wnd);

  ShowWindow(wv.config_wnd, SW_SHOW);
  //win_update_shortcuts();
  set_dpi_auto_scaling(false);
}


static wstring oklabel = null;
static int oktype = MB_OK;

static LRESULT CALLBACK
set_labels(int nCode, WPARAM wParam, LPARAM lParam)
{
  (void)lParam;

#define dont_debug_message_box

  void setlabel(int id, wstring label) {
    HWND button = GetDlgItem((HWND)wParam, id);
#ifdef debug_message_box
    if (button) {
      wchar buf [99];
      GetWindowTextW(button, buf, 99);
      printf("%d [%8p] <%ls> -> <%ls>\n", id, button, buf, label);
    }
    else
      printf("%d %% (<%ls>)\n", id, label);
#endif
    if (button)
      SetWindowTextW(button, label);
  }

  if (nCode == HCBT_ACTIVATE) {
#ifdef debug_message_box
    bool from_mouse = ((CBTACTIVATESTRUCT *)lParam)->fMouse;
    printf("HCBT_ACTIVATE (mou %d) OK %d ok <%ls>\n", from_mouse, (oktype & MB_TYPEMASK) == MB_OK, oklabel);
#endif
    if (!hooked_window_activated) {
      // we need to distinguish re-focussing from initial activation
      // in order to avoid repeated setting of dialog item labels
      // because the IDOK item is renumbered after initial setting
      // (so we would overwrite the wrong label) - what a crap!
      // alternative check:
      // if (!((CBTACTIVATESTRUCT *)lParam)->fMouse)
      //   but that fails if re-focussing is done without mouse (e.g. Alt+TAB)
      if ((oktype & MB_TYPEMASK) == MB_OK)
        setlabel(IDOK, oklabel ?: _W("I see"));
      else
        setlabel(IDOK, oklabel ?: _W("OK"));
      setlabel(IDCANCEL, _W("Cancel"));
#ifdef we_would_use_these_in_any_message_box
#warning W -> _W to add the labels to the localization repository
#warning predefine button labels in config.c
      setlabel(IDABORT, W("&Abort"));
      setlabel(IDRETRY, W("&Retry"));
      setlabel(IDIGNORE, W("&Ignore"));
      setlabel(IDYES, oklabel ?: W("&Yes"));
      setlabel(IDNO, W("&No"));
      //IDCLOSE has no label
      setlabel(IDHELP, W("Help"));
      setlabel(IDTRYAGAIN, W("&Try Again"));
      setlabel(IDCONTINUE, W("&Continue"));
#endif
    }

    hooked_window_activated = true;
  }

  //return CallNextHookEx(0, nCode, wParam, lParam);
  return 0;  // 0: let default dialog box procedure process the message
}

int
message_box(HWND parwnd,const  char * text,const  char * caption, int type, wstring ok)
{
  if (!text)
    return 0;
  if (!caption)
    caption = _("Error");

  oklabel = ok;
  oktype = type;
  if (type != MB_OK)
    type |= MB_SETFOREGROUND;
  hook_windows(set_labels);
  int ret;
  if (nonascii(text) || nonascii(caption)) {
    wchar * wtext = cs__utftowcs(text);
    wchar * wcapt = cs__utftowcs(caption);
    ret = MessageBoxW(parwnd, wtext, wcapt, type);
    free(wtext);
    free(wcapt);
  }
  else
    ret = MessageBoxA(parwnd, text, caption, type);
  unhook_windows();
  return ret;
}

int
message_box_w(HWND parwnd, const wchar * wtext, const wchar * wcaption, int type, wstring ok)
{
  if (!wtext)
    return 0;
  if (!wcaption)
    wcaption = _W("Error");

  oklabel = ok;
  oktype = type;
  if (type != MB_OK)
    type |= MB_SETFOREGROUND;
  hook_windows(set_labels);
  int ret;
  ret = MessageBoxW(parwnd, wtext, wcaption, type);
  unhook_windows();
  return ret;
}

#ifdef about_version_check
static void CALLBACK
hhook(LPHELPINFO lpHelpInfo)
{
  // test
  SetWindowTextW(lpHelpInfo->hItemHandle, W("mintty %s available"));
}
#endif

void
win_show_about(void)
{
#if CYGWIN_VERSION_API_MINOR < 74
  char * aboutfmt = newn(char, 
    strlen(VERSION_TEXT) + strlen(COPYRIGHT) + strlen(LICENSE_TEXT) +strlen(RELEASEINFO) + strlen(_(WARRANTY_TEXT)) + strlen(_(ABOUT_TEXT)) + 11);
  sprintf(aboutfmt, "%s\n%s\n%s\n%s\n%s\n\n%s", 
           VERSION_TEXT, COPYRIGHT, LICENSE_TEXT,RELEASEINFO, _(WARRANTY_TEXT), _(ABOUT_TEXT));
  char * abouttext = newn(char, strlen(aboutfmt) + strlen(WEBSITE));
  sprintf(abouttext, aboutfmt, WEBSITE);
#else
  DWORD win_version = GetVersion();
  uint build = HIWORD(win_version);
  char * aboutfmt =
    asform("%s [Windows %u]\n%s\n%s\n%s\n%s\n\n%s", 
           VERSION_TEXT, build, COPYRIGHT, LICENSE_TEXT,RELEASEINFO, _(WARRANTY_TEXT), _(ABOUT_TEXT));
  char * abouttext = asform(aboutfmt, WEBSITE);
#endif
  free(aboutfmt);
  wchar * wmsg = cs__utftowcs(abouttext);
  free(abouttext);
  oklabel = null;
  oktype = MB_OK;
  hook_windows(set_labels);
  MessageBoxIndirectW(&(MSGBOXPARAMSW){
    .cbSize = sizeof(MSGBOXPARAMSW),
    .hwndOwner = wv.config_wnd,
    .hInstance = wv.inst,
    .lpszCaption = W(APPNAME),
#ifdef about_version_check
    .dwStyle = MB_USERICON | MB_OK | MB_HELP,
    .lpfnMsgBoxCallback = hhook,
#else
    .dwStyle = MB_USERICON | MB_OK,
#endif
    .lpszIcon = MAKEINTRESOURCEW(IDI_MAINICON),
    .lpszText = wmsg
  });
  unhook_windows();
  free(wmsg);
}

void
win_show_error(const char * msg)
{
  message_box(0, msg, null, MB_ICONERROR, 0);
}

void
win_show_warning(const char * msg)
{
  message_box(0, msg, null, MB_ICONWARNING, 0);
}

bool
win_confirm_text(wchar * text, wchar * caption)
{
  int ret = message_box_w(0, text, caption, MB_OKCANCEL | MB_DEFBUTTON2, 0);
  return ret == IDOK;
}

