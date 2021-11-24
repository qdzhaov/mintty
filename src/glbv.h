
================ wintext.c 
// ZNT5
// colour values; this should perhaps be part of struct term
COLORREF colours[COLOUR_NUM];//ZNT2

// diagnostic information flag
bool show_charinfo = false;

// master font family properties
LOGFONT lfont;
// logical font size, as configured (< 0: pixel size)
int font_size;
// scaled font size; pure font height, without spacing
static int font_height;
// border padding:
int PADDING = 1;
int OFFSET = 0;
// width mode
bool font_ambig_wide;
 fontfamilies[11];
int line_scale;
//=========
static HDC dc;// need set to local 
//do_update,
//do_update,
  dc = GetDC(wnd);
  win_paint_exclude_search(dc);
  term_update_search();//no dc
  if (tek_mode)
    tek_paint();// HDC dc = GetDC(wnd);
  else {
    term_paint();//win_text
    winimgs_paint();// HDC dc = GetDC(wnd);
  }
  win_tab_paint(dc);
  ReleaseDC(wnd, dc);
//win_paint(void)
  dc = BeginPaint(wnd, &p);
  win_tab_actv();
  if (update_state != UPDATE_PENDING) {
    if (tek_mode)
      tek_paint();
    else {
      term_paint();
      winimgs_paint();
    }
  }
  win_tab_paint(dc);
  EndPaint(wnd, &p);
//win_text use 

win_char_width() //need Opt
static enum { UPDATE_IDLE, UPDATE_BLOCKED, UPDATE_PENDING } update_state;
static bool ime_open = false;

static int update_skipped = 0;
int lines_scrolled = 0;   //ZNTK 

#define dont_debug_cursor 1
//ZNT0
static struct charnameentry {
  xchar uc;
  string un;
} * charnametable = null;
static int charnametable_len = 0;
static int charnametable_alloced = 0;
static bool charnametable_init = false;
//======
//ZNT1
static bool tiled = false;
static bool ratio = false;
static bool wallp = false;
static int wallp_style;
static int alpha = -1;
static LONG w = 0, h = 0;
static HBRUSH bgbrush_bmp = 0;
//AAAAAAAAA
static GpBrush * bgbrush_img = 0;  
static GpGraphics * bg_graphics = 0;
// ===============


static SCRIPT_STRING_ANALYSIS ssa;
static bool use_uniscribe;//ZNT 5
================ wininput.c
//ZNT0 
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
static alt_state_t alt_state;
static uint alt_code;
static bool lctrl;  // Is left Ctrl pressed?
static int lctrl_time;

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

int tabctrling=0;
LONG last_tabk_time = 0;
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
================ tek.c
//ZNT5
static struct tekchar * tek_buf = 0;
static int tek_buf_len = 0;
static int tek_buf_size = 0;
static colour fg;
static short txt_y, txt_x;
static short out_y, out_x;
static wchar * txt = 0;
static int txt_len = 0;
static int txt_wid = 0;

enum tekmode tek_mode = TEKMODE_OFF;
static enum tekmode tek_mode_pre_gin;
bool tek_bypass = false;
static uchar intensity = 0x7F; // for point modes
static uchar style = 0;        // for vector modes
static uchar font = 0;
static short margin = 0;
static bool beam_defocused = false;
static bool beam_writethru = false;
static bool plotpen = false;
static bool apl_mode = false;

static short tek_y, tek_x;
static short gin_y, gin_x = -1;
static uchar lastfont = 0;
static int lastwidth = -1;
static wchar * tek_dyn_font = 0;

static int beam_glow = 1;
static int thru_glow = 5;

//ZNT0
static bool flash = false;

static wchar * copyfn = 0;
static wchar * APL = W(" ¨)<≤=>]∨∧≠÷,+./0123456789([;×:\\¯⍺⊥∩⌊∊_∇∆⍳∘'⎕∣⊤○⋆?⍴⌈∼↓∪ω⊃↑⊂←⊢→≥-⋄ABCDEFGHIJKLMNOPQRSTUVWXYZ{⊣}$ ");


============= config.c
//ZNT0
static struct {
  char * comment;
  uchar opti;
} file_opts[lengthof(options) + MAX_COMMENTS];
static uchar arg_opts[lengthof(options)];
static uint file_opts_num = 0;
static uint arg_opts_num;
================ termout.c //ZNT
//ZNT4
static short prev_state = 0;
static struct mode_entry * mode_stack = 0;
static int mode_stack_len = 0;
static wchar last_high = 0;
static wchar last_char = 0;
static int last_width = 0;
cattr last_attr = {.attr = ATTR_DEFAULT,
static bool scriptfonts_init = false;
static bool use_blockfonts = false;
static struct cattr_entry cattr_stack[10];
static int cattr_stack_len = 0;
static COLORREF * colours_stack[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int colours_cur = 0;
static int colours_num = 0;
static uchar esc_mod0 = 0;//ZNT5
static uchar esc_mod1 = 0;//ZNT5
============== textprint.c
//ZNT1
static wstring printer = 0;
static char * pf;
static int pd;
static const wchar BOM = 0xFEFF;
static uint np = 0;
static struct passwd * pw;
============= winmain.c
//ZNT3
static int scroll_len = 0;
static int scroll_dif = 0;
//ZNT1 maybe remove it
static wstring * jumplist_title = 0;
static wstring * jumplist_cmd = 0;
static wstring * jumplist_icon = 0;
static int * jumplist_ii = 0;
static int jumplist_len = 0;
//HWND g_hWnd = NULL;             //窗口句柄
//HHOOK g_hlowKeyHook = NULL;     //低级键盘钩子句柄
//static DWORD spid,stid;
//ZNT0
int per_monitor_dpi_aware = DPI_UNAWARE;  // dpi_awareness
uint dpi = 96;
// DPI handling V2
static bool is_in_dpi_change = false;
const int Process_System_DPI_Aware = 1;
const int Process_Per_Monitor_DPI_Aware = 2;

//ZNT0
FILE * mtlog = 0;
char * mintty_debug;

LOGFONT lfont;//for 输入法
extern bool click_focus_token;//wintext
char * home;
char *minttypath=NULL;
bool icon_is_from_shortcut = false;

static HFONT wguifont=0, guifnt = 0;
HINSTANCE inst;
HWND wnd;
HIMC imc;
ATOM class_atom;
SessDef sessdefs[]={
  {0,0,0,0},
  {1,"Wsl"        ,"wsl"             ,(char*[]){"wsl" ,0}},
  // {1,"Wsl"        ,"/bin/wslbridge2" ,(char*[]){"/bin/wslbridge2" ,0}},
  {0,"CygWin"     ,0                 ,(char*[]){0                 ,0}},
  {1,"CMD"        ,"cmd"             ,(char*[]){"cmd"             ,0}},
  {1,"PowerShell" ,"powershell"      ,(char*[]){"powershell"      ,0}},
  {0,0,0,0}
}; 
SessDef main_sd={0};
SessDef cursd={0};
static bool invoked_from_shortcut = false;
wstring shortcut = 0;
static bool invoked_with_appid = false;
static uint hotkey = 0;
static mod_keys hotkey_mods = 0;
static HHOOK kb_hook = 0;


//ZNT7
//filled by win_adjust_borders:

static int zoom_token = 0;  // for heuristic handling of Shift zoom (#467, #476)
static bool default_size_token = false;
//ZNT0
// State
bool win_is_fullscreen;
static bool is_init = false;
bool win_is_always_on_top = false;
static bool go_fullscr_on_max;
static bool resizing;
static bool moving = false;
static bool wm_user = false;
static bool disable_poschange = true;
static bool poschanging = false;
bool clipboard_token = false;
bool keep_screen_on = false;

// Options
//ZNT0
static int border_style = 0;
static string report_geom = 0;
static bool report_moni = false;
bool report_child_pid = false;
static bool report_winpid = false;
static int monitor = 0;
static bool center = false;
static bool right = false;
static bool bottom = false;
static bool left = false;
static bool top = false;
static bool maxwidth = false;
static bool maxheight = false;
static bool store_taskbar_properties = false;
static bool prevent_pinning = false;
bool support_wsl = false;
wchar * wslname = 0;
wstring wsl_basepath = W("");
static uint wsl_ver = 0;
static char * wsl_guid = 0;
static bool wsl_launch = false;
static bool start_home = false;
#ifdef WSLTTY_APPX
static bool wsltty_appx = true;
#else
static bool wsltty_appx = false;
#endif

extern int lwinkey,rwinkey,winkey;
extern ULONG last_wink_time;
static uint pressedkey=-1,pkeys,pwinkey;

static HBITMAP caretbm;

================ winclip.c
static uint buf_len, buf_pos; //need rename
static char * buf;
================= term.c
//ZNTK OK
extern int lines_scrolled;
static int markpos = 0;
static bool markpos_valid = false;
//ZNT0
STerm dterm={0};
STerm *cterm=&dterm;

//ZNT7 
static char * * links = 0;
static int nlinks = 0;
static int linkid = 0;
//ZNT0
static struct {
  uint code, fold;
} * case_folding;
static int case_foldn = 0;
============ winimg.c

static tempfile_t *tempfile_current = NULL;
static size_t tempfile_num = 0;
static size_t const TEMPFILE_MAX_SIZE = 1024 * 1024 * 16;  /* 16MB */
static size_t const TEMPFILE_MAX_NUM = 16;

// protection limit against exhaustion of resources (Windows handles)
// if we'd create ~10000 "CompatibleDCs", handle handling will fail...
static int cdc = 999;
================= windialog.c

static HFONT cfdfont=0;

============= config.c

config cfg, new_cfg, file_cfg;
static control *cols_box, *rows_box, *locale_box, *charset_box;
static control *transparency_valbox, *transparency_selbox;
static control *font_sample, *font_list, *font_weights;
static HKEY muicache = 0;
static HKEY evlabels = 0;
============ charset.c
//ZNT7
static string term_locale = 0;  // Locale set via terminal control sequence.
//ZNT0
static cs_mode mode = CSM_DEFAULT;

static string default_locale = 0;  // Used unless UTF-8 or ACP mode is on.

static string config_locale = 0;  // Locale configured in the options.
static string env_locale = 0;     // Locale determined by the environment.
#if HAS_LOCALES
static bool valid_default_locale = false;
static bool use_locale = false;
#endif
bool cs_ambig_wide = false;
bool cs_single_forced = false;

static uint codepage = 0;
static int default_codepage;

static wchar cp_default_wchar;
static char cp_default_char[4];

int cs_cur_max;

static const struct {
  ushort cp;
  string name;
}
cs_names[] = {
static const struct {
  ushort cp;
  string desc;
}
cs_descs[] = {
string locale_menu[8];
string charset_menu[lengthof(cs_descs) + 4];
============= sckdev.h

static int previous_transparency;
static bool transparency_tuned;
============= winsearch.c
//need fix 
static bool search_initialised = false;
static int prev_height = 0;

static HWND search_wnd;
static HWND search_close_wnd;
static HWND search_prev_wnd;
static HWND search_next_wnd;
static HWND search_edit_wnd;
static WNDPROC default_edit_proc;
static HFONT search_font = 0;
int SEARCHBAR_HEIGHT = 26;
================= wintabz.c

static unsigned int ntabs=0,mtabs=0 ,active_tab = 0;
static STab** tabs=NULL;
static bool tab_bar_visible = false;
static STab *stabs[16];
static int stabs_i=0;
static int tab_paint_width = 0;
static const char* g_home;
============== winctls.c

static HHOOK windows_hook = 0;
static bool hooked_window_activated = false;
=============== child.c 
bool logging = false; //ZNT1

static int win_fd=-1;
static struct winsize prev_winsize = (struct winsize){0, 0, 0, 0};
=========== wintip.c

static HWND tip_wnd = 0;
static ATOM tip_class = 0;
static HFONT tip_font;
static COLORREF tip_bg;
static COLORREF tip_text;

static char sizetip[32] = "";
