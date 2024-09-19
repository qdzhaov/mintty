//===============================================
#ifdef CFGDEFT 
// define variable in struct config ,included in config.h,
#define DEFVAR 18971
// define init value in default_cfg ,included in config.c
#define DEFVAL 18972
// define cfg_option ,included in config.c
#define DEFCFG 18973
/*
_CFGDEFA:Define array
_CFGDEFB:Define Array Elements 
_CFGDEFC: opt comment,novalue
_CFGDEFD: opt data
_CFGDEFE: Legacy options for Backward Compatibility
 */
#if CFGDEFT == DEFVAR
#define _CFGDEFA(t,T,n,v    ) t v;
#define _CFGDEFB(t,T,n,v,c,d) 
#define _CFGDEFC(t,T,n     ,c) 
#define _CFGDEFD(t,T,n,v,c,d) t v;
#define _CFGDEFE(t,T,n,v,c  ) 
#elif CFGDEFT == DEFVAL
#define _CFGDEFA(t,T,n,v    ) 
#define _CFGDEFB(t,T,n,v,c,d) .v = d,
#define _CFGDEFC(t,T,n     ,c) 
#define _CFGDEFD(t,T,n,v,c,d) .v = d,
#define _CFGDEFE(t,T,n,v,c  ) 
#elif CFGDEFT == DEFCFG
#define _CFGDEFA(t,T,n,v    ) 
#define _CFGDEFB(t,T,n,v,c,d) {n , T, offcfg(v),c},
#define _CFGDEFC(t,T,n  ,c  ) {n , T, 0        ,c},
#define _CFGDEFD(t,T,n,v,c,d) {n , T, offcfg(v),c},
#define _CFGDEFE(t,T,n,v,c  ) {n , T, offcfg(v),c},
#else
#error "Error:include configdef.h must define CFGDEFT as DEFVAR,DEFVAL,DEFCFG"
#endif
#define CLRP(F,B) {F,B}
#define FNT(N,S,W,B) {N,S,W,B}
#define CTYPE int
#define CBOOL CTYPE
  _CFGDEFC(colour   ,OPT_COMMENT           ,__("Looks")                                             ,__("Looks Options"))
  _CFGDEFD(colour   ,OPT_CLR               ,"ForegroundColour"         ,fg_colour                   ,__("Foreground Colour")         ,0xBFBFBF)
  _CFGDEFD(colour   ,OPT_CLR               ,"BackgroundColour"         ,bg_colour                   ,__("Background Colour")         ,0)
  _CFGDEFD(colour   ,OPT_CLR               ,"CursorColour"             ,cursor_colour               ,__("CursorColour")             ,0xBFBFBF)

  _CFGDEFD(colour   ,OPT_CLR               ,"BoldColour"               ,bold_colour                 ,__("BoldColour")               ,0xFFFFFF)
  _CFGDEFD(colour   ,OPT_CLR               ,"BlinkColour"              ,blink_colour                ,__("BlinkColour")              ,0xFFFFFF)
  _CFGDEFD(colour   ,OPT_CLR               ,"TekForegroundColour"      ,tek_fg_colour               ,__("TekForegroundColour")      ,0xFFFFFF)
  _CFGDEFD(colour   ,OPT_CLR               ,"TekBackgroundColour"      ,tek_bg_colour               ,__("TekBackgroundColour")      ,0xFFFFFF)
  _CFGDEFD(colour   ,OPT_CLR               ,"TekCursorColour"          ,tek_cursor_colour           ,__("TekCursorColour")          ,0xFFFFFF)
  _CFGDEFD(colour   ,OPT_CLR               ,"TekWriteThruColour"       ,tek_write_thru_colour       ,__("TekWriteThruColour")       ,0xFFFFFF)
  _CFGDEFD(colour   ,OPT_CLR               ,"TekDefocusedColour"       ,tek_defocused_colour        ,__("TekDefocusedColour")       ,0xFFFFFF)
  _CFGDEFD(colour   ,OPT_INT               ,"TekGlow"                  ,tek_glow                    ,__("TekGlow")                  ,1)
  _CFGDEFD(colour   ,OPT_INT               ,"TekStrap"                 ,tek_strap                   ,__("TekStrap")                 ,0)
  _CFGDEFD(colour   ,OPT_CLR               ,"UnderlineColour"          ,underl_colour               ,__("UnderlineColour")          ,0)
  _CFGDEFD(colour   ,OPT_CLR               ,"HoverColour"              ,hover_colour                ,__("HoverColour")              ,0)
#if 1

  _CFGDEFA(colourfg ,0                     ,""                         ,ansi_colours[16]                                       )
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"Black"                    ,ansi_colours[BLACK_I]       ,__("Black")                    ,CLRP( RGB(  0,   0,   0), RGB(  0,   0,   0) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"Red"                      ,ansi_colours[RED_I]         ,__("Red")                      ,CLRP( RGB(212,  44,  58), RGB(162,  30,  41) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"Green"                    ,ansi_colours[GREEN_I]       ,__("Green")                    ,CLRP( RGB( 28, 168,   0), RGB( 28, 168,   0) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"Yellow"                   ,ansi_colours[YELLOW_I]      ,__("Yellow")                   ,CLRP( RGB(192, 160,   0), RGB(192, 160,   0) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"Blue"                     ,ansi_colours[BLUE_I]        ,__("Blue")                     ,CLRP( RGB(  0,  93, 255), RGB(  0,  32, 192) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"Magenta"                  ,ansi_colours[MAGENTA_I]     ,__("Magenta")                  ,CLRP( RGB(177,  72, 198), RGB(134,  54, 150) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"Cyan"                     ,ansi_colours[CYAN_I]        ,__("Cyan")                     ,CLRP( RGB(  0, 168, 154), RGB(  0, 168, 154) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"White"                    ,ansi_colours[WHITE_I]       ,__("White")                    ,CLRP( RGB(191, 191, 191), RGB(191, 191, 191) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"BoldBlack"                ,ansi_colours[BOLD_BLACK_I]  ,__("BoldBlack")                ,CLRP( RGB( 96,  96,  96), RGB( 72,  72,  72) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"BoldRed"                  ,ansi_colours[BOLD_RED_I]    ,__("BoldRed")                  ,CLRP( RGB(255, 118, 118), RGB(255, 118, 118) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"BoldGreen"                ,ansi_colours[BOLD_GREEN_I]  ,__("BoldGreen")                ,CLRP( RGB(  0, 242,   0), RGB(  0, 242,   0) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"BoldYellow"               ,ansi_colours[BOLD_YELLOW_I] ,__("BoldYellow")               ,CLRP( RGB(242, 242,   0), RGB(242, 242,   0) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"BoldBlue"                 ,ansi_colours[BOLD_BLUE_I]   ,__("BoldBlue")                 ,CLRP( RGB(125, 151, 255), RGB(125, 151, 255) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"BoldMagenta"              ,ansi_colours[BOLD_MAGENTA_I],__("BoldMagenta")              ,CLRP( RGB(255, 112, 255), RGB(255, 112, 255) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"BoldCyan"                 ,ansi_colours[BOLD_CYAN_I]   ,__("BoldCyan")                 ,CLRP( RGB(  0, 240, 240), RGB(  0, 240, 240) ))
  _CFGDEFB(0        ,OPT_THEME|OPT_CLRFG   ,"BoldWhite"                ,ansi_colours[BOLD_WHITE_I]  ,__("BoldWhite")                ,CLRP( RGB(255, 255, 255), RGB(255, 255, 255) ))
                                                                                                           
  _CFGDEFD(int      ,OPT_INT               ,"DispSpace"                ,disp_space                  ,__("DispSpace")                ,0)
  _CFGDEFD(int      ,OPT_INT               ,"DispClear"                ,disp_clear                  ,__("DispClear")                ,0)
  _CFGDEFD(int      ,OPT_INT               ,"DispTab"                  ,disp_tab                    ,__("DispTab")                  ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"UnderlineManual"          ,underl_manual               ,__("UnderlineManual")          ,0)
  _CFGDEFD(colour   ,OPT_CLR               ,"HighlightBackgroundColour",sel_bg_colour               ,__("HighlightBackgroundColour"),0xFFFFFF)
  _CFGDEFD(colour   ,OPT_CLR               ,"HighlightForegroundColour",sel_fg_colour               ,__("HighlightForegroundColour"),0xFFFFFF)
  _CFGDEFD(colour   ,OPT_CLR               ,"SearchForegroundColour"   ,search_fg_colour            ,__("SearchForegroundColour")   ,0x000000)
  _CFGDEFD(colour   ,OPT_CLR               ,"SearchBackgroundColour"   ,search_bg_colour            ,__("SearchBackgroundColour")   ,0x00DDDD)
  _CFGDEFD(colour   ,OPT_CLR               ,"SearchCurrentColour"      ,search_current_colour       ,__("SearchCurrentColour")      ,0x0099DD)
  _CFGDEFD(string   ,OPT_STR               ,"ThemeFile"                ,theme_file                  ,__("ThemeFile")                ,(""))
  _CFGDEFD(wstring  ,OPT_WSTR              ,"Background"               ,background                  ,__("Background")               ,W(""))
  _CFGDEFD(string   ,OPT_STR               ,"ColourScheme"             ,colour_scheme               ,__("ColourScheme")             ,"")
  _CFGDEFD(CTYPE    ,OPT_TRANS             ,"Transparency"             ,transparency                ,__("Transparency")             ,0)
#ifdef support_blurred                                                                            
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"Blur"                     ,blurred                     ,__("Blur")                     ,0)
#endif                                                                                               
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"OpaqueWhenFocused"        ,opaque_when_focused         ,__("OpaqueWhenFocused")        ,0)
  _CFGDEFD(CTYPE    ,OPT_CURSOR            ,"CursorType"               ,cursor_type                 ,__("CursorType")               ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"CursorBlinks"             ,cursor_blinks               ,__("CursorBlinks")             ,0)
                                                                                                      
  _CFGDEFD(colour   ,OPT_CLR               ,"TabForegroundColour"      ,tab_fg_colour               ,__("TabForegroundColour")      ,0x00FF00)
  _CFGDEFD(colour   ,OPT_CLR               ,"TabBackgroundColour"      ,tab_bg_colour               ,__("TabBackgroundColour")      ,0x000000)
  _CFGDEFD(colour   ,OPT_CLR               ,"TabAttentionColour"       ,tab_attention_bg_colour     ,__("TabAttentionColour")       ,0x323232)
  _CFGDEFD(colour   ,OPT_CLR               ,"TabActiveColour"          ,tab_active_bg_colour        ,__("TabActiveColour")          ,0x003200)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"tab_font_size"            ,tab_font_size               ,__("tab_font_size")            ,16)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"tab_bar_show"             ,tab_bar_show                ,__("tab_bar_show")             ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"indicator"                ,indicator                   ,__("indicator")                ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"indicatorx"               ,indicatorx                  ,__("indicatorx")               ,30)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"indicatory"               ,indicatory                  ,__("indicatory")               ,30)
  _CFGDEFD(int      ,OPT_INT               ,"gui_font_size"            ,gui_font_size               ,__("gui_font_size")            ,12)
  _CFGDEFD(int      ,OPT_INT               ,"scale_options_width"      ,scale_options_width         ,__("scale_options_width")      ,100)
  _CFGDEFD(int      ,OPT_INT               ,"Padding"                  ,padding                     ,__("Padding")                  ,1)
  _CFGDEFD(int      ,OPT_INT               ,"partline"                 ,partline                    ,__("partline")                 ,4)
  _CFGDEFD(int      ,OPT_INT               ,"usepartline"              ,usepartline                 ,__("usepartline")              ,0)
                                                                                                      
                                                                                                      
  _CFGDEFC(CTYPE    ,OPT_COMMENT           ,__("Text")                                              ,__("Text Options"))

  _CFGDEFD(font_spec,OPT_FONT              ,"Font"                     ,font                        ,__("Font")                     ,FNT(W("Lucida Console"),16,400,0))
  _CFGDEFA(font_spec,0                     ,                           ,fontfams[12]                                          )
  _CFGDEFB(0        ,OPT_FONT              ,"Font1"                    ,fontfams[1]                 ,__("Font1")                    ,FNT(W(""),16,400,0))
  _CFGDEFB(0        ,OPT_FONT              ,"Font2"                    ,fontfams[2]                 ,__("Font2")                    ,FNT(W(""),16,400,0))
  _CFGDEFB(0        ,OPT_FONT              ,"Font3"                    ,fontfams[3]                 ,__("Font3")                    ,FNT(W(""),16,400,0))
  _CFGDEFB(0        ,OPT_FONT              ,"Font4"                    ,fontfams[4]                 ,__("Font4")                    ,FNT(W(""),16,400,0))
  _CFGDEFB(0        ,OPT_FONT              ,"Font5"                    ,fontfams[5]                 ,__("Font5")                    ,FNT(W(""),16,400,0))
  _CFGDEFB(0        ,OPT_FONT              ,"Font6"                    ,fontfams[6]                 ,__("Font6")                    ,FNT(W(""),16,400,0))
  _CFGDEFB(0        ,OPT_FONT              ,"Font7"                    ,fontfams[7]                 ,__("Font7")                    ,FNT(W(""),16,400,0))
  _CFGDEFB(0        ,OPT_FONT              ,"Font8"                    ,fontfams[8]                 ,__("Font8")                    ,FNT(W(""),16,400,0))
  _CFGDEFB(0        ,OPT_FONT              ,"Font9"                    ,fontfams[9]                 ,__("Font9")                    ,FNT(W(""),16,400,0))
  _CFGDEFB(0        ,OPT_FONT              ,"Font10"                   ,fontfams[10]                ,__("Font10")                   ,FNT(W(""),16,400,0))
  _CFGDEFB(0        ,OPT_FONT              ,"FontRTL"                  ,fontfams[11]                ,__("FontRTL")                  ,FNT(W("Courier New"),16,400,0))
                                                                                                      
  _CFGDEFD(wstring  ,OPT_WSTR              ,"FontChoice"               ,font_choice                 ,__("FontChoice")               ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR              ,"FontSample"               ,font_sample                 ,__("FontSample")               ,W(""))
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ShowHiddenFonts"          ,show_hidden_fonts           ,__("ShowHiddenFonts")          ,0)
  _CFGDEFD(CTYPE    ,OPT_FONTSMT           ,"FontSmoothing"            ,font_smoothing              ,__("FontSmoothing")            ,FS_DEFAULT)
  _CFGDEFD(CTYPE    ,OPT_FONTRENDER        ,"FontRender"               ,font_render                 ,__("FontRender")               ,FR_UNISCRIBE)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"DimAsFont"                ,dim_as_font                 ,__("Dim as font")              ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"BoldAsFont"               ,bold_as_font                ,__("BoldAsFont")               ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"BoldAsColour"             ,bold_as_colour              ,__("BoldAsColour")             ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"AllowBlinking"            ,allow_blinking              ,__("AllowBlinking")            ,0)
  _CFGDEFD(string   ,OPT_STR               ,"Locale"                   ,locale                      ,__("Locale")                   ,"")
  _CFGDEFD(string   ,OPT_STR               ,"Charset"                  ,charset                     ,__("Charset")                  ,"")
  _CFGDEFD(CTYPE    ,OPT_CHARWIDTH         ,"Charwidth"                ,charwidth                   ,__("Charwidth")                ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"OldLocale"                ,old_locale                  ,__("OldLocale")                ,0)
  _CFGDEFD(int      ,OPT_INT               ,"FontMenu"                 ,fontmenu                    ,__("FontMenu")                 ,-1)
  _CFGDEFD(wstring  ,OPT_WSTR              ,"TekFont"                  ,tek_font                    ,__("TekFont")                  ,W(""))
                                                                                                      
                                                                                                      
  _CFGDEFC(CTYPE    ,OPT_COMMENT           ,"Keys"                                                  ,__("Keys Options"))
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"capmapesc"                ,capmapesc                   ,__("capmapesc")                ,1) 
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"BackspaceSendsBS"         ,backspace_sends_bs          ,__("BackspaceSendsBS")         ,'\b')
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"DeleteSendsDEL"           ,delete_sends_del            ,__("DeleteSendsDEL")           ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"CtrlAltIsAltGr"           ,ctrl_alt_is_altgr           ,__("CtrlAltIsAltGr")           ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"AltGrIsAlsoAlt"           ,altgr_is_alt                ,__("AltGrIsAlsoAlt")           ,0)
  _CFGDEFD(int      ,OPT_INT               ,"CtrlAltDelayAltGr"        ,ctrl_alt_delay_altgr        ,__("CtrlAltDelayAltGr")        ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"KeyAlphaMode"             ,key_alpha_mode              ,__("key_alpha_mode")           ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"OldAltGrDetection"        ,old_altgr_detection         ,__("OldAltGrDetection")        ,0)
  _CFGDEFD(int      ,OPT_INT               ,"OldModifyKeys"            ,old_modify_keys             ,__("OldModifyKeys")            ,0)
  _CFGDEFD(int      ,OPT_INT               ,"FormatOtherKeys"          ,format_other_keys           ,__("FormatOtherKeys")          ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"AutoRepeat"               ,auto_repeat                 ,__("AutoRepeat")               ,1)
  _CFGDEFD(int      ,OPT_INT               ,"SupportExternalHotkeys"   ,external_hotkeys            ,__("SupportExternalHotkeys")   ,2)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ClipShortcuts"            ,clip_shortcuts              ,__("ClipShortcuts")            ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"WindowShortcuts"          ,window_shortcuts            ,__("WindowShortcuts")          ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"SwitchShortcuts"          ,switch_shortcuts            ,__("SwitchShortcuts")          ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ZoomShortcuts"            ,zoom_shortcuts              ,__("ZoomShortcuts")            ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ZoomFontWithWindow"       ,zoom_font_with_window       ,__("ZoomFontWithWindow")       ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"AltFnShortcuts"           ,alt_fn_shortcuts            ,__("AltFnShortcuts")           ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"WinShortcuts"             ,win_shortcuts               ,__("WinShortcuts")             ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"hkwinkeyall"              ,hkwinkeyall                 ,__("hkwinkeyall")              ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"hook_keyboard"            ,hook_keyboard               ,__("hook_keyboard")            ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"CtrlShiftShortcuts"       ,ctrl_shift_shortcuts        ,__("CtrlShiftShortcuts")       ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"CtrlExchangeShift"        ,ctrl_exchange_shift         ,__("CtrlExchangeShift")        ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"CtrlControls"             ,ctrl_controls               ,__("CtrlControls")             ,0)
  _CFGDEFD(CTYPE    ,OPT_COMPOSE_KEY       ,"ComposeKey"               ,compose_key                 ,__("ComposeKey")               ,0)
  _CFGDEFD(string   ,OPT_STR               ,"Key_PrintScreen"          ,key_prtscreen               ,__("Key_PrintScreen")          ,"")    // VK_SNAPSHOT
  _CFGDEFD(string   ,OPT_STR               ,"Key_Pause"                ,key_pause                   ,__("Key_Pause")                ,"")  // VK_PAUSE
  _CFGDEFD(string   ,OPT_STR               ,"Key_Break"                ,key_break                   ,__("Key_Break")                ,"")  // VK_CANCEL
  _CFGDEFD(string   ,OPT_STR               ,"Key_Menu"                 ,key_menu                    ,__("Key_Menu")                 ,"")    // VK_APPS
  _CFGDEFD(string   ,OPT_STR               ,"Key_ScrollLock"           ,key_scrlock                 ,__("Key_ScrollLock")           ,"")    // VK_SCROLL
  _CFGDEFD(wstring  ,OPT_WSTR|OPT_KEEPCR   ,"KeyFunctions"             ,key_commands                ,__("KeyFunctions")             ,W(""))
  _CFGDEFD(int      ,OPT_INT               ,"ManageLEDs"               ,manage_leds                 ,__("ManageLEDs")               ,7)
//  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ShootFoot"                ,enable_remap_ctrls          ,__("ShootFoot")                ,0)
//  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"OldKeyFunctionsKeypad"    ,old_keyfuncs_keypad         ,__("OldKeyFunctionsKeypad")    ,0)
                                                                                                      
                                                                                                      
  _CFGDEFC(CTYPE    ,OPT_COMMENT           ,"Mouse"                                                 ,__("Mouse Options"))
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ClicksPlaceCursor"        ,clicks_place_cursor         ,__("ClicksPlaceCursor")        ,0)
  _CFGDEFD(CTYPE    ,OPT_MIDDLECLICK       ,"MiddleClickAction"        ,middle_click_action         ,__("MiddleClickAction")        ,MC_PASTE)
  _CFGDEFD(CTYPE    ,OPT_RIGHTCLICK        ,"RightClickAction"         ,right_click_action          ,__("RightClickAction")         ,RC_MENU)
  _CFGDEFD(int      ,OPT_INT               ,"OpeningClicks"            ,opening_clicks              ,__("OpeningClicks")            ,1)
  _CFGDEFD(CTYPE    ,OPT_MOD               ,"OpeningMod"               ,opening_mod                 ,__("OpeningMod")               ,MDK_CTRL)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ZoomMouse"                ,zoom_mouse                  ,__("ZoomMouse")                ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ClicksTargetApp"          ,clicks_target_app           ,__("ClicksTargetApp")          ,1)
  _CFGDEFD(CTYPE    ,OPT_MOD               ,"ClickTargetMod"           ,click_target_mod            ,__("ClickTargetMod")           ,MDK_SHIFT)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"HideMouse"                ,hide_mouse                  ,__("HideMouse")                ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ElasticMouse"             ,elastic_mouse               ,__("ElasticMouse")             ,0)
  _CFGDEFD(int      ,OPT_INT               ,"LinesPerMouseWheelNotch"  ,lines_per_notch             ,__("LinesPerMouseWheelNotch")  ,0)
  _CFGDEFD(wstring  ,OPT_WSTR              ,"MousePointer"             ,mouse_pointer               ,__("MousePointer")             ,W("ibeam"))
  _CFGDEFD(wstring  ,OPT_WSTR              ,"AppMousePointer"          ,appmouse_pointer            ,__("AppMousePointer")          ,W("arrow"))

  _CFGDEFC(CTYPE    ,OPT_COMMENT           ,"Selection"                                             ,__("Selection Options")) // Selection
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ClearSelectionOnInput"    ,input_clears_selection      ,__("ClearSelectionOnInput")    ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"CopyOnSelect"             ,copy_on_select              ,__("CopyOnSelect")             ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"CopyTab"                  ,copy_tabs                   ,__("CopyTab")                  ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"CopyAsRTF"                ,copy_as_rtf                 ,__("CopyAsRTF")                ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"CopyAsHTML"               ,copy_as_html                ,__("CopyAsHTML")               ,0)
  _CFGDEFD(wstring  ,OPT_WSTR              ,"CopyAsRTFFont"            ,copy_as_rtf_font            ,__("CopyAsRTFFont")            ,W(""))
  _CFGDEFD(int      ,OPT_INT               ,"CopyAsRTFFontHeight"      ,copy_as_rtf_font_size       ,__("CopyAsRTFFontHeight")      ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"TrimSelection"            ,trim_selection              ,__("TrimSelection")            ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"AllowSetSelection"        ,allow_set_selection         ,__("AllowSetSelection")        ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"AllowPasteSelection"      ,allow_paste_selection       ,__("allow_paste_selection")    ,0)
  _CFGDEFD(int      ,OPT_INT               ,"SelectionShowSize"        ,selection_show_size         ,__("SelectionShowSize")        ,0)
  _CFGDEFC(CTYPE    ,OPT_COMMENT           ,"Window"                                                ,__("Window Options"))  // Window
  _CFGDEFD(int      ,OPT_INT               ,"Columns"                  ,cols                        ,__("Columns")                  ,80)
  _CFGDEFD(int      ,OPT_INT               ,"Rows"                     ,rows                        ,__("Rows")                     ,24)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"RewrapOnResize"           ,rewrap_on_resize            ,__("RewrapOnResize")           ,0)
  _CFGDEFD(CTYPE    ,OPT_SCROLLBAR         ,"Scrollbar"                ,scrollbar                   ,__("Scrollbar")                ,1)
  _CFGDEFD(int      ,OPT_INT               ,"ScrollbackLines"          ,scrollback_lines            ,__("ScrollbackLines")          ,10000)
  _CFGDEFD(int      ,OPT_INT               ,"MaxScrollbackLines"       ,max_scrollback_lines        ,__("MaxScrollbackLines")       ,250000)
  _CFGDEFD(CTYPE    ,OPT_MOD               ,"ScrollMod"                ,scroll_mod                  ,__("ScrollMod")                ,MDK_SHIFT)
  _CFGDEFD(CTYPE    ,OPT_BORDER            ,"BorderStyle"              ,border_style                ,__("BorderStyle")              ,BORDER_NORMAL)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"PgUpDnScroll"             ,pgupdn_scroll               ,__("PgUpDnScroll")             ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"allocconsole"             ,allocconsole                ,__("allocconsole")             ,0)
  _CFGDEFD(string   ,OPT_STR               ,"Language"                 ,lang                        ,__("Language")                 ,(""))
  _CFGDEFD(wstring  ,OPT_WSTR              ,"SearchBar"                ,search_bar                  ,__("SearchBar")                ,W(""))
  _CFGDEFD(int      ,OPT_INT               ,"SearchContext"            ,search_context              ,__("SearchContext")            ,0)
  _CFGDEFC(CTYPE    ,OPT_COMMENT           ,"Terminal"                                              ,__("Terminal Options"))  // 
  _CFGDEFD(string   ,OPT_STR               ,"Term"                     ,Term                        ,__("Term")                     ,"xterm")
  _CFGDEFD(string   ,OPT_STR               ,"Answerback"               ,answerback                  ,__("Answerback")               ,"")
  _CFGDEFD(int      ,OPT_INT               ,"WrapTab"                  ,wrap_tab                    ,__("WrapTab")                  ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"OldWrapModes"             ,old_wrapmodes               ,__("OldWrapModes")             ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"Enable132ColumnSwitching" ,enable_deccolm_init         ,__("Enable132ColumnSwitching") ,0)
  _CFGDEFD(int      ,OPT_INT               ,"BellType"                 ,bell_type                   ,__("BellType")                 ,1)
  _CFGDEFA(string   ,0                     ,0                          ,bell_file[7]                                          )
  _CFGDEFB(string   ,OPT_STR               ,"BellFile"                 ,bell_file[6]                ,__("BellFile")                 ,"")
  _CFGDEFB(string   ,OPT_STR               ,"BellFile2"                ,bell_file[0]                ,__("BellFile2")                ,"")
  _CFGDEFB(string   ,OPT_STR               ,"BellFile3"                ,bell_file[1]                ,__("BellFile3")                ,"")
  _CFGDEFB(string   ,OPT_STR               ,"BellFile4"                ,bell_file[2]                ,__("BellFile4")                ,"")
  _CFGDEFB(string   ,OPT_STR               ,"BellFile5"                ,bell_file[3]                ,__("BellFile5")                ,"")
  _CFGDEFB(string   ,OPT_STR               ,"BellFile6"                ,bell_file[4]                ,__("BellFile6")                ,"")
  _CFGDEFB(string   ,OPT_STR               ,"BellFile7"                ,bell_file[5]                ,__("BellFile7")                ,"")
  _CFGDEFD(int      ,OPT_INT               ,"BellFreq"                 ,bell_freq                   ,__("BellFreq")                 ,0)
  _CFGDEFD(int      ,OPT_INT               ,"BellLen"                  ,bell_len                    ,__("BellLen")                  ,400)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"BellFlash"                ,bell_flash                  ,__("BellFlash")                ,0)// xterm: visualBell
  _CFGDEFD(int      ,OPT_INT               ,"BellFlashStyle"           ,bell_flash_style            ,__("BellFlashStyle")           ,FLASH_FULL)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"BellTaskbar"              ,bell_taskbar                ,__("BellTaskbar")              ,1)// xterm: bellIsUrgent
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"BellPopup"                ,bell_popup                  ,__("BellPopup")                ,0)// xterm: popOnBell
  _CFGDEFD(int      ,OPT_INT               ,"BellInterval"             ,bell_interval               ,__("BellInterval")             ,100)
  _CFGDEFD(int      ,OPT_INT               ,"PlayTone"                 ,play_tone                   ,__("PlayTone")                 ,2)
  _CFGDEFD(wstring  ,OPT_WSTR              ,"Printer"                  ,printer                     ,__("Printer")                  ,W(""))
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ConfirmExit"              ,confirm_exit                ,__("ConfirmExit")              ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ConfirmReset"             ,confirm_reset               ,__("ConfirmReset")             ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ConfirmMultiLinePasting"  ,confirm_multi_line_pasting  ,__("ConfirmMultiLinePasting")  ,0)
  _CFGDEFC(CTYPE    ,OPT_COMMENT           ,"Command line"                                          ,__("Command line Options"))  // 
  _CFGDEFD(wstring  ,OPT_WSTR              ,"Class"                    ,class                       ,__("Class")                    ,W(""))
  _CFGDEFD(CTYPE    ,OPT_HOLD              ,"Hold"                     ,hold                        ,__("Hold")                     ,HOLD_START)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ExitWrite"                ,exit_write                  ,__("ExitWrite")                ,0)
  _CFGDEFD(wstring  ,OPT_WSTR              ,"ExitTitle"                ,exit_title                  ,__("ExitTitle")                ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR              ,"Icon"                     ,icon                        ,__("Icon")                     ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR              ,"Log"                      ,log                         ,__("Log")                      ,W(""))
  _CFGDEFD(int      ,OPT_INT               ,"Logging"                  ,logging                     ,__("Logging")                  ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"Utmp"                     ,create_utmp                 ,__("Utmp")                     ,0)
  _CFGDEFD(wstring  ,OPT_WSTR              ,"Title"                    ,title                       ,__("Title")                    ,W(""))
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"title_settable"           ,title_settable              ,__("title_settable")           ,1)
  _CFGDEFD(CTYPE    ,OPT_WINDOW            ,"Window"                   ,window                      ,__("Window")                   ,0)
  _CFGDEFD(int      ,OPT_INT               ,"X"                        ,x                           ,__("X")                        ,0)
  _CFGDEFD(int      ,OPT_INT               ,"Y"                        ,y                           ,__("Y")                        ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"Daemonize"                ,daemonize                   ,__("Daemonize")                ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"DaemonizeAlways"          ,daemonize_always            ,__("DaemonizeAlways")          ,0)
  _CFGDEFC(CTYPE    ,OPT_COMMENT           ,"Hidden"                                                ,__("Hidden Options"))  
  _CFGDEFD(int      ,OPT_INT               ,"Bidi"                     ,bidi                        ,__("Bidi")                     ,2)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"NoAltScreen"              ,disable_alternate_screen    ,__("NoAltScreen")              ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"EraseToScrollback"        ,erase_to_scrollback         ,__("EraseToScrollback")        ,1)
  _CFGDEFD(int      ,OPT_INT               ,"DisplaySpeedup"           ,display_speedup             ,__("DisplaySpeedup")           ,6)
  _CFGDEFD(string   ,OPT_STR               ,"SuppressSGR"              ,suppress_sgr                ,__("SuppressSGR")              ,"")
  _CFGDEFD(string   ,OPT_STR               ,"SuppressDEC"              ,suppress_dec                ,__("SuppressDEC")              ,"")
  _CFGDEFD(string   ,OPT_STR               ,"SuppressWIN"              ,suppress_win                ,__("SuppressWIN")              ,"")
  _CFGDEFD(string   ,OPT_STR               ,"SuppressOSC"              ,suppress_osc                ,__("SuppressOSC")              ,"")
  _CFGDEFD(string   ,OPT_STR               ,"SuppressNRC"              ,suppress_nrc                ,__("SuppressNRC")              ,"")
  _CFGDEFD(string   ,OPT_STR               ,"SuppressMouseWheel"       ,suppress_wheel              ,__("SuppressMouseWheel")       ,"")
  _CFGDEFD(string   ,OPT_STR               ,"FilterPasteControls"      ,filter_paste                ,__("FilterPasteControls")      ,"STTY")
  _CFGDEFD(int      ,OPT_INT               ,"GuardNetworkPaths"        ,guard_path                  ,__("GuardNetworkPaths")               ,7)
  _CFGDEFD(int      ,OPT_INT               ,"BracketedPasteByLine"     ,bracketed_paste_split       ,__("BracketedPasteByLine")     ,0)
  _CFGDEFD(int      ,OPT_INT               ,"SuspendWhileSelecting"    ,suspbuf_max                 ,__("SuspendWhileSelecting")    ,8080)
  _CFGDEFD(int      ,OPT_INT               ,"PrintableControls"        ,printable_controls          ,__("PrintableControls")        ,0)
  _CFGDEFD(int      ,OPT_INT               ,"CharNarrowing"            ,char_narrowing              ,__("CharNarrowing")            ,75)
  _CFGDEFD(CTYPE    ,OPT_EMOJIS            ,"Emojis"                   ,emojis                      ,__("Emojis")                   ,0)
  _CFGDEFD(CTYPE    ,OPT_EMOJI_PLACEMENT   ,"EmojiPlacement"           ,emoji_placement             ,__("EmojiPlacement")           ,EMPL_STRETCH)
  _CFGDEFD(wstring  ,OPT_WSTR              ,"SaveFilename"             ,save_filename               ,__("SaveFilename")             ,W("mintty.%Y-%m-%d_%H-%M-%S"))
  _CFGDEFD(wstring  ,OPT_WSTR              ,"AppID"                    ,app_id                      ,__("AppID")                    ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR              ,"AppName"                  ,app_name                    ,__("AppName")                  ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR              ,"AppLaunchCmd"             ,app_launch_cmd              ,__("AppLaunchCmd")             ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR|OPT_KEEPCR   ,"DropCommands"             ,drop_commands               ,__("DropCommands")             ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR|OPT_KEEPCR   ,"ExitCommands"             ,exit_commands               ,__("ExitCommands")             ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR|OPT_KEEPCR   ,"UserCommands"             ,user_commands               ,__("UserCommands")             ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR|OPT_KEEPCR   ,"CtxMenuFunctions"         ,ctx_user_commands           ,__("CtxMenuFunctions")         ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR|OPT_KEEPCR   ,"SysMenuFunctions"         ,sys_user_commands           ,__("SysMenuFunctions")         ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR              ,"UserCommandsPath"         ,user_commands_path          ,__("UserCommandsPath")         ,W("/bin:%s"))
  _CFGDEFD(wstring  ,OPT_WSTR|OPT_KEEPCR   ,"SessionCommands"          ,session_commands            ,__("SessionCommands")          ,W(""))
  _CFGDEFD(wstring  ,OPT_WSTR|OPT_KEEPCR   ,"TaskCommands"             ,task_commands               ,__("TaskCommands")             ,W(""))
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ConPTY"                   ,conpty_support              ,__("ConPTY")                   ,-1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"LoginFromShortcut"        ,login_from_shortcut         ,__("LoginFromShortcut")        ,1)
  _CFGDEFD(string   ,OPT_STR               ,"MenuMouse"                ,menu_mouse                  ,__("MenuMouse")                ,"b")
  _CFGDEFD(string   ,OPT_STR               ,"MenuCtrlMouse"            ,menu_ctrlmouse              ,__("MenuCtrlMouse")            ,"e|ls")
  _CFGDEFD(string   ,OPT_STR               ,"MenuMouse5"               ,menu_altmouse               ,__("MenuMouse5")               ,"ls"  )
//_CFGDEFD(string   ,OPT_STR               ,"MenuMenu"                 ,menu_menu                   ,__("MenuMenu")                 ,"bs"  )
//_CFGDEFD(string   ,OPT_STR               ,"MenuCtrlMenu"             ,menu_ctrlmenu               ,__("MenuCtrlMenu")             ,"e|ls")
  _CFGDEFD(string   ,OPT_STR               ,"MenuTitleCtrlLeft"        ,menu_title_ctrl_l           ,__("MenuTitleCtrlLeft")        ,"Ws")
  _CFGDEFD(string   ,OPT_STR               ,"MenuTitleCtrlRight"       ,menu_title_ctrl_r           ,__("MenuTitleCtrlRight")       ,"Ws")
  _CFGDEFD(int      ,OPT_INT               ,"SessionGeomSync"          ,geom_sync                   ,__("SessionGeomSync")          ,0)
  _CFGDEFD(int      ,OPT_INT               ,"ColSpacing"               ,col_spacing                 ,__("ColSpacing")               ,0)
  _CFGDEFD(int      ,OPT_INT               ,"RowSpacing"               ,row_spacing                 ,__("RowSpacing")               ,0)
  _CFGDEFD(int      ,OPT_INT               ,"AutoLeading"              ,auto_leading                ,__("AutoLeading")              ,2)
  _CFGDEFD(int      ,OPT_INT               ,"Ligatures"                ,ligatures                   ,__("Ligatures")                ,1)
  _CFGDEFD(int      ,OPT_INT               ,"LigaturesSupport"         ,ligatures_support           ,__("LigaturesSupport")         ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"BoxDrawing"               ,box_drawing                 ,__("BoxDrawing")               ,1)
  _CFGDEFD(int      ,OPT_INT               ,"HandleDPI"                ,handle_dpichanged           ,__("HandleDPI")                ,2)
  _CFGDEFD(int      ,OPT_INT               ,"CheckVersionUpdate"       ,check_version_update        ,__("CheckVersionUpdate")       ,0)
  _CFGDEFD(string   ,OPT_STR               ,"WordChars"                ,word_chars                  ,__("WordChars")                ,"")
  _CFGDEFD(string   ,OPT_STR               ,"WordCharsExcl"            ,word_chars_excl             ,__("WordCharsExcl")            ,"")
  _CFGDEFD(colour   ,OPT_CLR               ,"IMECursorColour"          ,ime_cursor_colour           ,__("IMECursorColour")          ,DEFAULT_COLOUR)
  _CFGDEFD(wstring  ,OPT_WSTR              ,"SixelClipChars"           ,sixel_clip_char             ,__("SixelClipChars")           ,W(""))
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"OldBold"                  ,old_bold                    ,__("OldBold")                  ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ShortLongOpts"            ,short_long_opts             ,__("ShortLongOpts")            ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"BoldAsRainbowSparkles"    ,bold_as_special             ,__("BoldAsRainbowSparkles")    ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"HoverTitle"               ,hover_title                 ,__("HoverTitle")               ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"ProgressBar"              ,progress_bar                ,__("ProgressBar")              ,0)
  _CFGDEFD(int      ,OPT_INT               ,"ProgressScan"             ,progress_scan               ,__("ProgressScan")             ,1)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"DimMargins"               ,dim_margins                 ,__("DimMargins")               ,0)
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"StatusLine"               ,status_line                 ,__("StatusLine")               ,0)
//_CFGDEFD(int      ,OPT_INT               ,"StatusDebug"              ,status_debug                ,__("status_debug")             ,0)
  _CFGDEFD(int      ,OPT_INT               ,"Baud"                     ,baud                        ,__("Baud")                     ,0)
  _CFGDEFD(int      ,OPT_INT               ,"Bloom"                    ,bloom                       ,__("Bloom")                    ,0)
  _CFGDEFD(wstring  ,OPT_WSTR              ,"OptionsFont"              ,options_font                ,__("OptionsFont")              ,W(""))
  _CFGDEFD(int      ,OPT_INT               ,"OptionsFontHeight"        ,options_fontsize            ,__("OptionsFontHeight")        ,0)
  _CFGDEFD(string   ,OPT_STR               ,"OldOptions"               ,old_options                 ,__("OldOptions")               ,"")
  _CFGDEFD(CBOOL    ,OPT_BOOL              ,"OldXButtons"              ,old_xbuttons                ,__("OldXButtons")              ,0)
  _CFGDEFD(int      ,OPT_INT               ,"WSLBridge"                ,wslbridge                   ,__("wslbridge version,0:nobridge,1:wslbridge,2:wslbreadge2") ,0)
  _CFGDEFD(string   ,OPT_STR               ,"WSLName"                  ,wslname                     ,__("wslDistribution,null is default") ,"")
                                                                                                      
  // Legacy

  _CFGDEFE(int      ,OPT_INT|OPT_LEGACY    ,"FontSize"                 ,font.size                   ,__("FontSize")                 )
  _CFGDEFE(string   ,OPT_STR|OPT_LEGACY    ,"Break"                    ,key_break                   ,__("Break")                    )
  _CFGDEFE(string   ,OPT_STR|OPT_LEGACY    ,"Pause"                    ,key_pause                   ,__("Pause")                    )
  _CFGDEFE(int      ,OPT_INT|OPT_LEGACY    ,"CopyAsRTFFontSize"        ,copy_as_rtf_font_size       ,__("CopyAsRTFFontSize")        )
  _CFGDEFE(int      ,OPT_INT|OPT_LEGACY    ,"OldFontMenu"              ,fontmenu                    ,__("OldFontMenu")              )
  _CFGDEFE(int      ,OPT_INT|OPT_LEGACY    ,"OptionsFontSize"          ,options_fontsize            ,__("OptionsFontSize")          )
  _CFGDEFE(CBOOL    ,OPT_BOOL|OPT_LEGACY   ,"BoldAsBright"             ,bold_as_colour              ,__("BoldAsBright")             )
  _CFGDEFE(CTYPE    ,OPT_FONTSMT|OPT_LEGACY,"FontQuality"              ,font_smoothing              ,__("FontQuality")              )

  _CFGDEFD(colour   ,OPT_BOOL|OPT_LEGACY   ,"UseSystemColours"         ,use_system_colours          ,__("UseSystemColours")         ,0)
#endif
#undef CFGDEFT  
#undef _CFGDEFA
#undef _CFGDEFB
#undef _CFGDEFC
#undef _CFGDEFD
#undef _CFGDEFE
#undef DEFVAR 
#undef DEFVAL 
#undef DEFCFG 
#endif
