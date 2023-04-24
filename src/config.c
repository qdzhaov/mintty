// config.c (part of mintty)
// Copyright 2008-2022 Andy Koppe, 2015-2022 Thomas Wolff
// Based on code from PuTTY-0.60 by Simon Tatham and team.
// Licensed under the terms of the GNU General Public License v3 or later.

// Internationalization approach:
// instead of refactoring a lot of framework functions (here, *ctrls.c)
// to use Unicode strings, the API is simply redefined to use UTF-8;
// non-ASCII strings are converted before being passed to the platform
// (using UTF-16 on Windows)

#include "term.h"
#include "ctrls.h"
#include "print.h"
#include "charset.h"
#include "win.h"
#include <fcntl.h>  // open+flags, mkdir

#include <windows.h>  // registry handling
#include "winpriv.h"  // support_wsl, load_library_func

#ifdef __CYGWIN__
#include <sys/cygwin.h>  // cygwin_internal
#endif


#define dont_support_blurred


string config_dir = 0;
static wstring rc_filename = 0;

#define CLRCPY(d,s) memcpy(d,s,16*sizeof(colour))
static colour def_colours[16];
static colour cfg_colours[16];
static colour theme_colours[16];
static wchar theme_file[128]={0};

#define HASHTS 31
static inline int HASHS(const void*s){
  return (*(char*)s?((*(unsigned short*)s)|0x2020)%(HASHTS):0);
}
// all entries need initialisation in options[] or crash...
const config default_cfg = {
  // Looks
  .fg_colour = 0xBFBFBF,
  .bold_colour = (colour)-1,
  .blink_colour = (colour)-1,
  .bg_colour = 0x000000,
  .cursor_colour = 0xBFBFBF,
  .tek_fg_colour = (colour)-1,
  .tek_bg_colour = (colour)-1,
  .tek_cursor_colour = (colour)-1,
  .tek_write_thru_colour = (colour)-1,
  .tek_defocused_colour = (colour)-1,
  .tek_glow = 1,
  .tek_strap = 0,
  .underl_colour = (colour)-1,
  .hover_colour = (colour)-1,
  .ansi_colours = {
    [BLACK_I]        = { RGB(  0,   0,   0), RGB(  0,   0,   0) },
    [RED_I]          = { RGB(212,  44,  58), RGB(162,  30,  41) },
    [GREEN_I]        = { RGB( 28, 168,   0), RGB( 28, 168,   0) },
    [YELLOW_I]       = { RGB(192, 160,   0), RGB(192, 160,   0) },
    [BLUE_I]         = { RGB(  0,  93, 255), RGB(  0,  32, 192) },
    [MAGENTA_I]      = { RGB(177,  72, 198), RGB(134,  54, 150) },
    [CYAN_I]         = { RGB(  0, 168, 154), RGB(  0, 168, 154) },
    [WHITE_I]        = { RGB(191, 191, 191), RGB(191, 191, 191) },
    [BOLD_BLACK_I]   = { RGB( 96,  96,  96), RGB( 72,  72,  72) },
    [BOLD_RED_I]     = { RGB(255, 118, 118), RGB(255, 118, 118) },
    [BOLD_GREEN_I]   = { RGB(  0, 242,   0), RGB(  0, 242,   0) },
    [BOLD_YELLOW_I]  = { RGB(242, 242,   0), RGB(242, 242,   0) },
    [BOLD_BLUE_I]    = { RGB(125, 151, 255), RGB(125, 151, 255) },
    [BOLD_MAGENTA_I] = { RGB(255, 112, 255), RGB(255, 112, 255) },
    [BOLD_CYAN_I]    = { RGB(  0, 240, 240), RGB(  0, 240, 240) },
    [BOLD_WHITE_I]   = { RGB(255, 255, 255), RGB(255, 255, 255) }
    //[BLACK_I]        = RGB(0x00, 0x00, 0x00),
    //[RED_I]          = RGB(0xBF, 0x00, 0x00),
    //[GREEN_I]        = RGB(0x00, 0xBF, 0x00),
    //[YELLOW_I]       = RGB(0xBF, 0xBF, 0x00),
    //[BLUE_I]         = RGB(0x00, 0x00, 0xBF),
    //[MAGENTA_I]      = RGB(0xBF, 0x00, 0xBF),
    //[CYAN_I]         = RGB(0x00, 0xBF, 0xBF),
    //[WHITE_I]        = RGB(0xBF, 0xBF, 0xBF),
    //[BOLD_BLACK_I]   = RGB(0x40, 0x40, 0x40),
    //[BOLD_RED_I]     = RGB(0xFF, 0x40, 0x40),
    //[BOLD_GREEN_I]   = RGB(0x40, 0xFF, 0x40),
    //[BOLD_YELLOW_I]  = RGB(0xFF, 0xFF, 0x40),
    //[BOLD_BLUE_I]    = RGB(0x60, 0x60, 0xFF),
    //[BOLD_MAGENTA_I] = RGB(0xFF, 0x40, 0xFF),
    //[BOLD_CYAN_I]    = RGB(0x40, 0xFF, 0xFF),
    //[BOLD_WHITE_I]   = RGB(0xFF, 0xFF, 0xFF)
  },
  .disp_space = 0,
  .disp_clear = 0,
  .disp_tab = 0,
  .underl_manual = false,
  .sel_fg_colour = (colour)-1,
  .sel_bg_colour = (colour)-1,
  .search_fg_colour = 0x000000,
  .search_bg_colour = 0x00DDDD,
  .search_current_colour = 0x0099DD,
  .theme_file = W(""),
  .background = W(""),
  .colour_scheme = "",
  .transparency = 0,
  .blurred = false,
  .opaque_when_focused = false,
  .cursor_type = CUR_LINE,
  .cursor_blinks = true,
  //need add 
  .tab_fg_colour = 0x00FF00,
  .tab_bg_colour = 0x000000,
  .tab_active_bg_colour = 0x323232,
  .tab_attention_bg_colour = 0x003200,
  .tab_bar_show = 1,
  .indicator = 1,
  .indicatorx = 30,
  .indicatory = 30,
  .tab_font_size= 16 ,
  .gui_font_size= 12 ,
  .scale_options_width =100,
  .padding = 1,
  .partline= 4,
  .usepartline= 0,
  // Text
  .font = {.name = W("Lucida Console"), .size = 16, .weight = 400, .isbold = false},
  .fontfams[1] = {.name = W(""), .weight = 400, .isbold = false},
  .fontfams[2] = {.name = W(""), .weight = 400, .isbold = false},
  .fontfams[3] = {.name = W(""), .weight = 400, .isbold = false},
  .fontfams[4] = {.name = W(""), .weight = 400, .isbold = false},
  .fontfams[5] = {.name = W(""), .weight = 400, .isbold = false},
  .fontfams[6] = {.name = W(""), .weight = 400, .isbold = false},
  .fontfams[7] = {.name = W(""), .weight = 400, .isbold = false},
  .fontfams[8] = {.name = W(""), .weight = 400, .isbold = false},
  .fontfams[9] = {.name = W(""), .weight = 400, .isbold = false},
  .fontfams[10] = {.name = W(""), .weight = 400, .isbold = false},
  .fontfams[11] = {.name = W("Courier New"), .weight = 400, .isbold = false},
  .font_choice = W(""),
  .font_sample = W(""),
  .show_hidden_fonts = false,
  .font_smoothing = FS_DEFAULT,
  .font_render = FR_UNISCRIBE,
  .bold_as_font = false,
  .bold_as_colour = true,
  .allow_blinking = false,
  .locale = "",
  .charset = "",
  .charwidth = 0,
  .old_locale = false,
  .fontmenu = -1,
  .tek_font = W(""),
  // Keys
  .backspace_sends_bs = CERASE == '\b',
  .delete_sends_del = false,
  .ctrl_alt_is_altgr = false,
  .altgr_is_alt = false,
  .ctrl_alt_delay_altgr = 0,
  .old_altgr_detection = false,
  .old_modify_keys = 0,
  .format_other_keys = 1,
  .auto_repeat = true,
  .external_hotkeys = 2,
  .clip_shortcuts = true,
  .window_shortcuts = true,
  .switch_shortcuts = true,
  .zoom_shortcuts = true,
  .zoom_font_with_window = true,
  .alt_fn_shortcuts = true,
  .win_shortcuts = true,
  .hkwinkeyall=0,
  .hook_keyboard=1,
  .ctrl_shift_shortcuts = false,
  .ctrl_exchange_shift = false,
  .ctrl_controls = true,
  .compose_key = 0,
  .key_prtscreen = "",	// VK_SNAPSHOT
  .key_pause = "",	// VK_PAUSE
  .key_break = "",	// VK_CANCEL
  .key_menu = "",	// VK_APPS
  .key_scrlock = "",	// VK_SCROLL
  .key_commands = W(""),
  .manage_leds = 7,
  .enable_remap_ctrls = false,
  .old_keyfuncs_keypad = false,
  // Mouse
  .clicks_place_cursor = false,
  .middle_click_action = MC_PASTE,
  .right_click_action = RC_MENU,
  .opening_clicks = 1,
  .opening_mod = MDK_CTRL,
  .zoom_mouse = true,
  .clicks_target_app = true,
  .click_target_mod = MDK_SHIFT,
  .hide_mouse = true,
  .elastic_mouse = false,
  .lines_per_notch = 0,
  .mouse_pointer = W("ibeam"),
  .appmouse_pointer = W("arrow"),
  // Selection
  .input_clears_selection = true,
  .copy_on_select = true,
  .copy_tabs = false,
  .copy_as_rtf = true,
  .copy_as_html = 0,
  .copy_as_rtf_font = W(""),
  .copy_as_rtf_font_size = 0,
  .trim_selection = true,
  .allow_set_selection = false,
  .selection_show_size = false,
  // Window
  .cols = 80,
  .rows = 24,
  .rewrap_on_resize = false,
  .scrollbar = 1,
  .scrollback_lines = 10000,
  .max_scrollback_lines = 250000,
  .scroll_mod = MDK_SHIFT,
  .pgupdn_scroll = false,
  .allocconsole=false,
  .lang = W(""),
  .search_bar = W(""),
  .search_context = 0,
  // Terminal
  .Term = "xterm",
  .answerback = (""),
  .wrap_tab = 0,
  .old_wrapmodes = false,
  .enable_deccolm_init = false,
  .bell_type = 1,
  .bell_file = {W(""), W(""), W(""), W(""), W(""), W(""), W("")},
  .bell_freq = 0,
  .bell_len = 400,
  .bell_flash = false,  // xterm: visualBell
  .bell_flash_style = FLASH_FULL,
  .bell_taskbar = true, // xterm: bellIsUrgent
  .bell_popup = false,  // xterm: popOnBell
  .bell_interval = 100,
  .play_tone = 2,
  .printer = W(""),
  .confirm_exit = true,
  .confirm_reset = false,
  // Command line
  .class = W(""),
  .hold = HOLD_START,
  .exit_write = false,
  .exit_title = W(""),
  .icon = W(""),
  .log = W(""),
  .logging = 0,
  .create_utmp = false,
  .title = W(""),
  .title_settable = true,
  .window=0,
  .x=0, 
  .y=0,
  .daemonize = true,
  .daemonize_always = false,
  // "Hidden"
  .bidi = 2,
  .disable_alternate_screen = false,
  .erase_to_scrollback = true,
  .display_speedup = 6,
  .suppress_sgr = "",
  .suppress_dec = "",
  .suppress_win = "",
  .suppress_osc = "",
  .suppress_nrc = "",  // unused
  .suppress_wheel = "",
  .filter_paste = "",
  .bracketed_paste_split = 0,
  .suspbuf_max = 8080,
  .printable_controls = 0,
  .char_narrowing = 75,
  .emojis = 0,
  .emoji_placement = EMPL_STRETCH,
  .save_filename = W("mintty.%Y-%m-%d_%H-%M-%S"),
  .app_id = W(""),
  .app_name = W(""),
  .app_launch_cmd = W(""),
  .drop_commands = W(""),
  .exit_commands = W(""),
  .user_commands = W(""),
  .ctx_user_commands = W(""),
  .sys_user_commands = W(""),
  .user_commands_path = W("/bin:%s"),
  .session_commands = W(""),
  .task_commands = W(""),
  .conpty_support = -1,
  .login_from_shortcut = true,
  .menu_mouse = "b",
  .menu_ctrlmouse = "e|ls",
  .menu_altmouse = "ls",
  .menu_menu = "bs",
  .menu_ctrlmenu = "e|ls",
  .menu_title_ctrl_l = "Ws",
  .menu_title_ctrl_r = "Ws",
  .geom_sync = 0,
  .col_spacing = 0,
  .row_spacing = 0,
  .auto_leading = 2,
  .ligatures = 1,
  .ligatures_support = 0,
  .handle_dpichanged = 2,
  .check_version_update = 0,
  .word_chars = "",
  .word_chars_excl = "",
  .ime_cursor_colour = DEFAULT_COLOUR,
  .sixel_clip_char = W(" "),
  .short_long_opts = false,
  .bold_as_special = false,
  .hover_title = true,
  .progress_bar = 0,
  .progress_scan = 1,
  .dim_margins = false,
  .status_line = false,
  .baud = 0,
  .bloom = 0,
  .options_font = W(""),
  .options_fontsize = 0,
  .old_options = "",
  .old_xbuttons = false,
  // Legacy
  .use_system_colours = false,
  .old_bold = false
};
config cfg, new_cfg, file_cfg;

typedef enum {
  OPT_COMMENT,OPT_BOOL, OPT_MOD, OPT_TRANS, OPT_CURSOR, OPT_FONTSMOOTH, OPT_FONTRENDER,
  OPT_MIDDLECLICK, OPT_RIGHTCLICK, OPT_SCROLLBAR, OPT_WINDOW, OPT_HOLD,
  OPT_INT, OPT_COLOUR, OPT_COLOUR_PAIR, OPT_STRING, OPT_WSTRING,
  OPT_CHARWIDTH, OPT_EMOJIS, OPT_EMOJI_PLACEMENT,
  OPT_COMPOSE_KEY,
  OPT_TYPE_MASK = 0x1F,
  OPT_LEGACY = 0x20,
  OPT_KEEPCR = 0x40,
  OPT_THEME  = 0x80,
} opt_type;

#define offcfg(option) offsetof(config, option)


typedef const struct {
  string name;
  ushort type;
  ushort offset;
  string comment;
}cfg_option;

static cfg_option options[]= {
  // Colour base options;
  // check_legacy_options() assumes these are the first three here:
  // Looks
  {"Looks",OPT_COMMENT,0,0},
  {"ForegroundColour"   , OPT_COLOUR, offcfg(fg_colour),__("ForegroundColour")},
  {"BackgroundColour"   , OPT_COLOUR, offcfg(bg_colour),__("BackgroundColour")},
  {"UseSystemColours"   , OPT_BOOL | OPT_LEGACY, offcfg(use_system_colours),__("UseSystemColours")},
  {"BoldColour"         , OPT_COLOUR, offcfg(bold_colour),__("BoldColour")},
  {"BlinkColour"        , OPT_COLOUR, offcfg(blink_colour),__("BlinkColour")},
  {"CursorColour"       , OPT_COLOUR, offcfg(cursor_colour),__("CursorColour")},
  {"TekForegroundColour", OPT_COLOUR, offcfg(tek_fg_colour),__("TekForegroundColour")},
  {"TekBackgroundColour", OPT_COLOUR, offcfg(tek_bg_colour),__("TekBackgroundColour")},
  {"TekCursorColour"    , OPT_COLOUR, offcfg(tek_cursor_colour),__("TekCursorColour")},
  {"TekWriteThruColour" , OPT_COLOUR, offcfg(tek_write_thru_colour),__("TekWriteThruColour")},
  {"TekDefocusedColour" , OPT_COLOUR, offcfg(tek_defocused_colour),__("TekDefocusedColour")},
  {"TekGlow"            , OPT_INT   , offcfg(tek_glow),__("TekGlow")},
  {"TekStrap"           , OPT_INT   , offcfg(tek_strap),__("TekStrap")},
  {"UnderlineColour"    , OPT_COLOUR, offcfg(underl_colour),__("UnderlineColour")},
  // ANSI colours
  {"Black"              , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[BLACK_I]),__("Black")},
  {"Red"                , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[RED_I]),__("Red")},
  {"Green"              , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[GREEN_I]),__("Green")},
  {"Yellow"             , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[YELLOW_I]),__("Yellow")},
  {"Blue"               , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[BLUE_I]),__("Blue")},
  {"Magenta"            , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[MAGENTA_I]),__("Magenta")},
  {"Cyan"               , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[CYAN_I]),__("Cyan")},
  {"White"              , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[WHITE_I]),__("White")},
  {"BoldBlack"          , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[BOLD_BLACK_I]),__("BoldBlack")},
  {"BoldRed"            , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[BOLD_RED_I]),__("BoldRed")},
  {"BoldGreen"          , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[BOLD_GREEN_I]),__("BoldGreen")},
  {"BoldYellow"         , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[BOLD_YELLOW_I]),__("BoldYellow")},
  {"BoldBlue"           , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[BOLD_BLUE_I]),__("BoldBlue")},
  {"BoldMagenta"        , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[BOLD_MAGENTA_I]),__("BoldMagenta")},
  {"BoldCyan"           , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[BOLD_CYAN_I]),__("BoldCyan")},
  {"BoldWhite"          , OPT_THEME|OPT_COLOUR_PAIR, offcfg(ansi_colours[BOLD_WHITE_I]),__("BoldWhite")},

  {"DispSpace"      , OPT_INT, offcfg(disp_space),__("DispSpace")},
  {"DispClear"      , OPT_INT, offcfg(disp_clear),__("DispClear")},
  {"DispTab"        , OPT_INT, offcfg(disp_tab),__("DispTab")},
  {"HoverColour"    , OPT_COLOUR, offcfg(hover_colour),__("HoverColour")},
  {"UnderlineManual", OPT_BOOL, offcfg(underl_manual),__("UnderlineManual")},
  {"HighlightBackgroundColour", OPT_COLOUR, offcfg(sel_bg_colour),__("HighlightBackgroundColour")},
  {"HighlightForegroundColour", OPT_COLOUR, offcfg(sel_fg_colour),__("HighlightForegroundColour")},
  {"SearchForegroundColour", OPT_COLOUR, offcfg(search_fg_colour),__("SearchForegroundColour")},
  {"SearchBackgroundColour", OPT_COLOUR, offcfg(search_bg_colour),__("SearchBackgroundColour")},
  {"SearchCurrentColour", OPT_COLOUR, offcfg(search_current_colour),__("SearchCurrentColour")},
  {"ThemeFile", OPT_WSTRING, offcfg(theme_file),__("ThemeFile")},
  {"Background", OPT_WSTRING, offcfg(background),__("Background")},
  {"ColourScheme", OPT_STRING, offcfg(colour_scheme),__("ColourScheme")},
  {"Transparency", OPT_TRANS, offcfg(transparency),__("Transparency")},
#ifdef support_blurred
  {"Blur", OPT_BOOL, offcfg(blurred),__("Blur")},
#endif
  {"OpaqueWhenFocused", OPT_BOOL, offcfg(opaque_when_focused),__("OpaqueWhenFocused")},
  {"CursorType", OPT_CURSOR, offcfg(cursor_type),__("CursorType")},
  {"CursorBlinks", OPT_BOOL, offcfg(cursor_blinks),__("CursorBlinks")},
  //need add 
  {"TabForegroundColour", OPT_COLOUR, offcfg(tab_fg_colour),__("TabForegroundColour")},
  {"TabBackgroundColour", OPT_COLOUR, offcfg(tab_bg_colour),__("TabBackgroundColour")},
  {"TabAttentionColour", OPT_COLOUR, offcfg(tab_attention_bg_colour),__("TabAttentionColour")},
  {"TabActiveColour", OPT_COLOUR, offcfg(tab_active_bg_colour),__("TabActiveColour")},
  {"tab_bar_show", OPT_BOOL, offcfg(tab_bar_show),__("tab_bar_show")},
  {"indicator", OPT_BOOL, offcfg(indicator),__("indicator")},
  {"indicatorx", OPT_BOOL, offcfg(indicatorx),__("indicatorx")},
  {"indicatory", OPT_BOOL, offcfg(indicatory),__("indicatory")},
  {"tab_font_size", OPT_BOOL, offcfg(tab_font_size),__("tab_font_size")},
  {"gui_font_size", OPT_INT, offcfg(gui_font_size),__("gui_font_size")},
  {"scale_options_width",OPT_INT, offcfg(scale_options_width),__("scale_options_width")},
  {"partline", OPT_INT, offcfg(partline),__("partline")},
  {"usepartline", OPT_INT, offcfg(usepartline),__("usepartline")},

  // Text
  {"Text",OPT_COMMENT,0,0},
  {"Font", OPT_WSTRING, offcfg(font.name),__("Font")},
  {"FontSize", OPT_INT | OPT_LEGACY, offcfg(font.size),__("FontSize")},
  {"FontHeight", OPT_INT, offcfg(font.size),__("FontHeight")},
  {"FontWeight", OPT_INT, offcfg(font.weight),__("FontWeight")},
  {"FontIsBold", OPT_BOOL, offcfg(font.isbold),__("FontIsBold")},

  {"FontChoice", OPT_WSTRING, offcfg(font_choice),__("FontChoice")},
  {"FontSample", OPT_WSTRING, offcfg(font_sample),__("FontSample")},
  {"ShowHiddenFonts", OPT_BOOL, offcfg(show_hidden_fonts),__("ShowHiddenFonts")},
  {"FontSmoothing", OPT_FONTSMOOTH, offcfg(font_smoothing),__("FontSmoothing")},
  {"BoldAsFont", OPT_BOOL, offcfg(bold_as_font),__("BoldAsFont")},
  {"BoldAsColour", OPT_BOOL, offcfg(bold_as_colour),__("BoldAsColour")},
  {"AllowBlinking", OPT_BOOL, offcfg(allow_blinking),__("AllowBlinking")},
  {"Locale", OPT_STRING, offcfg(locale),__("Locale")},
  {"Charset", OPT_STRING, offcfg(charset),__("Charset")},
  {"Charwidth", OPT_CHARWIDTH, offcfg(charwidth),__("Charwidth")},
  {"OldLocale", OPT_BOOL, offcfg(old_locale),__("OldLocale")},
  {"FontRender", OPT_FONTRENDER, offcfg(font_render),__("FontRender")},
  {"FontMenu", OPT_INT, offcfg(fontmenu),__("FontMenu")},
  {"OldFontMenu", OPT_INT | OPT_LEGACY, offcfg(fontmenu),__("OldFontMenu")},
  {"Font1", OPT_WSTRING, offcfg(fontfams[1].name),__("Font1")},
  {"Font1Weight", OPT_INT, offcfg(fontfams[1].weight),__("Font1Weight")},
  {"Font2", OPT_WSTRING, offcfg(fontfams[2].name),__("Font2")},
  {"Font2Weight", OPT_INT, offcfg(fontfams[2].weight),__("Font2Weight")},
  {"Font3", OPT_WSTRING, offcfg(fontfams[3].name),__("Font3")},
  {"Font3Weight", OPT_INT, offcfg(fontfams[3].weight),__("Font3Weight")},
  {"Font4", OPT_WSTRING, offcfg(fontfams[4].name),__("Font4")},
  {"Font4Weight", OPT_INT, offcfg(fontfams[4].weight),__("Font4Weight")},
  {"Font5", OPT_WSTRING, offcfg(fontfams[5].name),__("Font5")},
  {"Font5Weight", OPT_INT, offcfg(fontfams[5].weight),__("Font5Weight")},
  {"Font6", OPT_WSTRING, offcfg(fontfams[6].name),__("Font6")},
  {"Font6Weight", OPT_INT, offcfg(fontfams[6].weight),__("Font6Weight")},
  {"Font7", OPT_WSTRING, offcfg(fontfams[7].name),__("Font7")},
  {"Font7Weight", OPT_INT, offcfg(fontfams[7].weight),__("Font7Weight")},
  {"Font8", OPT_WSTRING, offcfg(fontfams[8].name),__("Font8")},
  {"Font8Weight", OPT_INT, offcfg(fontfams[8].weight),__("Font8Weight")},
  {"Font9", OPT_WSTRING, offcfg(fontfams[9].name),__("Font9")},
  {"Font9Weight", OPT_INT, offcfg(fontfams[9].weight),__("Font9Weight")},
  {"Font10", OPT_WSTRING, offcfg(fontfams[10].name),__("Font10")},
  {"Font10Weight", OPT_INT, offcfg(fontfams[10].weight),__("Font10Weight")},
  {"FontRTL", OPT_WSTRING, offcfg(fontfams[11].name),__("FontRTL")},
  {"FontRTLWeight", OPT_INT, offcfg(fontfams[11].weight),__("FontRTLWeight")},
  {"TekFont", OPT_WSTRING, offcfg(tek_font),__("TekFont")},

  // Keys
  {"Keys",OPT_COMMENT,0,0},
  {"BackspaceSendsBS", OPT_BOOL, offcfg(backspace_sends_bs),__("BackspaceSendsBS")},
  {"DeleteSendsDEL", OPT_BOOL, offcfg(delete_sends_del),__("DeleteSendsDEL")},
  {"CtrlAltIsAltGr", OPT_BOOL, offcfg(ctrl_alt_is_altgr),__("CtrlAltIsAltGr")},
  {"AltGrIsAlsoAlt", OPT_BOOL, offcfg(altgr_is_alt),__("AltGrIsAlsoAlt")},
  {"CtrlAltDelayAltGr", OPT_INT, offcfg(ctrl_alt_delay_altgr),__("CtrlAltDelayAltGr")},
  {"OldAltGrDetection", OPT_BOOL, offcfg(old_altgr_detection),__("OldAltGrDetection")},
  {"OldModifyKeys", OPT_INT, offcfg(old_modify_keys),__("OldModifyKeys")},
  {"FormatOtherKeys", OPT_INT, offcfg(format_other_keys),__("FormatOtherKeys")},
  {"AutoRepeat", OPT_BOOL, offcfg(auto_repeat),__("AutoRepeat")},
  {"SupportExternalHotkeys", OPT_INT, offcfg(external_hotkeys),__("SupportExternalHotkeys")},
  {"ClipShortcuts", OPT_BOOL, offcfg(clip_shortcuts),__("ClipShortcuts")},
  {"WindowShortcuts", OPT_BOOL, offcfg(window_shortcuts),__("WindowShortcuts")},
  {"SwitchShortcuts", OPT_BOOL, offcfg(switch_shortcuts),__("SwitchShortcuts")},
  {"ZoomShortcuts", OPT_BOOL, offcfg(zoom_shortcuts),__("ZoomShortcuts")},
  {"ZoomFontWithWindow", OPT_BOOL, offcfg(zoom_font_with_window),__("ZoomFontWithWindow")},
  {"AltFnShortcuts", OPT_BOOL, offcfg(alt_fn_shortcuts),__("AltFnShortcuts")},
  {"WinShortcuts", OPT_BOOL, offcfg(win_shortcuts),__("WinShortcuts")},
  {"hkwinkeyall", OPT_BOOL, offcfg(hkwinkeyall),__("hkwinkeyall")},
  {"hook_keyboard", OPT_BOOL, offcfg(hook_keyboard),__("hook_keyboard")},
  {"CtrlShiftShortcuts", OPT_BOOL, offcfg(ctrl_shift_shortcuts),__("CtrlShiftShortcuts")},
  {"CtrlExchangeShift", OPT_BOOL, offcfg(ctrl_exchange_shift),__("CtrlExchangeShift")},
  {"CtrlControls", OPT_BOOL, offcfg(ctrl_controls),__("CtrlControls")},
  {"ComposeKey", OPT_COMPOSE_KEY, offcfg(compose_key),__("ComposeKey")},
  {"Key_PrintScreen", OPT_STRING, offcfg(key_prtscreen),__("Key_PrintScreen")},
  {"Key_Pause", OPT_STRING, offcfg(key_pause),__("Key_Pause")},
  {"Key_Break", OPT_STRING, offcfg(key_break),__("Key_Break")},
  {"Key_Menu", OPT_STRING, offcfg(key_menu),__("Key_Menu")},
  {"Key_ScrollLock", OPT_STRING, offcfg(key_scrlock),__("Key_ScrollLock")},
  {"Break", OPT_STRING | OPT_LEGACY, offcfg(key_break),__("Break")},
  {"Pause", OPT_STRING | OPT_LEGACY, offcfg(key_pause),__("Pause")},
  {"KeyFunctions", OPT_WSTRING | OPT_KEEPCR, offcfg(key_commands),__("KeyFunctions")},
  {"ManageLEDs", OPT_INT, offcfg(manage_leds),__("ManageLEDs")},
  {"ShootFoot", OPT_BOOL, offcfg(enable_remap_ctrls),__("ShootFoot")},
  {"OldKeyFunctionsKeypad", OPT_BOOL, offcfg(old_keyfuncs_keypad),__("OldKeyFunctionsKeypad")},

  // Mouse
  {"Mouse",OPT_COMMENT,0,0},
  {"ClicksPlaceCursor", OPT_BOOL, offcfg(clicks_place_cursor),__("ClicksPlaceCursor")},
  {"MiddleClickAction", OPT_MIDDLECLICK, offcfg(middle_click_action),__("MiddleClickAction")},
  {"RightClickAction", OPT_RIGHTCLICK, offcfg(right_click_action),__("RightClickAction")},
  {"OpeningClicks", OPT_INT, offcfg(opening_clicks),__("OpeningClicks")},
  {"OpeningMod", OPT_MOD, offcfg(opening_mod),__("OpeningMod")},
  {"WrapTab", OPT_INT, offcfg(wrap_tab),__("WrapTab")},
  {"ZoomMouse", OPT_BOOL, offcfg(zoom_mouse),__("ZoomMouse")},
  {"ClicksTargetApp", OPT_BOOL, offcfg(clicks_target_app),__("ClicksTargetApp")},
  {"ClickTargetMod", OPT_MOD, offcfg(click_target_mod),__("ClickTargetMod")},
  {"HideMouse", OPT_BOOL, offcfg(hide_mouse),__("HideMouse")},
  {"ElasticMouse", OPT_BOOL, offcfg(elastic_mouse),__("ElasticMouse")},
  {"LinesPerMouseWheelNotch", OPT_INT, offcfg(lines_per_notch),__("LinesPerMouseWheelNotch")},
  {"MousePointer", OPT_WSTRING, offcfg(mouse_pointer),__("MousePointer")},
  {"AppMousePointer", OPT_WSTRING, offcfg(appmouse_pointer),__("AppMousePointer")},

  // Selection
  {"Selection",OPT_COMMENT,0,0},
  {"ClearSelectionOnInput", OPT_BOOL, offcfg(input_clears_selection),__("ClearSelectionOnInput")},
  {"CopyOnSelect", OPT_BOOL, offcfg(copy_on_select),__("CopyOnSelect")},
  {"CopyTab", OPT_BOOL, offcfg(copy_tabs),__("CopyTab")},
  {"CopyAsRTF", OPT_BOOL, offcfg(copy_as_rtf),__("CopyAsRTF")},
  {"CopyAsHTML", OPT_BOOL, offcfg(copy_as_html),__("CopyAsHTML")},
  {"CopyAsRTFFont", OPT_WSTRING, offcfg(copy_as_rtf_font),__("CopyAsRTFFont")},
  {"CopyAsRTFFontSize", OPT_INT | OPT_LEGACY, offcfg(copy_as_rtf_font_size),__("CopyAsRTFFontSize")},
  {"CopyAsRTFFontHeight", OPT_INT, offcfg(copy_as_rtf_font_size),__("CopyAsRTFFontHeight")},
  {"TrimSelection", OPT_BOOL, offcfg(trim_selection),__("TrimSelection")},
  {"AllowSetSelection", OPT_BOOL, offcfg(allow_set_selection),__("AllowSetSelection")},
  {"SelectionShowSize", OPT_INT, offcfg(selection_show_size),__("SelectionShowSize")},

  // Window
  {"Window",OPT_COMMENT,0,0},
  {"Columns", OPT_INT, offcfg(cols),__("Columns")},
  {"Rows", OPT_INT, offcfg(rows),__("Rows")},
  {"RewrapOnResize", OPT_BOOL, offcfg(rewrap_on_resize),__("RewrapOnResize")},
  {"ScrollbackLines", OPT_INT, offcfg(scrollback_lines),__("ScrollbackLines")},
  {"MaxScrollbackLines", OPT_INT, offcfg(max_scrollback_lines),__("MaxScrollbackLines")},
  {"Scrollbar", OPT_SCROLLBAR, offcfg(scrollbar),__("Scrollbar")},
  {"ScrollMod", OPT_MOD, offcfg(scroll_mod),__("ScrollMod")},
  {"PgUpDnScroll", OPT_BOOL, offcfg(pgupdn_scroll),__("PgUpDnScroll")},
  {"allocconsole", OPT_BOOL, offcfg(allocconsole),__("allocconsole")},
  {"Language", OPT_WSTRING, offcfg(lang),__("Language")},
  {"SearchBar", OPT_WSTRING, offcfg(search_bar),__("SearchBar")},
  {"SearchContext", OPT_INT, offcfg(search_context),__("SearchContext")},

  // Terminal
  {"Terminal",OPT_COMMENT,0,0},
  {"Term", OPT_STRING, offcfg(Term),__("Term")},
  {"Answerback", OPT_STRING, offcfg(answerback),__("Answerback")},
  {"OldWrapModes", OPT_BOOL, offcfg(old_wrapmodes),__("OldWrapModes")},
  {"Enable132ColumnSwitching", OPT_BOOL, offcfg(enable_deccolm_init),__("Enable132ColumnSwitching")},
  {"BellType", OPT_INT, offcfg(bell_type),__("BellType")},
  {"BellFile", OPT_WSTRING, offcfg(bell_file[6]),__("BellFile")},
  {"BellFile2", OPT_WSTRING, offcfg(bell_file[0]),__("BellFile2")},
  {"BellFile3", OPT_WSTRING, offcfg(bell_file[1]),__("BellFile3")},
  {"BellFile4", OPT_WSTRING, offcfg(bell_file[2]),__("BellFile4")},
  {"BellFile5", OPT_WSTRING, offcfg(bell_file[3]),__("BellFile5")},
  {"BellFile6", OPT_WSTRING, offcfg(bell_file[4]),__("BellFile6")},
  {"BellFile7", OPT_WSTRING, offcfg(bell_file[5]),__("BellFile7")},
  {"BellFreq", OPT_INT, offcfg(bell_freq),__("BellFreq")},
  {"BellLen", OPT_INT, offcfg(bell_len),__("BellLen")},
  {"BellFlash", OPT_BOOL, offcfg(bell_flash),__("BellFlash")},
  {"BellFlashStyle", OPT_INT, offcfg(bell_flash_style),__("BellFlashStyle")},
  {"BellTaskbar", OPT_BOOL, offcfg(bell_taskbar),__("BellTaskbar")},
  {"BellPopup", OPT_BOOL, offcfg(bell_popup),__("BellPopup")},
  {"BellInterval", OPT_INT, offcfg(bell_interval),__("BellInterval")},
  {"PlayTone", OPT_INT, offcfg(play_tone),__("PlayTone")},
  {"Printer", OPT_WSTRING, offcfg(printer),__("Printer")},
  {"ConfirmExit", OPT_BOOL, offcfg(confirm_exit),__("ConfirmExit")},
  {"ConfirmReset", OPT_BOOL, offcfg(confirm_reset),__("ConfirmReset")},

  // Command line
  {"Command line",OPT_COMMENT,0,0},
  {"Class", OPT_WSTRING, offcfg(class),__("Class")},
  {"Hold", OPT_HOLD, offcfg(hold),__("Hold")},
  {"Daemonize", OPT_BOOL, offcfg(daemonize),__("Daemonize")},
  {"DaemonizeAlways", OPT_BOOL, offcfg(daemonize_always),__("DaemonizeAlways")},
  {"ExitWrite", OPT_BOOL, offcfg(exit_write),__("ExitWrite")},
  {"ExitTitle", OPT_WSTRING, offcfg(exit_title),__("ExitTitle")},
  {"Icon", OPT_WSTRING, offcfg(icon),__("Icon")},
  {"Log", OPT_WSTRING, offcfg(log),__("Log")},
  {"Logging", OPT_INT, offcfg(logging),__("Logging")},
  {"Title", OPT_WSTRING, offcfg(title),__("Title")},
  {"title_settable", OPT_BOOL, offcfg(title_settable),__("title_settable")},
  {"Utmp", OPT_BOOL, offcfg(create_utmp),__("Utmp")},
  {"Window", OPT_WINDOW, offcfg(window),__("Window")},
  {"X", OPT_INT, offcfg(x),__("X")},
  {"Y", OPT_INT, offcfg(y),__("Y")},

  // "Hidden"
  {"Hidden",OPT_COMMENT,0,0},
  {"Bidi", OPT_INT, offcfg(bidi),__("Bidi")},
  {"NoAltScreen", OPT_BOOL, offcfg(disable_alternate_screen),__("NoAltScreen")},
  {"EraseToScrollback", OPT_BOOL, offcfg(erase_to_scrollback),__("EraseToScrollback")},
  {"DisplaySpeedup", OPT_INT, offcfg(display_speedup),__("DisplaySpeedup")},
  {"SuppressSGR", OPT_STRING, offcfg(suppress_sgr),__("SuppressSGR")},
  {"SuppressDEC", OPT_STRING, offcfg(suppress_dec),__("SuppressDEC")},
  {"SuppressWIN", OPT_STRING, offcfg(suppress_win),__("SuppressWIN")},
  {"SuppressOSC", OPT_STRING, offcfg(suppress_osc),__("SuppressOSC")},
  {"SuppressNRC", OPT_STRING, offcfg(suppress_nrc),__("SuppressNRC")},  // unused
  {"SuppressMouseWheel", OPT_STRING, offcfg(suppress_wheel),__("SuppressMouseWheel")},
  {"FilterPasteControls", OPT_STRING, offcfg(filter_paste),__("FilterPasteControls")},
  {"BracketedPasteByLine", OPT_INT, offcfg(bracketed_paste_split),__("BracketedPasteByLine")},
  {"SuspendWhileSelecting", OPT_INT, offcfg(suspbuf_max),__("SuspendWhileSelecting")},
  {"PrintableControls", OPT_INT, offcfg(printable_controls),__("PrintableControls")},
  {"CharNarrowing", OPT_INT, offcfg(char_narrowing),__("CharNarrowing")},
  {"Emojis", OPT_EMOJIS, offcfg(emojis),__("Emojis")},
  {"EmojiPlacement", OPT_EMOJI_PLACEMENT, offcfg(emoji_placement),__("EmojiPlacement")},
  {"SaveFilename", OPT_WSTRING, offcfg(save_filename),__("SaveFilename")},
  {"AppID", OPT_WSTRING, offcfg(app_id),__("AppID")},
  {"AppName", OPT_WSTRING, offcfg(app_name),__("AppName")},
  {"AppLaunchCmd", OPT_WSTRING, offcfg(app_launch_cmd),__("AppLaunchCmd")},

  {"DropCommands", OPT_WSTRING | OPT_KEEPCR, offcfg(drop_commands),__("DropCommands")},
  {"ExitCommands", OPT_WSTRING | OPT_KEEPCR, offcfg(exit_commands),__("ExitCommands")},
  {"ExitCommands", OPT_WSTRING | OPT_KEEPCR, offcfg(exit_commands),__("ExitCommands")},
  {"UserCommands", OPT_WSTRING | OPT_KEEPCR, offcfg(user_commands),__("UserCommands")},
  {"CtxMenuFunctions", OPT_WSTRING | OPT_KEEPCR, offcfg(ctx_user_commands),__("CtxMenuFunctions")},
  {"SysMenuFunctions", OPT_WSTRING | OPT_KEEPCR, offcfg(sys_user_commands),__("SysMenuFunctions")},
  {"UserCommandsPath", OPT_WSTRING, offcfg(user_commands_path),__("UserCommandsPath")},
  {"SessionCommands", OPT_WSTRING | OPT_KEEPCR, offcfg(session_commands),__("SessionCommands")},
  {"TaskCommands", OPT_WSTRING | OPT_KEEPCR, offcfg(task_commands),__("TaskCommands")},
  {"ConPTY", OPT_BOOL, offcfg(conpty_support),__("ConPTY")},
  {"LoginFromShortcut", OPT_BOOL, offcfg(login_from_shortcut),__("LoginFromShortcut")},
  {"MenuMouse", OPT_STRING, offcfg(menu_mouse),__("MenuMouse")},
  {"MenuCtrlMouse", OPT_STRING, offcfg(menu_ctrlmouse),__("MenuCtrlMouse")},
  {"MenuMouse5", OPT_STRING, offcfg(menu_altmouse),__("MenuMouse5")},
  {"MenuMenu", OPT_STRING, offcfg(menu_menu),__("MenuMenu")},
  {"MenuCtrlMenu", OPT_STRING, offcfg(menu_ctrlmenu),__("MenuCtrlMenu")},
  {"MenuTitleCtrlLeft", OPT_STRING, offcfg(menu_title_ctrl_l),__("MenuTitleCtrlLeft")},
  {"MenuTitleCtrlRight", OPT_STRING, offcfg(menu_title_ctrl_r),__("MenuTitleCtrlRight")},

  {"SessionGeomSync", OPT_INT, offcfg(geom_sync),__("SessionGeomSync")},


  {"ColSpacing", OPT_INT, offcfg(col_spacing),__("ColSpacing")},
  {"RowSpacing", OPT_INT, offcfg(row_spacing),__("RowSpacing")},
  {"AutoLeading", OPT_INT, offcfg(auto_leading),__("AutoLeading")},
  {"Padding", OPT_INT, offcfg(padding),__("Padding")},
  {"Ligatures", OPT_INT, offcfg(ligatures),__("Ligatures")},
  {"LigaturesSupport", OPT_INT, offcfg(ligatures_support),__("LigaturesSupport")},
  {"HandleDPI", OPT_INT, offcfg(handle_dpichanged),__("HandleDPI")},
  {"CheckVersionUpdate", OPT_INT, offcfg(check_version_update),__("CheckVersionUpdate")},
  {"WordChars", OPT_STRING, offcfg(word_chars),__("WordChars")},
  {"WordCharsExcl", OPT_STRING, offcfg(word_chars_excl),__("WordCharsExcl")},
  {"IMECursorColour", OPT_COLOUR, offcfg(ime_cursor_colour),__("IMECursorColour")},
  {"SixelClipChars", OPT_WSTRING, offcfg(sixel_clip_char),__("SixelClipChars")},
  {"OldBold", OPT_BOOL, offcfg(old_bold),__("OldBold")},
  {"ShortLongOpts", OPT_BOOL, offcfg(short_long_opts),__("ShortLongOpts")},
  {"BoldAsRainbowSparkles", OPT_BOOL, offcfg(bold_as_special),__("BoldAsRainbowSparkles")},
  {"HoverTitle", OPT_BOOL, offcfg(hover_title),__("HoverTitle")},
  {"ProgressBar", OPT_BOOL, offcfg(progress_bar),__("ProgressBar")},
  {"ProgressScan", OPT_INT, offcfg(progress_scan),__("ProgressScan")},
  {"Baud", OPT_INT, offcfg(baud),__("Baud")},
  {"Bloom", OPT_INT, offcfg(bloom),__("Bloom")},
  {"DimMargins", OPT_BOOL, offcfg(dim_margins),__("DimMargins")},
  {"StatusLine", OPT_BOOL, offcfg(status_line),__("StatusLine")},
  {"OldXButtons", OPT_BOOL, offcfg(old_xbuttons),__("OldXButtons")},
  {"OptionsFont", OPT_WSTRING, offcfg(options_font),__("OptionsFont")},
  {"OptionsFontSize", OPT_INT | OPT_LEGACY, offcfg(options_fontsize),__("OptionsFontSize")},
  {"OptionsFontHeight", OPT_INT, offcfg(options_fontsize),__("OptionsFontHeight")},
  {"OldOptions", OPT_STRING, offcfg(old_options),__("OldOptions")},


  // Legacy
  {"BoldAsBright", OPT_BOOL | OPT_LEGACY, offcfg(bold_as_colour),__("BoldAsBright")},
  {"FontQuality", OPT_FONTSMOOTH | OPT_LEGACY, offcfg(font_smoothing),__("FontQuality")},
  {0}
};

typedef const struct {
  string name;
  char val;
} opt_val;

static opt_val * const opt_vals[] = {
  [OPT_BOOL] = (opt_val[]) {
    //__ Setting false for Boolean options (localization optional)
    {__("no"), false},
    //__ Setting true for Boolean options (localization optional)
    {__("yes"), true},
    //__ Setting false for Boolean options (localization optional)
    {__("false"), false},
    //__ Setting true for Boolean options (localization optional)
    {__("true"), true},
    //__ Setting false for Boolean options (localization optional)
    {__("off"), false},
    //__ Setting true for Boolean options (localization optional)
    {__("on"), true},
    {0, 0}
  },
  [OPT_CHARWIDTH] = (opt_val[]) {
    {__("locale"), 0},
    {__("unicode"), 1},
    {__("ambig-wide"), 2},
    {__("ambig-narrow"), 3},
    {__("single"), 10},
    {__("single-unicode"), 11},
    {0, 0}
  },
  [OPT_EMOJIS] = (opt_val[]) {
    {__("none"), EMOJIS_NONE},
    {__("openmoji"), EMOJIS_OPENMOJI},
    {__("noto"), EMOJIS_NOTO},
    {__("joypixels"), EMOJIS_JOYPIXELS},
    {__("emojione"), EMOJIS_ONE},
    {__("apple"), EMOJIS_APPLE},
    {__("google"), EMOJIS_GOOGLE},
    {__("twitter"), EMOJIS_TWITTER},
    {__("facebook"), EMOJIS_FB},
    {__("samsung"), EMOJIS_SAMSUNG},
    {__("windows"), EMOJIS_WINDOWS},
    {__("zoom"), EMOJIS_ZOOM},
    {0, 0}
  },
  [OPT_EMOJI_PLACEMENT] = (opt_val[]) {
    //__ Options - Text - Emojis - Placement (localization optional)
    {__("stretch"), EMPL_STRETCH},
    //__ Options - Text - Emojis - Placement (localization optional)
    {__("align"), EMPL_ALIGN},
    //__ Options - Text - Emojis - Placement (localization optional)
    {__("middle"), EMPL_MIDDLE},
    //__ Options - Text - Emojis - Placement (localization optional)
    {__("full"), EMPL_FULL},
    {0, 0}
  },
  [OPT_MOD] = (opt_val[]) {
    {("off"), 0},
    {("shift"), MDK_SHIFT},
    {("alt"), MDK_ALT},
    {("ctrl"), MDK_CTRL},
    {("win"), MDK_WIN},
    {("super"), MDK_SUPER},
    {("hyper"), MDK_HYPER},
    {("capslock"), MDK_CAPSLOCK},
    {0, 0}
  },
  [OPT_COMPOSE_KEY] = (opt_val[]) {
    {("off"), 0},
    {("shift"), MDK_SHIFT},
    {("alt"), MDK_ALT},
    {("ctrl"), MDK_CTRL},
    {("super"), MDK_SUPER},
    {("hyper"), MDK_HYPER},
    {0, 0}
  },
  [OPT_TRANS] = (opt_val[]) {
    {__("off"), TR_OFF},
    {__("low"), TR_LOW},
    {__("medium"), TR_MEDIUM},
    {__("high"), TR_HIGH},
    {__("glass"), TR_GLASS},
    {0, 0}
  },
  [OPT_CURSOR] = (opt_val[]) {
    {__("line"), CUR_LINE},
    {__("block"), CUR_BLOCK},
    {__("box"), CUR_BOX},
    {__("underscore"), CUR_UNDERSCORE},
    {0, 0}
  },
  [OPT_FONTSMOOTH] = (opt_val[]) {
    {__("Default"), FS_DEFAULT},
    {__("None"), FS_NONE},
    {__("Partial"), FS_PARTIAL},
    {__("Full"), FS_FULL},
    {0, 0}
  },
  [OPT_FONTRENDER] = (opt_val[]) {
    {__("textout"), FR_TEXTOUT},
    {__("uniscribe"), FR_UNISCRIBE},
    {0, 0}
  },
  [OPT_MIDDLECLICK] = (opt_val[]) {
    {__("enter"), MC_ENTER},
    {__("paste"), MC_PASTE},
    {__("extend"), MC_EXTEND},
    {__("void"), MC_VOID},
    {0, 0}
  },
  [OPT_RIGHTCLICK] = (opt_val[]) {
    {__("enter"), RC_ENTER},
    {__("paste"), RC_PASTE},
    {__("extend"), RC_EXTEND},
    {__("menu"), RC_MENU},
    {0, 0}
  },
  [OPT_SCROLLBAR] = (opt_val[]) {
    {__("left"), -1},
    {__("right"), 1},
    {__("none"), 0},
    {0, 0}
  },
  [OPT_WINDOW] = (opt_val[]){
    {__("hide"), 0},   // SW_HIDE
    {__("normal"), 1}, // SW_SHOWNORMAL
    {__("min"), 2},    // SW_SHOWMINIMIZED
    {__("max"), 3},    // SW_SHOWMAXIMIZED
    {__("FULL"), -1},
    {0, 0}
  },
  [OPT_HOLD] = (opt_val[]) {
    {__("never"), HOLD_NEVER},
    {__("start"), HOLD_START},
    {__("error"), HOLD_ERROR},
    {__("always"), HOLD_ALWAYS},
    {0, 0}
  }
};


#ifdef debug_theme
#define trace_theme(params)	printf params
#else
#define trace_theme(params)
#endif


char *
save_filename(const char * suf)
{
  char * pat = cs__wcstombs(cfg.save_filename);

  // expand initial ~ or $variable
  char * sep;
  if (*pat == '~' && pat[1] == '/') {
    char * pat1 = asform("%s%s", wv.home, pat + 1);
    delete(pat);
    pat = pat1;
  }
  else if (*pat == '$' && (sep = strchr(pat, '/'))) {
    *sep = 0;
    if (getenv(pat + 1)) {
      char * pat1 = asform("%s/%s", getenv(pat + 1), sep + 1);
      delete(pat);
      pat = pat1;
    }
  }
  wchar * wpat = cs__mbstowcs(pat);
  delete(pat);
  pat = path_win_w_to_posix(wpat);
  delete(wpat);
  //printf("save_filename pat %ls -> %s\n", cfg.save_filename, pat);

  struct timeval now;
  gettimeofday(& now, 0);
  char * fn = newn(char, MAX_PATH + 1 + strlen(suf));
  strftime(fn, MAX_PATH, pat, localtime(& now.tv_sec));
  //printf("save_filename [%s] (%s) -> %s%s\n", pat, suf, fn, suf);
  delete(pat);
  strcat(fn, suf);

  // make sure directory exists
  char * basesep = strrchr(fn, '/');
  if (basesep) {
    *basesep = 0;
    if (access(fn, X_OK | W_OK) != 0) {
      mkdir(fn, 0755);
    }
    *basesep = '/';
  }

  return fn;
}


#define dont_debug_opterror

static void
opterror(string msg, bool utf8params, string p1, string p2)
{
  print_opterror(stderr, msg, utf8params, p1, p2);
}
#define USEHASHOPT
#ifdef USEHASHOPT
typedef struct {
  int n,m;
  int*t;
}s_co_hashtab;
static s_co_hashtab  hopts[HASHTS] = {0};
static int initedhopts=0;
static void inithopt()
{
  string nm;
  for (uint i = 0; (nm=options[i].name); i++) {
    s_co_hashtab *pt=&hopts[HASHS(nm)];
    if (pt->n>= pt->m) {
      pt->m+= 20;
      pt->t= renewn(pt->t, pt->m);
    }
    pt->t[pt->n]=i;
    pt->n++;
  }
}
static int
find_option(bool from_file, string name){
  if(initedhopts==0)inithopt();
  s_co_hashtab *pt=&hopts[HASHS(name)];
  if(pt){
    for (uint i = 0; pt->n; i++) {
      if (!strcasecmp(name, options[pt->t[i]].name))
        return pt->t[i];
    }
  }
  //__ %s: unknown option name
  opterror(_("Ignoring unknown option '%s'"), from_file, name, 0);
  return -1;
}
#else
static int
find_option(bool from_file, string name)
{
  for (uint i = 0; options[i].name; i++) {
    if (!strcasecmp(name, options[i].name))
      return i;
  }
  //__ %s: unknown option name
  opterror(_("Ignoring unknown option '%s'"), from_file, name, 0);
  return -1;
}
#endif
typedef struct {
  char * comment;
  uchar opti;
} cfg_file_opt;
static cfg_file_opt *file_opts=NULL;
static uchar *arg_opts=NULL;
static uint file_opts_num = 0,maxfileopt=0;
static uint arg_opts_num=0,maxargopt=0;

static void
clear_opts(void)
{
  for (uint n = 0; n < file_opts_num; n++)
    if (file_opts[n].comment)
      delete(file_opts[n].comment);
  file_opts_num = 0;
  arg_opts_num = 0;
}

static bool
seen_file_option(uint i)
{
//  return memchr(file_opts, i, file_opts_num);
  for (uint n = 0; n < file_opts_num; n++)
    if (!file_opts[n].comment && file_opts[n].opti == i)
      return true;
  return false;
}

static bool
seen_arg_option(uint i)
{
  if(!arg_opts)return 0;
  return memchr(arg_opts, i, arg_opts_num);
}

static void
remember_file_option(const char * tag, uint i)
{
  (void)tag;
  trace_theme(("[%s] remember_file_option (file %d arg %d) %d %s\n", tag, seen_file_option(i), seen_arg_option(i), i, options[i].name));

  if (!seen_file_option(i)) {
    if(file_opts_num>=maxfileopt){
      maxfileopt+=16;
      file_opts=renewn(file_opts,maxfileopt);
    }
    file_opts[file_opts_num].comment = null;
    file_opts[file_opts_num].opti = i;
    file_opts_num++;
  }
}

static void
remember_file_comment(const char * comment)
{
  trace_theme(("[] remember_file_comment <%s>\n", comment));
  if(file_opts_num>=maxfileopt){
    maxfileopt+=16;
    file_opts=renewn(file_opts,maxfileopt);
  }

  file_opts[file_opts_num].comment = strdup(comment);
  file_opts[file_opts_num].opti = 0;
  file_opts_num++;
}

static void
remember_arg_option(const char * tag, uint i)
{
  (void)tag;
  trace_theme(("[%s] remember_arg_option (file %d arg %d) %d %s\n", tag, seen_file_option(i), seen_arg_option(i), i, options[i].name));

  if (!seen_arg_option(i)){
    if(arg_opts_num>=maxargopt){
      maxargopt+=16;
      arg_opts=renewn(arg_opts,maxargopt);
    }
    arg_opts[arg_opts_num++] = i;
  }
}

static void
check_legacy_options(void (*remember_option)(const char * tag, uint))
{
  if (cfg.use_system_colours) {
    // Translate 'UseSystemColours' to colour settings.
    cfg.fg_colour = cfg.cursor_colour = win_get_sys_colour(COLOR_WINDOWTEXT);
    cfg.bg_colour = win_get_sys_colour(COLOR_WINDOW);
    cfg.use_system_colours = false;

    // Make sure they're written to the config file.
    // This assumes that the colour options are the first three in options[].
    remember_option("legacy", 0);
    remember_option("legacy", 1);
    remember_option("legacy", 2);
  }
}

static struct {
  uchar r, g, b;
  char * name;
} xcolours[] = {
#include "rgb.t"
};

bool
parse_colour(string s, colour *cp)
{
  uint r, g, b ,ok=0;
  float c, m, y, k = 0;
  switch(s[0]){
    when '#':
        if (sscanf(s, "#%2x%2x%2x", &r, &g, &b) == 3) ok=1;
        else if (sscanf(s, "#%4x%4x%4x", &r, &g, &b) == 3) {
          r >>= 8;
          g >>= 8;
          b >>= 8;
          ok=2;
        }
    when 'r':
        if (sscanf(s, "rgb:%2x/%2x/%2x", &r, &g, &b) == 3) ok=3;
        else if (sscanf(s, "rgb:%4x/%4x/%4x", &r, &g, &b) == 3)
          r >>= 8, g >>= 8, b >>= 8,ok=4;
    when 'c':
        if (sscanf(s, "cmy:%f/%f/%f", &c, &m, &y) == 3
          || sscanf(s, "cmyk:%f/%f/%f/%f", &c, &m, &y, &k) == 4
          ){
          if (c >= 0 && c <= 1 && m >= 0 && m <= 1 && y >= 0 && y <= 1 && k >= 0 && k <= 1) {
            r = (1 - c) * (1 - k) * 255;
            g = (1 - m) * (1 - k) * 255;
            b = (1 - y) * (1 - k) * 255;
            ok=5;
          }
        }
    when '0' ... '9':
        if (sscanf(s, "%u,%u,%u", &r, &g, &b) == 3) ok=6;
  }
  if(!ok){
    int coli = -1;
    int len = strlen(s);
    while (len && s[len - 1] == ' ') len--;
    for (uint i = 0; i < lengthof(xcolours); i++)
      if (0 == strncasecmp(s, xcolours[i].name, len)) {
        r = xcolours[i].r;
        g = xcolours[i].g;
        b = xcolours[i].b;
        coli = i;
        break;
      }
    if (coli < 0)
      return false;
    ok=7;
  }

  *cp = make_colour(r, g, b);
  return true;
}

static int
set_option(config * p,string name, string val_str, bool from_file)
{
  int i = find_option(from_file, name);
  if (i < 0)
    return i;

  void *val_p = (void *)p + options[i].offset;
  if (!(options[i].type & OPT_KEEPCR))
    ((char *)val_str)[strcspn(val_str, "\r")] = 0;
  uint type = options[i].type & OPT_TYPE_MASK;

  switch (type) {
    when OPT_STRING:
      strset(val_p, val_str);
      return i;
    when OPT_WSTRING: {
      wchar * ws;
      if (from_file)
        ws = cs__utforansitowcs(val_str);
      else
        ws = cs__mbstowcs(val_str);
      wstrset(val_p, ws);
      delete(ws);
      return i;
    }
    when OPT_COMMENT: 
    when OPT_INT: {
      char *val_end;
      int val = strtol(val_str, &val_end, 0);
      if (val_end != val_str) {
        *(int *)val_p = val;
        return i;
      }
    }
    when OPT_COLOUR:
#ifdef debug_theme
      printf("set_option <%s> <%s>\n", name, val_str);
#endif
      if (parse_colour(val_str, val_p))
        return i;
    when OPT_COLOUR_PAIR: {
#ifdef debug_theme
      printf("set_option <%s> <%s>\n", name, val_str);
#endif
      colour_pair *pair = val_p;
      if (parse_colour(val_str, &pair->fg)) {
        const char *sep = strchr(val_str, ';');
        if (!sep) {
          pair->bg = pair->fg;
          return i;
        }
        else if (parse_colour(sep + 1, &pair->bg))
          return i;
      }
    }
    otherwise: {
      int len = strlen(val_str);
      if (!len)
        break;
      for (opt_val *o = opt_vals[type]; o->name; o++) {
        if (!strncasecmp(val_str, o->name, len)) {
          *(char *)val_p = o->val;
          return i;
        }
      }
      // Value not found: try interpreting it as a number.
      char *val_end;
      int val = strtol(val_str, &val_end, 0);
      if (val_end != val_str) {
        *(char *)val_p = val;
        return i;
      }
    }
  }
  //__ %2$s: option name, %1$s: invalid value
  if (!wv.wnd)  // report errors only during initialisation
    opterror(_("Ignoring invalid value '%s' for option '%s'"), 
             from_file, val_str, name);
  return -1;
}

static int
parse_option(config * p,string option, bool from_file)
{
  const char *eq = strchr(option, '=');
  if (!eq) {
    ((char *)option)[strcspn(option, "\r")] = 0;
    //__ %s: option name
    opterror(_("Ignoring option '%s' with missing value"), 
             from_file, option, 0);
    return -1;
  }

  const char *name_end = eq;
  while (isspace((uchar)name_end[-1]))
    name_end--;

  uint name_len = name_end - option;
  char name[name_len + 1];
  memcpy(name, option, name_len);
  name[name_len] = 0;

  const char *val = eq + 1;
  while (isspace((uchar)*val))
    val++;

  return set_option(p,name, val, from_file);
}

static void
check_arg_option(int i)
{
  if (i >= 0) {
    remember_arg_option("chk_arg", i);
    check_legacy_options(remember_arg_option);
  }
}

void
set_arg_option(string name, string val)
{
  check_arg_option(set_option(&cfg,name, val, false));
}

void
parse_arg_option(string option)
{
  check_arg_option(parse_option(&cfg,option, false));
}


/*
   In a configuration parameter list, map tag to value.
 */
char *
matchconf(char * conf,const char * item)
{
  char * cmdp = conf;
  char sepch = ';';
  if ((uchar)*cmdp <= (uchar)' ')
    sepch = *cmdp++;

  char * paramp;
  while ((paramp = strchr(cmdp, ':'))) {
    *paramp = '\0';
    paramp++;
    char * sepp = strchr(paramp, sepch);
    if (sepp)
      *sepp = '\0';

    if (!strcmp(cmdp, item))
      return paramp;

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
  return 0;
}


static string * config_dirs = 0;
static int last_config_dir = -1;

static void
init_config_dirs(void)
{
  if (config_dirs)
    return;

  int ncd = 3;
  char * appdata = getenv("APPDATA");
  if (appdata)
    ncd++;
  if (config_dir)
    ncd++;
  config_dirs = newn(string, ncd);

  // /usr/share/mintty , $APPDATA/mintty , ~/.config/mintty , ~/.mintty
  config_dirs[++last_config_dir] = "/usr/share/mintty";
  if (appdata) {
    appdata = newn(char, strlen(appdata) + 8);
    sprintf(appdata, "%s/mintty", getenv("APPDATA"));
    config_dirs[++last_config_dir] = appdata;
  }
  if (!wv.support_wsl) {
    char * xdgconf = newn(char, strlen(wv.home) + 16);
    sprintf(xdgconf, "%s/.config/mintty", wv.home);
    config_dirs[++last_config_dir] = xdgconf;
    char * homeconf = newn(char, strlen(wv.home) + 9);
    sprintf(homeconf, "%s/.mintty", wv.home);
    config_dirs[++last_config_dir] = homeconf;
  }
  if (config_dir) {
    config_dirs[++last_config_dir] = config_dir;
  }
}

char *
get_resource_file(wstring sub, wstring res, bool towrite)
{
  init_config_dirs();
  int fd;
  for (int i = last_config_dir; i >= 0; i--) {
    char * resfn = asform(("%s/%ls/%ls"), config_dirs[i], sub,res);
    fd = open(resfn, towrite ? O_CREAT |  O_WRONLY | O_BINARY : O_RDONLY | O_BINARY, 0644);
#if CYGWIN_VERSION_API_MINOR >= 74
    if (towrite && fd < 0 && errno == ENOENT) {
      // try to create resource subdirectories
      int dd = open(config_dirs[i], O_RDONLY | O_DIRECTORY);
      if (dd) {
        mkdirat(dd, "themes", 0755);
        mkdirat(dd, "sounds", 0755);
        mkdirat(dd, "lang", 0755);
        close(dd);
      }
      // retry
      fd = open(resfn, O_CREAT | O_WRONLY | O_BINARY, 0644);
    }
#endif
    if (fd >= 0) {
      close(fd);
      return resfn;
    }
    delete(resfn);
    if (errno == EACCES || errno == EEXIST)
      break;
  }
  return null;
}


#define dont_debug_messages 1

typedef struct {
  string msg;
  string locmsg;
  wstring wmsg;
}s_message;
static void setmsg(s_message*pm,string msg, string locmsg){
  pm->msg = msg;
  pm->locmsg = locmsg;
  pm->wmsg = null;
}
static void freemsg(s_message *pm){
    VFREE((void*)pm->msg);
    VFREE((void*)pm->locmsg);
    if  (pm->wmsg) VFREE((void*)pm->wmsg);
}
#define usehashtab
#ifdef usehashtab
typedef struct {
  int n,m;
  s_message *t;
}s_hashtab;
static s_hashtab  hmsgt[HASHTS] = {0};
static s_message*findmsg(string msg){
  s_hashtab *pt=&hmsgt[HASHS(msg)];
  for (int i = 0; i < pt->n; i++) {
    if (strcmp(msg, pt->t[i].msg) == 0) { return &pt->t[i]; }
  }
  return NULL;
}
static void clear_messages()
{
  for (int j = 0; j < HASHTS; j++) { 
    s_hashtab *pt=&hmsgt[j];
    for (int i = 0; i < pt->n; i++) { freemsg(pt->t+i); }
    VFREE(pt->t);
    pt->t=0;pt->n=0;pt->m=0;
  }
}
static void add_message(string msg, string locmsg)
{
  s_hashtab *pt=&hmsgt[HASHS(msg)];
  if (pt->n>= pt->m) {
    pt->m+= 20;
    pt->t= renewn(pt->t, pt->m);
  }
  setmsg(& pt->t[pt->n], msg, locmsg);
  pt->n++;
}
#else
static s_message * messages = {0};
int nmessages = 0;
int maxmessages = 0;
static s_message*findmsg(string msg){
  for (int i = 0; i < nmessages; i++) {
    if (strcmp(msg, messages[i].msg) == 0) { return &messages[i]; }
  }
  return NULL;
}
static void clear_messages()
{
  for (int i = 0; i < nmessages; i++) { freemsg(messages+i); }
  nmessages = 0;
}
static void add_message(string msg, string locmsg)
{
  if (nmessages >= maxmessages) {
    if (maxmessages) maxmessages += 20;
    else maxmessages = 180;
    messages = renewn(messages, maxmessages);
  }
#if defined(debug_messages) && debug_messages > 3
  printf("add %d <%s> <%s>\n", nmessages, msg, locmsg);
#endif
  setmsg(& messages[nmessages], msg, locmsg);
  nmessages ++;
}
#endif

const char*loctext(string msg)
{
  s_message *pm=findmsg(msg);
  if(pm)return (char*)pm->locmsg;
  return  (char*)msg;
}

const wchar *wloctext(string msg)
{
  s_message *pm=findmsg(msg);
  if(pm){
    if (pm->wmsg == null)
      pm->wmsg = cs__utftowcs(pm->locmsg);
    return (wchar*)pm->wmsg;
  }
  add_message(strdup(msg), strdup(msg));
  return wloctext(msg);
}

static char *
readtext(char * buf, int len, FILE * file)
{
  char * unescape(char * s)
  {
    char * t = s;
    while (*s && *s != '"') {
      if (*s == '\\') {
        s++;
        switch (*s) {
          when 't': *t = '\t';
          when 'n': *t = '\n';
          otherwise: *t = *s;
        }
      }
      else
        *t = *s;
      t++;
      s++;
    }
    *t = '\0';
    return t;
  }

  char * p = buf;
  while (*p != ' ')
    p++;
  while (*p == ' ')
    p++;
  if (strncmp(p, "\"\"", 2) == 0) {
    // scan multi-line text
    char * str = newn(char, 1);
    *str = '\0';
    while (fgets(buf, len, file) && *buf == '"') {
      p = buf + 1;
      char * f = unescape(p);
      if (!*f) {
        str = renewn(str, strlen(str) + strlen(p) + 1);
        strcat(str, p);
      }
      else {
        delete(str);
        return null;
      }
    }
    return str;
  }
  else {
    // scan single-line text
    p++;
    char * f = unescape(p);
    if (!*f) {
      char * str = strdup(p);
      fgets(buf, len, file);
      return str;
    }
  }
  return null;
}

static void
load_messages_file(const char * textdbf)
{
  char linebuf[444];
  FILE * file = fopen(textdbf, "r");
  int nmsgs=0;
  if (file) {
    clear_messages();

    while (fgets(linebuf, sizeof linebuf, file)) {
      linebuf[strcspn(linebuf, "\r\n")] = 0;  /* trim newline */
      if (strncmp(linebuf, "msgid ", 6) == 0) {
        char * msg = readtext(linebuf, sizeof linebuf, file);
        if (strncmp(linebuf, "msgstr ", 7) == 0) {
          char * locmsg = readtext(linebuf, sizeof linebuf, file);
          if (msg && *msg && locmsg && *locmsg){
            nmsgs++; add_message(msg, locmsg);
          }
        }
      }
      else {
      }
    }
    fclose(file);
  }
#ifdef debug_messages
  printf("read %d messages\n", nmsgs);
#endif
}

static bool
load_messages_lang_w(wstring lang, bool fallback)
{
  if (lang) {
    wchar * wl = newn(wchar, wcslen(lang) + 4);
    wcscpy(wl, lang);
    if (fallback) {
      wchar * _ = wcschr(wl, '_');
      if (_) {
        *_ = '\0';
        // continue below
      }
      else
        return false;
    }
    wcscat(wl, W(".po"));
    const char * textdbf = get_resource_file(W("lang"), wl, false);
    delete(wl);
#ifdef debug_messages
    printf("Trying to load messages from <%ls>: <%s>\n", lang, textdbf);
#endif
    if (textdbf) {
      load_messages_file(textdbf);
      delete(textdbf);
      return true;
    }
  }
  return false;
}

static bool
load_messages_lang(string lang, bool fallback)
{
  wchar * wlang = cs__utftowcs(lang);
  bool res = load_messages_lang_w((wstring)wlang, fallback);
  delete(wlang);
  return res;
}

static void
load_messages(config * cfg_p)
{
  if (cfg_p->lang) for (int fallback = false; fallback <= true; fallback++) {
#ifdef debug_messages
    printf("Loading localization <%ls> (fallback %d)\n", cfg_p->lang, fallback);
#endif
    clear_messages();
    if (wcscmp(cfg_p->lang, W("=")) == 0) {
      if (load_messages_lang(cfg_p->locale, fallback))
        return;
    }
    else if (wcscmp(cfg_p->lang, W("@")) == 0) {
      // locale_menu[1] is transformed from GetUserDefaultUILanguage()
      if (load_messages_lang(locale_menu[1], fallback))
        return;
    }
    else if (wcscmp(cfg_p->lang, W("*")) == 0) {
      // determine UI language from environment
      const char * lang = getenv("LANGUAGE");
      if (lang) {
        lang = strdup(lang);
        while (lang && *lang) {
          char * sep = strchr(lang, ':');
          if (sep)
            *sep = '\0';
          if (load_messages_lang(lang, fallback))
            return;
          lang = sep;
          if (lang)
            lang++;
        }
        delete(lang);
      }
      lang = getlocenvcat("LC_MESSAGES");
      if (lang && *lang) {
        lang = strdup(lang);
        char * dot = strchr(lang, '.');
        if (dot)
          *dot = '\0';
        if (load_messages_lang(lang, fallback))
          return;
        delete(lang);
      }
    }
    else {
      if (load_messages_lang_w(cfg_p->lang, fallback))
        return;
    }
  }
}

static int 
load_configr(config * p,string filename,int to_save)
{
  char linebuf[444];
  bool free_filename = false;
  if (*filename == '~' && filename[1] == '/') {
    filename = asform("%s%s", wv.home, filename + 1);
    free_filename = true;
  }
  FILE * file = fopen(filename, "r");

  if (free_filename) delete(filename);
  if(!file)return 0;

  while (fgets(linebuf, sizeof linebuf, file)) {
    char * lbuf = linebuf;
    int len;
    while (len = strlen(lbuf),
      (len && lbuf[len - 1] != '\n') ||
      (len > 1 && lbuf[len - 1] == '\n' && lbuf[len - 2] == '\\')
      )
    {
      if (lbuf == linebuf) {
        // make lbuf dynamic
        lbuf = strdup(lbuf);
      }
      // append to lbuf
      len = strlen(lbuf);
      lbuf = renewn(lbuf, len + sizeof linebuf);
      if (!fgets(&lbuf[len], sizeof linebuf, file))
        break;
    }

    if (lbuf[len - 1] == '\n')
      lbuf[len - 1] = 0;

    if (lbuf[0] == '#' || lbuf[0] == '\0') {
      // preserve comment lines and empty lines
      if (to_save)
        remember_file_comment(lbuf);
    }
    else {
      // apply config options
      int i = parse_option(p,lbuf, true);
      // remember config options for saving
      if (to_save) {
        if (i >= 0)
          remember_file_option("load", i);
        else
          // preserve unknown options as comment lines
          remember_file_comment(lbuf);
      }
    }
    if (lbuf != linebuf)
      delete(lbuf);
  }
  fclose(file);
  return 1;
}

// to_save:
// 0 read config from filename
// 1 use filename for saving if file exists and is writable
// 2 use filename for saving if none was previously determined
// 3 use filename for saving (override)
void
load_config(string filename, int to_save)
{
  int rok;
  trace_theme(("load_config <%s> %d\n", filename, to_save));
  if (!to_save) {
    // restore base configuration, without theme mix-ins
    copy_config("load", &cfg, &file_cfg);
  }

  bool free_filename = false;
  if (*filename == '~' && filename[1] == '/') {
    filename = asform("%s%s", wv.home, filename + 1);
    free_filename = true;
  }
  rok=access(filename, R_OK) == 0;

  if (rok && access(filename, W_OK) < 0)
    to_save = false;

  if (wv.report_config)
    printf("loading config <%s>\n", filename);

  if (to_save) {
    if (rok|| (!rc_filename && to_save == 2) || to_save == 3) {
      clear_opts();

      delete(rc_filename);
      rc_filename = path_posix_to_win_w(filename);
      if (wv.report_config)
        printf("save to config <%ls>\n", rc_filename);
    }
  }

  if(load_configr(&cfg,filename, to_save)){
    CLRCPY(cfg_colours,cfg.ansi_colours);
  }
  if (free_filename) delete(filename);
  check_legacy_options(remember_file_option);
  if (to_save) {
    copy_config("after load", &file_cfg, &cfg);
  }
}

void
copy_config(string tag, config * dst_p, const config * src_p)
{
#ifdef debug_theme
  char * _cfg(const config * p) {
    return p == &new_cfg ? "new" : p == &file_cfg ? "file" : p == &cfg ? "cfg" : "?";
  }
  printf("[%s] copy_config %s <- %s\n", tag, _cfg(dst_p), _cfg(src_p));
#else
  (void)tag;
#endif
  for (uint i = 0; options[i].name; i++) {
    opt_type type = options[i].type;
    if (!(type & OPT_LEGACY)) {
      uint offset = options[i].offset;
      void *dst_val_p = (void *)dst_p + offset;
      void *src_val_p = (void *)src_p + offset;
      switch (type & OPT_TYPE_MASK) {
        when OPT_STRING:
          strset(dst_val_p, *(string *)src_val_p);
        when OPT_WSTRING:
          wstrset(dst_val_p, *(wstring *)src_val_p);
        when OPT_COMMENT: 
        when OPT_INT:
          *(int *)dst_val_p = *(int *)src_val_p;
        when OPT_COLOUR:
          *(colour *)dst_val_p = *(colour *)src_val_p;
        when OPT_COLOUR_PAIR:
          *(colour_pair *)dst_val_p = *(colour_pair *)src_val_p;
        otherwise:
          *(char *)dst_val_p = *(char *)src_val_p;
      }
    }
  }
}

void
init_config(void)
{
  copy_config("init", &cfg, &default_cfg);
  CLRCPY(def_colours,default_cfg.ansi_colours);
}

static void
fix_config(void)
{
  // Avoid negative sizes.
  cfg.rows = max(1, cfg.rows);
  cfg.cols = max(1, cfg.cols);
  cfg.scrollback_lines = max(0, cfg.scrollback_lines);

  // Limit size of scrollback buffer.
  cfg.scrollback_lines = min(cfg.scrollback_lines, cfg.max_scrollback_lines);
}

void
finish_config(void)
{
  if (*cfg.lang && (wcscmp(cfg.lang, W("=")) != 0 || *cfg.locale))
    load_messages(&cfg);
#if defined(debug_messages) && debug_messages > 1
  else
    (void)load_messages_lang("messages",0);
#endif
#ifdef debug_opterror
  opterror("Tast L %s %s", false, "bh", "bh");
  opterror("Tast U %s %s", true, "bh", "bh");
#endif

  fix_config();

  // Ignore charset setting if we haven't got a locale.
  if (!*cfg.locale)
    strset(&cfg.charset, "");

  // bold_as_font used to be implied by !bold_as_colour.
  //printf("finish_config bd %d\n", cfg.bold_as_font);
#ifdef previous_patch_for_242
  // This tweak was added in commit/964b3097e4624d4b5a3231389d34c00eb5cd1d6d
  // to support bold display as both font and colour (#242)
  // but it does not seem necessary anymore with the current code and options
  // handling, and it confuses option initialization (mintty/wsltty#103),
  // so it's removed.
  if (cfg.bold_as_font == -1) {
    cfg.bold_as_font = !cfg.bold_as_colour;
    remember_file_option("finish", find_option(true, "BoldAsFont"));
  }
#endif

  if (0 < cfg.transparency && cfg.transparency <= 3)
    cfg.transparency *= 16;
  //printf("finish_config bd %d\n", cfg.bold_as_font);
}
static void printOptVar(FILE*file,int type,const void*val_p){
  switch (type & OPT_TYPE_MASK) {
    when OPT_STRING:
        fprintf(file, "%s", *(string *)val_p);
    when OPT_WSTRING: {
      char * s = cs__wcstoutf(*(wstring *)val_p);
      fprintf(file, "%s", s);
      delete(s);
    }
    when OPT_INT:
        fprintf(file, "%i", *(int *)val_p);
    when OPT_COMMENT: 
    when OPT_COLOUR: {
      colour c = *(colour *)val_p;
      fprintf(file, "%u,%u,%u", red(c), green(c), blue(c));
    }
    when OPT_COLOUR_PAIR: {
      colour_pair p = *(colour_pair *)val_p;
      fprintf(file, "%u,%u,%u", red(p.fg), green(p.fg), blue(p.fg));
      if (p.fg != p.bg)
        fprintf(file, ";%u,%u,%u", red(p.bg), green(p.bg), blue(p.bg));
    }
    otherwise: {
      int val = *(char *)val_p;
      opt_val *o = opt_vals[type];
      for (; o->name; o++) {
        if (o->val == val)
          break;
      }
      if (o->name) fputs(o->name, file);
      else fprintf(file, "%i", val);
    }
  }
}

static void
save_config(void)
{
  string filename;

  filename = path_win_w_to_posix(rc_filename);

  FILE *file = fopen(filename, "w");
  delete(filename);

  if (!file) {
    // Should we report the failed Windows or POSIX path? (see mintty/wsltty#42)
    // In either case, we must transform to Unicode.
    // For WSL, it's probably not a good idea to report a POSIX path 
    // because it would be mistaken for a WSL path.
    char *msg;
    char * up = cs__wcstoutf(rc_filename);
    //__ %1$s: config file name, %2$s: error message
    int len = asprintf(&msg, _("Could not save options to '%s':\n%s."),
                       up, strerror(errno));
    delete(up);
    if (len > 0) {
      win_show_error(msg);
      delete(msg);
    }
    return ;
  }
  void *cfg_p = &file_cfg;
  for (uint j = 0; j < file_opts_num; j++) {
    if (file_opts[j].comment) {
      fprintf(file, "%s\n", file_opts[j].comment);
      continue;
    }
    uint i = file_opts[j].opti;
    opt_type type = options[i].type;
    if (!(type & OPT_LEGACY)) {
      fprintf(file, "%s=", options[i].name);
      printOptVar(file,type,cfg_p+options[i].offset);
      fputc('\n', file);
    }
  }
  fclose(file);
}

static int 
load_themer(config *p,wstring theme)
{
  char * thf=NULL;
  if (!*theme) return 0;
  if (wcschr(theme, L'/') || wcschr(theme, L'\\')) 
    thf = path_win_w_to_posix(theme);
  else 
    thf = get_resource_file(W("themes"), theme, false);
  if(!thf)return 0;
  int r=load_configr(p,thf, false);
  delete(thf);
  if(!r)return 0;
  CLRCPY(theme_colours,p->ansi_colours);
  wcsncpy(theme_file,theme,128);
  return 1;
}
void 
load_theme(wstring theme){
  load_themer(&cfg,theme);
}

static void
load_schemer(config*p,string cs)
{
  //copy_config("scheme", &cfg, &file_cfg);

  // analyse scheme description
  char * scheme = strdup(cs);
  char * sch = scheme;
  char * param = scheme;
  char * value = null;
  while (*sch) {
    if (*sch == '=') {
      *sch++ = '\0';
      value = sch;
    }
    else if (*sch == ';') {
      *sch++ = '\0';
      if (value) {
        set_option(p,param, value, false);
      }
      param = sch;
      value = null;
    }
    else
      sch++;
  }
  delete(scheme);
}
void
load_scheme(string cs){
  load_schemer(&cfg,cs);
}

static control *cols_box, *rows_box, *locale_box, *charset_box;
static control *transparency_valbox, *transparency_selbox;
static control *font_sample, *font_list, *font_weights;

void
apply_config(bool save)
{
  // Record what's changed
  for (uint i = 0; options[i].name; i++) {
    opt_type type = options[i].type;
    uint offset = options[i].offset;
    //void *val_p = (void *)&cfg + offset;
    void *val_p = (void *)&file_cfg + offset;
    void *new_val_p = (void *)&new_cfg + offset;
    if (!(type & OPT_LEGACY)) {
      bool changed;
      switch (type & OPT_TYPE_MASK) {
        when OPT_STRING:
          changed = strcmp(*(string *)val_p, *(string *)new_val_p);
        when OPT_WSTRING:
          changed = wcscmp(*(wstring *)val_p, *(wstring *)new_val_p);
        when OPT_INT:
          changed = (*(int *)val_p != *(int *)new_val_p);
        when OPT_COLOUR:
          changed = (*(colour *)val_p != *(colour *)new_val_p);
        when OPT_COLOUR_PAIR:
          changed = memcmp(val_p, new_val_p, sizeof(colour_pair));
        otherwise:
          changed = (*(char *)val_p != *(char *)new_val_p);
      }
      if (changed)
        remember_file_option("apply", i);
    }
  }

  copy_config("apply", &file_cfg, &new_cfg);
  if (wcscmp(new_cfg.lang, cfg.lang) != 0
      || (wcscmp(new_cfg.lang, W("=")) == 0 && new_cfg.locale != cfg.locale)
     ){
    load_messages(&new_cfg);
  }
  if(memcmp(theme_colours,file_cfg.ansi_colours,16*sizeof(colour))==0){
    CLRCPY(file_cfg.ansi_colours,cfg_colours);
  }else{
    *(wchar*)file_cfg.theme_file=0;
  }
  win_reconfig();  // copy_config(&cfg, &new_cfg);
  fix_config();
  if (save)
    save_config();
  bool had_theme = !!*cfg.theme_file;

  if (*cfg.colour_scheme) {
    load_scheme(cfg.colour_scheme);
    win_reset_colours();
    win_invalidate_all(false);
  }
  else if (*cfg.theme_file) {
    load_theme(cfg.theme_file);
    win_reset_colours();
    win_invalidate_all(false);
  }
  else if (had_theme) {
    win_reset_colours();
    win_invalidate_all(false);
  }
  //printf("apply_config %d bd %d\n", save, cfg.bold_as_font);
}


// Registry handling (for retrieving localized sound labels)

static HKEY
regopen(HKEY key,const char * subkey)
{
  HKEY hk = 0;
  RegOpenKeyA(key, subkey, &hk);
  return hk;
}

static HKEY
getmuicache()
{
  HKEY hk = regopen(HKEY_CURRENT_USER, "Software\\Classes\\Local Settings\\MuiCache");
  if (!hk)
    return 0;

  char sk[256];
  if (RegEnumKeyA(hk, 0, sk, 256) != ERROR_SUCCESS)
    return 0;

  HKEY hk1 = regopen(hk, sk);
  RegCloseKey(hk);
  if (!hk1)
    return 0;

  if (RegEnumKeyA(hk1, 0, sk, 256) != ERROR_SUCCESS)
    return 0;

  hk = regopen(hk1, sk);
  RegCloseKey(hk1);
  if (!hk)
    return 0;

  return hk;
}

static HKEY muicache = 0;
static HKEY evlabels = 0;

static void
retrievemuicache()
{
  muicache = getmuicache();
  if (muicache) {
    evlabels = regopen(HKEY_CURRENT_USER, "AppEvents\\EventLabels");
    if (!evlabels) {
      RegCloseKey(muicache);
      muicache = 0;
    }
  }
}

static void
closemuicache()
{
  if (muicache) {
    RegCloseKey(evlabels);
    RegCloseKey(muicache);
  }
}

wchar *
getregstr(HKEY key, wstring subkey, wstring attribute)
{
#if CYGWIN_VERSION_API_MINOR < 74
  (void)key;
  (void)subkey;
  (void)attribute;
  return 0;
#else
  // RegGetValueW is easier but not supported on Windows XP
  HKEY sk = 0;
  RegOpenKeyW(key, subkey, &sk);
  if (!sk)
    return 0;
  DWORD type;
  DWORD len;
  int res = RegQueryValueExW(sk, attribute, 0, &type, 0, &len);
  if (res)
    return 0;
  if (!(type == REG_SZ || type == REG_EXPAND_SZ || type == REG_MULTI_SZ))
    return 0;
  wchar * val = malloc (len);
  res = RegQueryValueExW(sk, attribute, 0, &type, (void *)val, &len);
  RegCloseKey(sk);
  if (res) {
    delete(val);
    return 0;
  }
  return val;
#endif
}

uint
getregval(HKEY key, wstring subkey, wstring attribute)
{
#if CYGWIN_VERSION_API_MINOR < 74
  (void)key;
  (void)subkey;
  (void)attribute;
  return 0;
#else
  // RegGetValueW is easier but not supported on Windows XP
  HKEY sk = 0;
  RegOpenKeyW(key, subkey, &sk);
  if (!sk)
    return 0;
  DWORD type;
  DWORD len;
  int res = RegQueryValueExW(sk, attribute, 0, &type, 0, &len);
  if (res)
    return 0;
  if (type == REG_DWORD) {
    DWORD val;
    len = sizeof(DWORD);
    res = RegQueryValueExW(sk, attribute, 0, &type, (void *)&val, &len);
    RegCloseKey(sk);
    if (!res)
      return (uint)val;
  }
  return 0;
#endif
}

static wchar *
muieventlabel(const wchar * event)
{
  // HKEY_CURRENT_USER\AppEvents\EventLabels\SystemAsterisk
  // DispFileName -> "@mmres.dll,-5843"
  wchar * rsr = getregstr(evlabels, event, W("DispFileName"));
  if (!rsr)
    return 0;
  // HKEY_CURRENT_USER\Software\Classes\Local Settings\MuiCache\N\M
  // "@mmres.dll,-5843" -> "Sternchen"
  wchar * lbl = getregstr(muicache, 0, rsr);
  delete(rsr);
  return lbl;
}


// Options dialog handlers

static void
ok_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION) {
    apply_config(true);
    dlg_end();
  }
}

static void
cancel_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    dlg_end();
}

static void
apply_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    apply_config(false);
}

static void
about_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    win_show_about();
}


#if CYGWIN_VERSION_API_MINOR < 74
#define use_findfile
#endif

#ifdef use_findfile
static void
do_file_resources(control *ctrl, wstring pattern, bool list_dirs, str_fn fnh)
{
  static wstring *lst=NULL;
  static int nl=0,ml=0;
  void clrlist(){
    for (int i=0;i<nl;i++){
      delete(lst[i]);
    }
    nl=0;ml=0;
    delete(lst);
    lst=NULL;
  }
  int addlist(wstring s){
    for (int i=0;i<nl;i++){
      if(wcscasecmp(lst[i],s)==0)return i;
    }
    if(nl>=ml){
      ml+=16;
      lst=renewn(lst,ml);
    }
    lst[nl++]=wcsdup(s);
    return -nl;
  }
  clrlist();
  init_config_dirs();
  //printf("add_file_resources <%ls> dirs %d\n", pattern, list_dirs);


  for (int i = last_config_dir; i >= 0; i--) {
    wstring suf = wcsrchr(pattern, L'.');
    int sufl = suf ? wcslen(suf) : 0;
    wchar * rc = awsform(W("%s/%ls"), config_dirs[i], pattern);
    wchar * rcpat = path_posix_to_win_w(rc);
    wchar *rt=wcsrchr(rc,L'/');
    int len=0;
    if(rt)len=rt-rc+1;

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(rcpat, &ffd);
    int ok = hFind != INVALID_HANDLE_VALUE;
    rcpat[len]=0;
    delete(rcpat);
    if (ok) {
      while (ok) {
        if (list_dirs && (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
          if (ffd.cFileName[0] != '.' && !!wcscmp(ffd.cFileName, W("common")))
            // exclude the [0-7] links left over by the `getemojis` script
            if (wcslen(ffd.cFileName) > 1)
              if(addlist(ffd.cFileName)<0){
                if (ctrl){
                  dlg_listbox_add_w(ctrl, ffd.cFileName);
                }
              }
        }
        else if (!list_dirs) {
          //LARGE_INTEGER filesize = {.LowPart = ffd.nFileSizeLow, .HighPart = ffd.nFileSizeHigh};
          //long s = filesize.QuadPart;

          // strip suffix
          int len = wcslen(ffd.cFileName);
          if (ffd.cFileName[0] != '.' && ffd.cFileName[len - 1] != '~') {
            ffd.cFileName[len - sufl] = 0;
            if(addlist(ffd.cFileName)<0){
              if (ctrl)
                dlg_listbox_add_w(ctrl, ffd.cFileName);
              else if(fnh){
                char * fn= asform(("%ls/%ls"), rc, ffd.cFileName);
                fnh(fn);  // CYGWIN_VERSION_API_MINOR < 74
                delete(fn);
              }
            }
          }
        }
        ok = FindNextFileW(hFind, &ffd);
      }
      FindClose(hFind);

      //break;
    }
    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
      // empty valid dir
      //break;
    }
  }
  clrlist();
}
#else
static void
do_file_resources(control *ctrl, wstring pattern, bool list_dirs, str_fn fnh)
{
  static string *lst=NULL;
  static int nl=0,ml=0;
  void clrlist(){
    for (int i=0;i<nl;i++){
      delete(lst[i]);
    }
    nl=0;ml=0;
    delete(lst);
    lst=NULL;
  }
  int addlist(string s){
    for (int i=0;i<nl;i++){
      if(strcasecmp(lst[i],s)==0)return i;
    }
    if(nl>=ml){
      ml+=16;
      lst=renewn(lst,ml);
    }
    lst[nl++]=strdup(s);
    return -nl;
  }
  init_config_dirs();
  //printf("add_file_resources <%ls> dirs %d\n", pattern, list_dirs);
  clrlist();


#include <dirent.h>
  for (int i = last_config_dir; i >= 0; i--) {
    char * pat = cs__wcstombs(pattern);
    char * patsuf = strrchr(pat, '.');
    int patsuflen = patsuf ? strlen(patsuf) : 0;
    char * patbase = strrchr(pat, '/');
    if (patbase)
      *patbase = 0;
    char * rcpat = asform("%s/%s", config_dirs[i], pat);

    DIR * dir = opendir(rcpat);
    if (dir) {
      struct dirent * direntry;
      while ((direntry = readdir (dir)) != 0) {
        if (patsuf && !strstr(direntry->d_name, patsuf))
          continue;

        if (list_dirs && direntry->d_type == DT_DIR) {
          if (direntry->d_name[0] != '.' && !!strcmp(direntry->d_name, "common"))
            // exclude the [0-7] links left over by the `getemojis` script
            if (strlen(direntry->d_name) > 1) {
              if(addlist(direntry->d_name)<0){
                if (ctrl)
                  dlg_listbox_add(ctrl, direntry->d_name);
              }
            }
        }
        else if (!list_dirs) {
          // strip suffix
          int len = strlen(direntry->d_name);
          if (direntry->d_name[0] != '.' && direntry->d_name[len - 1] != '~') {
            direntry->d_name[len - patsuflen] = 0;
            if(addlist(direntry->d_name)<0){
              if (ctrl)
                dlg_listbox_add(ctrl, direntry->d_name);
              else {
                char * fn = asform(("%s/%s"), rcpat, direntry->d_name);
                if(fnh)fnh(fn);
                delete(fn);
              }
            }
          }
        }
      }
      closedir(dir);
    }
    delete(rcpat);
    delete(pat);
  }
  clrlist();
}
#endif

static void
add_file_resources(control *ctrl, wstring pattern, bool list_dirs)
{
  do_file_resources(ctrl, pattern, list_dirs, 0);
}

void
handle_file_resources(wstring pattern, str_fn fn_handler)
{
  do_file_resources(0, pattern, false, fn_handler);
}


static void
current_size_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION) {
    new_cfg.cols = term.cols;
    new_cfg.rows = term.rows;
    dlg_refresh(cols_box);
    dlg_refresh(rows_box);
  }
}

static void
printer_handler(control *ctrl, int event)
{
  const wstring NONE = _W("◇ None (printing disabled) ◇");  // ♢◇
  const wstring CFG_NONE = W("");
  const wstring DEFAULT = _W("◆ Default printer ◆");  // ♦◆
  const wstring CFG_DEFAULT = W("*");
  wstring printer = new_cfg.printer;
  if (event == EVENT_REFRESH) {
    dlg_listbox_clear(ctrl);
    dlg_listbox_add_w(ctrl, NONE);
    dlg_listbox_add_w(ctrl, DEFAULT);
    uint num = printer_start_enum();
    for (uint i = 0; i < num; i++)
      dlg_listbox_add_w(ctrl, printer_get_name(i));
    printer_finish_enum();
    if (*printer == '*')
      dlg_editbox_set_w(ctrl, DEFAULT);
    else
      dlg_editbox_set_w(ctrl, *printer ? printer : NONE);
  }
  else if (event == EVENT_VALCHANGE || event == EVENT_SELCHANGE) {
    int n = dlg_listbox_getcur(ctrl);
    if (n == 0)
      wstrset(&printer, CFG_NONE);
    else if (n == 1)
      wstrset(&printer, CFG_DEFAULT);
    else
      dlg_editbox_get_w(ctrl, &printer);

    new_cfg.printer = printer;
  }
}

static void
set_charset(string charset)
{
  strset(&new_cfg.charset, charset);
  dlg_editbox_set(charset_box, charset);
}

static void
locale_handler(control *ctrl, int event)
{
  string locale = new_cfg.locale;
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      string l;
      for (int i = 0; (l = locale_menu[i]); i++)
        dlg_listbox_add(ctrl, l);
      dlg_editbox_set(ctrl, locale);
    when EVENT_UNFOCUS:
      dlg_editbox_set(ctrl, locale);
      if (!*locale)
        set_charset("");
    when EVENT_VALCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.locale);
    when EVENT_SELCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.locale);
      if (*locale == '(')
        strset(&locale, "");
      if (!*locale)
        set_charset("");
#if HAS_LOCALES
      else if (!*new_cfg.charset)
        set_charset("UTF-8");
#endif
      new_cfg.locale = locale;
  }
}

static void
check_locale(void)
{
  if (!*new_cfg.locale) {
    strset(&new_cfg.locale, "C");
    dlg_editbox_set(locale_box, "C");
  }
}

static void
charset_handler(control *ctrl, int event)
{
  string charset = new_cfg.charset;
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      string cs;
      for (int i = 0; (cs = charset_menu[i]); i++)
        dlg_listbox_add(ctrl, cs);
      dlg_editbox_set(ctrl, charset);
    when EVENT_UNFOCUS:
      dlg_editbox_set(ctrl, charset);
      if (*charset)
        check_locale();
    when EVENT_VALCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.charset);
    when EVENT_SELCHANGE:
      dlg_editbox_get(ctrl, &charset);
      if (*charset == '(')
        strset(&charset, "");
      else {
        *strchr(charset, ' ') = 0;
        check_locale();
      }
      new_cfg.charset = charset;
  }
}

static void
lang_handler(control *ctrl, int event)
{
  //__ UI localization disabled
  const wstring NONE = _W("- None -");
  //__ UI localization: use Windows desktop setting
  const wstring WINLOC = _W("@ Windows language @");
  //__ UI localization: use environment variable setting (LANGUAGE, LC_*)
  const wstring LOCENV = _W("* Locale environm. *");
  //__ UI localization: use mintty configuration setting (Text - Locale)
  const wstring LOCALE = _W("= cfg. Text Locale =");
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      dlg_listbox_add_w(ctrl, NONE);
      dlg_listbox_add_w(ctrl, WINLOC);
      dlg_listbox_add_w(ctrl, LOCENV);
      dlg_listbox_add_w(ctrl, LOCALE);
      add_file_resources(ctrl, W("lang/*.po"), false);
      if (wcscmp(new_cfg.lang, W("")) == 0)
        dlg_editbox_set_w(ctrl, NONE);
      else if (wcscmp(new_cfg.lang, W("@")) == 0)
        dlg_editbox_set_w(ctrl, WINLOC);
      else if (wcscmp(new_cfg.lang, W("*")) == 0)
        dlg_editbox_set_w(ctrl, LOCENV);
      else if (wcscmp(new_cfg.lang, W("=")) == 0)
        dlg_editbox_set_w(ctrl, LOCALE);
      else
        dlg_editbox_set_w(ctrl, new_cfg.lang);
    when EVENT_VALCHANGE or EVENT_SELCHANGE: {
      int n = dlg_listbox_getcur(ctrl);
      if (n == 0)
        wstrset(&new_cfg.lang, W(""));
      else if (n == 1)
        wstrset(&new_cfg.lang, W("@"));
      else if (n == 2)
        wstrset(&new_cfg.lang, W("*"));
      else if (n == 3)
        wstrset(&new_cfg.lang, W("="));
      else
        dlg_editbox_get_w(ctrl, &new_cfg.lang);
    }
  }
}

static void
term_handler(control *ctrl, int event)
{
  bool terminfo_exists(const char * ti) {
    bool terminfo_exists_in(const char * dir, const char * sub, const char * ti) {
      char * terminfo = asform("%s%s/%x/%s", dir, sub ?: "", *ti, ti);
      bool exists = !access(terminfo, R_OK);
      delete(terminfo);
      if (wv.support_wsl && !exists) {
        terminfo = asform("%s%s/%c/%s", dir, sub ?: "", *ti, ti);
        exists = !access(terminfo, R_OK);
        //printf("exists %d <%s>\n", exists, terminfo);
        delete(terminfo);
      }
      return exists;
    }
    if (wv.support_wsl) {
      char * wslroot;
      if (wv.wslname) {
        char * wslnamec = cs__wcstombs(wv.wslname);
        wslroot = asform("//wsl$/%s", wslnamec);
        delete(wslnamec);
      }
      else if (*wv.wsl_basepath)
        wslroot = path_win_w_to_posix(wv.wsl_basepath);
      else
        wslroot = strdup("");
      bool ex = terminfo_exists_in(wslroot, "/usr/share/terminfo", ti);
      delete(wslroot);
      return ex;
    }
    else
      return terminfo_exists_in("/usr/share/terminfo", 0, ti)
          || terminfo_exists_in(wv.home, "/.terminfo", ti)
           ;
  }
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      dlg_listbox_add(ctrl, "xterm");
      dlg_listbox_add(ctrl, "xterm-256color");
      if (terminfo_exists("xterm-direct"))
        dlg_listbox_add(ctrl, "xterm-direct");
      dlg_listbox_add(ctrl, "xterm-vt220");
      dlg_listbox_add(ctrl, "vt100");
      dlg_listbox_add(ctrl, "vt220");
      dlg_listbox_add(ctrl, "vt340");
      dlg_listbox_add(ctrl, "vt420");
      dlg_listbox_add(ctrl, "vt525");
      if (terminfo_exists("mintty"))
        dlg_listbox_add(ctrl, "mintty");
      if (terminfo_exists("mintty-direct"))
        dlg_listbox_add(ctrl, "mintty-direct");
      dlg_editbox_set(ctrl, new_cfg.Term);
    when EVENT_VALCHANGE or EVENT_SELCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.Term);
  }
}

//  1 -> 0x00000000 MB_OK              Default Beep
//  2 -> 0x00000010 MB_ICONSTOP        Critical Stop
//  3 -> 0x00000020 MB_ICONQUESTION    Question
//  4 -> 0x00000030 MB_ICONEXCLAMATION Exclamation
//  5 -> 0x00000040 MB_ICONASTERISK    Asterisk
// -1 -> 0xFFFFFFFF                    Simple Beep
static struct {
  string name;
  wchar * event;
} beeps[] = {
  {__("simple beep"), null},
  {__("no beep"), null},
  {__("Default Beep"),	W(".Default")},
  {__("Critical Stop"),	W("SystemHand")},
  {__("Question"),	W("SystemQuestion")},
  {__("Exclamation"),	W("SystemExclamation")},
  {__("Asterisk"),	W("SystemAsterisk")},
};

static void
bell_handler(control *ctrl, int event)
{
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      retrievemuicache();
      for (uint i = 0; i < lengthof(beeps); i++) {
        string beepname = _(beeps[i].name);
        if (beepname == beeps[i].name) {
          // no localization entry, try to retrieve system localization
          if (muicache && beeps[i].event) {
            wchar * lbl = muieventlabel(beeps[i].event);
            if (lbl) {
              dlg_listbox_add_w(ctrl, lbl);
              if ((int)i == new_cfg.bell_type + 1)
                dlg_editbox_set_w(ctrl, lbl);
              beepname = null;
              delete(lbl);
            }
          }
        }
        if (beepname) {
          dlg_listbox_add(ctrl, beepname);
          if ((int)i == new_cfg.bell_type + 1)
            dlg_editbox_set(ctrl, beepname);
        }
      }
      closemuicache();
    when EVENT_VALCHANGE or EVENT_SELCHANGE: {
      new_cfg.bell_type = dlg_listbox_getcur(ctrl) - 1;

      win_bell(&new_cfg);
    }
  }
}

static void
bellfile_handler(control *ctrl, int event)
{
  const wstring NONE = _W("◇ None (system sound) ◇");  // ♢◇
  const wstring CFG_NONE = W("");
  wstring bell_file = new_cfg.bell_file[6];
  if (event == EVENT_REFRESH) {
    dlg_listbox_clear(ctrl);
    dlg_listbox_add_w(ctrl, NONE);
    add_file_resources(ctrl, W("sounds/*.wav"), false);
    // strip std dir prefix...
    dlg_editbox_set_w(ctrl, *bell_file ? bell_file : NONE);
  }
  else if (event == EVENT_VALCHANGE || event == EVENT_SELCHANGE) {
    if (dlg_listbox_getcur(ctrl) == 0)
      wstrset(&bell_file, CFG_NONE);
    else
      dlg_editbox_get_w(ctrl, &bell_file);

    // add std dir prefix?
    new_cfg.bell_file[6] = bell_file;
    win_bell(&new_cfg);
  }
  else if (event == EVENT_DROP) {
    dlg_editbox_set_w(ctrl, dragndrop);
    wstrset(&new_cfg.bell_file[6], dragndrop);
    win_bell(&new_cfg);
  }
}

static control * theme = null;
static control * store_button = null;

static void
enable_widget(control * ctrl, bool enable)
{
  if (!ctrl)
    return;

  HWND wid = ctrl->widget;
  EnableWindow(wid, enable);
}

#define dont_debug_scheme 2

/*
   Load scheme from URL or file, convert .itermcolors and .json formats
 */
static char *
download_scheme(const char * url)
{
  if (strchr(url, '\''))
    return null;  // Insecure link

  char linebuf[444];
  FILE * sf = 0;
  char * sfn = 0;
  if (url[1] == ':') {
    sf = fopen(url, "r");
  }
  else {
#ifdef use_curl
    static string cmdpat = "curl '%s' -o - 2> /dev/null";
    char * cmd = newn(char, strlen(cmdpat) -1 + strlen(url));
    sprintf(cmd, cmdpat, url);
    sf = popen(cmd, "r");
#else
    HRESULT (WINAPI * pURLDownloadToFile)(void *, LPCSTR, LPCSTR, DWORD, void *) = 0;
    pURLDownloadToFile = load_library_func("urlmon.dll", "URLDownloadToFileA");
    bool ok = false;
    sfn = asform("%s/.mintty-scheme.%d", tmpdir(), getpid());
    if (pURLDownloadToFile) {
# ifdef __CYGWIN__
      /* Need to sync the Windows environment */
      cygwin_internal(CW_SYNC_WINENV);
# endif
      char * wfn = path_posix_to_win_a(sfn);
      ok = S_OK == pURLDownloadToFile(NULL, url, wfn, 0, NULL);
      delete(wfn);
    }
    if (!ok)
      delete(sfn);
    sf = fopen(sfn, "r");
    //printf("URL <%s> file <%s> OK %d\n", url, sfn, !!sf);
    if (!sf) {
      remove(sfn);
      delete(sfn);
    }
#endif
  }
  if (!sf)
    return null;
#ifdef debug_scheme
  printf("URL <%s> OK %d\n", url, !!sf);
#endif

  // colour scheme string
  char * sch = null;
  // colour modifications, common for .itermcolors and .json (unused there)
  colour ansi_colours[16] = 
    {(colour)-1, (colour)-1, (colour)-1, (colour)-1, 
     (colour)-1, (colour)-1, (colour)-1, (colour)-1, 
     (colour)-1, (colour)-1, (colour)-1, (colour)-1, 
     (colour)-1, (colour)-1, (colour)-1, (colour)-1};
  colour fg_colour = (colour)-1, bg_colour = (colour)-1;
  colour bold_colour = (colour)-1, blink_colour = (colour)-1;
  colour cursor_colour = (colour)-1, sel_fg_colour = (colour)-1, sel_bg_colour = (colour)-1;
  colour underl_colour = (colour)-1, hover_colour = (colour)-1;
  // construct a ColourScheme string
  void schapp(const char * opt, colour c)
  {
#if defined(debug_scheme) && debug_scheme > 1
    printf("schapp %s %06X\n", opt, c);
#endif
    if (c != (colour)-1) {
      char colval[strlen(opt) + 14];
      sprintf(colval, "%s=%u,%u,%u;", opt, red(c), green(c), blue(c));
      int len = sch ? strlen(sch) : 0;
      sch = renewn(sch, len + strlen(colval) + 1);
      strcpy(&sch[len], colval);
    }
  }
  // collect all modified colours in a colour scheme string
  void schappall()
  {
    schapp("ForegroundColour", fg_colour);
    schapp("BackgroundColour", bg_colour);
    schapp("BoldColour", bold_colour);
    schapp("BlinkColour", blink_colour);
    schapp("CursorColour", cursor_colour);
    schapp("UnderlineColour", underl_colour);
    schapp("HoverColour", hover_colour);
    schapp("HighlightBackgroundColour", sel_bg_colour);
    schapp("HighlightForegroundColour", sel_fg_colour);
    schapp("Black", ansi_colours[BLACK_I]);
    schapp("Red", ansi_colours[RED_I]);
    schapp("Green", ansi_colours[GREEN_I]);
    schapp("Yellow", ansi_colours[YELLOW_I]);
    schapp("Blue", ansi_colours[BLUE_I]);
    schapp("Magenta", ansi_colours[MAGENTA_I]);
    schapp("Cyan", ansi_colours[CYAN_I]);
    schapp("White", ansi_colours[WHITE_I]);
    schapp("BoldBlack", ansi_colours[BOLD_BLACK_I]);
    schapp("BoldRed", ansi_colours[BOLD_RED_I]);
    schapp("BoldGreen", ansi_colours[BOLD_GREEN_I]);
    schapp("BoldYellow", ansi_colours[BOLD_YELLOW_I]);
    schapp("BoldBlue", ansi_colours[BOLD_BLUE_I]);
    schapp("BoldMagenta", ansi_colours[BOLD_MAGENTA_I]);
    schapp("BoldCyan", ansi_colours[BOLD_CYAN_I]);
    schapp("BoldWhite", ansi_colours[BOLD_WHITE_I]);
  }

  char * urlsuf = strrchr(url, '.');
  if (urlsuf && !strcmp(urlsuf, ".itermcolors")) {
    int level = 0;
    colour * key = 0;
    int component = -1;
    while (fgets(linebuf, sizeof(linebuf) - 1, sf)) {
      if (strstr(linebuf, "<dict>"))
        level++;
      else if (strstr(linebuf, "</dict>"))
        level--;
      else {
        char * entity = strstr(linebuf, "<key>");
        if (entity) {
          entity += 5;
          char * fini = strchr(entity, '<');
          if (fini)
            *fini = 0;
          if (level == 2) {
            if (0 == strcmp(entity, "Blue Component"))
              component = 2;
            else if (0 == strcmp(entity, "Green Component"))
              component = 1;
            else if (0 == strcmp(entity, "Red Component"))
              component = 0;
            else if (0 == strcmp(entity, "Alpha Component"))
              ;
            else if (0 == strcmp(entity, "Color Space"))
              ;
            else {
              component = -1;
            }
          }
          else if (level == 1) {
#if defined(debug_scheme) && debug_scheme > 1
            printf("iterm entity <%s>\n", entity);
#endif
            int coli;
            if (0 == strcmp(entity, "Foreground Color"))
              key = &fg_colour;
            else if (0 == strcmp(entity, "Bold Color"))
              key = &bold_colour;
            else if (0 == strcmp(entity, "Background Color"))
              key = &bg_colour;
            else if (0 == strcmp(entity, "Cursor Color"))
              key = &cursor_colour;
            //else if (0 == strcmp(entity, "Cursor Text Color"))
            else if (0 == strcmp(entity, "Selected Text Color"))
              key = &sel_fg_colour;
            else if (0 == strcmp(entity, "Selection Color"))
              key = &sel_bg_colour;
            else if (0 == strcmp(entity, "Underline Color"))
              key = &underl_colour;
            else if (0 == strcmp(entity, "Link Color"))
              key = &hover_colour;  // ?
            //else if (0 == strcmp(entity, "Cursor Guide Color"))
            //else if (0 == strcmp(entity, "Tab Color"))
            //else if (0 == strcmp(entity, "Badge Color"))
            else if (sscanf(entity, "Ansi %d Color", &coli) == 1 && coli >= 0 && coli < 16) {
              key = &ansi_colours[coli];
            }
            else
              key = 0;
          }
        }
        else if (level == 2 && key) {
#if defined(debug_scheme) && debug_scheme > 1
          printf("iterm value <%s>\n", linebuf);
#endif
          entity = strstr(linebuf, "<real>");
          double val;
          if (entity && sscanf(entity, "<real>%lf<", &val) == 1 && val >= 0.0 && val <= 1.0) {
            int ival = val * 255.0 + 0.5;
            switch (component) {
              when 0:  // red
                *key = (*key & 0xFFFF00) | ival;
              when 1:  // green
                *key = (*key & 0xFF00FF) | ival << 8;
              when 2:  // blue
                *key = (*key & 0x00FFFF) | ival << 16;
            }
          }
        }
      }
    }
    // collect modified colours into colour scheme string
    schappall();
  }
  else if (urlsuf && !strcmp(urlsuf, ".json")) {
    // support .json theme files in either vscode or Windows terminal format
    while (fgets(linebuf, sizeof(linebuf) - 1, sf)) {
      char * scan = strchr(linebuf, '"');
      char * key;
      char * val;
      if (scan) {
        scan++;
        // strip vscode prefixes
        if (strncmp(scan, "terminal.", 9) == 0) {
          scan += 9;
          if (strncmp(scan, "ansi", 4) == 0)
            scan += 4;
        }
        key = scan;
        scan = strchr(scan, '"');
      }
      if (scan) {
        *scan = 0;
        scan++;
        scan = strchr(scan, '"');
        if (scan) {
          scan++;
          val = scan;
          scan = strchr(scan, '"');
          if (scan) {
            *scan = 0;
          }
        }
      }
      if (scan) {
#if defined(debug_scheme) && debug_scheme > 1
        printf("<%s> <%s> (%s)\n", key, val, linebuf);
#endif
        // transform .json colour names
        void schapp(const char * jname, const char * name)
        {
          if (strcasecmp(key, jname) == 0) {
#if defined(debug_scheme) && debug_scheme > 1
            printf("%s=%s\n", name, val);
#endif
            int len = sch ? strlen(sch) : 0;
            sch = renewn(sch, len + strlen(name) + strlen(val) + 3);
            sprintf(&sch[len], "%s=%s;", name, val);
          }
        }
        schapp("black", "Black");
        schapp("red", "Red");
        schapp("green", "Green");
        schapp("yellow", "Yellow");
        schapp("blue", "Blue");
        schapp("magenta", "Magenta");
        schapp("purple", "Magenta");
        schapp("cyan", "Cyan");
        schapp("white", "White");
        schapp("brightblack", "BoldBlack");
        schapp("brightred", "BoldRed");
        schapp("brightgreen", "BoldGreen");
        schapp("brightyellow", "BoldYellow");
        schapp("brightblue", "BoldBlue");
        schapp("brightmagenta", "BoldMagenta");
        schapp("brightpurple", "BoldMagenta");
        schapp("brightcyan", "BoldCyan");
        schapp("brightwhite", "BoldWhite");
        schapp("foreground", "ForegroundColour");
        schapp("background", "BackgroundColour");
      }
    }
  }
  else {
    while (fgets(linebuf, sizeof(linebuf) - 1, sf)) {
      char * eq = linebuf;
      while ((eq = strchr(++eq, '='))) {
        int dum;
        if (sscanf(eq, "= %d , %d , %d", &dum, &dum, &dum) == 3) {
          char *cp = eq;
          while (strchr("=0123456789, ", *cp))
            cp++;
          *cp++ = ';';
          *cp = '\0';
          cp = eq;
          if (cp != linebuf)
            cp--;
          while (strchr("BCFGMRWYacdeghiklnorstuwy ", *cp)) {
            eq = cp;
            if (cp == linebuf)
              break;
            else
              cp--;
          }
          while (*eq == ' ')
            eq++;
          if (*eq != '=') {
            // squeeze white space
            char * src = eq;
            char * dst = eq;
            while (*src) {
              if (*src != ' ' && *src != '\t')
                *dst++ = *src;
              src++;
            }
            *dst = '\0';

            int len = sch ? strlen(sch) : 0;
            sch = renewn(sch, len + strlen(eq) + 1);
            strcpy(&sch[len], eq);
          }
          break;
        }
      }
    }
  }
#ifdef use_curl
  pclose(sf);
#else
  fclose(sf);
  if (sfn) {
    remove(sfn);
    delete(sfn);
  }
#endif

#if defined(debug_scheme) && debug_scheme > 1
  printf("download_scheme -> <%s>\n", sch);
#endif
  return sch;
}
static int themechanged(){
  wstring theme_name = new_cfg.theme_file;
  if(!*theme_name)return 0;
  if(wcschr(theme_name, L'/') || wcschr(theme_name, L'\\'))return 0;
  if(memcmp(theme_colours,new_cfg.ansi_colours,16*sizeof(colour)))return 1;
  if(wcscmp(theme_name,theme_file))return 1;
  return 0;
}

static void
theme_handler(control *ctrl, int event)
{
  //__ terminal theme / colour scheme
  const wstring NONE = _W("◇ None ◇");  // ♢◇
  const wstring CFG_NONE = W("");
  //__ indicator of unsaved downloaded colour scheme
  const wstring DOWNLOADED = _W("downloaded / give me a name!");
  // downloaded theme indicator must contain a slash
  // to steer enabled state of Store button properly
  const wstring CFG_DOWNLOADED = W("@/@");
  wstring theme_name = new_cfg.theme_file;
  int upd=0;
  static int ued=0;
  switch(event){
    when EVENT_REFRESH: {
      ued=0;
      dlg_listbox_clear(ctrl);
      dlg_listbox_add_w(ctrl, NONE);
      add_file_resources(ctrl, W("themes/*"), false);
#ifdef attempt_to_keep_scheme_hidden
      if (*new_cfg.colour_scheme)
        // don't do this, rather keep previously entered name to store scheme
        // scheme string will not be entered here anyway
        dlg_editbox_set_w(ctrl, W(""));
      else
#endif
        dlg_editbox_set_w(ctrl, !wcscmp(theme_name, CFG_DOWNLOADED) ? DOWNLOADED : *theme_name ? theme_name : NONE);
    }
    when EVENT_SELCHANGE: {  // pull-down selection
      if (dlg_listbox_getcur(ctrl) == 0)
        wstrset(&theme_name, CFG_NONE);
      else
        dlg_editbox_get_w(ctrl, &theme_name);

      new_cfg.theme_file = theme_name;
      // clear pending colour scheme
      strset(&new_cfg.colour_scheme, "");
      enable_widget(store_button, false);
      upd=1;
      ued=0;
    }
    when EVENT_VALCHANGE: {  // pasted or typed-in
      ued=1;
    }
    when EVENT_UNFOCUS: {  // edit end
      if(ued){
        ued=0;
        dlg_editbox_get_w(ctrl, &theme_name);
        new_cfg.theme_file = theme_name;
        enable_widget(store_button,themechanged());
        upd=1;
      }
    }
    when EVENT_DROP: {
      ued=0;
#ifdef debug_scheme
      printf("EVENT_DROP <%ls>\n", dragndrop);
#endif
      if (wcsncmp(W("data:text/plain,"), dragndrop, 16) == 0) {
        // indicate availability of downloaded scheme to be stored
        dlg_editbox_set_w(ctrl, DOWNLOADED);
        wstrset(&new_cfg.theme_file, CFG_DOWNLOADED);
        // un-URL-escape scheme description
        char * scheme = cs__wcstoutf(&dragndrop[16]);
        char * url = scheme;
        char * sch = scheme;
        while (*url) {
          int c;
          if (sscanf(url, "%%%02X", &c) == 1) {
            url += 3;
          }
          else
            c = *url++;
          if (c == '\n')
            *sch++ = ';';
          else if (c != '\r')
            *sch++ = c;
        }
        *sch = '\0';
        strset(&new_cfg.colour_scheme, scheme);
        delete(scheme);
        enable_widget(store_button, false);
      }
      else if (wcsncmp(W("http:"), dragndrop, 5) == 0
        || wcsncmp(W("https:"), dragndrop, 6) == 0
        || wcsncmp(W("ftp:"), dragndrop, 4) == 0
        || wcsncmp(W("ftps:"), dragndrop, 5) == 0
        || wcsncmp(W("file:"), dragndrop, 5) == 0
#if CYGWIN_VERSION_API_MINOR >= 74
        || (dragndrop[1] == ':' &&
            (wcsstr(dragndrop, W(".itermcolors")) ||
             wcsstr(dragndrop, W(".json"))
            )
           )
#endif
        )
      {
        char * url = cs__wcstoutf(dragndrop);
        char * sch = download_scheme(url);
        if (sch) {
          wchar * urlpoi = wcschr(dragndrop, '?');
          if (urlpoi)
            *urlpoi = 0;
          // find URL basename
          urlpoi = wcsrchr(dragndrop, '/');
          if (urlpoi) {
            // set theme name proposal to url base name
            urlpoi++;
            dlg_editbox_set_w(ctrl, urlpoi);
            wstrset(&new_cfg.theme_file, urlpoi);
            // set scheme
            strset(&new_cfg.colour_scheme, sch);

            enable_widget(store_button, true);
          }
          delete(sch);
        }
        else {
          win_bell(&new_cfg);  // Could not load web theme
          win_show_warning(_("Could not load web theme"));
        }
        delete(url);
      }
      else {
        dlg_editbox_set_w(ctrl, dragndrop);
        wstrset(&new_cfg.theme_file, dragndrop);
        enable_widget(store_button, false);
      }
    }
  }
  // apply changed theme immediately
  if (strcmp(new_cfg.colour_scheme, cfg.colour_scheme) ){
    load_schemer(&new_cfg,cfg.colour_scheme);
    upd=1;
  }else if( wcscmp(theme_name, theme_file)){
    upd=load_themer(&new_cfg,theme_name);
  }
  else if(!*theme_name) { 
    if(upd){
      CLRCPY(new_cfg.ansi_colours,cfg_colours); 
      upd=1;
    }
  }
  if(upd)InvalidateRect(GetParent((HWND)ctrl->widget),NULL,true);
  //if(upd)InvalidateRect((HWND)ctrl->widget,NULL,true);
}
#define dont_debug_dragndrop
static void
theme_saver(control *ctrl, int event)
{
  wstring theme_name = new_cfg.theme_file;
  if (event == EVENT_REFRESH) {
    enable_widget(ctrl,themechanged());
  }
  else if (event == EVENT_ACTION) {
#ifdef debug_dragndrop
    printf("%ls <- <%s>\n", new_cfg.theme_file, new_cfg.colour_scheme);
#endif
    if(!themechanged()) return;
    char * sn = get_resource_file(W("themes"), theme_name, true);
    if (!sn) {
      win_bell(&new_cfg);  // Cannot store theme file
      win_show_warning(_("Cannot store theme file"));
      return;
    }
    // save colour_scheme to theme_file
    FILE * file= fopen(sn, "w");
    delete(sn);
    if (!file) {
      win_bell(&new_cfg);  // Cannot store theme file
      win_show_warning(_("Cannot store theme file"));
      return;
    }
    void *cfg_p = &new_cfg;
    for (uint i = 0; options[i].name; i++) {
      opt_type type = options[i].type;
      if (!(type & OPT_THEME)) continue;
      fprintf(file, "%s=", options[i].name);
      printOptVar(file,type,cfg_p+options[i].offset);
      fputc('\n', file);
    }
    fclose(file);
    enable_widget(ctrl,false);
  }
}

static void
scheme_saver(control *ctrl, int event)
{
  wstring theme_name = new_cfg.theme_file;
  if (event == EVENT_REFRESH) {
    enable_widget(ctrl,
                  *new_cfg.colour_scheme && *theme_name
                  && !wcschr(theme_name, L'/') && !wcschr(theme_name, L'\\')
                 );
  }
  else if (event == EVENT_ACTION) {
#ifdef debug_dragndrop
    printf("%ls <- <%s>\n", new_cfg.theme_file, new_cfg.colour_scheme);
#endif
    if (*new_cfg.colour_scheme && *theme_name)
      if (!wcschr(theme_name, L'/') && !wcschr(theme_name, L'\\')) {
        char * sn = get_resource_file(W("themes"), theme_name, true);
        if (sn) {
          // save colour_scheme to theme_file
          FILE * thf = fopen(sn, "w");
          delete(sn);
          if (thf) {
            char * sch = (char *)new_cfg.colour_scheme;
            for (int i = 0; sch[i]; i++) {
              if (sch[i] == ';')
                sch[i] = '\n';
            }
            fprintf(thf, "%s", sch);
            fclose(thf);

            strset(&new_cfg.colour_scheme, "");
            enable_widget(store_button, false);
          }
          else {
            win_bell(&new_cfg);  // Cannot write theme file
            win_show_warning(_("Cannot write theme file"));
          }
        }
        else {
          win_bell(&new_cfg);  // Cannot store theme file
          win_show_warning(_("Cannot store theme file"));
        }
      }
  }
}

static void
bell_tester(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    win_bell(&new_cfg);
}

static void
url_opener(control *ctrl, int event)
{
  if (event == EVENT_ACTION) {
    wstring url = ctrl->context;
    win_open(wcsdup(url), true);  // win_open frees its argument
  }
  else if (event == EVENT_DROP) {
    theme_handler(theme, EVENT_DROP);
  }
}

struct fontlist {
  wstring fn;
  struct weight {
    int weight;
    wstring style;
  } * weights;
  uint weightsn;
};
static struct fontlist * fontlist = 0;
static uint fontlistn = 0;

static void
clearfontlist()
{
  for (uint fi = 0; fi < fontlistn; fi++) {
    delete(fontlist[fi].fn);
    for (uint wi = 0; wi < fontlist[fi].weightsn; wi++) {
      delete(fontlist[fi].weights[wi].style);
    }
    delete(fontlist[fi].weights);
  }
  fontlistn = 0;
  delete(fontlist);
  fontlist = 0;
}

/* Windows LOGFONT values
	100	FW_THIN
	200	FW_EXTRALIGHT
	200	FW_ULTRALIGHT
	300	FW_LIGHT
	400	FW_NORMAL
	400	FW_REGULAR
	500	FW_MEDIUM
	600	FW_SEMIBOLD
	600	FW_DEMIBOLD
	700	FW_BOLD
	800	FW_EXTRABOLD
	800	FW_ULTRABOLD
	900	FW_HEAVY
	900	FW_BLACK
   Other weight names (http://www.webtype.com/info/articles/fonts-weights/)
    100    Extra Light or Ultra Light
    200    Light or Thin
    300    Book or Demi
    400    Normal or Regular
    500    Medium
    600    Semibold, Demibold
    700    Bold
    800    Black, Extra Bold or Heavy
    900    Extra Black, Fat, Poster or Ultra Black
 */
static wstring weights[] = {
  // the first 9 weight names are used for display and name filtering
  W("Thin"),    	// 100, 200
  W("Extralight"),	// 200, 100
  W("Light"),   	// 300, 200
  W("Regular"), 	// 400
  W("Medium"),  	// 500
  W("Semibold"),	// 600
  W("Bold"),    	// 700
  W("Extrabold"),	// 800
  W("Heavy"),   	// 900, 800
  // the remaining weight names are only used for name filtering
  W("Ultralight"),	// 200, 100
  W("Normal"),  	// 400
  W("Demibold"),	// 600
  W("Ultrabold"),	// 800
  W("Black"),   	// 900, 800
  W("Book"),    	// 300, 400
  W("Demi"),    	// 300
  W("Extrablack"),	// 900
  W("Fat"),     	// 900
  W("Poster"),  	// 900
  W("Ultrablack"),	// 900
};

static string sizes[] = {
  "8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24", "28",
  "32", "36", "40", "44", "48", "56", "64", "72"
};

static void
enterfontlist(const wchar * fn, int weight, const wchar * style)
{
  if (*fn == '@') {
    delete(fn);
    // ignore vertical font
    return;
  }

  bool found = false;
  uint fi = 0;
  while (fi < fontlistn) {
    int cmp = wcscmp(fn, fontlist[fi].fn);
    if (cmp <= 0) {
      if (cmp == 0)
        found = true;
      break;
    }
    fi++;
  }

  if (found) {
    delete(fn);

    bool found = false;
    uint wi = 0;
    while (wi < fontlist[fi].weightsn) {
      int cmp = weight - fontlist[fi].weights[wi].weight;
      if (cmp <= 0) {
        if (cmp == 0)
          found = true;
        break;
      }
      wi++;
    }
    if (found)
      delete(style);
    else {
      fontlist[fi].weightsn++;
      fontlist[fi].weights = renewn(fontlist[fi].weights, fontlist[fi].weightsn);
      for (uint j = fontlist[fi].weightsn - 1; j > wi; j--)
        fontlist[fi].weights[j] = fontlist[fi].weights[j - 1];
      fontlist[fi].weights[wi].weight = weight;
      fontlist[fi].weights[wi].style = style;
    }
  }
  else {
    if (fontlist) {
      fontlistn++;
      fontlist = renewn(fontlist, fontlistn);
      for (uint j = fontlistn - 1; j > fi; j--)
        fontlist[j] = fontlist[j - 1];
    }
    else
      fontlist = newn(struct fontlist, 1);

    fontlist[fi].fn = fn;

    fontlist[fi].weightsn = 1;
    fontlist[fi].weights = newn(struct weight, 1);
    fontlist[fi].weights[0].weight = weight;
    fontlist[fi].weights[0].style = style;
  }
}

static void
display_font_sample()
{
  dlg_text_paint(font_sample);
}

static void
font_weight_handler(control *ctrl, int event)
{
  uint fi = dlg_listbox_getcur(font_list);
  if (event == EVENT_REFRESH) {
    dlg_listbox_clear(ctrl);
    if (fi < fontlistn) {
      for (uint w = 0; w < fontlist[fi].weightsn; w++)
        dlg_listbox_add_w(ctrl, fontlist[fi].weights[w].style);
    }
  }
  else if (event == EVENT_VALCHANGE || event == EVENT_SELCHANGE) {
    wstring wname = newn(wchar, 1);
    dlg_editbox_get_w(ctrl, &wname);
    int weight = FW_NORMAL;
    for (uint wi = 0; wi < fontlist[fi].weightsn; wi++)
      if (0 == wcscmp(wname, fontlist[fi].weights[wi].style)) {
        weight = fontlist[fi].weights[wi].weight;
        break;
      }
    delete(wname);
    new_cfg.font.weight = weight;
    new_cfg.font.isbold = weight >= FW_BOLD;
    display_font_sample();
  }
}

static void
font_size_handler(control *ctrl, int event)
{
  if (event == EVENT_REFRESH) {
    dlg_listbox_clear(ctrl);
    for (uint i = 0; i < lengthof(sizes); i++)
      dlg_listbox_add(ctrl, sizes[i]);
    char size[12];
    sprintf(size, "%d", new_cfg.font.size);
    dlg_editbox_set(ctrl, size);
  }
  else if (event == EVENT_VALCHANGE || event == EVENT_SELCHANGE) {
    string size = newn(char, 3);
    dlg_editbox_get(ctrl, &size);
    new_cfg.font.size = atoi(size);
    delete(size);
    display_font_sample();
  }
}

struct data_fontenum {
  HDC dc;
  bool report;
  bool outer;
};

static int CALLBACK
fontenum(const ENUMLOGFONTW *lpelf, const NEWTEXTMETRICW *lpntm, DWORD fontType, LPARAM lParam)
{
  const LOGFONTW * lfp = &lpelf->elfLogFont;
  struct data_fontenum * pdata = (struct data_fontenum *)lParam;
  (void)lpntm, (void)fontType;

  if (pdata->outer) {
    // here we recurse into the fonts of one font family
    struct data_fontenum rdata = {
      .dc = pdata->dc, .report = pdata->report, .outer = false
    };
    if ((lfp->lfPitchAndFamily & 3) == FIXED_PITCH && !lfp->lfCharSet)
      EnumFontFamiliesW(pdata->dc, lfp->lfFaceName, (FONTENUMPROCW)fontenum, (LPARAM)&rdata);
  }
  else if (!lfp->lfItalic && !lfp->lfCharSet) {
    if (lfp->lfFaceName[0] == '@')
      // skip vertical font families
      return 1;

    const wchar * tagsplit(const wchar * fn, wstring style)
    {
#if CYGWIN_VERSION_API_MINOR >= 74
      wchar * tag = wcsstr(fn, style);
      if (tag) {
        int n = wcslen(style);
        if (tag[n] <= ' ' && tag != fn && tag[-1] == ' ') {
          tag[-1] = 0;
          tag[n] = 0;
          return tag;
        }
      }
#else
      (void)fn; (void)style;
#endif
      return 0;
    }

    /**
	Courier|
	FreeMono|Medium
	Inconsolata|Medium
	Source Code Pro ExtraLight|ExtraLight
	@BatangChe|Regular
	Iosevka Term Slab Medium Obliqu|Regular
	Lucida Sans Typewriter|Bold
	TIFAX|Alpha
	HanaMinA|Regular
	DejaVu Sans Mono|Book
     */
    wchar * fn = wcsdup(lfp->lfFaceName);
    const wchar * st = tagsplit(fn, W("Oblique"));
    if ((st = tagsplit(fn, lpelf->elfStyle))) {
      //   Source Code Pro ExtraLight|ExtraLight
      //-> Source Code Pro|ExtraLight
    }
    else {
      const wchar * fnst = fn;
#if CYGWIN_VERSION_API_MINOR >= 74
      int digsi = wcscspn(fn, W("0123456789"));
      int nodigsi = wcsspn(&fn[digsi], W("0123456789"));
      if (nodigsi)
        fnst = &fn[digsi + nodigsi];
#endif
      for (uint i = 0; i < lengthof(weights); i++)
        if ((st = tagsplit(fnst, weights[i]))) {
          //   Iosevka Term Slab Medium Obliqu|Regular
          //-> Iosevka Term Slab|Medium
          break;
        }
    }
    if (!st || !*st)
      st = lpelf->elfStyle;
    if (!*st)
      st = W("Regular");
    st = wcsdup(st);
    fn = renewn(fn, wcslen(fn) + 1);

    if (pdata->report)
      printf("%03ld %ls|%ls [2m[%ls|%ls][0m\n", (long int)lfp->lfWeight, fn, st, lfp->lfFaceName, lpelf->elfStyle);
    else
      enterfontlist(fn, lfp->lfWeight, st);
  }

  return 1;
}

void
list_fonts(bool report)
{
  struct data_fontenum data = {
    .dc = GetDC(0),
    .report = report,
    .outer = true
  };

  EnumFontFamiliesW(data.dc, 0, (FONTENUMPROCW)fontenum, (LPARAM)&data);
  ReleaseDC(0, data.dc);
}

static void
font_handler(control *ctrl, int event)
{
  if (event == EVENT_REFRESH) {
    dlg_listbox_clear(ctrl);
    clearfontlist();

    list_fonts(false);
    int weighti = (new_cfg.font.weight - 50) / 100;
    if (weighti > 8)
      weighti = 8;
    else if (weighti < 0)
      weighti = 0;
    wchar * weight = wcsdup(weights[weighti]);
    enterfontlist(wcsdup(new_cfg.font.name), new_cfg.font.weight, weight);
    //sortfontlist();  // already insert-sorted above

    for (uint i = 0; i < fontlistn; i++)
      dlg_listbox_add_w(ctrl, fontlist[i].fn);

    dlg_editbox_set_w(ctrl, new_cfg.font.name);
    display_font_sample();
  }
  else if (event == EVENT_VALCHANGE || event == EVENT_SELCHANGE) {
    //int n = dlg_listbox_getcur(ctrl);
    dlg_editbox_get_w(ctrl, &new_cfg.font.name);
    font_weight_handler(font_weights, EVENT_REFRESH);
    display_font_sample();
  }
}

static void
modifier_handler(control *ctrl, int event)
{
  char *cp = ctrl->context;
  int col = ctrl->column;
  char mask = 1 << col;
  //printf("mod %02X ev %d col %d <%s>\n", *cp, event, col, ctrl->label);
  if (event == EVENT_REFRESH)
    dlg_checkbox_set(ctrl, *cp & mask);
  else if (event == EVENT_VALCHANGE)
    *cp = (*cp & ~mask) | (dlg_checkbox_get(ctrl) << col);
  //printf(" -> %02X\n", *cp);
}

static void
emojis_handler(control *ctrl, int event)
{
  //__ emojis style
  const string NONE = _("? None ?");  // ??
  string emojis = NONE;
  for (opt_val * o = opt_vals[OPT_EMOJIS]; o->name; o++) {
    if (new_cfg.emojis == o->val) {
      emojis = o->name;
      break;
    }
  }

  if (event == EVENT_REFRESH) {
    dlg_listbox_clear(ctrl);
    dlg_listbox_add(ctrl, NONE);
    add_file_resources(ctrl, W("emojis/*"), true);
    // strip std dir prefix...
    dlg_editbox_set(ctrl, *emojis ? emojis : NONE);
  }
  else if (event == EVENT_VALCHANGE || event == EVENT_SELCHANGE) {
    if (dlg_listbox_getcur(ctrl) == 0)
      new_cfg.emojis = 0;
    else {
      new_cfg.emojis = 0;
      emojis = newn(char, 1);
      dlg_editbox_get(ctrl, &emojis);
      for (opt_val * o = opt_vals[OPT_EMOJIS]; o->name; o++) {
        if (!strcasecmp(emojis, o->name)) {
          new_cfg.emojis = o->val;
          break;
        }
      }
      delete(emojis);
    }
  }
}

static void
opt_handler(control *ctrl, int event,char * popt, opt_val * ovals)
{
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      while (ovals->name) {
        dlg_listbox_add(ctrl, _(ovals->name));
        if (*popt == ovals->val)
          dlg_editbox_set(ctrl, _(ovals->name));
        ovals++;
      }
    when EVENT_VALCHANGE or EVENT_SELCHANGE: {
      int i = 0;
      while (ovals->name) {
        if (dlg_listbox_getcur(ctrl) == i++)
          *popt = ovals->val;
        ovals++;
      }
    }
  }
}

static void
emoji_placement_handler(control *ctrl, int event)
{
  opt_handler(ctrl, event, &new_cfg.emoji_placement, opt_vals[OPT_EMOJI_PLACEMENT]);
}

static void
compose_key_handler(control *ctrl, int event)
{
  opt_handler(ctrl, event, &new_cfg.compose_key, opt_vals[OPT_COMPOSE_KEY]);
}

static void
smoothing_handler(control *ctrl, int event)
{
  opt_handler(ctrl, event, &new_cfg.font_smoothing, opt_vals[OPT_FONTSMOOTH]);
}

static opt_val * const showbold_vals =
(opt_val[]) {
    {__("as font"), 1},
    {__("as colour"), 2},
    {__("as font & as colour"), 3},
    {__("xterm"), 0},
    {0, 0}
};

static char showbold;

static void
showbold_handler(control *ctrl, int event)
{
  showbold = new_cfg.bold_as_font | ((char)new_cfg.bold_as_colour) << 1;
  //printf("bold as font %d as colour %d event %d\n", new_cfg.bold_as_font, new_cfg.bold_as_colour, event);
  opt_handler(ctrl, event, &showbold, showbold_vals);
  new_cfg.bold_as_font = showbold & 1;
  new_cfg.bold_as_colour = showbold & 2;
  //printf("bold as font %d as colour %d\n", new_cfg.bold_as_font, new_cfg.bold_as_colour);
}

static bool bold_like_xterm;

static void
checkbox_option_set(control *ctrl, bool checked)
{
  if (ctrl) {
    bool *bp = ctrl->context;
    *bp = checked;
    dlg_checkbox_set(ctrl, checked);
  }
}

void
bold_handler(control *ctrl, int event)
{
  bool *bp = ctrl->context;
  static control * ctrl_bold_as_font = 0;
  static control * ctrl_bold_as_colour = 0;
  static control * ctrl_bold_like_xterm = 0;
  if (event == EVENT_REFRESH) {
    bold_like_xterm = !new_cfg.bold_as_font && !new_cfg.bold_as_colour;
    dlg_checkbox_set(ctrl, *bp);
    if (bp == &new_cfg.bold_as_font)
      ctrl_bold_as_font = ctrl;
    else if (bp == &new_cfg.bold_as_colour)
      ctrl_bold_as_colour = ctrl;
    else
      ctrl_bold_like_xterm = ctrl;
  }
  else if (event == EVENT_VALCHANGE) {
    *bp = dlg_checkbox_get(ctrl);
    if (bp == &bold_like_xterm) {
      if (*bp) {
        checkbox_option_set(ctrl_bold_as_font, false);
        checkbox_option_set(ctrl_bold_as_colour, false);
      }
      else {
        //checkbox_option_set(ctrl_bold_as_font, false);
        //checkbox_option_set(ctrl_bold_as_colour, true);
        // disable switching off: restore "true"
        checkbox_option_set(ctrl_bold_like_xterm, true);
      }
    }
    else if (!new_cfg.bold_as_font && !new_cfg.bold_as_colour)
      checkbox_option_set(ctrl_bold_like_xterm, true);
    else
      checkbox_option_set(ctrl_bold_like_xterm, false);
  }
}

void
transparency_valhandler(control *ctrl, int event)
{
  if (event == EVENT_VALCHANGE) {
    string val = 0;
    dlg_editbox_get(ctrl, &val);
    new_cfg.transparency = atoi(val);
    delete(val);
  }
  else if (event == EVENT_UNFOCUS) {
    string val = 0;
    dlg_editbox_get(ctrl, &val);
    int transp = atoi(val);
    delete(val);
    // adjust value if out of range
    if (transp < 4)
      transp = 0;
    else if (transp > 254)
      transp = 254;
    new_cfg.transparency = transp;
    // refresh box in case we changed it
    char buf[16];
    sprintf(buf, "%i", transp);
    dlg_editbox_set(ctrl, buf);
    // update radio buttons
    dlg_stdradiobutton_handler(transparency_selbox, EVENT_REFRESH);
  }
  else if (event == EVENT_REFRESH) {
    char buf[16];
    sprintf(buf, "%i", (uchar)new_cfg.transparency);
    dlg_editbox_set(ctrl, buf);
  }
}

void
transparency_selhandler(control *ctrl, int event)
{
  dlg_stdradiobutton_handler(ctrl, event);
  if (event == EVENT_VALCHANGE) {
    transparency_valhandler(transparency_valbox, EVENT_REFRESH);
  }
}

static void
transparency_slider(control *ctrl, int event)
{
  string dir = ctrl->context;
  mod_keys mods = get_mods();
  if (event == EVENT_ACTION) {
    int step = *dir == '-' ? -4 : 4;
    if (mods & MDK_SHIFT)
      step *= 4;
    else if (mods & MDK_CTRL)
      step /= 4;
    int transp = (uchar)new_cfg.transparency;
    transp += step;
    if (transp < 4)
      transp = step > 0 ? 4 : 0;
    else if (transp > 254)
      transp = 254;
    new_cfg.transparency = transp;
    transparency_valhandler(transparency_valbox, EVENT_REFRESH);
    // call the lower-level function for update, to avoid recursion
    dlg_stdradiobutton_handler(transparency_selbox, EVENT_REFRESH);
  }
}

static void
ansicolour_handler(control *ctrl, int event)
{
  colour c = *(colour *)ctrl->context;
  dlg_stdcolour_handler(ctrl, event);
  if (event == EVENT_CALLBACK) {
    colour cn = *(colour *)ctrl->context;
    if(c!=cn){
      InvalidateRect(ctrl->widget,NULL,1);
      enable_widget(store_button,themechanged());
      //InvalidateRect(GetParent(ctrl->widget),NULL,1);
    }
  }
}

void
setup_config_box(controlbox * b)
{
  controlset *s;
  control *c;
  copy_config("dialog", &new_cfg, &file_cfg);
  if(*theme_file)CLRCPY(new_cfg.ansi_colours,theme_colours );
 /*
  * The standard panel that appears at the bottom of all panels:
  * Open, Cancel, Apply etc.
  */
  s = ctrl_new_set(b, W(""), W(""), W(""));
  ctrl_columns(s, 5, 20, 20, 20, 20, 20);
  c = ctrl_pushbutton(s,0, _W("About..."),0, about_handler, 0);
  c = ctrl_pushbutton(s,2, _W("&Save"),0, ok_handler, 0);
  c->button.isdefault = true;
  c = ctrl_pushbutton(s,3, _W("&Cancel"),0, cancel_handler, 0);
  c->button.iscancel = true;
  c = ctrl_pushbutton(s,4, _W("&Apply"),0, apply_handler, 0);
#ifdef __gettext
  __W("I see");
  __W("OK");
#endif

 /*
  * The Looks panel.
  */
  s = ctrl_new_set(b, _W("Looks"), _W("Looks in Terminal"), _W("Colours"));
  ctrl_columns(s, 3, 33, 33, 33);
  ctrl_pushbutton( s,0, _W("&Foreground..."),0, dlg_stdcolour_handler, &new_cfg.fg_colour);
  ctrl_pushbutton( s,1, _W("&Background..."),0, dlg_stdcolour_handler, &new_cfg.bg_colour);
  ctrl_pushbutton( s,2, _W("&Cursor..."    ),0, dlg_stdcolour_handler, &new_cfg.cursor_colour);

  ctrl_columns(s, 2, 70,30);
  theme = ctrl_combobox( s,0, _W("&Theme"),0, 70, theme_handler, &new_cfg.theme_file);
  store_button = ctrl_pushbutton(s,1, _W("T&heme Save"),0, theme_saver, 0) ;
  //ctrl_columns(s, 1, 100);
  //ctrl_label    ( s, 0, _W("Modify Ansi Color:"),0);
  ctrl_columns(s, 9,15,10,10,10,10, 10,10,10,10);
  ctrl_label    ( s, 0, _W("Ansi:"),0);
  ctrl_clrbutton( s,1,0, _W("Black"       ), ansicolour_handler, &new_cfg.ansi_colours[ 0]);
  ctrl_clrbutton( s,2,0, _W("Red"         ), ansicolour_handler, &new_cfg.ansi_colours[ 1]);
  ctrl_clrbutton( s,3,0, _W("Green"       ), ansicolour_handler, &new_cfg.ansi_colours[ 2]);
  ctrl_clrbutton( s,4,0, _W("Yellow"      ), ansicolour_handler, &new_cfg.ansi_colours[ 3]);
  ctrl_clrbutton( s,5,0, _W("Blue"        ), ansicolour_handler, &new_cfg.ansi_colours[ 4]);
  ctrl_clrbutton( s,6,0, _W("Magenta"     ), ansicolour_handler, &new_cfg.ansi_colours[ 5]);
  ctrl_clrbutton( s,7,0, _W("Cyan"        ), ansicolour_handler, &new_cfg.ansi_colours[ 6]);
  ctrl_clrbutton( s,8,0, _W("White"       ), ansicolour_handler, &new_cfg.ansi_colours[ 7]);
  ctrl_columns(s, 9,15,10,10,10,10, 10,10,10,10);
  ctrl_label    ( s, 0, _W("BOLD:"),0);
  ctrl_clrbutton( s,1,0, _W("BoldBlack"  ), ansicolour_handler, &new_cfg.ansi_colours[ 8]);
  ctrl_clrbutton( s,2,0, _W("BoldRed"    ), ansicolour_handler, &new_cfg.ansi_colours[ 9]);
  ctrl_clrbutton( s,3,0, _W("BoldGreen"  ), ansicolour_handler, &new_cfg.ansi_colours[10]);
  ctrl_clrbutton( s,4,0, _W("BoldYellow" ), ansicolour_handler, &new_cfg.ansi_colours[11]);
  ctrl_clrbutton( s,5,0, _W("BoldBlue"   ), ansicolour_handler, &new_cfg.ansi_colours[12]);
  ctrl_clrbutton( s,6,0, _W("BoldMagenta"), ansicolour_handler, &new_cfg.ansi_colours[13]);
  ctrl_clrbutton( s,7,0, _W("BoldCyan"   ), ansicolour_handler, &new_cfg.ansi_colours[14]);
  ctrl_clrbutton( s,8,0, _W("BoldWhite"  ), ansicolour_handler, &new_cfg.ansi_colours[15]);
  ctrl_columns(s, 2, 80, 20);
  (void)url_opener;(void)scheme_saver;
  //ctrl_pushbutton(s,0, _W("Color Scheme Designer"),0, url_opener, W("http://ciembor.github.io/4bit/")) ;
  //store_button = ctrl_pushbutton(s,1, _W("Store"),0, scheme_saver, 0) ;

  s = ctrl_new_set(b, _W("Looks"), null, _W("Transparency"));
  bool with_glass = win_is_glass_available();
  transparency_selbox = ctrl_radiobuttons( s,-1, null,0, 4 + with_glass,
    transparency_selhandler, &new_cfg.transparency,
    _W("&Off"), TR_OFF,
    _W("&Low"), TR_LOW,
    with_glass ? _W("&Med.") : _W("&Medium"), TR_MEDIUM,
    _W("&High"), TR_HIGH,
    with_glass ? _W("Gla&ss") : null, TR_GLASS,
    null
  );
#ifdef support_blurred
  ctrl_columns(s, 2, with_glass ? 80 : 75, with_glass ? 20 : 25);
  ctrl_checkbox( s,0, _W("Opa&que when focused"),0, dlg_stdcheckbox_handler, &new_cfg.opaque_when_focused);
  ctrl_checkbox( s,1, _W("Blu&r"),0, dlg_stdcheckbox_handler, &new_cfg.blurred);
#else
#ifdef no_transparency_pseudo_slider
  ctrl_checkbox( s,-1, _W("Opa&que when focused"),0, dlg_stdcheckbox_handler, &new_cfg.opaque_when_focused);
#else
  ctrl_columns(s, 4, 64, 10, 16, 10);
  ctrl_checkbox( s,0, _W("Opa&que when focused"),0, dlg_stdcheckbox_handler, &new_cfg.opaque_when_focused);
  transparency_valbox = ctrl_editbox( s,2, 0,0, 100, transparency_valhandler, &new_cfg.transparency);
  ctrl_pushbutton( s,1, _W("◄"),0, transparency_slider, "-");
  ctrl_pushbutton( s,3, _W("►"),0, transparency_slider, "+");
#endif
#endif

  s = ctrl_new_set(b, _W("Looks"), null, _W("Cursor"));
  ctrl_radiobuttons( s,-1, null,0, 4,
    dlg_stdradiobutton_handler, &new_cfg.cursor_type,
    _W("Li&ne"), CUR_LINE,
    _W("Bloc&k"), CUR_BLOCK,
#ifdef cursor_type_box
    _W("Bo&x"), CUR_BOX,
#endif
    _W("&Underscore"), CUR_UNDERSCORE,
    null
  );
  ctrl_checkbox( s,-1, _W("Blinkin&g"),0, dlg_stdcheckbox_handler, &new_cfg.cursor_blinks);

 /*
  * The Text panel.
  */
  s = ctrl_new_set(b, _W("Text"), _W("Text and Font properties"), _W("Font"));
  if (cfg.fontmenu == 0) {  // use built-in inline font menu
    ctrl_columns(s, 2, 70, 30);
    font_list = ctrl_listbox( s,0, null,0, 4, 100, font_handler, 0);
    font_weights = ctrl_listbox( s,1, _W("Font st&yle:"),0, 3, 100, font_weight_handler, 0);
    ctrl_columns(s, 2, 70, 30);
    ctrl_combobox( s,1, _W("&Size:"),0, 50, font_size_handler, 0);
    font_sample = ctrl_pushbutton(s,0, null,0, apply_handler, 0);
    // emoji style here, right after font?
    if (strstr(cfg.old_options, "bold")) {
      s = ctrl_new_set(b, _W("Text"), null, null);
      ctrl_columns(s, 2, 50, 50);
      ctrl_checkbox( s,0, _W("Sho&w bold as font"),0, dlg_stdcheckbox_handler, &new_cfg.bold_as_font);
      ctrl_checkbox( s,1, _W("Show &bold as colour"),0, dlg_stdcheckbox_handler, &new_cfg.bold_as_colour);
    }
    else {
     if (0 != strstr(cfg.old_options, "blinking")) {
      s = ctrl_new_set(b, _W("Text"), null, _W("Show bold"));
      ctrl_columns(s, 3, 35, 35, 30);
      ctrl_checkbox( s,0, _W("as font"),0, bold_handler, &new_cfg.bold_as_font);
      ctrl_checkbox( s,1, _W("as colour"),0, bold_handler, &new_cfg.bold_as_colour);
      ctrl_checkbox( s,2, _W("xterm"),0, bold_handler, &bold_like_xterm);
     }
     else {
      ctrl_combobox( s,-1, _W("Show bold"),0,
        50, showbold_handler, 0);
      ctrl_checkbox( s,-1, _W("&Allow blinking"),0,
        dlg_stdcheckbox_handler, &new_cfg.allow_blinking);
     }
    }
  }
  else {
    ctrl_fontsel( s,-1, null,0, dlg_stdfontsel_handler, &new_cfg.font);

    // emoji style here, right after font?

    if (strstr(cfg.old_options, "bold")) {
      s = ctrl_new_set(b, _W("Text"), null, null);
      ctrl_columns(s, 2, 50, 50);
      ctrl_radiobuttons( s,1, _W("Font smoothing"),0, 2,
        dlg_stdradiobutton_handler, &new_cfg.font_smoothing,
        _W("&Default"), FS_DEFAULT,
        _W("&None"), FS_NONE,
        _W("&Partial"), FS_PARTIAL,
        _W("&Full"), FS_FULL,
        null
      );

      ctrl_checkbox( s,0, _W("Sho&w bold as font"),0, dlg_stdcheckbox_handler, &new_cfg.bold_as_font);
      ctrl_checkbox( s,0, _W("Show &bold as colour"),0, dlg_stdcheckbox_handler, &new_cfg.bold_as_colour);
      ctrl_checkbox( s,0, _W("&Allow blinking"),0, dlg_stdcheckbox_handler, &new_cfg.allow_blinking);
    }
    else {
     if (0 != strstr(cfg.old_options, "blinking")) {
      ctrl_radiobuttons( s,0, _W("Font smoothing"),0, 4,
        dlg_stdradiobutton_handler, &new_cfg.font_smoothing,
        _W("&Default"), FS_DEFAULT,
        _W("&None"), FS_NONE,
        _W("&Partial"), FS_PARTIAL,
        _W("&Full"), FS_FULL,
        null
      );

      s = ctrl_new_set(b, _W("Text"), null, _W("Show bold"));
      ctrl_columns(s, 3, 35, 35, 30);
      ctrl_checkbox( s,0, _W("as font"),0, bold_handler, &new_cfg.bold_as_font);
      ctrl_checkbox( s,1, _W("as colour"),0, bold_handler, &new_cfg.bold_as_colour);
      ctrl_checkbox( s,2, _W("xterm"),0, bold_handler, &bold_like_xterm);
     }
     else {
      ctrl_combobox( s,-1, _W("Font smoothing"),0, 50, smoothing_handler, 0);
      s = ctrl_new_set(b, _W("Text"), null, null);
      ctrl_combobox( s,-1, _W("Show bold"),0,
        50, showbold_handler, 0);
      ctrl_checkbox( s,-1, _W("&Allow blinking"),0,
        dlg_stdcheckbox_handler, &new_cfg.allow_blinking);
     }
    }
  }

  s = ctrl_new_set(b, _W("Text"), null, null);
  ctrl_columns(s, 2, 29, 71);
  locale_box = ctrl_combobox( s,0, _W("&Locale"),0, 100, locale_handler, 0);
  charset_box = ctrl_combobox( s,1, _W("&Character set"),0, 100, charset_handler, 0);

  // emoji style here, after locale?
  if (!strstr(cfg.old_options, "emoj")) {
    if (cfg.fontmenu == 0 && !strstr(cfg.old_options, "bold")) {
      // save some space
      s = ctrl_new_set(b, _W("Text"), null, null);
      ctrl_columns(s, 2, 50, 50);
      ctrl_combobox( s,0, _W("Emojis"),0, 100, emojis_handler, 0);
    }
    else {
      s = ctrl_new_set(b, _W("Text"), null, _W("Emojis"));
      ctrl_columns(s, 2, 50, 50);
      ctrl_combobox( s,0, _W("Style"),0, 100, emojis_handler, 0);
    }
    ctrl_combobox( s,1, _W("Placement"),0, 100, emoji_placement_handler, 0);
  }

 /*
  * The Keys panel.
  */
  s = ctrl_new_set(b, _W("Keys"), _W("Keyboard features"), null);
  ctrl_columns(s, 2, 50, 50);
  ctrl_checkbox( s,0, _W("&Backarrow sends ^H"),0, dlg_stdcheckbox_handler, &new_cfg.backspace_sends_bs);
  ctrl_checkbox( s,1, _W("&Delete sends DEL"),0, dlg_stdcheckbox_handler, &new_cfg.delete_sends_del);
  ctrl_checkbox( s,-1, _W("Ctrl+LeftAlt is Alt&Gr"),0, dlg_stdcheckbox_handler, &new_cfg.ctrl_alt_is_altgr);
  ctrl_checkbox( s,-1, _W("AltGr is also Alt"),0, dlg_stdcheckbox_handler, &new_cfg.altgr_is_alt);

  s = ctrl_new_set(b, _W("Keys"), null, _W("Shortcuts"));
  ctrl_checkbox( s,-1, _W("Cop&y and Paste (Ctrl/Shift+Ins)"),0, dlg_stdcheckbox_handler, &new_cfg.clip_shortcuts);
  ctrl_checkbox( s,-1, _W("&Menu and Full Screen (Alt+Space/Enter)"),0, dlg_stdcheckbox_handler, &new_cfg.window_shortcuts);
  ctrl_checkbox( s,-1, _W("&Switch window (Ctrl+[Shift+]Tab)"),0, dlg_stdcheckbox_handler, &new_cfg.switch_shortcuts);
  ctrl_checkbox( s,-1, _W("&Zoom (Ctrl+plus/minus/zero)"),0, dlg_stdcheckbox_handler, &new_cfg.zoom_shortcuts);
  ctrl_checkbox( s,-1, _W("&Alt+Fn shortcuts"),0, dlg_stdcheckbox_handler, &new_cfg.alt_fn_shortcuts);
  ctrl_checkbox( s,-1, _W("&Win+x shortcuts"),0, dlg_stdcheckbox_handler, &new_cfg.win_shortcuts);
  ctrl_checkbox( s,-1, _W("&Ctrl+Shift+letter shortcuts"),0, dlg_stdcheckbox_handler, &new_cfg.ctrl_shift_shortcuts);

  if (strstr(cfg.old_options, "composekey")) {
    s = ctrl_new_set(b, _W("Keys"), null, _W("Compose key"));
    ctrl_radiobuttons( s,-1, null,0, 4,
      dlg_stdradiobutton_handler, &new_cfg.compose_key,
      _W("&Shift"), MDK_SHIFT,
      _W("&Ctrl"), MDK_CTRL,
      _W("&Alt"), MDK_ALT,
      _W("&Off"), 0,
      null
    );
  }
  else {
    s = ctrl_new_set(b, _W("Keys"), null, null);
    ctrl_combobox(
      s,-1, _W("Compose key"),0, 50, compose_key_handler, 0);
  }

 /*
  * The Mouse panel.
  */
  s = ctrl_new_set(b, _W("Mouse"), _W("Mouse functions"), null);
  ctrl_columns(s, 2, 50, 50);
  if (strstr(cfg.old_options, "sel")) {
//#define copy_as_html_checkbox
//#define copy_as_html_right
#ifdef copy_as_html_checkbox
    ctrl_checkbox( s,0, _W("Cop&y on select"),0, dlg_stdcheckbox_handler, &new_cfg.copy_on_select);
    ctrl_checkbox( s,0, _W("Copy with TABs"),0, dlg_stdcheckbox_handler, &new_cfg.copy_tabs);
    ctrl_checkbox( s,1, _W("Copy as &rich text"),0, dlg_stdcheckbox_handler, &new_cfg.copy_as_rtf);
    ctrl_columns(s, 2, 50, 50);
    ctrl_checkbox( s,1, _W("Copy as &HTML"),0, dlg_stdcheckbox_handler, &new_cfg.copy_as_html);
#else
#ifdef copy_as_html_right
    ctrl_radiobuttons( s,1, _W("Copy as &HTML"),0, 2,
      dlg_stdradiobutton_handler, &new_cfg.copy_as_html,
      _W("&None"), 0,
      _W("&Partial"), 1,
      _W("&Default"), 2,
      _W("&Full"), 3,
      null
    );
    ctrl_checkbox( s,0, _W("Cop&y on select"),0, dlg_stdcheckbox_handler, &new_cfg.copy_on_select);
    ctrl_checkbox( s,0, _W("Copy with TABs"),0, dlg_stdcheckbox_handler, &new_cfg.copy_tabs);
    ctrl_checkbox( s,0, _W("Copy as &rich text"),0, dlg_stdcheckbox_handler, &new_cfg.copy_as_rtf);
#else
    ctrl_checkbox( s,0, _W("Cop&y on select"),0, dlg_stdcheckbox_handler, &new_cfg.copy_on_select);
    // no space for "Copy with TABs"
    ctrl_checkbox( s,1, _W("Copy as &rich text"),0, dlg_stdcheckbox_handler, &new_cfg.copy_as_rtf);
    ctrl_columns(s, 2, 100, 0);
    ctrl_radiobuttons( s,-1, _W("Copy as &HTML"),0, 4,
      dlg_stdradiobutton_handler, &new_cfg.copy_as_html,
      _W("&None"), 0,
      _W("&Partial"), 1,
      _W("&Default"), 2,
      _W("&Full"), 3,
      null
    );
#endif
#endif
  }
  ctrl_checkbox( s,-1, _W("Clic&ks place command line cursor"),0, dlg_stdcheckbox_handler, &new_cfg.clicks_place_cursor);
  ctrl_editbox( s,-1, _W("Delimiters:"), 0,70, dlg_stdstringbox_handler, &new_cfg.word_chars_excl );
  ctrl_editbox( s,-1, _W("Word Characters:"), 0,70, dlg_stdstringbox_handler, &new_cfg.word_chars);

  s = ctrl_new_set(b, _W("Mouse"), null, _W("Click actions"));
  ctrl_radiobuttons( s,-1, _W("Right mouse button"),0, 4,
    dlg_stdradiobutton_handler, &new_cfg.right_click_action,
    _W("&Paste"), RC_PASTE,
    _W("E&xtend"), RC_EXTEND,
    _W("&Menu"), RC_MENU,
    _W("Ente&r"), RC_ENTER,
    null
  );
  ctrl_radiobuttons( s,-1, _W("Middle mouse button"),0, 4,
    dlg_stdradiobutton_handler, &new_cfg.middle_click_action,
    _W("&Paste"), MC_PASTE,
    _W("E&xtend"), MC_EXTEND,
    _W("&Nothing"), MC_VOID,
    _W("Ente&r"), MC_ENTER,
    null
  );

  s = ctrl_new_set(b, _W("Mouse"), null, _W("Application mouse mode"));
  ctrl_radiobuttons( s,-1, _W("Default click target"),0, 4,
    dlg_stdradiobutton_handler, &new_cfg.clicks_target_app,
    _W("&Window"), false,
    _W("&Application"), true,
    null
  );
#define appl_override_buttons
#ifdef appl_override_buttons
  ctrl_radiobuttons( s,-1, _W("Modifier for overriding default"),0, 5,
    dlg_stdradiobutton_handler, &new_cfg.click_target_mod,
    _W("&Shift"), MDK_SHIFT,
    _W("&Ctrl"), MDK_CTRL,
    _W("&Alt"), MDK_ALT,
    _W("&Win"), MDK_WIN,
    _W("&Off"), 0,
    null
  );
#else
#warning needs some coding
  ctrl_label( s,-1, _W("Modifier for overriding default"),0);
  ctrl_columns(s, 6, 20, 16, 16, 16, 16, 16);
  ctrl_checkbox( s,0, _W("&Shift"),0, modifier_handler, &new_cfg.click_target_mod);
  ctrl_checkbox( s,1, _W("&Alt"),0, modifier_handler, &new_cfg.click_target_mod);
  ctrl_checkbox( s,2, _W("&Ctrl"),0, modifier_handler, &new_cfg.click_target_mod);
  ctrl_checkbox( s,3, _W("&Win"),0, modifier_handler, &new_cfg.click_target_mod);
  ctrl_checkbox( s,4, _W("&Sup"),0, modifier_handler, &new_cfg.click_target_mod);
  ctrl_checkbox( s,5, _W("&Hyp"),0, modifier_handler, &new_cfg.click_target_mod);
  ctrl_columns(s, 1, 100);  // reset column stuff so we can rearrange them
#endif

  if (!strstr(cfg.old_options, "sel")) {
   /*
    * The Selection and clipboard panel.
    */
    s = ctrl_new_set(b, _W("Selection"), _W("Selection and clipboard"), null);
    ctrl_columns(s, 2, 100, 0);
    ctrl_checkbox( s,-1, _W("Clear selection on input"),0, dlg_stdcheckbox_handler, &new_cfg.input_clears_selection);

#define copy_as_html_single_line

    s = ctrl_new_set(b, _W("Selection"), null, _W("Clipboard"));
    ctrl_columns(s, 2, 50, 50);
    ctrl_checkbox( s,0, _W("Cop&y on select"),0, dlg_stdcheckbox_handler, &new_cfg.copy_on_select);
    ctrl_checkbox( s,0, _W("Copy with TABs"),0, dlg_stdcheckbox_handler, &new_cfg.copy_tabs);
    ctrl_columns(s, 2, 50, 50);
    ctrl_checkbox( s,0, _W("Copy as &rich text"),0,
      dlg_stdcheckbox_handler, &new_cfg.copy_as_rtf
    );
#ifndef copy_as_html_single_line
    ctrl_radiobuttons( s,1, _W("Copy as &HTML"),0, 2,
      dlg_stdradiobutton_handler, &new_cfg.copy_as_html,
      _W("&None"), 0,
      _W("&Partial"), 1,
      _W("&Default"), 2,
      _W("&Full"), 3,
      null
    );
#else
    ctrl_columns(s, 2, 100, 0);
    ctrl_radiobuttons( s,-1, _W("Copy as &HTML"),0, 4,
      dlg_stdradiobutton_handler, &new_cfg.copy_as_html,
      _W("&None"), 0,
      _W("&Partial"), 1,
      _W("&Default"), 2,
      _W("&Full"), 3,
      null
    );
#endif

    ctrl_columns(s, 2, 50, 50);
    ctrl_checkbox( s,-1, _W("Trim space from selection"),0, dlg_stdcheckbox_handler, &new_cfg.trim_selection);
    ctrl_checkbox( s,-1, _W("Allow setting selection"),0, dlg_stdcheckbox_handler, &new_cfg.allow_set_selection);

    s = ctrl_new_set(b, _W("Selection"), null, _W("Window"));
    ctrl_columns(s, 2, 100, 0);
    // window-related
    //__ Options - Selection: clock position of info popup for text size
    ctrl_editbox( s,-1, _W("Show size while selecting (0..12)"), 0,15, dlg_stdintbox_handler, &new_cfg.selection_show_size);
#define dont_config_suspbuf
#ifdef config_suspbuf
    ctrl_editbox( s,-1, _W("Suspend output while selecting"),0, 24, dlg_stdintbox_handler, &new_cfg.suspbuf_max);
#endif
  }

 /*
  * The Window panel.
  */
  s = ctrl_new_set(b, _W("Window"), _W("Window properties"), _W("Default size"));
  ctrl_columns(s, 5, 35, 4, 28, 3, 30);
  cols_box = ctrl_editbox( s,0, _W("Colu&mns"),0, 44, dlg_stdintbox_handler, &new_cfg.cols);
  rows_box = ctrl_editbox( s,2, _W("Ro&ws"),0, 55, dlg_stdintbox_handler, &new_cfg.rows);
  ctrl_pushbutton( s,4, _W("C&urrent size"),0, current_size_handler, 0);

  ctrl_columns(s, 1, 100);
  ctrl_checkbox(
    //__ Options - Window:
    s, 0,_W("Re&wrap on resize"),0,
    dlg_stdcheckbox_handler, &new_cfg.rewrap_on_resize
  );
  s = ctrl_new_set(b, _W("Window"), null, null);
  ctrl_columns(s, 2, 80, 20);
  ctrl_editbox( s,0, _W("Scroll&back lines"),0, 50, dlg_stdintbox_handler, &new_cfg.scrollback_lines);
  ctrl_radiobuttons( s,-1, _W("Scrollbar"),0, 4,
    dlg_stdradiobutton_handler, &new_cfg.scrollbar,
    _W("&Left"), -1,
    _W("&None"), 0,
    _W("&Right"), 1,
    null
  );
#ifdef scroll_mod_buttons
  ctrl_radiobuttons( s,-1, _W("Modifier for scrolling"),0, 5,
    dlg_stdradiobutton_handler, &new_cfg.scroll_mod,
    _W("&Shift"), MDK_SHIFT,
    _W("&Ctrl"), MDK_CTRL,
    _W("&Alt"), MDK_ALT,
    _W("&Win"), MDK_WIN,
    _W("&Off"), 0,
    null
  );
#else
  ctrl_columns(s, 1, 100);  // reset column stuff so we can rearrange them
  ctrl_label( s,-1, _W("Modifier for scrolling"),0);
  ctrl_columns(s, 6, 20, 16, 16, 16, 16, 16);
  ctrl_checkbox( s,0, _W("&Shift"),0, modifier_handler, &new_cfg.scroll_mod);
  ctrl_checkbox( s,1, _W("&Alt"),0, modifier_handler, &new_cfg.scroll_mod);
  ctrl_checkbox( s,2, _W("&Ctrl"),0, modifier_handler, &new_cfg.scroll_mod);
  ctrl_checkbox( s,3, _W("&Win"),0, modifier_handler, &new_cfg.scroll_mod);
  ctrl_checkbox( s,4, _W("&Sup"),0, modifier_handler, &new_cfg.scroll_mod);
  ctrl_checkbox( s,5, _W("&Hyp"),0, modifier_handler, &new_cfg.scroll_mod);
#endif
  ctrl_checkbox( s,-1, _W("&PgUp and PgDn scroll without modifier"),0, dlg_stdcheckbox_handler, &new_cfg.pgupdn_scroll);
  ctrl_checkbox( s,-1, _W("&AllocConsole ,Some console programs need it."),0, dlg_stdcheckbox_handler, &new_cfg.allocconsole);

  s = ctrl_new_set(b, _W("Window"), null, _W("UI language"));
  ctrl_columns(s, 2, 60, 40);
  ctrl_combobox( s,0, null,0, 100, lang_handler, 0);

 /*
  * The Terminal panel.
  */
  s = ctrl_new_set(b, _W("Terminal"), _W("Terminal features"), null);
  ctrl_columns(s, 2, 50, 50);
  ctrl_combobox( s,0, _W("&Type"),0, 100, term_handler, 0);
    //__ Options - Terminal: answerback string for ^E request
  ctrl_editbox( s,1, _W("&Answerback"),0, 100, dlg_stdstringbox_handler, &new_cfg.answerback);

  s = ctrl_new_set(b, _W("Terminal"), null, _W("Bell"));
  ctrl_columns(s, 2, 73, 27);
  ctrl_combobox( s,0, null,0, 100, bell_handler, 0);
  ctrl_pushbutton( s,1, _W("► &Play"),0, bell_tester, 0);
  ctrl_columns(s, 1, 100);  // reset column stuff so we can rearrange them
  ctrl_columns(s, 2, 100, 0);
  ctrl_combobox( s,0, _W("&Wave"),0, 83, bellfile_handler, &new_cfg.bell_file[6]);
  ctrl_columns(s, 1, 100);  // reset column stuff so we can rearrange them
  // balance column widths of the following 3 fields 
  // to accommodate different length of localized labels
  int strwidth(wstring s0) {
    int len = 0;
    wchar * sp = (wchar *)s0;
    while (*sp) {
      if ((*sp >= 0xE3 && *sp <= 0xED) || 
          (*sp == 0xF0 && *(sp + 1) >= 0xA0 && *(sp + 1) <= 0xBF))
        // approx. CJK range
        len += 4;
      else if (strchr(" il.,'()!:;[]|", *sp))
        len ++;
      else if (*sp != '&' && (*sp & 0xC0) != 0x80)
        len += 2;
      sp++;
    }
    return len;
  }
  wstring lbl_flash = _W("&Flash");
  wstring lbl_highl = _W("&Highlight in taskbar");
  wstring lbl_popup = _W("&Popup");
  int len = strwidth(lbl_flash) + strwidth(lbl_highl) + strwidth(lbl_popup);
# define cbw 14
  int l00_flash = (100 - 3 * cbw) * strwidth(lbl_flash) / len + cbw;
  int l00_highl = (100 - 3 * cbw) * strwidth(lbl_highl) / len + cbw;
  int l00_popup = (100 - 3 * cbw) * strwidth(lbl_popup) / len + cbw;
  ctrl_columns(s, 3, l00_flash, l00_highl, l00_popup);
  ctrl_checkbox( s,0, _W("&Flash"),0, dlg_stdcheckbox_handler, &new_cfg.bell_flash);
  ctrl_checkbox( s,1, _W("&Highlight in taskbar"),0, dlg_stdcheckbox_handler, &new_cfg.bell_taskbar);
  ctrl_checkbox( s,2, _W("&Popup"),0, dlg_stdcheckbox_handler, &new_cfg.bell_popup);

  s = ctrl_new_set(b, _W("Terminal"), null, _W("Printer"));
#ifdef use_multi_listbox_for_printers
#warning left in here just to demonstrate the usage of ctrl_listbox
  ctrl_listbox( s,-1, null,0, 4, 100, printer_handler, 0);
#else
  ctrl_combobox( s,-1, null,0, 100, printer_handler, 0);
#endif
  s = ctrl_new_set(b, _W("Terminal"), null, null);
    //__ Options - Terminal:
  ctrl_checkbox( s,-1, _W("Prompt about running processes on &close"),0, dlg_stdcheckbox_handler, &new_cfg.confirm_exit);
    //__ Options - Terminal:
  ctrl_checkbox( s,-1, _W("Status Line"),0, dlg_stdcheckbox_handler, &new_cfg.status_line);
}
