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
#define _CFGDEFA(t,v,d,i,T,F,l,c,s,n,b) t v;
#define _CFGDEFB(t,v,d,i,T,F,l,c,s,n,b) 
#define _CFGDEFC(t,v,d,i,T,F,l,c,s,n,b) 
#define _CFGDEFD(t,v,d,i,T,F,l,c,s,n,b) t v;
#define _CFGDEFE(t,v,d,i,T,F,l,c,s,n,b) 
#elif CFGDEFT == DEFVAL    
#define INTP(x,y) {x,y}
#define CLRP(F,B) {F,B}
#define CLR_BLACK        {RGB(  0,  0,  0),RGB(  0,  0,  0)}
#define CLR_RED          {RGB(212, 44, 58),RGB(162, 30, 41)}
#define CLR_GREEN        {RGB( 28,168,  0),RGB( 28,168,  0)}
#define CLR_YELLOW       {RGB(192,160,  0),RGB(192,160,  0)}
#define CLR_BLUE         {RGB(  0, 93,255),RGB(  0, 32,192)}
#define CLR_MAGENTA      {RGB(177, 72,198),RGB(134, 54,150)}
#define CLR_CYAN         {RGB(  0,168,154),RGB(  0,168,154)}
#define CLR_WHITE        {RGB(191,191,191),RGB(191,191,191)}
#define CLR_BOLD_BLACK   {RGB( 96, 96, 96),RGB( 72, 72, 72)}
#define CLR_BOLD_RED     {RGB(255,118,118),RGB(255,118,118)}
#define CLR_BOLD_GREEN   {RGB(  0,242,  0),RGB(  0,242,  0)}
#define CLR_BOLD_YELLOW  {RGB(242,242,  0),RGB(242,242,  0)}
#define CLR_BOLD_BLUE    {RGB(125,151,255),RGB(125,151,255)}
#define CLR_BOLD_MAGENTA {RGB(255,112,255),RGB(255,112,255)}
#define CLR_BOLD_CYAN    {RGB(  0,240,240),RGB(  0,240,240)}
#define CLR_BOLD_WHITE   {RGB(255,255,255),RGB(255,255,255)}
#define CLR_FORE RGB(192,192,192)
#define CLR_BACK RGB(0,0,0)
#define CLR_FG   {CLR_FORE,CLR_BACK}
#define FNT(n,s,w,b) {W(n),s,w,b}
#define FNTD(n) {W(n),16,400,0}
#define FNT_DEF  FNTD("Lucida Console")
#define FNT_RTL  FNTD("Courier New")       
#define FNT_NON  FNTD("")
#define FNT_NUL  {W(""),16,400,0}
#define SAVE_FN W("mintty.%Y%m%d_%H%M%S")
#define _CFGDEFA(t,v,d,i,T,F,l,c,s,n,b) 
#define _CFGDEFB(t,v,d,i,T,F,l,c,s,n,b) .v = d,
#define _CFGDEFC(t,v,d,i,T,F,l,c,s,n,b) 
#define _CFGDEFD(t,v,d,i,T,F,l,c,s,n,b) .v = d,
#define _CFGDEFE(t,v,d,i,T,F,l,c,s,n,b) 
#elif CFGDEFT == DEFCFG   
#define _CFGDEFA(t,v,d,i,T,F,l,c,s,n,b) 
#define _CFGDEFB(t,v,d,i,T,F,l,c,s,n,b) {n,offcfg(v),T,F,i,l,c,s,b,null},
#define _CFGDEFC(t,v,d,i,T,F,l,c,s,n,b) {n,0        ,T,F,i,l,c,s,b,null},
#define _CFGDEFD(t,v,d,i,T,F,l,c,s,n,b) {n,offcfg(v),T,F,i,l,c,s,b,null},
#define _CFGDEFE(t,v,d,i,T,F,l,c,s,n,b) {n,offcfg(v),T,F,i,l,c,s,b,null},
#else                                                     
#error "Error:include configdef.h must define CFGDEFT as DEFVAR,DEFVAL,DEFCFG"
#endif
#define VAC(i) ansi_colours[i]
#define IV(i0,i1) .i={i0,i1}
#define IS(i0) .s=i0
#define BGF(f,t,a) {W(f),t,a,0}

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_PANE ,9,0,0,__("Looks"),__("Looks in Terminal"))
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Looks"    ,__("Colours"))
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,3,0,""  ,0)
  _CFGDEFD(colourfg ,colour                 ,CLR_FG          ,       IS(0),OPT_ARR          ,OPF_H    ,3,0,0,"Colour"                   ,__("Colour")                   )
  _CFGDEFE(0        ,colour.fg              ,CLR_FORE        ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,5,"ForegroundColour"         ,__("&Foreground...")           )
  _CFGDEFE(0        ,colour.bg              ,0               ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,5,"BackgroundColour"         ,__("&Background...")           )
  _CFGDEFD(colour   ,cursor_colour          ,0xBFBFBF        ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,5,"CursorColour"             ,__("CursorColour")             )
  _CFGDEFD(string   ,theme_file             ,("")            ,       IS(0),OPT_STR          ,OPF_THEME,9,0,7,"ThemeFile"                ,__("ThemeFile")                )
  _CFGDEFA(colourfg ,ansi_colours[16]       ,0               ,       IS(0),OPT_ARR          ,OPF_H    ,3,0,0,"ansi_colours"             ,__("ansi_colours")             )
  _CFGDEFE(0        ,ansi_colours           ,0               ,       IS(0),OPT_ARR          ,OPF_ACLR ,9,0,7,"ansi_colours"             ,__("ansi_colours")             )
  _CFGDEFB(0        ,VAC(BLACK_I)           ,CLR_BLACK       ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"Black"                    ,__("Black")                    )
  _CFGDEFB(0        ,VAC(RED_I)             ,CLR_RED         ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"Red"                      ,__("Red")                      )
  _CFGDEFB(0        ,VAC(GREEN_I)           ,CLR_GREEN       ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"Green"                    ,__("Green")                    )
  _CFGDEFB(0        ,VAC(YELLOW_I)          ,CLR_YELLOW      ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"Yellow"                   ,__("Yellow")                   )
  _CFGDEFB(0        ,VAC(BLUE_I)            ,CLR_BLUE        ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"Blue"                     ,__("Blue")                     )
  _CFGDEFB(0        ,VAC(MAGENTA_I)         ,CLR_MAGENTA     ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"Magenta"                  ,__("Magenta")                  )
  _CFGDEFB(0        ,VAC(CYAN_I)            ,CLR_CYAN        ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"Cyan"                     ,__("Cyan")                     )
  _CFGDEFB(0        ,VAC(WHITE_I)           ,CLR_WHITE       ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"White"                    ,__("White")                    )
  _CFGDEFB(0        ,VAC(BOLD_BLACK_I)      ,CLR_BOLD_BLACK  ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"BoldBlack"                ,__("BoldBlack")                )
  _CFGDEFB(0        ,VAC(BOLD_RED_I)        ,CLR_BOLD_RED    ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"BoldRed"                  ,__("BoldRed")                  )
  _CFGDEFB(0        ,VAC(BOLD_GREEN_I)      ,CLR_BOLD_GREEN  ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"BoldGreen"                ,__("BoldGreen")                )
  _CFGDEFB(0        ,VAC(BOLD_YELLOW_I)     ,CLR_BOLD_YELLOW ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"BoldYellow"               ,__("BoldYellow")               )
  _CFGDEFB(0        ,VAC(BOLD_BLUE_I)       ,CLR_BOLD_BLUE   ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"BoldBlue"                 ,__("BoldBlue")                 )
  _CFGDEFB(0        ,VAC(BOLD_MAGENTA_I)    ,CLR_BOLD_MAGENTA,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"BoldMagenta"              ,__("BoldMagenta")              )
  _CFGDEFB(0        ,VAC(BOLD_CYAN_I)       ,CLR_BOLD_CYAN   ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"BoldCyan"                 ,__("BoldCyan")                 )
  _CFGDEFB(0        ,VAC(BOLD_WHITE_I)      ,CLR_BOLD_WHITE  ,       IS(0),OPT_CLRFG|OPT_THM,OPF_H    ,3,0,0,"BoldWhite"                ,__("BoldWhite")                )
  _CFGDEFD(string   ,colour_scheme          ,""              ,       IS(0),OPT_STR          ,OPF_H    ,0,0,7,"ColourScheme"             ,__("ColourScheme")             )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Looks"    ,__("Transparency"))
  _CFGDEFD(CTYPE    ,transparency           ,0               ,       IS(0),OPT_TRANS        ,OPF_TRANS,9,0,7,"Transparency"             ,  ("")                         )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(CBOOL    ,opaque_when_focused    ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,7,0,"OpaqueWhenFocused"        ,__("Opa&que when focused")     )
  _CFGDEFD(CBOOL    ,blurred                ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,3,0,"Blur"                     ,__("Blu&r")                    )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Looks"    ,__("Cursor"))
  _CFGDEFD(CTYPE    ,cursor_type            ,0               ,       IS(0),OPT_CURSOR       ,OPF_LSTR ,9,0,7,"CursorType"               ,__("CursorType")               )
  _CFGDEFD(CBOOL    ,cursor_blinks          ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"CursorBlinks"             ,__("CursorBlinks")             )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,3,0,0,"Looks"    ,__("Disp"))
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,3,2,0,""  ,0)
  _CFGDEFD(CBOOL    ,underl_manual          ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"UnderlineManual"          ,__("UnderlineManual")          )
  _CFGDEFD(CBOOL    ,usepartline            ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"usepartline"              ,__("usepartline")              )
  _CFGDEFD(int      ,partline               ,4               ,IV( 0,    6),OPT_INT          ,OPF_INT  ,3,5,4,"partline"                 ,__("partline")                 )
  _CFGDEFD(int      ,padding                ,1               ,IV( 0,    8),OPT_INT          ,OPF_INT  ,3,5,4,"Padding"                  ,__("Padding")                  )
  _CFGDEFD(int      ,disp_clear             ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,5,4,"DispClear"                ,__("DispClear")                )
  _CFGDEFD(int      ,disp_tab               ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,5,4,"DispTab"                  ,__("DispTab")                  )
  _CFGDEFD(int      ,disp_space             ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,5,4,"DispSpace"                ,__("DispSpace")                )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_PANE ,9,0,0,__("Looks1")    ,__("Looks in Terminal1"))
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Looks1"    ,__("background"))
  _CFGDEFD(bg_file  ,backgfile              ,BGF("",0,255)   ,       IS(0),OPT_BFILE        ,OPF_BACKF,9,0,7,"Background"               ,__("Background")               )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,4,0,""  ,0)
  _CFGDEFD(colour   ,bold_colour            ,0xFFFFFF        ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,0,"BoldColour"               ,__("BoldColour")               )
  _CFGDEFD(colour   ,blink_colour           ,0xFFFFFF        ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,0,"BlinkColour"              ,__("BlinkColour")              )
  _CFGDEFD(colour   ,underl_colour          ,0               ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,0,"UnderlineColour"          ,__("UnderlineColour")          )
  _CFGDEFD(colour   ,hover_colour           ,0               ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,0,"HoverColour"              ,__("HoverColour")              )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Looks1"    ,__("Colours"))
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,4,0,""  ,0)
  _CFGDEFD(colour   ,sel_bg_colour          ,0xFFFFFF        ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,0,"HighlightBackgroundColour",__("HighlightBackgroundColour"))
  _CFGDEFD(colour   ,sel_fg_colour          ,0xFFFFFF        ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,0,"HighlightForegroundColour",__("HighlightForegroundColour"))
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,4,0,""  ,0)
  _CFGDEFD(colour   ,search_fg_colour       ,0x000000        ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,0,"SearchForegroundColour"   ,__("SearchForegroundColour")   )
  _CFGDEFD(colour   ,search_bg_colour       ,0x00DDDD        ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,0,"SearchBackgroundColour"   ,__("SearchBackgroundColour")   )
  _CFGDEFD(colour   ,search_current_colour  ,0x0099DD        ,       IS(0),OPT_CLR          ,OPF_CLR  ,9,5,0,"SearchCurrentColour"      ,__("SearchCurrentColour")      )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,3,0,0,"Looks1"    ,__("Tek Colour"))
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,3,3,0,""  ,0)
  _CFGDEFD(colour   ,tek_fg_colour          ,0xFFFFFF        ,       IS(0),OPT_CLR          ,OPF_CLR  ,3,5,0,"TekForegroundColour"      ,__("TekForegroundColour")      )
  _CFGDEFD(colour   ,tek_bg_colour          ,0xFFFFFF        ,       IS(0),OPT_CLR          ,OPF_CLR  ,3,5,0,"TekBackgroundColour"      ,__("TekBackgroundColour")      )
  _CFGDEFD(colour   ,tek_cursor_colour      ,0xFFFFFF        ,       IS(0),OPT_CLR          ,OPF_CLR  ,3,5,0,"TekCursorColour"          ,__("TekCursorColour")          )
  _CFGDEFD(colour   ,tek_write_thru_colour  ,0xFFFFFF        ,       IS(0),OPT_CLR          ,OPF_CLR  ,3,5,0,"TekWriteThruColour"       ,__("TekWriteThruColour")       )
  _CFGDEFD(colour   ,tek_defocused_colour   ,0xFFFFFF        ,       IS(0),OPT_CLR          ,OPF_CLR  ,3,5,0,"TekDefocusedColour"       ,__("TekDefocusedColour")       )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,3,2,0,""  ,0)
  _CFGDEFD(int      ,tek_glow               ,1               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,5,3,"TekGlow"                  ,__("TekGlow")                  )
  _CFGDEFD(int      ,tek_strap              ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,5,3,"TekStrap"                 ,__("TekStrap")                 )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,3,0,0,"Looks"    ,__("Tab"))
  _CFGDEFD(CBOOL    ,tab_bar_show           ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,0,0,"tab_bar_show"             ,__("tab_bar_show")             )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,3,3,0,""  ,0)
  _CFGDEFD(colour   ,tab_fg_colour          ,0x00FF00        ,       IS(0),OPT_CLR          ,OPF_CLR  ,3,5,0,"TabForegroundColour"      ,__("TabForegroundColour")      )
  _CFGDEFD(colour   ,tab_bg_colour          ,0x000000        ,       IS(0),OPT_CLR          ,OPF_CLR  ,3,5,0,"TabBackgroundColour"      ,__("TabBackgroundColour")      )
  _CFGDEFD(colour   ,tab_attention_bg_colour,0x323232        ,       IS(0),OPT_CLR          ,OPF_CLR  ,3,5,0,"TabAttentionColour"       ,__("TabAttentionColour")       )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,3,2,0,""  ,0)
  _CFGDEFD(colour   ,tab_active_bg_colour   ,0x003200        ,       IS(0),OPT_CLR          ,OPF_CLR  ,3,3,0,"TabActiveColour"          ,__("TabActiveColour")          )
  _CFGDEFD(int      ,tab_font_size          ,16              ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,6,3,"tab_font_size"            ,__("tab_font_size")            )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,3,2,0,""  ,0)
  _CFGDEFD(CBOOL    ,indicator              ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,0,0,"indicator"                ,__("indicator")                )
  _CFGDEFD(int      ,indicatorx             ,30              ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,5,3,"indicatorx"               ,__("indicatorx")               )
  _CFGDEFD(int      ,indicatory             ,30              ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,5,3,"indicatory"               ,__("indicatory")               )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_PANE ,9,0,0,__("Text") ,__("Text and Font properties")  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Text"     ,__("Font") )
  _CFGDEFD(font_spec,font                   ,FNT_DEF         ,       IS(0),OPT_FONT         ,OPF_FONT ,9,0,7,"Font"                     ,__("Font")                     ) 
  _CFGDEFD(CTYPE    ,font_smoothing         ,FS_DEFAULT      ,       IS(0),OPT_FONTST       ,OPF_LSTC ,9,0,7,"FontSmoothing"            ,__("Font smoothing")           ) 
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(CBOOL    ,allow_blinking         ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"AllowBlinking"            ,__("AllowBlinking")            ) 
  _CFGDEFD(CBOOL    ,dim_as_font            ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"DimAsFont"                ,__("Dim as font")              ) 
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(CBOOL    ,bold_as_font           ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"BoldAsFont"               ,__("Bold as font")             ) 
  _CFGDEFD(CBOOL    ,bold_as_colour         ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"BoldAsColour"             ,__("Bold as colour")           ) 
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Text"     ,__("Locale") )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(string   ,locale                 ,""              ,       IS(0),OPT_STR          ,OPF_LCL  ,9,7,7,"Locale"                   ,__("Locale")                   ) 
  //charset must be followd locale
  _CFGDEFD(string   ,charset                ,""              ,       IS(0),OPT_STR          ,OPF_CHSET,9,7,7,"Charset"                  ,__("Charset")                  ) 
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Text"     ,__("Emojis") )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(CTYPE    ,emojis                 ,0               ,       IS(0),OPT_EMOJIS       ,OPF_LSTC ,9,7,7,"Emojis"                   ,__("Style")                    ) 
  _CFGDEFD(CTYPE    ,emoji_placement        ,EMPL_STRETCH    ,       IS(0),OPT_EM_PLACE     ,OPF_LSTC ,9,7,7,"EmojiPlacement"           ,__("Placement")                ) 

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,3,2,0,""  ,0)
  _CFGDEFD(CBOOL    ,show_hidden_fonts      ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"ShowHiddenFonts"          ,__("ShowHiddenFonts")          ) 
  _CFGDEFD(CBOOL    ,old_locale             ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"OldLocale"                ,__("OldLocale")                ) 
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,3,2,0,""  ,0)
  _CFGDEFD(wstring  ,font_choice            ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,3,5,4,"FontChoice"               ,__("FontChoice")               ) 
  _CFGDEFD(wstring  ,font_sample            ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,3,5,4,"FontSample"               ,__("FontSample")               ) 
  _CFGDEFD(CTYPE    ,font_render            ,FR_UNISCRIBE    ,       IS(0),OPT_FONTRD       ,OPF_LSTR ,3,0,0,"FontRender"               ,__("FontRender")               ) 
  _CFGDEFD(CTYPE    ,charwidth              ,0               ,       IS(0),OPT_CHARWD       ,OPF_LSTR ,3,0,0,"Charwidth"                ,__("Charwidth")                ) 
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(int      ,fontmenu               ,-1              ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,4,3,"FontMenu"                 ,__("FontMenu")                 ) 
  _CFGDEFD(wstring  ,tek_font               ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,3,6,6,"TekFont"                  ,__("TekFont")                  ) 
  _CFGDEFA(font_spec,fontfams[12]           ,0               ,       IS(0),OPT_ARR          ,OPF_H    ,0,0,0,""                         ,__("fonts")                    ) 
  _CFGDEFB(0        ,fontfams[1]            ,FNT_NON         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,0,"Font1"                    ,__("Font1")                    ) 
  _CFGDEFB(0        ,fontfams[2]            ,FNT_NON         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,0,"Font2"                    ,__("Font2")                    ) 
  _CFGDEFB(0        ,fontfams[3]            ,FNT_NON         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,0,"Font3"                    ,__("Font3")                    ) 
  _CFGDEFB(0        ,fontfams[4]            ,FNT_NON         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,0,"Font4"                    ,__("Font4")                    ) 
  _CFGDEFB(0        ,fontfams[5]            ,FNT_NON         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,0,"Font5"                    ,__("Font5")                    ) 
  _CFGDEFB(0        ,fontfams[6]            ,FNT_NON         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,0,"Font6"                    ,__("Font6")                    ) 
  _CFGDEFB(0        ,fontfams[7]            ,FNT_NON         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,0,"Font7"                    ,__("Font7")                    ) 
  _CFGDEFB(0        ,fontfams[8]            ,FNT_NON         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,0,"Font8"                    ,__("Font8")                    ) 
  _CFGDEFB(0        ,fontfams[9]            ,FNT_NON         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,0,"Font9"                    ,__("Font9")                    ) 
  _CFGDEFB(0        ,fontfams[10]           ,FNT_NON         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,0,"Font10"                   ,__("Font10")                   ) 
  _CFGDEFB(0        ,fontfams[11]           ,FNT_RTL         ,       IS(0),OPT_FONT         ,OPF_FONT ,3,0,0,"FontRTL"                  ,__("FontRTL")                  ) 

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_PANE ,9,0,0,__("Keys")     ,__("Keyboard features")  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Keys"     ,  ""  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(CBOOL    ,backspace_sends_bs     ,'\b'            ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"BackspaceSendsBS"         ,__("&Backarrow sends ^H")      ) 
  _CFGDEFD(CBOOL    ,delete_sends_del       ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"DeleteSendsDEL"           ,__("&Delete sends DEL")        ) 
  _CFGDEFD(CBOOL    ,ctrl_alt_is_altgr      ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"CtrlAltIsAltGr"           ,__("Ctrl+LeftAlt is Alt&Gr")   ) 
  _CFGDEFD(CBOOL    ,altgr_is_alt           ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"AltGrIsAlsoAlt"           ,__("AltGr is also Alt")        ) 
  _CFGDEFD(CBOOL    ,capmapesc              ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"capmapesc"                ,__("Caplock As Escape")        )  
  _CFGDEFD(CBOOL    ,key_alpha_mode         ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"KeyAlphaMode"             ,__("&Esc/Enter reset IME to alphanumeric")   )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,3,0,""  ,0)
  _CFGDEFD(CBOOL    ,old_altgr_detection    ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"OldAltGrDetection"        ,__("OldAltGrDetection")        )
  _CFGDEFD(CBOOL    ,auto_repeat            ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"AutoRepeat"               ,__("AutoRepeat")               )
  _CFGDEFD(CBOOL    ,format_other_keys      ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"FormatOtherKeys"          ,__("FormatOtherKeys")          )
  _CFGDEFD(int      ,ctrl_alt_delay_altgr   ,100             ,IV( 0, 1000),OPT_INT          ,OPF_INT  ,3,0,3,"CtrlAltDelayAltGr"        ,__("CtrlAltDelayAltGr")        ) 
  _CFGDEFD(CTYPE    ,old_modify_keys        ,0               ,       IS(0),OPT_MOD          ,OPF_LSTR ,3,0,4,"OldModifyKeys"            ,__("OldModifyKeys")            )
  _CFGDEFD(int      ,external_hotkeys       ,2               ,IV( 0,    2),OPT_INT          ,OPF_INT  ,3,0,4,"SupportExternalHotkeys"   ,__("SupportExternalHotkeys")   )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Keys"     ,__("Shortcuts")  )
  _CFGDEFD(CBOOL    ,clip_shortcuts         ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"ClipShortcuts"            ,__("Cop&y and Paste (Ctrl/Shift+Ins)")       )
  _CFGDEFD(CBOOL    ,window_shortcuts       ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"WindowShortcuts"          ,__("&Menu and Full Screen (Alt+Space/Enter)"))
  _CFGDEFD(CBOOL    ,switch_shortcuts       ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"SwitchShortcuts"          ,__("&Switch window (Ctrl+[Shift+]Tab)")      )
  _CFGDEFD(CBOOL    ,zoom_shortcuts         ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"ZoomShortcuts"            ,__("&Zoom (Ctrl+plus/minus/zero)")           )
  _CFGDEFD(CBOOL    ,alt_fn_shortcuts       ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"AltFnShortcuts"           ,__("&Alt+Fn shortcuts")                      )
  _CFGDEFD(CBOOL    ,win_shortcuts          ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"WinShortcuts"             ,__("&Win+x shortcuts")                       )
  _CFGDEFD(CBOOL    ,ctrl_shift_shortcuts   ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"CtrlShiftShortcuts"       ,__("&Ctrl+Shift+letter shortcuts")           )
  _CFGDEFD(CTYPE    ,compose_key            ,0               ,       IS(0),OPT_COMPKEY      ,OPF_LSTR ,9,0,0,"ComposeKey"               ,__("Compose key")                            )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(CBOOL    ,zoom_font_with_window  ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"ZoomFontWithWindow"       ,__("ZoomFontWithWindow")                     )
  _CFGDEFD(CBOOL    ,hkwinkeyall            ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"hkwinkeyall"              ,__("hkwinkeyall")                            )
  _CFGDEFD(CBOOL    ,hook_keyboard          ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"hook_keyboard"            ,__("hook_keyboard")                          )
  _CFGDEFD(CBOOL    ,ctrl_exchange_shift    ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"CtrlExchangeShift"        ,__("CtrlExchangeShift")                      )
  _CFGDEFD(CBOOL    ,ctrl_controls          ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"CtrlControls"             ,__("CtrlControls")                           )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(string   ,key_prtscreen          ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,3,5,4,"Key_PrintScreen"          ,__("Key_PrintScreen")                        )  // VK_SNAPSHOT
  _CFGDEFD(string   ,key_pause              ,""            ,       IS(0),OPT_STR          ,OPF_STR  ,3,5,4,"Key_Pause"                ,__("Key_Pause")                              )  // VK_PAUSE
  _CFGDEFD(string   ,key_break              ,""            ,       IS(0),OPT_STR          ,OPF_STR  ,3,5,4,"Key_Break"                ,__("Key_Break")                              )  // VK_CANCEL
  _CFGDEFD(string   ,key_menu               ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,3,5,4,"Key_Menu"                 ,__("Key_Menu")                               )  // VK_APPS
  _CFGDEFD(string   ,key_scrlock            ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,3,5,4,"Key_ScrollLock"           ,__("Key_ScrollLock")                         )  // VK_SCROLL
  _CFGDEFD(int      ,manage_leds            ,7               ,IV( 0,    4),OPT_INT          ,OPF_INT  ,3,5,4,"ManageLEDs"               ,__("ManageLEDs")                             )
//_CFGDEFD(CBOOL    ,enable_remap_ctrls     ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,0,0,"ShootFoot"                ,__("ShootFoot")                              )
//_CFGDEFD(CBOOL    ,old_keyfuncs_keypad    ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,0,0,"OldKeyFunctionsKeypad"    ,__("OldKeyFunctionsKeypad")                  )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_PANE ,1,0,0,__("HotKeys")     ,__("Hotkey Setting")  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,1,0,0,"HotKeys"     ,  ""  )
  _CFGDEFD(wstring  ,key_commands           ,W("")           ,       IS(0),OPT_WSTRK        ,OPF_HKEY ,1,0,4,"KeyFunctions"             ,__("KeyFunctions")                           )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_PANE ,9,0,0,__("Mouse")    ,__("Mouse functions")  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Mouse"    ,  ""  )
  _CFGDEFD(CBOOL    ,clicks_place_cursor    ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"ClicksPlaceCursor"        ,__("Clic&ks place command line cursor")  )
  _CFGDEFD(string   ,word_chars_excl        ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,9,0,7,"WordCharsExcl"            ,__("Delimiters:")                        )
  _CFGDEFD(string   ,word_chars             ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,9,0,7,"WordChars"                ,__("Word Characters:")                   )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Mouse"                                             ,__("Click actions")  )
  _CFGDEFD(CTYPE    ,right_click_action     ,RC_MENU         ,       IS(0),OPT_RTCLICK      ,OPF_LSTR ,9,0,7,"RightClickAction"         ,__("Right mouse button")                 )
  _CFGDEFD(CTYPE    ,middle_click_action    ,MC_PASTE        ,       IS(0),OPT_MDCLICK      ,OPF_LSTR ,9,0,7,"MiddleClickAction"        ,__("Middle mouse button")                )
  _CFGDEFD(CBOOL    ,clicks_target_app      ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"ClicksTargetApp"          ,__("Clicks Send to App")                 )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Mouse"    ,__("Modifier for overriding default")  )
  _CFGDEFD(CTYPE    ,click_target_mod       ,MDK_SHIFT       ,       IS(0),OPT_MOD          ,OPF_LSTR ,9,0,9,"ClickTargetMod"           ,  ("")    )
  _CFGDEFD(CBOOL    ,opening_clicks         ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,0,0,"OpeningClicks"            ,__("OpeningClicks")                      )
  _CFGDEFD(CTYPE    ,opening_mod            ,MDK_CTRL        ,       IS(0),OPT_MOD          ,OPF_LSTR ,3,0,0,"OpeningMod"               ,__("OpeningMod")                         )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(CBOOL    ,zoom_mouse             ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"ZoomMouse"                ,__("ZoomMouse")                          )
  _CFGDEFD(CBOOL    ,hide_mouse             ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"HideMouse"                ,__("HideMouse")                          )
  _CFGDEFD(CBOOL    ,elastic_mouse          ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"ElasticMouse"             ,__("ElasticMouse")                       )
  _CFGDEFD(int      ,lines_per_notch        ,0               ,IV( 0, 2000),OPT_INT          ,OPF_INT  ,3,5,3,"LinesPerMouseWheelNotch"  ,__("LinesPerMouseWheelNotch")            )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(wstring  ,mouse_pointer          ,W("ibeam")      ,       IS(0),OPT_WSTR         ,OPF_CURS ,3,5,4,"MousePointer"             ,__("MousePointer")                       )
  _CFGDEFD(wstring  ,appmouse_pointer       ,W("arrow")      ,       IS(0),OPT_WSTR         ,OPF_CURS ,3,5,4,"AppMousePointer"          ,__("AppMousePointer")                    )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_PANE ,9,0,0,__("Selection"),__("Selection and clipboard")  ) 
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Selection",  ""  )
  _CFGDEFD(CBOOL    ,input_clears_selection ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"ClearSelectionOnInput"    ,__("Clear selection on input")           )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Selection",__("Clipboard")  )
  _CFGDEFD(CBOOL    ,copy_on_select         ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"CopyOnSelect"             ,__("Cop&y on select")                    )
  _CFGDEFD(CBOOL    ,copy_tabs              ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"CopyTab"                  ,__("Copy with TABs")                     )
  _CFGDEFD(CBOOL    ,copy_as_rtf            ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"CopyAsRTF"                ,__("Copy as &rich text")                 )
  _CFGDEFD(CTYPE    ,copy_as_html           ,0               ,       IS(0),OPT_CPYHTML      ,OPF_LSTR ,9,0,7,"CopyAsHTML"               ,__("CopyAsHTML")                         )
  _CFGDEFD(font_spec,rtf_font               ,FNT_NUL         ,       IS(0),OPT_FONT         ,OPF_FONT ,1,0,7,"CopyAsRTFFont"            ,__("CopyAsRTFFont")                     ) 
  _CFGDEFE(wstring  ,rtf_font.name          ,W("")           ,       IS(0),OPT_WSTR         ,OPF_H    ,0,0,7,"CopyAsRTFFontName"        ,__("CopyAsRTFFontName")                      )
  _CFGDEFE(int      ,rtf_font.size          ,0               ,IV( 0,  100),OPT_INT          ,OPF_H    ,0,0,7,"CopyAsRTFFontHeight"      ,__("CopyAsRTFFontHeight")                )
  _CFGDEFD(CBOOL    ,trim_selection         ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"TrimSelection"            ,__("Trim space from selection")          )
  _CFGDEFD(CBOOL    ,allow_set_selection    ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"AllowSetSelection"        ,__("Allow setting selection")            )
  _CFGDEFD(CBOOL    ,allow_paste_selection  ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,0,0,"AllowPasteSelection"      ,__("allow_paste_selection")              )
  _CFGDEFD(int      ,selection_show_size    ,0               ,IV( 0,   11),OPT_INT          ,OPF_INT  ,9,0,2,"SelectionShowSize"        ,__("Show size while selecting (0..12)")  )
  _CFGDEFD(int      ,suspbuf_max            ,8080            ,IV( 0,99999),OPT_INT          ,OPF_INT  ,9,0,2,"SuspendWhileSelecting"    ,__("Suspend output while selecting")     )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_PANE ,9,0,0,__("Window")   ,__("Window Options")  )  
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Window"   ,__("Default size")  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,3,0,""  ,0)
  _CFGDEFE(int      ,winsize.x              ,80              ,IV( 0, 9999),OPT_INT          ,OPF_INT  ,9,5,5,"Columns"                  ,__("Colu&mns")                           )
  _CFGDEFE(int      ,winsize.y              ,24              ,IV( 0, 9999),OPT_INT          ,OPF_INT  ,9,5,5,"Rows"                     ,__("Ro&ws")                              )
  _CFGDEFD(intpair  ,winsize                ,INTP(80,24)     ,IV( 0,    0),OPT_INTP         ,OPF_WSIZE,9,5,5,"winsize"                  ,__("C&urrent size")                      )
  _CFGDEFD(CBOOL    ,rewrap_on_resize       ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"RewrapOnResize"           ,__("Re&wrap on resize")                  )
  _CFGDEFD(CTYPE    ,scrollbar              ,0               ,       IS(0),OPT_SCRLBAR      ,OPF_LSTR ,9,0,7,"Scrollbar"                ,__("Scrollbar")                          )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(int      ,scrollback_lines       ,50000           ,IV( 0,    0),OPT_INT          ,OPF_INT  ,9,4,3,"ScrollbackLines"          ,__("Scroll&back lines")                  )
  _CFGDEFD(int      ,max_scrollback_lines   ,250000          ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,5,3,"MaxScrollbackLines"       ,__("MaxScrollbackLines")                 )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Window"   ,__("Modifier for scrolling")  )
  _CFGDEFD(CTYPE    ,scroll_mod             ,MDK_SHIFT       ,       IS(0),OPT_MOD          ,OPF_LSTR ,9,0,9,"ScrollMod"                ,  ("")             )
  _CFGDEFD(CBOOL    ,pgupdn_scroll          ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"PgUpDnScroll"             ,__("&PgUp and PgDn scroll without modifier")       )
  _CFGDEFD(CTYPE    ,border_style           ,BORDER_NORMAL   ,       IS(0),OPT_BORDER       ,OPF_LSTR ,9,0,7,"BorderStyle"              ,__("BorderStyle")                                  )
  _CFGDEFD(CBOOL    ,allocconsole           ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,0,0,"allocconsole"             ,__("&AllocConsole ,Some console programs need it."))

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,3,0,0,"Window"   ,""  )
  _CFGDEFD(wstring  ,search_bar             ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,3,0,7,"SearchBar"                ,__("Search Bar")               )
  _CFGDEFD(int      ,search_context         ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,0,7,"SearchContext"            ,__("SearchContext")            )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_PANE ,9,0,0,__("Terminal") ,__("Terminal features")  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Terminal" ,  ""  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(string   ,Term                   ,"xterm"         ,       IS(0),OPT_STR          ,OPF_TERM ,9,5,5,"Term"                     ,__("Term")                     )
  _CFGDEFD(string   ,answerback             ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,9,5,6,"Answerback"               ,__("&Answerback")              )
  _CFGDEFD(int      ,wrap_tab               ,0               ,IV( 0,    2),OPT_INT          ,OPF_INT  ,3,0,7,"WrapTab"                  ,__("WrapTab")                  )
  _CFGDEFD(CBOOL    ,old_wrapmodes          ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,0,0,"OldWrapModes"             ,__("OldWrapModes")             )
  _CFGDEFD(CBOOL    ,enable_deccolm_init    ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,0,0,"Enable132ColumnSwitching" ,__("Enable132ColumnSwitching") )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"Terminal" ,__("Bell")  )
  _CFGDEFD(CTYPE    ,bell_type              ,1               ,       IS(0),OPT_BELL         ,OPF_BELL ,9,0,7,"BellType"                 ,__("BellType")                 )
  _CFGDEFA(string   ,bell_file[7]           ,0               ,       IS(0),OPT_ARR          ,OPF_H    ,3,0,0,""                         ,__("BellFiles")                )
  _CFGDEFB(string   ,bell_file[6]           ,""              ,       IS(0),OPT_STR          ,OPF_BELLF,9,0,7,"BellFile"                 ,__("BellFile")                 )
  _CFGDEFB(string   ,bell_file[0]           ,""              ,       IS(0),OPT_STR          ,OPF_BELLF,3,0,7,"BellFile2"                ,__("BellFile2")                )
  _CFGDEFB(string   ,bell_file[1]           ,""              ,       IS(0),OPT_STR          ,OPF_BELLF,3,0,7,"BellFile3"                ,__("BellFile3")                )
  _CFGDEFB(string   ,bell_file[2]           ,""              ,       IS(0),OPT_STR          ,OPF_BELLF,3,0,7,"BellFile4"                ,__("BellFile4")                )
  _CFGDEFB(string   ,bell_file[3]           ,""              ,       IS(0),OPT_STR          ,OPF_BELLF,3,0,7,"BellFile5"                ,__("BellFile5")                )
  _CFGDEFB(string   ,bell_file[4]           ,""              ,       IS(0),OPT_STR          ,OPF_BELLF,3,0,7,"BellFile6"                ,__("BellFile6")                )
  _CFGDEFB(string   ,bell_file[5]           ,""              ,       IS(0),OPT_STR          ,OPF_BELLF,3,0,7,"BellFile7"                ,__("BellFile7")                )
  _CFGDEFD(int      ,bell_freq              ,0               ,IV(20,20000),OPT_INT          ,OPF_INT  ,3,0,7,"BellFreq"                 ,__("BellFreq")                 )
  _CFGDEFD(int      ,bell_len               ,400             ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,0,7,"BellLen"                  ,__("BellLen")                  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,3,0,""  ,0)
  _CFGDEFD(CBOOL    ,bell_taskbar           ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"BellTaskbar"              ,__("&Highlight in taskbar")    )// xterm: bellIsUrgent
  _CFGDEFD(CBOOL    ,bell_popup             ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"BellPopup"                ,__("&Popup")                   )// xterm: popOnBell
  _CFGDEFD(CBOOL    ,bell_flash             ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"BellFlash"                ,__("&Flash")                   )// xterm: visualBell
  _CFGDEFD(CTYPE    ,bell_flash_style       ,FLASH_FULL      ,       IS(0),OPT_FLASH        ,OPF_LSTR ,3,0,0,"BellFlashStyle"           ,__("BellFlashStyle")           )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(int      ,bell_interval          ,100             ,IV( 0,    0),OPT_INT          ,OPF_INT  ,3,5,7,"BellInterval"             ,__("BellInterval")             )
  _CFGDEFD(int      ,play_tone              ,2               ,IV( 1,    5),OPT_INT          ,OPF_INT  ,3,5,7,"PlayTone"                 ,__("PlayTone")                 )
  _CFGDEFD(wstring  ,printer                ,W("")           ,       IS(0),OPT_WSTR         ,OPF_PRINT,9,0,7,"Printer"                  ,__("Printer")                  )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_PANE ,9,0,0,__("System") ,__("system setting")  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"System"     ,__("Dialog"))
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(int      ,optlevel               ,5               ,IV( 0,    9),OPT_INT          ,OPF_INT  ,9,5,4,"OptionsLevel"             ,__("Options Level")             )
  _CFGDEFD(string   ,lang                   ,("")            ,       IS(0),OPT_STR          ,OPF_LANG ,9,6,5,"Language"                 ,__("Language")                 )
  _CFGDEFD(int      ,scale_options_width    ,100             ,IV(80,  200),OPT_INT          ,OPF_H    ,5,5,4,"scale_options_width"      ,__("scale_options_width")      )

  _CFGDEFD(font_spec,opt_font               ,FNT("",12,0,0)  ,       IS(0),OPT_FONT         ,OPF_FONT ,9,0,7,"OptionsFont"              ,__("Options Font")                     ) 
  _CFGDEFE(wstring  ,opt_font.name          ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,0,0,7,"OptionsFontname"          ,__("OptionsFontname")              )
  _CFGDEFE(int      ,opt_font.size          ,12              ,IV( 0,    0),OPT_INT          ,OPF_H    ,0,0,7,"OptionsFontHeight"        ,__("OptionsFontHeight")        )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,9,0,0,"System"   ,  ""  )
  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_COL  ,9,2,0,""  ,0)
  _CFGDEFD(CBOOL    ,confirm_exit           ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"ConfirmExit"              ,__("Prompt on &close"))
  _CFGDEFD(CBOOL    ,confirm_reset          ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"ConfirmReset"             ,__("ConfirmReset")             )
  _CFGDEFD(CBOOL    ,confirm_mline_past     ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"ConfirmMultiLinePasting"  ,__("ConfirmMultiLinePasting")  )
  _CFGDEFD(CBOOL    ,no_alt_screen          ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"NoAltScreen"              ,__("NoAltScreen")              )
  _CFGDEFD(CBOOL    ,erase_to_scrollback    ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"EraseToScrollback"        ,__("EraseToScrollback")        )
  _CFGDEFD(CBOOL    ,status_line            ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,9,5,0,"StatusLine"               ,__("Status Line")              )
  _CFGDEFD(CBOOL    ,bidi                   ,2               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,3,5,0,"Bidi"                     ,__("Bidi")                     )
  _CFGDEFD(int      ,display_speedup        ,6               ,IV( 0,    9),OPT_INT          ,OPF_INT  ,3,5,3,"DisplaySpeedup"           ,__("DisplaySpeedup")           )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,2,0,0,"System"          ,__("Command line Options"))   
  _CFGDEFD(wstring  ,class                  ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,2,0,7,"Class"                    ,__("Class")                    )
  _CFGDEFD(CTYPE    ,hold                   ,HOLD_START      ,       IS(0),OPT_HOLD         ,OPF_LSTR ,2,0,0,"Hold"                     ,__("Hold")                     )
  _CFGDEFD(CBOOL    ,exit_write             ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,2,0,0,"ExitWrite"                ,__("ExitWrite")                )
  _CFGDEFD(wstring  ,exit_title             ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,2,0,5,"ExitTitle"                ,__("ExitTitle")                )
  _CFGDEFD(wstring  ,icon                   ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,2,0,5,"Icon"                     ,__("Icon")                     )
  _CFGDEFD(wstring  ,log                    ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,2,0,5,"Log"                      ,__("Log")                      )
  _CFGDEFD(CTYPE    ,logging                ,0               ,IV( 0,    3),OPT_LOG          ,OPF_LSTR ,2,0,0,"Logging"                  ,__("Logging")                  )
  _CFGDEFD(CBOOL    ,create_utmp            ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,2,0,0,"Utmp"                     ,__("Utmp")                     )
  _CFGDEFD(wstring  ,title                  ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,2,0,7,"Title"                    ,__("Title")                    )
  _CFGDEFD(CBOOL    ,title_settable         ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,2,0,0,"title_settable"           ,__("title_settable")           )
  _CFGDEFD(CTYPE    ,window                 ,0               ,       IS(0),OPT_WINDOW       ,OPF_LSTR ,2,0,0,"Window"                   ,__("Window")                   )
  _CFGDEFD(int      ,x                      ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,2,0,7,"X"                        ,__("X")                        )
  _CFGDEFD(int      ,y                      ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,2,0,7,"Y"                        ,__("Y")                        )
  _CFGDEFD(CBOOL    ,daemonize              ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,2,0,0,"Daemonize"                ,__("Daemonize")                )
  _CFGDEFD(CBOOL    ,daemonize_always       ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,2,0,0,"DaemonizeAlways"          ,__("Daemonize Always")          )

  _CFGDEFC(0        ,=======                ,0               ,       IS(0),OPT_CMT          ,OPF_GRP  ,1,0,0,"Hidden"   ,__("Hidden Options"))  
  _CFGDEFD(string   ,suppress_sgr           ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,7,"SuppressSGR"              ,__("SuppressSGR")              )
  _CFGDEFD(string   ,suppress_dec           ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,7,"SuppressDEC"              ,__("SuppressDEC")              )
  _CFGDEFD(string   ,suppress_win           ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,7,"SuppressWIN"              ,__("SuppressWIN")              )
  _CFGDEFD(string   ,suppress_osc           ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,7,"SuppressOSC"              ,__("SuppressOSC")              )
  _CFGDEFD(string   ,suppress_nrc           ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,7,"SuppressNRC"              ,__("SuppressNRC")              )
  _CFGDEFD(string   ,suppress_wheel         ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,7,"SuppressMouseWheel"       ,__("SuppressMouseWheel")       )
  _CFGDEFD(string   ,filter_paste           ,"STTY"          ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,7,"FilterPasteControls"      ,__("FilterPasteControls")      )
  _CFGDEFD(CBOOL    ,guard_path             ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"GuardNetworkPaths"        ,__("GuardNetworkPaths")               )
  _CFGDEFD(CTYPE    ,bracketed_paste_split  ,0               ,       IS(0),OPT_SPBRK        ,OPF_LSTR ,1,0,0,"BracketedPasteByLine"     ,__("BracketedPasteByLine")     )
  _CFGDEFD(CTYPE    ,printable_controls     ,0               ,       IS(0),OPT_PCTRL        ,OPF_LSTR ,1,0,0,"PrintableControls"        ,__("PrintableControls")        )
  _CFGDEFD(int      ,char_narrowing         ,75              ,IV(50,  100),OPT_INT          ,OPF_INT  ,1,0,7,"CharNarrowing"            ,__("CharNarrowing")            )
  _CFGDEFD(wstring  ,save_filename          ,SAVE_FN         ,       IS(0),OPT_WSTR         ,OPF_WSTR ,1,0,7,"SaveFilename"             ,__("SaveFilename")             )
  _CFGDEFD(wstring  ,app_id                 ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,1,0,7,"AppID"                    ,__("AppID")                    )
  _CFGDEFD(wstring  ,app_name               ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,1,0,7,"AppName"                  ,__("AppName")                  )
  _CFGDEFD(wstring  ,app_launch_cmd         ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,1,0,7,"AppLaunchCmd"             ,__("AppLaunchCmd")             )
  _CFGDEFD(wstring  ,drop_commands          ,W("")           ,       IS(0),OPT_WSTRK        ,OPF_WSTR ,1,0,7,"DropCommands"             ,__("DropCommands")             )
  _CFGDEFD(wstring  ,exit_commands          ,W("")           ,       IS(0),OPT_WSTRK        ,OPF_WSTR ,1,0,7,"ExitCommands"             ,__("ExitCommands")             )
  _CFGDEFD(wstring  ,user_commands          ,W("")           ,       IS(0),OPT_WSTRK        ,OPF_WSTR ,1,0,7,"UserCommands"             ,__("UserCommands")             )
  _CFGDEFD(wstring  ,ctx_user_commands      ,W("")           ,       IS(0),OPT_WSTRK        ,OPF_WSTR ,1,0,7,"CtxMenuFunctions"         ,__("CtxMenuFunctions")         )
  _CFGDEFD(wstring  ,sys_user_commands      ,W("")           ,       IS(0),OPT_WSTRK        ,OPF_WSTR ,1,0,7,"SysMenuFunctions"         ,__("SysMenuFunctions")         )
  _CFGDEFD(wstring  ,user_commands_path     ,W("/bin:%s")    ,       IS(0),OPT_WSTR         ,OPF_WSTR ,1,0,7,"UserCommandsPath"         ,__("UserCommandsPath")         )
  _CFGDEFD(wstring  ,session_commands       ,W("")           ,       IS(0),OPT_WSTRK        ,OPF_WSTR ,1,0,7,"SessionCommands"          ,__("SessionCommands")          )
  _CFGDEFD(wstring  ,task_commands          ,W("")           ,       IS(0),OPT_WSTRK        ,OPF_WSTR ,1,0,7,"TaskCommands"             ,__("TaskCommands")             )
  _CFGDEFD(CBOOL    ,conpty_support         ,-1              ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"ConPTY"                   ,__("ConPTY")                   )
  _CFGDEFD(CBOOL    ,login_from_shortcut    ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"LoginFromShortcut"        ,__("LoginFromShortcut")        )
  _CFGDEFD(string   ,menu_mouse             ,"b"             ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,5,"MenuMouse"                ,__("MenuMouse")                )
  _CFGDEFD(string   ,menu_ctrlmouse         ,"e|ls"          ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,5,"MenuCtrlMouse"            ,__("MenuCtrlMouse")            )
  _CFGDEFD(string   ,menu_altmouse          ,"ls"            ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,5,"MenuMouse5"               ,__("MenuMouse5")               )
  _CFGDEFD(string   ,menu_title_ctrl_l      ,"Ws"            ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,5,"MenuTitleCtrlLeft"        ,__("MenuTitleCtrlLeft")        )
  _CFGDEFD(string   ,menu_title_ctrl_r      ,"Ws"            ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,5,"MenuTitleCtrlRight"       ,__("MenuTitleCtrlRight")       )
  _CFGDEFD(int      ,geom_sync              ,0               ,IV( 0,    0),OPT_INT          ,OPF_H    ,1,0,5,"SessionGeomSync"          ,__("SessionGeomSync")          )
  _CFGDEFD(int      ,col_spacing            ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,1,0,5,"ColSpacing"               ,__("ColSpacing")               )
  _CFGDEFD(int      ,row_spacing            ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,1,0,5,"RowSpacing"               ,__("RowSpacing")               )
  _CFGDEFD(CTYPE    ,auto_leading           ,2               ,IV( 0,    2),OPT_ALEAD        ,OPF_LSTR ,1,0,0,"AutoLeading"              ,__("AutoLeading")              )
  _CFGDEFD(CBOOL    ,ligatures              ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"Ligatures"                ,__("Ligatures")                )
  _CFGDEFD(CTYPE    ,ligatures_support      ,0               ,IV( 0,    2),OPT_LIGS         ,OPF_LSTR ,1,0,0,"LigaturesSupport"         ,__("LigaturesSupport")         )
  _CFGDEFD(CBOOL    ,box_drawing            ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"BoxDrawing"               ,__("BoxDrawing")               )
  _CFGDEFD(CTYPE    ,handle_dpichanged      ,2               ,       IS(0),OPT_DPIA         ,OPF_LSTR ,1,0,0,"HandleDPI"                ,__("HandleDPI")                )
  _CFGDEFD(CBOOL    ,check_version_update   ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"CheckVersionUpdate"       ,__("CheckVersionUpdate")       )
  _CFGDEFD(colour   ,ime_cursor_colour      ,DEFAULT_COLOUR  ,       IS(0),OPT_CLR          ,OPF_CLR  ,1,0,0,"IMECursorColour"          ,__("IMECursorColour")          )
  _CFGDEFD(wstring  ,sixel_clip_char        ,W("")           ,       IS(0),OPT_WSTR         ,OPF_WSTR ,1,0,7,"SixelClipChars"           ,__("SixelClipChars")           )
  _CFGDEFD(CBOOL    ,old_bold               ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"OldBold"                  ,__("OldBold")                  )
  _CFGDEFD(CBOOL    ,short_long_opts        ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"ShortLongOpts"            ,__("ShortLongOpts")            )
  _CFGDEFD(CBOOL    ,bold_as_special        ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"BoldAsRainbowSparkles"    ,__("BoldAsRainbowSparkles")    )
  _CFGDEFD(CBOOL    ,hover_title            ,1               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"HoverTitle"               ,__("HoverTitle")               )
  _CFGDEFD(CBOOL    ,progress_bar           ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"ProgressBar"              ,__("ProgressBar")              )
  _CFGDEFD(CTYPE    ,progress_scan          ,1               ,       IS(0),OPT_PRSC         ,OPF_LSTR ,1,0,0,"ProgressScan"             ,__("ProgressScan")             )
  _CFGDEFD(CBOOL    ,dim_margins            ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"DimMargins"               ,__("DimMargins")               )
  _CFGDEFD(int      ,baud                   ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,1,0,5,"Baud"                     ,__("Baud")                     )
  _CFGDEFD(CBOOL    ,bloom                  ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"Bloom"                    ,__("Bloom")                    )
  _CFGDEFD(string   ,old_options            ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,5,"OldOptions"               ,__("OldOptions")               )
  _CFGDEFD(CBOOL    ,old_xbuttons           ,0               ,       IS(0),OPT_BOOL         ,OPF_CHK  ,1,0,0,"OldXButtons"              ,__("OldXButtons")              )
  _CFGDEFD(int      ,wslbridge              ,2               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,1,0,5,"WSLBridge"                ,__("wslbridge version,0:nobridge,1:wslbridge,2:wslbreadge2") )
  _CFGDEFD(string   ,wslname                ,""              ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,5,"WSLName"                  ,__("wslDistribution,null is default"))
  // Legacy
  _CFGDEFD(int      ,status_debug           ,0               ,IV( 0,    0),OPT_INT          ,OPF_INT  ,1,0,5,"StatusDebug"              ,__("status_debug")             )
//_CFGDEFD(string   ,menu_menu              ,"bs"            ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,5,"MenuMenu"                 ,__("MenuMenu")                 )
//_CFGDEFD(string   ,menu_ctrlmenu          ,"e|ls"          ,       IS(0),OPT_STR          ,OPF_STR  ,1,0,5,"MenuCtrlMenu"             ,__("MenuCtrlMenu")             )
  _CFGDEFE(int      ,font.size              ,0               ,IV( 0,    0),OPT_INT|OPT_LG   ,OPF_H    ,0,0,0,"FontSize"                 ,__("FontSize")  )
  _CFGDEFE(string   ,key_break              ,0               ,       IS(0),OPT_STR|OPT_LG   ,OPF_H    ,0,0,0,"Break"                    ,__("Break")  )
  _CFGDEFE(string   ,key_pause              ,0               ,       IS(0),OPT_STR|OPT_LG   ,OPF_H    ,0,0,0,"Pause"                    ,__("Pause")  )
  _CFGDEFE(int      ,rtf_font.size          ,0               ,IV( 0,    0),OPT_INT|OPT_LG   ,OPF_H    ,0,0,0,"CopyAsRTFFontSize"        ,__("CopyAsRTFFontSize")  )
  _CFGDEFE(int      ,fontmenu               ,0               ,IV( 0,    0),OPT_INT|OPT_LG   ,OPF_H    ,0,0,0,"OldFontMenu"              ,__("OldFontMenu")  )
  _CFGDEFE(int      ,opt_font.size          ,0               ,IV( 0,    0),OPT_INT|OPT_LG   ,OPF_H    ,0,0,0,"OptionsFontSize"          ,__("OptionsFontSize")  )
  _CFGDEFE(int      ,opt_font.size          ,12              ,IV( 0,    0),OPT_INT|OPT_LG   ,OPF_INT  ,5,5,5,"gui_font_size"            ,__("gui_font_size")            )
  _CFGDEFE(CBOOL    ,bold_as_colour         ,0               ,       IS(0),OPT_BOOL|OPT_LG  ,OPF_H    ,0,0,0,"BoldAsBright"             ,__("BoldAsBright")  )
  _CFGDEFE(CTYPE    ,font_smoothing         ,0               ,       IS(0),OPT_FONTST|OPT_LG,OPF_H    ,0,0,0,"FontQuality"              ,__("FontQuality")  )
  _CFGDEFD(colour   ,use_system_colours     ,0               ,       IS(0),OPT_BOOL|OPT_LG  ,OPF_H    ,0,0,0,"UseSystemColours"         ,__("UseSystemColours")         )
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
