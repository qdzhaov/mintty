#ifndef CHILD_H
#define CHILD_H

#include <termios.h>

struct STerm;
typedef struct SChild
{
  char *home, *cmd;
  string _child_dir;
  pid_t _pid;
  bool _killed;
  int _pty_fd;
  struct STerm* pterm;
}SChild;
#define TAB_NTITLE 16
#define TAB_LTITLE 128
typedef struct Tab {
    struct STerm*  terminal;
    SChild* chld;
    struct {
        wchar_t titles[TAB_LTITLE][TAB_NTITLE];
        uint titles_i;
        bool attention;
    } info;
}Tab ;
extern struct SChild*cchild;
Tab**win_tabs();

extern void child_update_charset(void);
extern void child_create(SChild* child,struct  STerm* pterm, char *argv[], struct winsize *winp, const char* path);
extern void toggle_logging(void);
extern bool logging;
extern void child_proc(void);
extern void child_kill(bool point_blank);
extern void child_terminate(SChild* child);
extern void child_write(const char *, uint len);
extern void child_break(void);
extern void child_printf(const char * fmt, ...) __attribute__((format(printf, 1, 2)));
extern void child_send(const char *, uint len);
extern void child_sendw(const wchar *, uint len);
extern void child_resize(struct winsize * winp);
extern void child_free(SChild* child);
extern bool child_is_alive(void);
extern bool child_is_parent(SChild *child);
extern wchar * grandchild_process_list(void);
extern char * child_tty(void);
extern char * foreground_prog(void);  // to be free()d
extern void user_command(wstring commands, int n);
extern wstring child_conv_path(wstring, bool adjust_dir);
extern void child_fork(int argc, char * argv[], int moni, bool config_size);
extern void child_set_fork_dir(char *);
extern void setenvi(char * env, int val);
extern void child_launch(int n, int argc, char * argv[], int moni);

#endif
