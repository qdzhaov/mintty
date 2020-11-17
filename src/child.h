#ifndef CHILD_H
#define CHILD_H

#include <termios.h>

typedef struct STerm STerm;
#define TAB_NTITLE 16
#define TAB_LTITLE 128
typedef struct STab {
    struct STerm*  terminal;
    struct {
        wchar_t titles[TAB_LTITLE][TAB_NTITLE];
        uint titles_i;
        bool attention;
    } info;
}STab ;
STab**win_tabs();
typedef struct SChild SChild;
extern void child_update_charset(STerm* pterm);
extern void child_create(STerm* pterm, char *argv[], struct winsize *winp, const char* path);
extern void toggle_logging(void);
extern bool logging;
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
extern wchar * grandchild_process_list(STerm* pterm);
extern char * child_tty(STerm* pterm);
extern char * foreground_prog(STerm* pterm);  // to be free()d
extern wstring child_conv_path(STerm* pterm,wstring, bool adjust_dir);
extern void setenvi(char * env, int val);

extern void child_set_fork_dir(STerm* pterm,char *);
extern void child_launch(STerm* pterm,int n, int argc, char * argv[], int moni);
extern void child_fork(STerm* pterm,int argc, char * argv[], int moni, bool config_size);
extern void user_command(STerm* pterm,wstring commands, int n);

#endif
