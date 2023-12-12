#ifndef CHILD_H
#define CHILD_H

struct winsize;
enum IDSS{IDSS_CUR=-1, IDSS_DEF=0, IDSS_WSL , IDSS_CYG , IDSS_CMD , IDSS_PSH , IDSS_VIEW,IDSS_USR };
typedef struct STerm STerm;
#define TAB_NTITLE 16
#define TAB_LTITLE 128
typedef struct SessDef{
  int argc;
  int ID;
  const wchar_t *menu;
  const char*title;
  const char*cmd;
  const char**argv;
}SessDef;
typedef struct STab {
  wchar_t titles[TAB_LTITLE][TAB_NTITLE];
  uint titles_i;
  bool attention;
  SessDef sd;
  struct STerm*  terminal;
}STab ;
STab**win_tabs();
typedef struct SChild SChild;
extern void child_update_charset(STerm* pterm);
extern void child_create(STerm* pterm, SessDef*sd, struct winsize *winp, const char* path);
extern void toggle_logging(void);
extern void child_proc(void);
extern void child_kill(bool point_blank);
extern void child_terminate(STerm* pterm);
extern void child_break(STerm* pterm);
extern int  child_get_pid(STerm*pterm);

extern void child_write(STerm* pterm,const char *, uint len);
extern void child_printf(STerm* pterm,const char * fmt, ...) __attribute__((format(printf, 2, 3)));
extern void child_send(STerm* pterm,const char *, uint len);
extern void child_sendw(STerm* pterm,const wchar *, uint len);

extern void child_resize(STerm* pterm,struct winsize * winp);
extern void child_free(STerm* pterm);
extern bool child_is_alive(STerm* pterm);
extern bool child_is_parent(SChild *child);
extern char * procres(int pid, const char * res);
extern uchar * child_termios_chars(STerm* pterm);
extern wchar * grandchild_process_list(STerm* pterm);
extern char * child_tty(STerm* pterm);
extern char * foreground_prog(STerm* pterm);  // to be free()d
extern wstring child_conv_path(STerm* pterm,wstring, bool adjust_dir);
extern void setenvi(const char * env, int val);

extern void child_set_fork_dir(STerm* pterm,const char *);
extern void child_launch(STerm* pterm,int n, SessDef*sd, int moni);
extern void child_fork(STerm* pterm,SessDef*sd, int moni, bool config_size, bool in_cwd);
extern void user_command(STerm* pterm,wstring commands, int n);

//========= for wintab 
extern void win_tog_scrollbar();
extern void win_tog_partline();
extern STerm* win_tab_active_term() ;
extern void win_tab_show();
extern void win_tab_indicator();
extern void win_tab_init(const char* home,SessDef*sd,const  int width, int height) ;
extern void win_tab_create(SessDef*sd) ;
extern void win_tab_clean() ;
extern bool win_tab_should_die();
extern STab*win_tab_active();     
extern int  win_tab_count() ;
extern void win_tab_for_each(void (*cb)(STerm* pterm));
extern void win_tab_paint(HDC dc);
extern void win_tab_change(int change) ;
extern void win_tab_go(int index) ;
extern void win_tab_mouse_click(int x) ;
extern void win_tab_move(int amount) ;
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

void tab_prev	    ();
void tab_next	    ();
void tab_move_prev();
void tab_move_next();
void new_tab_def();
void new_tab_wsl();
void new_tab_cyg();
void new_tab_cmd();
void new_tab_psh();
void new_tab_usr();
void new_win_wsl();
void new_win_cyg();
void new_win_cmd();
void new_win_psh();
void new_win_usr();
void new_win_def();
void new_win(int idss,int moni);
void new_tab(int idss);
#endif
