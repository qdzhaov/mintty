//code for ShortCuts Key Process
//included by wininput.c
/* Support functions */

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
    transparency_tuned = false;
  }
  if (cfg.opaque_when_focused)
    win_update_transparency(cfg.transparency, false);
}

static void
transparency_opaque()
{
  win_update_transparency(cfg.transparency, true);
}

static void
newwin(uint key, mod_keys mods)
{
	// defer send_syscommand(IDM_NEW) until key released
	// monitor cursor keys to collect parameters meanwhile
  newwin_pending = true;
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
  return 1;
}
static uint mflags_defsize() { return (IsZoomed(wv.wnd) || term.cols != cfg.cols || term.rows != cfg.rows) ? MF_ENABLED : MF_GRAYED; }
#ifdef allow_disabling_scrollbar
static uint mflags_scrollbar_outer() { return term.show_scrollbar ? MF_CHECKED : MF_UNCHECKED | cfg.scrollbar ? 0 : MF_GRAYED ; }
#else
static uint mflags_scrollbar_outer() { return term.show_scrollbar ? MF_CHECKED : MF_UNCHECKED ; }
#endif
static uint mflags_scrollbar_inner() { return cfg.scrollbar?(  term.show_scrollbar ? MF_CHECKED : MF_UNCHECKED): MF_GRAYED; }
static uint mflags_logging() { return (wv.logging ? MF_CHECKED : MF_UNCHECKED) ; }
static uint mflags_bidi() { return (cfg.bidi == 0 || (cfg.bidi == 1 && (term.on_alt_screen ^ term.show_other_screen))) ? MF_GRAYED : term.disable_bidi ? MF_UNCHECKED : MF_CHECKED; }
static uint mflags_dim_margins() { return term.dim_margins ? MF_CHECKED : MF_UNCHECKED; }
static uint mflags_status_line() { return term.st_type == 1 ? MF_CHECKED : MF_UNCHECKED; }
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
static int  vtabclose    (){if(!child_is_alive(&term)) {win_tab_clean();return 1;}return 0;}
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
  else term_set_status_type(1, 0);
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
  DFDN(transparency-opaque,transparency_opaque, 0),

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
  SETSCK(0,VK_RETURN     ,FT_NORV,vtabclose);
  SETSCK(0,VK_ESCAPE     ,FT_NORV,vtabclose);
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
    SETSCK(W ,'Q'        ,FT_NORM,app_close    );
    SETSCK(W ,'T'        ,FT_CMD ,IDM_NEWTAB   );
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
    SETSCK(A,VK_F3   ,FT_NORM,IDM_SEARCH);
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
    SETSCK(CS,'F'     ,FT_NORM,IDM_FULLSCREEN_ZOOM );
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

