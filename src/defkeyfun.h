#define SK MDK_SHIFT
#define AK MDK_ALT
#define CK MDK_CTRL
#define WK MDK_WIN
#define AS MDK_ALT+MDK_SHIFT
#define CS MDK_CTRL+MDK_SHIFT
#define WS MDK_WIN+MDK_SHIFT
#define HK0() {{0}}
#define HK1(f,m,k) {{m,f,k},{0}}
#define HK2(f,m,k,f1,m1,k1) {{m,f,k},{m1,f1,k1},{0}}
#define HK3(f,m,k,f1,m1,k1,f2,m2,k2) {{m,f,k},{m1,f1,k1},{m2,f2,k2}}
#define MF(a) mflags_##a
#define DFDZ(l,n,f,t,s,K) {l,n,t,.fv=(void*)f,s,tip_##f,K},
//Window Oprator
  DFDZ(5,"move"                ,SC_MOVE              ,FT_CMD , 0,              HK0())
  DFDZ(5,"resize"              ,SC_SIZE              ,FT_CMD , 0,              HK0())
  DFDZ(5,"minimize"            ,SC_MINIMIZE          ,FT_CMD , 0,              HK0())
  DFDZ(5,"maximize"            ,SC_MAXIMIZE          ,FT_CMD , 0,              HK0())
  DFDZ(0,"menu"                ,SC_KEYMENU           ,FT_CMD , 0,              HK2(1,SK,VK_APPS,2,AK,VK_SPACE))
  DFDZ(0,"close"               ,SC_CLOSE             ,FT_CMD , 0,              HK1(6,AK,VK_F4))
  DFDZ(0,"app-close"           ,app_close            ,FT_NORM, 0,              HK1(4,WK,'Q'))
  DFDZ(0,"close"               ,win_close            ,FT_NORM, 0,              HK2(4,WK,'W',7,CS,'W'))
  DFDZ(0,"newtab"              ,IDM_NEWTAB           ,FT_CMD , 0,              HK2(4,WK,'T'      ,7,CS,'N' ) ) //new_tab_def
  DFDZ(9,"new"                 ,IDM_NEW              ,FT_CMD , 0,              HK0()) //new_win_def
  DFDZ(9,"new-window"          ,IDM_NEW              ,FT_CMD , 0,              HK0())
  DFDZ(9,"new-key"             ,IDM_NEW              ,FT_CMD , 0,              HK0())
  DFDZ(0,"newwin"              ,newwin               ,FT_KEY , 0,              HK1(6,AK,VK_F2))
  DFDZ(9,"new-monitor"         ,IDM_NEW_MONI         ,FT_CMD , 0,              HK0())  //new_win(IDSS_DEF,lp);
  DFDZ(0,"default-size"        ,IDM_DEFSIZE          ,FT_CMD , 0,              HK1(7,CS,'D')) //wm:default_size 
  DFDZ(0,"default-size"        ,IDM_DEFSIZE_ZOOM     ,FT_CMD , MF(defsize),    HK1(6,AK,VK_F10)) //wm:default_size,wt:win_zoom_font
  DFDZ(0,"toggle-fullscreen"   ,IDM_FULLSCREEN_ZOOM  ,FT_CMD , MF(fullscreen), HK2(1,AS,VK_RETURN,7,CS,'F')) //wm:win_maximise
  DFDZ(0,"fullscreen"          ,IDM_FULLSCREEN       ,FT_CMD , MF(fullscreen), HK2(1,AK,VK_RETURN,6,AK,VK_F11))//wm:win_maximise
  DFDZ(5,"win-max"             ,window_max           ,FT_NORM, MF(zoomed),     HK0())
  DFDZ(5,"win-toggle-max"      ,window_toggle_max    ,FT_NORM, MF(zoomed),     HK0())
  DFDZ(5,"win-restore"         ,window_restore       ,FT_NORM, 0,              HK0())
  DFDZ(5,"win-icon"            ,window_min           ,FT_NORM, 0,              HK0())
  DFDZ(0,"win_hide"            ,win_hide             ,FT_NORM, 0,              HK1(4,WK,'Z'      ) )
  DFDZ(5,"win-toggle-on-top"   ,win_toggle_on_top    ,FT_NORM, MF(always_top), HK0()) //wm
  DFDZ(5,"win-toggle-screen-on",win_toggle_screen_on ,FT_NORM, MF(screen_on ), HK0())

  DFDZ(0,"tab-prev"            ,tab_prev             ,FT_NORM, 0,              HK2(3,CS,VK_TAB,4,WK,VK_LEFT))
  DFDZ(0,"tab-next"            ,tab_next             ,FT_NORM, 0,              HK2(3,CK,VK_TAB,4,WK,VK_RIGHT))
  DFDZ(0,"tab-move-prev"       ,tab_move_prev        ,FT_NORM, 0,              HK1(4,WS,VK_LEFT))
  DFDZ(0,"tab-move-next"       ,tab_move_next        ,FT_NORM, 0,              HK1(4,WS,VK_RIGHT))
  DFDZ(9,"switch-prev"         ,tab_prev             ,FT_NORM, 0,              HK0())
  DFDZ(9,"switch-next"         ,tab_next             ,FT_NORM, 0,              HK0())
  DFDZ(9,"switch-visible-prev" ,tab_prev             ,FT_NORM, 0,              HK0())
  DFDZ(9,"switch-visible-next" ,tab_next             ,FT_NORM, 0,              HK0())

  DFDZ(7,"win-tab-change"      ,win_tab_change       ,FT_PAR1, 0,              HK0()) //wtabz
  DFDZ(7,"win-tab-go"          ,win_tab_go           ,FT_PAR1, 0,              HK0()) //wtabz
  DFDZ(7,"win-tab-move"        ,win_tab_move         ,FT_PAR1, 0,              HK0()) //wtabz
  DFDZ(0,"zoom-font-out"       ,zoom_font_out        ,FT_NORM, 0,              HK2(8,CK,VK_SUBTRACT,8,CK,VK_OEM_MINUS))
  DFDZ(0,"zoom-font-in"        ,zoom_font_in         ,FT_NORM, 0,              HK2(8,CK,VK_ADD,8,CK,VK_OEM_PLUS))
  DFDZ(0,"zoom-font-reset"     ,zoom_font_reset      ,FT_NORM, 0,              HK2(8,CK,VK_NUMPAD0,8,CK,'0'))
  DFDZ(0,"zoom-font-sout"      ,zoom_font_sout       ,FT_NORM, 0,              HK1(8,CS,VK_SUBTRACT))
  DFDZ(0,"zoom-font-sin"       ,zoom_font_sin        ,FT_NORM, 0,              HK1(8,CS,VK_ADD))
  DFDZ(0,"zoom-font-sreset"    ,zoom_font_sreset     ,FT_NORM, 0,              HK1(8,CS,VK_NUMPAD0))
  DFDZ(7,"win-zoom-font"       ,win_zoom_font        ,FT_PAR2, 0,              HK0()) //wt

  DFDZ(5,"hor-left-1"          ,hor_left_1           ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-right-1"         ,hor_right_1          ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-left-mult"       ,hor_left_mult        ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-right-mult"      ,hor_right_mult       ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-out-1"           ,hor_out_1            ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-in-1"            ,hor_in_1             ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-out-mult"        ,hor_out_mult         ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-in-mult"         ,hor_in_mult          ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-narrow-1"        ,hor_narrow_1         ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-wide-1"          ,hor_wide_1           ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-narrow-mult"     ,hor_narrow_mult      ,FT_NORM, 0,              HK0())
  DFDZ(5,"hor-wide-mult"       ,hor_wide_mult        ,FT_NORM, 0,              HK0())

  DFDZ(0,"scroll-top"          ,scroll_top           ,FT_NORM, 0,              HK0())
  DFDZ(0,"scroll-end"          ,scroll_end           ,FT_NORM, 0,              HK0())
  DFDZ(0,"scroll-pgup"         ,scroll_pgup          ,FT_NORM, 0,              HK0())
  DFDZ(0,"scroll-pgdn"         ,scroll_pgdn          ,FT_NORM, 0,              HK0())
  DFDZ(0,"scroll-lnup"         ,scroll_lnup          ,FT_NORM, 0,              HK0())
  DFDZ(0,"scroll-lndn"         ,scroll_lndn          ,FT_NORM, 0,              HK0())
  DFDZ(0,"scroll-prev"         ,scroll_prev          ,FT_NORM, 0,              HK0())
  DFDZ(0,"scroll-next"         ,scroll_next          ,FT_NORM, 0,              HK0())

  DFDZ(5,"clear-scroll-lock"   ,clear_scroll_lock    ,FT_NORM, MF(no_scroll),  HK0())
  DFDZ(5,"no-scroll"           ,no_scroll            ,FT_NORM, MF(no_scroll),  HK0())
  DFDZ(5,"toggle-no-scroll"    ,toggle_no_scroll     ,FT_NORM, MF(no_scroll),  HK0())
  DFDZ(5,"scroll-mode"         ,scroll_mode          ,FT_NORM, MF(scroll_mode),HK0())
  DFDZ(5,"toggle-scroll-mode"  ,toggle_scroll_mode   ,FT_NORM, MF(scroll_mode),HK0())

  DFDZ(0,"options"             ,IDM_OPTIONS          ,FT_CMD , MF(options),    HK0()) //wm:win_open_config
  DFDZ(0,"win_ctrlmode"        ,win_ctrlmode         ,FT_NORM, 0,              HK1(4,WK,'X'      ) )
  DFDZ(0,"win-key-menu"        ,win_key_menu         ,FT_KEYV, 0,              HK2(1,CK,VK_APPS,1,0 ,VK_APPS))
  DFDZ(0,"menu-text"           ,menu_text            ,FT_NORM, 0,              HK0())
  DFDZ(0,"menu-pointer"        ,menu_pointer         ,FT_NORM, 0,              HK1(7,CS,'I'))
  DFDZ(9,"win-popup-menu"      ,menu_pointer         ,FT_NORM, 0,              HK0())

  DFDZ(0,"kb-select"           ,kb_select            ,FT_KEYV, MF(kb_select),  HK0())
  DFDZ(0,"select-all"          ,IDM_SELALL           ,FT_CMD , 0,              HK1(7,CS,'A')) //tc:term_select_all
  DFDZ(9,"term-select-all"     ,term_select_all      ,FT_NORM, 0,              HK0()) //tc:
  DFDZ(0,"copy"                ,IDM_COPY             ,FT_CMD , MF(copy),       HK3(5,CK,VK_INSERT,5,CK,VK_F12,7,CS,'C')) //tc:term_copy
  DFDZ(9,"term-copy"           ,IDM_COPY             ,FT_NORM, 0,              HK0()) //tc:term_copy
  DFDZ(0,"paste"               ,IDM_PASTE            ,FT_CMD , MF(paste),      HK3(5,SK,VK_INSERT,5,SK,VK_F12,7,CS,'V')) //wc:win_paste
  DFDZ(9,"win-paste"           ,IDM_PASTE            ,FT_NORM, 0,              HK0()) //wc:win_paste
  DFDZ(5,"open"                ,IDM_OPEN             ,FT_CMD , MF(open),       HK0()) //tc:term_open
  DFDZ(5,"copy-text"           ,IDM_COPY_TEXT        ,FT_CMD , MF(copy),       HK0()) //tc:term_copy_as
  DFDZ(5,"copy-tabs"           ,IDM_COPY_TABS        ,FT_CMD , MF(copy),       HK0()) //tc:term_copy_as
  DFDZ(5,"copy-plain"          ,IDM_COPY_TXT         ,FT_CMD , MF(copy),       HK0()) //tc:term_copy_as
  DFDZ(5,"copy-rtf"            ,IDM_COPY_RTF         ,FT_CMD , MF(copy),       HK0()) //tc:term_copy_as
  DFDZ(5,"copy-html-text"      ,IDM_COPY_HTXT        ,FT_CMD , MF(copy),       HK0()) //tc:term_copy_as
  DFDZ(5,"copy-html-format"    ,IDM_COPY_HFMT        ,FT_CMD , MF(copy),       HK0()) //tc:term_copy_as
  DFDZ(5,"copy-html-full"      ,IDM_COPY_HTML        ,FT_CMD , MF(copy),       HK0()) //tc:term_copy_as
  DFDZ(5,"paste-path"          ,win_paste_path       ,FT_NORM, MF(paste),      HK0()) //wc:
  DFDZ(5,"copy-paste"          ,IDM_COPASTE          ,FT_CMD , MF(copy),       HK0()) //tc:term_copy(); wc:win_paste()
  DFDZ(5,"export-html"         ,IDM_HTML             ,FT_CMD , 0,              HK0()) //tc:term_export_html
  DFDZ(5,"print-screen"        ,print_screen         ,FT_NORM, 0,              HK0()) //tc:print_screen
  DFDZ(5,"tek-page"            ,IDM_TEKPAGE          ,FT_CMD , MF(tek_mode),   HK0()) //tek:tek_page
  DFDZ(5,"tek-copy"            ,IDM_TEKCOPY          ,FT_CMD , MF(tek_mode),   HK0()) //wm:term_save_image
  DFDZ(5,"copy-title"          ,IDM_COPYTITLE        ,FT_CMD , 0,              HK0()) //wm:win_copy_title
  DFDZ(5,"save-image"          ,IDM_SAVEIMG          ,FT_CMD , 0,              HK0()) //wm:term_save_image

  DFDZ(0,"search"              ,IDM_SEARCH           ,FT_CMD , 0,              HK2(6,AK,VK_F3,7,CS,'H')) //ws:win_open_search
  DFDZ(0,"scrollbar"           ,IDM_SCROLLBAR        ,FT_CMD , MF(scrollbar),  HK1(7,CS,'O')) //wm:win_tog_scrollbar
  DFDZ(0,"toggle-dim-margins"  ,toggle_dim_margins   ,FT_NORM, MF(dim_margins),HK0())
  DFDZ(0,"toggle-status-line"  ,toggle_status_line   ,FT_NORM, MF(status_line),HK0())
  DFDZ(0,"borders"             ,IDM_BORDERS          ,FT_CMD , 0,              HK0()) //wm:win_tog_border
  DFDZ(0,"win-tab-show"        ,win_tab_show         ,FT_NORM, 0,              HK0()) //wtab:
  DFDZ(0,"win-tab-indicator"   ,win_tab_indicator    ,FT_NORM, 0,              HK0()) //wtab:
  DFDZ(0,"win-tog-partline"    ,win_tog_partline     ,FT_NORM, 0,              HK0()) //wm:

  DFDZ(0,"cycle-pointer-style" ,cycle_pointer_style  ,FT_NORM, 0,              HK1(7,CS,'P'))
  DFDZ(0,"transparency-level"  ,transparency_level   ,FT_NORM, 0,              HK1(7,CS,'T'))
  DFDZ(9,"cycle-transpy-level" ,transparency_level   ,FT_NORM, 0,              HK0())
  DFDZ(9,"toggle-opaque"       ,toggle_opaque        ,FT_NORM, MF(opaque),     HK0())

  DFDZ(5,"clear-scrollback"    ,IDM_CLRSCRLBCK       ,FT_CMD , 0,              HK0()) //term:term_clear_scrollback
  DFDZ(0,"reset"               ,IDM_RESET            ,FT_CMD , 0,              HK1(6,AK,VK_F8)) //term:term_reset
  DFDZ(0,"reset-noask"         ,IDM_RESET_NOASK      ,FT_CMD , 0,              HK1(7,CS,'R')) //term:term_reset
  DFDZ(5,"tek-reset"           ,IDM_TEKRESET         ,FT_CMD , MF(tek_mode),   HK0()) //tek:tek_reset
  DFDZ(0,"flipscreen"          ,IDM_FLIPSCREEN       ,FT_CMD , MF(flipscreen), HK2(6,AK,VK_F12,7,CS,'S')) //term:term_flip_screen
  DFDZ(5,"refresh"             ,refresh              ,FT_NORM, 0,              HK0())
  DFDZ(5,"lock-title"          ,lock_title           ,FT_NORM, MF(lock_title), HK0())
  DFDZ(5,"clear-title"         ,clear_title          ,FT_NORM, 0,              HK0())

  DFDZ(0,"break"               ,IDM_BREAK            ,FT_CMD , 0,              HK0()) //child:child_break
  DFDZ(0,"intr"                ,child_intr           ,FT_NORM, 0,              HK0()) //child:child_intr

  DFDZ(0,"tog-log"             ,IDM_TOGLOG           ,FT_CMD , MF(logging),    HK0()) //child:toggle_logging
  DFDZ(5,"toggle-logging"      ,IDM_TOGLOG           ,FT_CMD , MF(logging),    HK0()) //child:toggle_logging
  DFDZ(5,"toggle-char-info"    ,IDM_TOGCHARINFO      ,FT_CMD , MF(char_info),  HK0())
  DFDZ(5,"toggle-vt220"        ,toggle_vt220         ,FT_NORM, MF(vt220),      HK0())
  DFDZ(5,"toggle-auto-repeat"  ,toggle_auto_repeat   ,FT_NORM, MF(auto_repeat),HK0())
  DFDZ(5,"toggle-bidi"         ,toggle_bidi          ,FT_NORM, MF(bidi),       HK0())

//DFDZ(5,"super"               ,super_down           ,FT_KEY , 0,              HK0())
//DFDZ(5,"hyper"               ,hyper_down           ,FT_KEY , 0,              HK0())
//DFDZ(5,"compose"             ,compose_down         ,FT_KEY , 0,              HK0())
  DFDZ(0,"unicode-char"        ,unicode_char         ,FT_NORM, 0,              HK1(7,CS,'U'))

#undef SK 
#undef AK 
#undef CK 
#undef WK 
#undef AS 
#undef CS 
#undef WS 
#undef HK0
#undef HK1
#undef HK2
#undef HK3
#undef DFDO
#undef DFDZ
#undef MF
