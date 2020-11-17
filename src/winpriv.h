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

extern HINSTANCE inst;  // The all-important instance handle
extern HWND wnd;        // the main terminal window
extern HIMC imc;        // the input method context
extern HWND config_wnd; // the options window
extern ATOM class_atom;

// Inter-window actions
enum {
  WIN_MINIMIZE = 0,
  WIN_MAXIMIZE = -1,
  WIN_FULLSCREEN = -2,
  WIN_TOP = 1,
  WIN_TITLE = 4,
  WIN_INIT_POS = 5
};

extern COLORREF colours[COLOUR_NUM];
extern colour brighten(colour c, colour against, bool monotone);

extern LOGFONT lfont;

extern int font_size;  // logical font size, as configured (< 0: pixel size)
extern int cell_width, cell_height;  // includes spacing
extern bool font_ambig_wide;
extern int line_scale;
extern int PADDING;
extern int OFFSET;
extern bool show_charinfo;
extern void toggle_charinfo(void);
extern void toggle_vt220(void);
extern char * fontpropinfo(void);

extern bool support_wsl;
extern wstring wsl_basepath;
extern bool report_child_pid;

extern int ini_width, ini_height;
extern bool win_is_fullscreen;
extern bool clipboard_token;
extern uint dpi;
extern int per_monitor_dpi_aware;

extern bool click_focus_token;
extern pos last_pos;
extern int lines_scrolled;
extern bool kb_input;
extern uint kb_trace;

extern void win_update_now(void);

extern bool fill_background(HDC dc, RECT * boxp);
extern void win_flush_background(bool clearbg);
extern void win_paint(void);

extern void win_init_fonts(int size);
extern wstring win_get_font(uint findex);
extern void win_change_font(uint findex, wstring fn);
extern void win_font_cs_reconfig(bool font_changed);

extern void win_update_scrollbar(bool inner);
extern void win_set_scrollview(int pos, int len, int height);

extern void win_adapt_term_size(bool sync_size_with_font, bool scale_font_with_size);
extern void scale_to_image_ratio(void);

extern void win_open_config(void);
extern void * load_library_func(string lib, string func);
extern void update_available_version(bool ok);
extern void set_dpi_auto_scaling(bool on);
extern void win_update_transparency(bool opaque);
extern void win_prefix_title(const wstring);
extern void win_unprefix_title(const wstring);
extern void win_set_icon(char * s, int icon_index);

extern void win_show_tip(int x, int y, int cols, int rows);
extern void win_destroy_tip(void);

extern void taskbar_progress(int percent);

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
extern bool win_key_up(WPARAM, LPARAM);
extern void do_win_key_toggle(int vk, bool on);
extern void win_csi_seq(char * pre, char * suf);

extern void win_led(int led, bool set);

extern wchar * dewsl(wchar * wpath);
extern void shell_exec(wstring wpath);
extern void win_init_drop_target(void);

extern wstring wslicon(wchar * params);

typedef struct SChild SChild;
extern char * foreground_cwd(STerm* pterm);

extern void win_switch(bool back, bool alternate);

extern int search_monitors(int * minx, int * miny, HMONITOR lookup_mon, int get_primary, MONITORINFO *mip);

extern void win_set_ime_open(bool);
extern void win_set_ime(bool open);
extern bool win_get_ime(void);

extern void win_dark_mode(HWND w);

extern void show_message(char * msg, UINT type);
extern void show_info(char * msg);

extern void win_close();
extern void app_close();

extern unsigned long mtime(void);

extern void term_save_image(void);
//========= for wintab 
extern void win_tog_scrollbar();
extern void win_tog_partline();
extern STerm* win_tab_active_term() ;
extern void win_tab_show();
extern void win_tab_indicator();
extern void win_tab_init(const char* home,const  char* cmd,char** argv,const  int width, int height, const char* title) ;
extern void win_tab_create() ;
extern void win_tab_clean() ;
extern bool win_tab_should_die();
extern int  win_tab_active();     
extern int  win_tab_count() ;
extern void win_tab_for_each(void (*cb)(STerm* pterm));
extern void win_tab_paint(HDC dc);
extern void win_tab_attention(STerm* pterm) ;
extern void win_tab_change(int change) ;
extern void win_tab_go(int index) ;
extern void win_tab_mouse_click(int x) ;
extern void win_tab_move(int amount) ;
extern void win_tab_set_argv(char** argv) ;
typedef struct STab STab;
extern void win_tab_push(STab *tab);
extern void win_tab_pop();
extern void win_tab_v(STab *tab);
extern void win_tab_actv();

extern void     win_tab_save_title(STerm* pterm);
extern void     win_tab_restore_title(STerm* pterm);
extern void     win_tab_set_title(STerm* pterm, const wchar_t* title) ;
extern wchar_t* win_tab_get_title(unsigned int idx) ;
extern void     win_tab_title_push(STerm* pterm) ;
extern wchar_t* win_tab_title_pop(STerm* pterm) ;

#endif
