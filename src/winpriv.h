#ifndef WINPRIV_H
#define WINPRIV_H

#include "win.h"
#include "winids.h"

#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <imm.h>
#define SB_PRIOR 100
#define SB_NEXT 101
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#ifndef WM_GETDPISCALEDSIZE
#define WM_GETDPISCALEDSIZE 0x02E4
#endif

// Inter-window actions
enum {
  WIN_MINIMIZE = 0,
  WIN_MAXIMIZE = -1,
  WIN_FULLSCREEN = -2,
  WIN_TOP = 1,
  WIN_TITLE = 4,
  WIN_INIT_POS = 5,
  WIN_HIDE = 8,
};
typedef struct {
// State
  int border_style ;
  bool win_is_fullscreen;
  bool win_is_always_on_top;
  bool clipboard_token ;
  bool keep_screen_on ;
  bool force_opaque;
  bool is_init ;
  bool go_fullscr_on_max;
  bool resizing;
  bool moving ;
  bool wm_user ;
  bool disable_poschange ;
  bool poschanging ;
  int  zoom_token ;  // for heuristic handling of Shift zoom (#467, #476)
  bool default_size_token ;
  int logging ;
  wstring shortcut ;
  bool icon_is_from_shortcut ;
  char * home;

  // Options
  bool report_child_pid ;
  bool report_child_tty;
  bool support_wsl ;
  wchar * wslname ;
  char * wslcmd,*wslcmd0;
  int wsltype;//0 raw wsl,1:wslbridge;2:wslbridge2
  wstring wsl_basepath ;
  bool report_config;
  string report_geom ;
  bool report_moni ;
  bool report_winpid ;
  int monitor ;
  bool center ;
  bool right ;
  bool bottom ;
  bool left ;
  bool top ;
  bool maxwidth ;
  bool maxheight ;
  bool store_taskbar_properties ;
  bool prevent_pinning ;
  uint wsl_ver ;
  char * wsl_guid ;
  bool wsl_launch ;
  bool start_home ;
  bool wsltty_appx ;
  //filled by win_adjust_borders:
  int term_width, term_height;
  int width, height;
  int extra_width, extra_height, norm_extra_width, norm_extra_height;
  int ini_width, ini_height;
  // character cell size, including spacing:
  int cell_width, cell_height;

  bool click_focus_token ;
  int tabctrling;
  LONG last_tabk_time  ;
  ULONG last_wink_time;
  uint pressedkey,pkeys,pwinkey;
  int lwinkey,rwinkey,winkey,twinkey;
  bool kb_input ;
  //
  HWND wnd;        // the main terminal window
  HINSTANCE inst;  // The all-important instance handle
  HWND config_wnd; // the options window
  COLORREF colours[COLOUR_NUM];
  // Xterm256 colour cube and greyscale
  colour xterm_colours[240];
  HIMC imc;        // the input method context
}winvar;
extern winvar wv;

extern int font_size;  // logical font size, as configured (< 0: pixel size)
extern bool font_ambig_wide;
extern int PADDING;
extern int OFFSET;
extern bool show_charinfo;

extern uint dpi;
extern int per_monitor_dpi_aware;

extern pos last_pos;
//extern uint kb_trace;//for keyboard debuging

extern colour brighten(colour c, colour against, bool monotone);
extern void toggle_charinfo(void);
extern void toggle_vt220(void);
extern wchar * fontpropinfo(void);


extern void win_update_now(void);

extern bool fill_background(HDC dc, RECT * boxp);
extern void win_flush_background(bool clearbg);
extern void win_paint(void);

extern void win_init_fonts(int size, bool allfonts);
WPARAM win_set_font(HWND hwnd);//set font for gui,user do not release it;
extern wstring win_get_font(uint findex);
extern void win_change_font(uint findex, wstring fn);
extern void win_font_cs_reconfig(bool font_changed);

extern void win_keep_screen_on(bool);

extern void win_update_scrollbar(bool inner);
extern void win_set_scrollview(int pos, int len, int height);

extern void win_adapt_term_size(bool sync_size_with_font, bool scale_font_with_size);
extern void scale_to_image_ratio(void);

extern void win_open_config(void);
extern void * load_library_func(string lib, string func);
extern void update_available_version(bool ok);
extern void set_dpi_auto_scaling(bool on);
extern void win_update_transparency(int transparency, bool opaque);
extern void win_prefix_title(const wstring);
extern void win_unprefix_title(const wstring);
extern void win_set_icon(const char * s, int icon_index);

extern void win_show_tip(int x, int y, wstring text);
extern void win_show_tip_size(int x, int y, int cols, int rows);
extern void win_hide_tip(void);
extern void win_destroy_tip(void);

extern void taskbar_progress(int percent);
extern HCURSOR win_get_cursor(bool appmouse);
extern void set_cursor_style(bool appmouse, const wchar * style);

extern void win_init_menus(void);
extern void win_update_menus(bool callback);
extern void user_function(wstring commands, int n);

extern void win_show_mouse(void);
extern bool win_mouse_click(mouse_button, LPARAM);
extern void win_mouse_release(mouse_button, LPARAM);
extern void win_mouse_wheel(POINT wpos, bool horizontal, int delta);
extern void win_mouse_move(bool nc, LPARAM);

extern mod_keys get_mods(void);
extern void win_key_reset(void);
extern void provide_input(wchar);
extern bool win_key_down(WPARAM, LPARAM);
extern bool win_whotkey(WPARAM, LPARAM);
extern void win_update_shortcuts();
extern bool win_key_up(WPARAM, LPARAM);
extern void do_win_key_toggle(int vk, bool on);
extern void win_csi_seq(const char * pre, const char * suf);

extern void win_led(int led, bool set);
extern bool get_scroll_lock(void);
extern void sync_scroll_lock(bool locked);

extern const wchar * dewsl(const wchar * wpath);
extern void shell_exec(wstring wpath);
extern void win_init_drop_target(void);

extern wstring wslicon(const wchar * params);

typedef struct SChild SChild;
extern char * foreground_cwd(STerm* pterm);

extern void toggle_status_line(void);

extern void win_switch(bool back, bool alternate);

extern int search_monitors(int * minx, int * miny, HMONITOR lookup_mon, int get_primary, MONITORINFO *mip);

extern void win_set_ime_open(bool);
extern void win_set_ime(bool open);
extern bool win_get_ime(void);

extern void win_dark_mode(HWND w);

extern void show_message(const char * msg, UINT type);
extern void show_info(const char * msg);

extern void win_close();
extern void win_toggle_on_top(void);
extern void app_close();

extern unsigned long mtime(void);

extern void term_save_image(bool do_open);
#endif
