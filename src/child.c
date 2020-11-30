// child.c (part of mintty)
// Copyright 2008-11 Andy Koppe, 2015-2017 Thomas Wolff
// Licensed under the terms of the GNU General Public License v3 or later.

#include "child.h"

#include "term.h"
#include "charset.h"

#include "winpriv.h"  /* win_prefix_title, win_update_now */
#include "appinfo.h"  /* APPNAME, VERSION */

#include <pwd.h>
#include <fcntl.h>
#include <utmp.h>
#include <dirent.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#ifdef __CYGWIN__
#include <sys/cygwin.h>  // cygwin_internal
#endif
#include <termios.h>

extern char * home;
extern bool report_child_pid;
extern bool icon_is_from_shortcut;
extern wstring shortcut;
extern bool kb_input;
#if CYGWIN_VERSION_API_MINOR >= 93
#include <pty.h>
#else
int forkpty(int *, char *, struct termios *, struct winsize *);
#endif

#include <winbase.h>

#if CYGWIN_VERSION_DLL_MAJOR < 1007
#include <winnls.h>
#include <wincon.h>
#include <wingdi.h>
#include <winuser.h>
#endif

// exit code to use for failure of `exec` (changed from 255, see #745)
// http://www.tldp.org/LDP/abs/html/exitcodes.html
#define mexit 126

static int win_fd=-1;
static int log_fd = -1;
bool logging = false;
#if CYGWIN_VERSION_API_MINOR >= 66
#include <langinfo.h>
#endif


#define dont_debug_dir

#ifdef debug_dir
#define trace_dir(d)	show_info(d)
#else
#define trace_dir(d)	
#endif
int child_get_pid(STerm*pterm){
  return pterm->child.pid;
}
static void
childerror(char * action, bool from_fork, int errno_code, int code)
{
#if CYGWIN_VERSION_API_MINOR >= 66
  bool utf8 = strcmp(nl_langinfo(CODESET), "UTF-8") == 0;
  char * oldloc;
#else
  char * oldloc = (char *)cs_get_locale();
  bool utf8 = strstr(oldloc, ".65001");
#endif
  if (utf8)
    oldloc = null;
  else {
    oldloc = strdup(cs_get_locale());
    cs_set_locale("C.UTF-8");
  }

  char s[33];
  bool colour_code = code && !errno_code;
  sprintf(s, "\033[30;%dm\033[K", from_fork ? 41 : colour_code ? code : 43);
  term_write(s, strlen(s));
  term_write(action, strlen(action));
  if (errno_code) {
    char * err = strerror(errno_code);
    if (from_fork && errno_code == ENOENT)
      err = _("There are no available terminals");
    term_write(": ", 2);
    term_write(err, strlen(err));
  }
  if (code && !colour_code) {
    sprintf(s, " (%d)", code);
    term_write(s, strlen(s));
  }
  term_write(".\033[0m\r\n", 7);

  if (oldloc) {
    cs_set_locale(oldloc);
    free(oldloc);
  }
}

static void sigexit(int sig) {
  for (STab**t=win_tabs();*t;t++){
    if ((*t)->terminal->child.pid)
      kill(-(*t)->terminal->child.pid, SIGHUP);
  }
  signal(sig, SIG_DFL);
  kill(getpid(), sig);
}

static void
open_logfile(bool toggling)
{
  // Open log file if any
  if (*cfg.log) {
    // use cygwin conversion function to escape unencoded characters 
    // and thus avoid the locale trick (2.2.3)

    if (0 == wcscmp(cfg.log, W("-"))) {
      log_fd = fileno(stdout);
      logging = true;
    }
    else {
      char * log;
      if (*cfg.log == '~' && cfg.log[1] == '/') {
        // substitute '~' -> home
        char * path = cs__wcstombs(&cfg.log[2]);
        log = asform("%s/%s", home, path);
        free(path);
      }
      else
        log = path_win_w_to_posix(cfg.log);
#ifdef debug_logfilename
      printf("<%ls> -> <%s>\n", cfg.log, log);
#endif
      char * format = strchr(log, '%');
      if (format && * ++ format == 'd' && !strchr(format, '%')) {
        char * logf = newn(char, strlen(log) + 20);
        sprintf(logf, log, getpid());
        free(log);
        log = logf;
      }
      else if (format) {
        struct timeval now;
        gettimeofday(& now, 0);
        char * logf = newn(char, MAX_PATH + 1);
        strftime(logf, MAX_PATH, log, localtime (& now.tv_sec));
        free(log);
        log = logf;
      }

      log_fd = open(log, O_WRONLY | O_CREAT | O_EXCL, 0600);
      if (log_fd < 0) {
        // report message and filename:
        wchar * wpath = path_posix_to_win_w(log);
        char * upath = cs__wcstoutf(wpath);
#ifdef debug_logfilename
        printf(" -> <%ls> -> <%s>\n", wpath, upath);
#endif
        char * msg = _("Error: Could not open log file");
        if (toggling) {
          char * err = strerror(errno);
          char * errmsg = newn(char, strlen(msg) + strlen(err) + strlen(upath) + 4);
          sprintf(errmsg, "%s: %s\n%s", msg, err, upath);
          win_show_warning(errmsg);
          free(errmsg);
        }
        else {
          childerror(msg, false, errno, 0);
          childerror(upath, false, 0, 0);
        }
        free(upath);
        free(wpath);
      }
      else
        logging = true;

      free(log);
    }
  }
}

void
toggle_logging()
{
  if (logging)
    logging = false;
  else if (log_fd >= 0)
    logging = true;
  else
    open_logfile(true);
}

void
child_update_charset(STerm *pterm   )
{
#ifdef IUTF8
  if (pterm&&pterm->child.pty_fd >= 0) {
    // Terminal line settings
    struct termios attr;
    tcgetattr(pterm->child.pty_fd, &attr);
    bool utf8 = strcmp(nl_langinfo(CODESET), "UTF-8") == 0;
    if (utf8)
      attr.c_iflag |= IUTF8;
    else
      attr.c_iflag &= ~IUTF8;
    tcsetattr(pterm->child.pty_fd, TCSANOW, &attr);
  }
#endif
}

void
child_create(struct STerm* pterm,SessDef*sd,
             struct winsize *winp, const char* path)
{
  pid_t pid;
  (pterm->child.pty_fd   ) = -1;
  win_tab_v(pterm->tab);
  char**argv=sd->argv;
  char*cmd=sd->cmd;
  trace_dir(asform("child_create: %s", getcwd(malloc(MAX_PATH), MAX_PATH)));

  // xterm and urxvt ignore SIGHUP, so let's do the same.
  signal(SIGHUP, SIG_IGN);

  signal(SIGINT, sigexit);
  signal(SIGTERM, sigexit);
  signal(SIGQUIT, sigexit);

  // Create the child process and pseudo terminal.
  pid = forkpty(&(pterm->child.pty_fd   ), 0, 0, winp);
  if (pid < 0) {
    bool rebase_prompt = (errno == EAGAIN);
    //ENOENT  There are no available terminals.
    //EAGAIN  Cannot allocate sufficient memory to allocate a task structure.
    //EAGAIN  Not possible to create a new process; RLIMIT_NPROC limit.
    //ENOMEM  Memory is tight.
    childerror(_("Error: Could not fork child process"), true, errno, pid);
    if (rebase_prompt)
      childerror(_("DLL rebasing may be required; see 'rebaseall / rebase --help'"), false, 0, 0);

    pid = 0;

    term_hide_cursor();
  }
  else if (!pid) { // Child process.
#if CYGWIN_VERSION_DLL_MAJOR < 1007
    // Some native console programs require a console to be attached to the
    // process, otherwise they pop one up themselves, which is rather annoying.
    // Cygwin's exec function from 1.5 onwards automatically allocates a console
    // on an invisible window station if necessary. Unfortunately that trick no
    // longer works on Windows 7, which is why Cygwin 1.7 contains a new hack
    // for creating the invisible console.
    // On Cygwin versions before 1.5 and on Cygwin 1.5 running on Windows 7,
    // we need to create the invisible console ourselves. The hack here is not
    // as clever as Cygwin's, with the console briefly flashing up on startup,
    // but it'll do.
#if CYGWIN_VERSION_DLL_MAJOR == 1005
    DWORD win_version = GetVersion();
    win_versiON = ((win_version & 0xff) << 8) | ((win_version >> 8) & 0xff);
    if (win_version >= 0x0601)  // Windows 7 is NT 6.1.
#endif
      if (AllocConsole()) {
        HMODULE kernel = GetModuleHandleA("kernel32");
        HWND (WINAPI *pGetConsoleWindow)(void) =
          (void *)GetProcAddress(kernel, "GetConsoleWindow");
        ShowWindowAsync(pGetConsoleWindow(), SW_HIDE);
      }
#endif

    // Reset signals
    signal(SIGHUP, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);

    // Mimick login's behavior by disabling the job control signals
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    setenv("TERM", cfg.Term, true);
    // unreliable info about terminal application (#881)
    setenv("TERM_PROGRAM", APPNAME, true);
    setenv("TERM_PROGRAM_VERSION", VERSION, true);

    // If option Locale is used, set locale variables?
    // https://github.com/mintty/mintty/issues/116#issuecomment-108888265
    // Variables are now set in update_locale() which sets one of 
    // LC_ALL or LC_CTYPE depending on previous setting of 
    // LC_ALL or LC_CTYPE or LANG, stripping @cjk modifiers for WSL.
    if (cfg.old_locale) {
      //string lang = cs_lang();
      string lang = cs_lang() ? cs_get_locale() : 0;
      if (lang) {
        unsetenv("LC_ALL");
        unsetenv("LC_COLLATE");
        unsetenv("LC_CTYPE");
        unsetenv("LC_MONETARY");
        unsetenv("LC_NUMERIC");
        unsetenv("LC_TIME");
        unsetenv("LC_MESSAGES");
        setenv("LANG", lang, true);
      }
    }

    // Terminal line settings
    struct termios attr;
    tcgetattr(0, &attr);
    attr.c_cc[VERASE] = cfg.backspace_sends_bs ? CTRL('H') : CDEL;
    attr.c_iflag |= IXANY | IMAXBEL;
#ifdef IUTF8
    bool utf8 = strcmp(nl_langinfo(CODESET), "UTF-8") == 0;
    if (utf8)
      attr.c_iflag |= IUTF8;
    else
      attr.c_iflag &= ~IUTF8;
#endif
    attr.c_lflag |= ECHOE | ECHOK | ECHOCTL | ECHOKE;
    tcsetattr(0, TCSANOW, &attr);
    if (path) chdir(path);
    // Invoke command
    execvp(cmd, argv);

    // If we get here, exec failed.
    fprintf(stderr, "\033]701;C.UTF-8\007");
    fprintf(stderr, "\033[30;41m\033[K");
    //__ %1$s: client command (e.g. shell) to be run; %2$s: error message
    fprintf(stderr, _("Failed to run '%s': %s"), cmd, strerror(errno));
    fprintf(stderr, "\r\n");
    fflush(stderr);

#if CYGWIN_VERSION_DLL_MAJOR < 1005
    // Before Cygwin 1.5, the message above doesn't appear if we exit
    // immediately. So have a little nap first.
    usleep(200000);
#endif

    exit(mexit);
  }
  else { // Parent process.
    (pterm->child.pid      )=pid;
    if (report_child_pid) {
      printf("%d\n", pid);
      fflush(stdout);
    }

#ifdef __midipix__
    // This corrupts CR in cygwin
    struct termios attr;
    tcgetattr(pterm->child.pty_fd   , &attr);
    cfmakeraw(&attr);
    tcsetattr(pterm->child.pty_fd   , TCSANOW, &attr);
#endif

    fcntl(pterm->child.pty_fd   , F_SETFL, O_NONBLOCK);

    //child_update_charset();  // could do it here or as above

    if (cfg.create_utmp) {
      char *dev = ptsname(pterm->child.pty_fd   );
      if (dev) {
        struct utmp ut;
        memset(&ut, 0, sizeof ut);

        if (!strncmp(dev, "/dev/", 5))
          dev += 5;
        strlcpy(ut.ut_line, dev, sizeof ut.ut_line);

        if (dev[1] == 't' && dev[2] == 'y')
          dev += 3;
        else if (!strncmp(dev, "pts/", 4))
          dev += 4;
        //strncpy(ut.ut_id, dev, sizeof ut.ut_id);
        for (uint i = 0; i < sizeof ut.ut_id && *dev; i++)
          ut.ut_id[i] = *dev++;

        ut.ut_type = USER_PROCESS;
        ut.ut_pid = pid;
        ut.ut_time = time(0);
        strlcpy(ut.ut_user, getlogin() ?: "?", sizeof ut.ut_user);
        gethostname(ut.ut_host, sizeof ut.ut_host);
        login(&ut);
      }
    }
  }

  if(win_fd==-1) win_fd = open("/dev/windows", O_RDONLY);
  if (cfg.logging) {
    // option Logging=yes => initially open log file if configured
    open_logfile(false);
  }
}

char *
child_tty(STerm* pterm)
{
  return ptsname((pterm->child.pty_fd   ));
}

#define patch_319
static void vprocclose(STerm* pterm){
  if (pterm->child.pid) {
    int status,tc=0 ;
    if (waitpid(pterm->child.pid, &status, WNOHANG) == pterm->child.pid) {
      pterm->child.pid = 0;
      // Decide whether we want to exit now or later
      if ((pterm->child.killed   ) || cfg.hold == HOLD_NEVER)tc=1;
      else if (cfg.hold == HOLD_START) {
        if (WIFSIGNALED(status) || WEXITSTATUS(status) != mexit)tc=1;
      }
      else if (cfg.hold == HOLD_ERROR) {
        if (WIFEXITED(status)) {
          if (WEXITSTATUS(status) == 0)tc=1;
        }
        else {
          const int error_sigs =
              1 << SIGILL | 1 << SIGTRAP | 1 << SIGABRT | 1 << SIGFPE |
              1 << SIGBUS | 1 << SIGSEGV | 1 << SIGPIPE | 1 << SIGSYS;
          if (!(error_sigs & 1 << WTERMSIG(status)))tc=1;
        }
      }
      char *s = 0;
      bool err = true;
      if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        if (code == 0) err = false;
        if ((code || cfg.exit_write) /*&& cfg.hold != HOLD_START*/)
          //__ %1$s: client command (e.g. shell) terminated, %2$i: exit code
          asprintf(&s, _("%s: Exit %i"), pterm->child.cmd, code);
      } else if (WIFSIGNALED(status))
        asprintf(&s, "%s: %s", pterm->child.cmd, strsignal(WTERMSIG(status)));

      if (!s && cfg.exit_write) {
        //__ default inline notification if ExitWrite=yes
        s = _("TERMINATED");
      }
      if (s) {
        char * wsl_pre = "\0337\033[H\033[L";
        char * wsl_post = "\0338\033[B";
        if (err && support_wsl)
          term_write(wsl_pre, strlen(wsl_pre));
        childerror(s, false, 0, err ? 41 : 42);
        if (err && support_wsl)
          term_write(wsl_post, strlen(wsl_post));
      }
      if (cfg.exit_title && *cfg.exit_title)
        win_prefix_title(cfg.exit_title);
      if(tc) win_tab_clean();
    }
  }
}

void
child_proc(void)
{
  for (;;) {
    for (STab**t=win_tabs();*t;t++){
      if ((*t)->terminal->paste_buffer){
        win_tab_v(*t);
        term_send_paste();
      }
    }
    struct timeval timeout = {0, 100000}, *timeout_p = 0;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(win_fd, &fds);
    int highfd = win_fd;
    for (STab**t=win_tabs();*t;t++){
      win_tab_v(*t);
      STerm *pterm=(*t)->terminal;
      if (pterm->child.pty_fd > highfd) highfd = pterm->child.pty_fd;
      if (pterm->child.pty_fd >= 0)
        FD_SET(pterm->child.pty_fd, &fds);
      vprocclose(pterm);
      if (pterm->child.pid != 0 && pterm->child.pty_fd < 0) // Pty gone, but process still there: keep checking
        timeout_p = &timeout;
    }

    if (select(highfd + 1, &fds, 0, 0, timeout_p) > 0) {
      for (STab**t=win_tabs();*t;t++){
        STerm *pterm=(*t)->terminal;
        win_tab_v(*t);
        if (pterm->child.pty_fd >= 0 && FD_ISSET(pterm->child.pty_fd, &fds)) {
          // Pty devices on old Cygwin versions (pre 1005) deliver only 4 bytes
          // at a time, and newer ones or MSYS2 deliver up to 256 at a time.
          // so call read() repeatedly until we have a worthwhile haul.
          // this avoids most partial updates, results in less flickering/tearing.
          static char buf[4096];
          uint len = 0;
#if CYGWIN_VERSION_API_MINOR >= 74
          if (cfg.baud > 0) {
            uint cps = cfg.baud / 10; // 1 start bit, 8 data bits, 1 stop bit
            uint nspc = 2000000000 / cps;

            static ulong prevtime = 0;
            static ulong exceeded = 0;
            static ulong granularity = 0;
            struct timespec tim;
            if (!granularity) {
              clock_getres(CLOCK_MONOTONIC, &tim); // cygwin granularity: 539ns
              granularity = tim.tv_nsec;
            }
            clock_gettime(CLOCK_MONOTONIC, &tim);
            ulong now = tim.tv_sec * (long)1000000000 + tim.tv_nsec;
            //printf("baud %d ns/char %d prev %ld now %ld delta\n", cfg.baud, nspc, prevtime, now);
            if (now < prevtime + nspc) {
              ulong delay = prevtime ? prevtime + nspc - now : 0;
              if (delay < exceeded)
                exceeded -= delay;
              else {
                tim.tv_sec = delay / 1000000000;
                tim.tv_nsec = delay % 1000000000;
                clock_nanosleep(CLOCK_MONOTONIC, 0, &tim, 0);
                clock_gettime(CLOCK_MONOTONIC, &tim);
                ulong then = tim.tv_sec * (long)1000000000 + tim.tv_nsec;
                //printf("nsleep %ld -> %ld\n", delay, then - now);
                if (then - now > delay)
                  exceeded = then - now - delay;
                now = then;
              }
            }
            prevtime = now;

            int ret = read(pterm->child.pty_fd, buf, 1);
            if (ret > 0)
              len = ret;
          }
          else
#endif
            do {
              int ret = read(pterm->child.pty_fd, buf + len, sizeof buf - len);
              if (ret > 0)
                len += ret;
              else
                break;
            } while (len < sizeof buf);

          if (len > 0) {
            buf[len]=0;
            term_write(buf, len);
            // accelerate keyboard echo if (unechoed) keyboard input is pending
            if (kb_input) {
              kb_input = false;
              if (cfg.display_speedup)
                // undocumented safeguard in case something goes wrong here
                win_update_now();
            }
            if (log_fd >= 0 && logging)
              write(log_fd, buf, len);
          }
          else {
            pterm->child.pty_fd = -1;
            term_hide_cursor();
            //win_tab_clean();
          }
        }
      }
      if (FD_ISSET(win_fd, &fds)) return;
    }
  }
}

void child_terminate(STerm* pterm) {
  kill(-(pterm->child.pid      ), SIGKILL);

  // Seems that sometimes cygwin leaves process in non-waitable and
  // non-alive state. The result for that is that there will be
  // unkillable tabs.
  // This stupid hack solves the problem.
  // TODO: Find out better way to solve this. Now the child processes are
  // not always cleaned up.
  int cpid = (pterm->child.pid      );
  for (STab**t=win_tabs();*t;t++){
    if ((*t)->terminal->child.pid == cpid){ 
      (*t)->terminal->child.pid = 0;
      (*t)->terminal->child.killed= 1;
      win_tab_clean();
    }
  }
}
void
child_kill(bool point_blank)
{
  for (STab**t=win_tabs();*t;t++){
    STerm *pterm=(*t)->terminal;
    kill(-pterm->child.pid, point_blank ? SIGKILL : SIGHUP);
    pterm->child.killed = true;
  }
}

bool
child_is_alive(STerm* pterm)
{
    return (pterm->child.pid >0 );
}

static struct procinfo {
  int pid;
  int ppid;
  int winpid;
  char * cmdline;
} * ttyprocs = 0;
static uint nttyprocs = 0;

static char *
procres(int pid, char * res)
{
  char fbuf[99];
  char * fn = asform("/proc/%d/%s", pid, res);
  int fd = open(fn, O_BINARY | O_RDONLY);
  free(fn);
  if (fd < 0)
    return 0;
  int n = read(fd, fbuf, sizeof fbuf - 1);
  close(fd);
  for (int i = 0; i < n - 1; i++)
    if (!fbuf[i])
      fbuf[i] = ' ';
  fbuf[n] = 0;
  char * nl = strchr(fbuf, '\n');
  if (nl)
    *nl = 0;
  return strdup(fbuf);
}

static int
procresi(int pid, char * res)
{
  char * si = procres(pid, res);
  int i = atoi(si);
  free(si);
  return i;
}

#ifndef HAS_LOCALES
#define wcwidth xcwidth
#else
#if CYGWIN_VERSION_API_MINOR < 74
#define wcwidth xcwidth
#endif
#endif

wchar *
grandchild_process_list(STerm* pterm)
{
  if (!(pterm->child.pid      ))
    return 0;
  DIR * d = opendir("/proc");
  if (!d)
    return 0;
  char * tty = child_tty(pterm);
  struct dirent * e;
  while ((e = readdir(d))) {
    char * pn = e->d_name;
    int thispid = atoi(pn);
    if (thispid && thispid != (pterm->child.pid      )) {
      char * ctty = procres(thispid, "ctty");
      if (ctty) {
        if (0 == strcmp(ctty, tty)) {
          int ppid = procresi(thispid, "ppid");
          int winpid = procresi(thispid, "winpid");
          // not including the direct child ((pterm->child.pid      ))
          ttyprocs = renewn(ttyprocs, nttyprocs + 1);
          ttyprocs[nttyprocs].pid = thispid;
          ttyprocs[nttyprocs].ppid = ppid;
          ttyprocs[nttyprocs].winpid = winpid;
          char * cmd = procres(thispid, "cmdline");
          ttyprocs[nttyprocs].cmdline = cmd;

          nttyprocs++;
        }
        free(ctty);
      }
    }
  }
  closedir(d);

  DWORD win_version = GetVersion();
  win_version = ((win_version & 0xff) << 8) | ((win_version >> 8) & 0xff);

  wchar * res = 0;
  for (uint i = 0; i < nttyprocs; i++) {
    char * proc = newn(char, 50 + strlen(ttyprocs[i].cmdline));
    sprintf(proc, " %5u %5u %s", ttyprocs[i].winpid, ttyprocs[i].pid, ttyprocs[i].cmdline);
    free(ttyprocs[i].cmdline);
    wchar * procw = cs__mbstowcs(proc);
    free(proc);
    if (win_version >= 0x0601)
      for (int i = 0; i < 13; i++)
        if (procw[i] == ' ')
          procw[i] = 0x2007;  // FIGURE SPACE
    int wid = min(wcslen(procw), 40);
    for (int i = 13; i < wid; i++)
      if (((cfg.charwidth % 10) ? xcwidth(procw[i]) : wcwidth(procw[i])) == 2)
        wid--;
    procw[wid] = 0;

    if (win_version >= 0x0601) {
      if (!res)
        res = wcsdup(W("╎ WPID   pid  COMMAND\n"));  // ┆┇┊┋╎╏
      res = renewn(res, wcslen(res) + wcslen(procw) + 3);
      wcscat(res, W("╎"));
    }
    else {
      if (!res)
        res = wcsdup(W("| WPID   pid  COMMAND\n"));  // ┆┇┊┋╎╏
      res = renewn(res, wcslen(res) + wcslen(procw) + 3);
      wcscat(res, W("|"));
    }

    wcscat(res, procw);
    wcscat(res, W("\n"));
    free(procw);
  }
  if (ttyprocs) {
    nttyprocs = 0;
    free(ttyprocs);
    ttyprocs = 0;
  }
  return res;
}

void
child_write(STerm* pterm,const char *buf, uint len)
{
  if ((pterm->child.pty_fd   ) >= 0)
    write((pterm->child.pty_fd   ), buf, len);
}

/*
  Simulate a BREAK event.
 */
void
child_break(STerm* pterm)
{
  int gid = tcgetpgrp((pterm->child.pty_fd   ));
  if (gid > 1) {
    struct termios attr;
    tcgetattr((pterm->child.pty_fd   ), &attr);
    if ((attr.c_iflag & (IGNBRK | BRKINT)) == BRKINT) {
      kill(gid, SIGINT);
    }
  }
}

void
child_printf(STerm* pterm ,const char *fmt, ...)
{
  if ((pterm->child.pty_fd   ) >= 0) {
    va_list va;
    va_start(va, fmt);
    char *s;
    int len = vasprintf(&s, fmt, va);
    va_end(va);
    if (len >= 0)
      write((pterm->child.pty_fd   ), s, len);
    free(s);
  }
}

void
child_send(STerm* pterm,const char *buf, uint len)
{
  term_reset_screen();
  if (pterm->echoing)
    term_write(buf, len);
  child_write(pterm,buf, len);
}

void
child_sendw(STerm* pterm,const wchar *ws, uint wlen)
{
  char s[wlen * cs_cur_max];
  int len = cs_wcntombn(s, ws, sizeof s, wlen);
  if (len > 0)
    child_send(pterm,s, len);
}

void
child_resize(STerm* pterm,struct winsize *winp)
{
  if ((pterm->child.pty_fd   ) >= 0)
    ioctl((pterm->child.pty_fd   ), TIOCSWINSZ, winp);
}

static int
foregroundpid(STerm* pterm)
{
  return ((pterm->child.pty_fd   ) >= 0) ? tcgetpgrp((pterm->child.pty_fd   )) : 0;
}

char *
foreground_cwd(STerm* pterm)
{
  // if working dir is communicated interactively, use it
  if ((pterm->child.child_dir) && *(pterm->child.child_dir))
    return strdup((pterm->child.child_dir));

  // for WSL, do not check foreground process; hope start dir is good
  if (support_wsl) {
    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)))
      return strdup(cwd);
    else
      return 0;
  }

#if CYGWIN_VERSION_DLL_MAJOR >= 1005
  int fgpid = foregroundpid(pterm);
  if (fgpid > 0) {
    char proc_cwd[32];
    sprintf(proc_cwd, "/proc/%u/cwd", fgpid);
    return realpath(proc_cwd, 0);
  }
#endif
  return 0;
}

char *
foreground_prog(STerm* pterm)
{
#if CYGWIN_VERSION_DLL_MAJOR >= 1005
  int fgpid = foregroundpid(pterm);
  if (fgpid > 0) {
    char exename[32];
    sprintf(exename, "/proc/%u/exename", fgpid);
    FILE * enf = fopen(exename, "r");
    if (enf) {
      char exepath[MAX_PATH + 1];
      fgets(exepath, sizeof exepath, enf);
      fclose(enf);
      // get basename of program path
      char * exebase = strrchr(exepath, '/');
      if (exebase)
        exebase++;
      else
        exebase = exepath;
      return strdup(exebase);
    }
  }
#endif
  return 0;
}

void
user_command(STerm* pterm,wstring commands, int n)
{
  if (*commands) {
    char * cmds = cs__wcstombs(commands);
    char * cmdp = cmds;
    char sepch = ';';
    if ((uchar)*cmdp <= (uchar)' ')
      sepch = *cmdp++;
    char * progp;
    while (n >= 0 && (progp = strchr(cmdp, ':'))) {
      progp++;
      char * sepp = strchr(progp, sepch);
      if (sepp)
        *sepp = '\0';

      if (n == 0) {
        int fgpid = foregroundpid(pterm);
        if (fgpid) {
          char * _fgpid = 0;
          asprintf(&_fgpid, "%d", fgpid);
          if (_fgpid) {
            setenv("MINTTYpid", _fgpid, true);
            free(_fgpid);
          }
        }
        char * fgp = foreground_prog(pterm);
        if (fgp) {
          setenv("MINTTY_PROG", fgp, true);
          free(fgp);
        }
        char * fgd = foreground_cwd(pterm);
        if (fgd) {
          setenv("MINTTY_CWD", fgd, true);
          free(fgd);
        }
        term_cmd(progp);
        unsetenv("MINTTY_CWD");
        unsetenv("MINTTY_PROG");
        unsetenv("MINTTYpid");
        break;
      }
      n--;

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
    free(cmds);
  }
}

/*
   used by win_open
*/
wstring
child_conv_path(STerm* pterm,wstring wpath, bool adjust_dir)
{
  // Need to convert POSIX path to Windows first
  if (support_wsl) {
    // First, we need to replicate some of the handling of relative paths 
    // as implemented in child_conv_path,
    // because the dewsl functionality would actually go in between 
    // the workflow of child_conv_path.
    // We cannot determine the WSL foreground process and its 
    // current directory, so we can only consider the working directory 
    // explicitly communicated via the OSC 7 escape sequence here.
    if (*wpath != '/' && wcsncmp(wpath, W("~/"), 2) != 0) {
      if (pterm->child.child_dir && *pterm->child.child_dir) {
        wchar * cd = cs__mbstowcs(pterm->child.child_dir);
        cd = renewn(cd, wcslen(cd) + wcslen(wpath) + 2);
        cd[wcslen(cd)] = '/';
        wcscpy(&cd[wcslen(cd) + 1], wpath);
        delete(wpath);
        wpath = cd;
      }
    }
    wpath = dewsl((wchar *)wpath);
  }
  int wlen = wcslen(wpath);
  int len = wlen * cs_cur_max;
  char path[len];
  len = cs_wcntombn(path, wpath, len, wlen);
  path[len] = 0;

  char * exp_path;  // expanded path
  if (*path == '~') {
    // Tilde expansion
    char * name = path + 1;
    char * rest = strchr(path, '/');
    if (rest)
      *rest++ = 0;
    else
      rest = "";
    char * base;
    if (!*name)
      base = home;
    else {
#if CYGWIN_VERSION_DLL_MAJOR >= 1005
      // Find named user's home directory
      struct passwd * pw = getpwnam(name);
      base = (pw ? pw->pw_dir : 0) ?: "";
#else
      // Pre-1.5 Cygwin simply copies HOME into pw_dir, which is no use here.
      base = "";
#endif
    }
    exp_path = asform("%s/%s", base, rest);
  }
  else if (*path != '/' && adjust_dir) {
#if CYGWIN_VERSION_DLL_MAJOR >= 1005
    // Handle relative paths. Finding the foreground process working directory
    // requires the /proc filesystem, which isn't available before Cygwin 1.5.

    // Find pty's foreground process, if any. Fall back to child process.
    int fgpid = ((pterm->child.pty_fd   ) >= 0) ? tcgetpgrp((pterm->child.pty_fd   )) : 0;
    if (fgpid <= 0)
      fgpid = (pterm->child.pid      );

    char * cwd = foreground_cwd(pterm);
    exp_path = asform("%s/%s", cwd ?: home, path);
    if (cwd)
      free(cwd);
#else
    // If we're lucky, the path is relative to the home directory.
    exp_path = asform("%s/%s", home, path);
#endif
  }
  else
    exp_path = path;

# if CYGWIN_VERSION_API_MINOR >= 222
  // CW_INT_SETLOCALE was introduced in API 0.222
  cygwin_internal(CW_INT_SETLOCALE);
# endif
  wchar *win_wpath = path_posix_to_win_w(exp_path);
  // Drop long path prefix if possible,
  // because some programs have trouble with them.
  if (win_wpath && wcslen(win_wpath) < MAX_PATH) {
    wchar *old_win_wpath = win_wpath;
    if (wcsncmp(win_wpath, W("\\\\?\\UNC\\"), 8) == 0) {
      win_wpath = wcsdup(win_wpath + 6);
      win_wpath[0] = '\\';  // Replace "\\?\UNC\" prefix with "\\"
      free(old_win_wpath);
    }
    else if (wcsncmp(win_wpath, W("\\\\?\\"), 4) == 0) {
      win_wpath = wcsdup(win_wpath + 4);  // Drop "\\?\" prefix
      free(old_win_wpath);
    }
  }

  if (exp_path != path)
    free(exp_path);

  return win_wpath;
}

void
child_set_fork_dir(STerm* pterm,char * dir)
{
  strset(&(pterm->child.child_dir), dir);
}

void
setenvi(char * env, int val)
{
  static char valbuf[22];  // static to prevent #530
  sprintf(valbuf, "%d", val);
  setenv(env, valbuf, true);
}
/*
  Called from Alt+F2 (or session launcher via child_launch).
 */
extern SessDef main_sd;
static void
do_child_fork(SessDef*sd, int moni, bool launch, bool config_size)
{
  char**argv=sd->argv;
  trace_dir(asform("do_child_fork: %s", getcwd(malloc(MAX_PATH), MAX_PATH)));
#ifdef control_AltF2_size_via_token
  void reset_fork_mode()
  {
    clone_size_token = true;
  }
#endif
  pid_t clone = fork();

  if (cfg.daemonize) {
    if (clone < 0) {
      childerror(_("Error: Could not fork child daemon"), true, errno, 0);
      //reset_fork_mode();
      return;  // assume next fork will fail too
    }
    if (clone > 0) {  // parent waits for intermediate child
      int status;
      waitpid(clone, &status, 0);
      //reset_fork_mode();
      return;
    }

    clone = fork();
    if (clone < 0) {
      exit(mexit);
    }
    if (clone > 0) {  // new parent / previous child
      exit(0);  // exit and make the grandchild a daemon
    }
  }

  if (clone == 0) {  // prepare child process to spawn new terminal
    if ((cterm->child.pty_fd   ) >= 0)
      close((cterm->child.pty_fd   ));
    if (log_fd >= 0)
      close(log_fd);
    close(win_fd);




    if ((cterm->child.child_dir) && *(cterm->child.child_dir)) {
      string set_dir = (cterm->child.child_dir);
      if (support_wsl) {
        wchar * wcd = cs__utftowcs((cterm->child.child_dir));
#ifdef debug_wsl
        printf("fork wsl <%ls>\n", wcd);
#endif
        wcd = dewsl(wcd);
#ifdef debug_wsl
        printf("fork wsl <%ls>\n", wcd);
#endif
        set_dir = (string)cs__wcstombs(wcd);
        delete(wcd);
      }

      chdir(set_dir);
      trace_dir(asform("child: %s", set_dir));
      setenv("PWD", set_dir, true);  // avoid softlink resolution
      // prevent shell startup from setting current directory to $HOME
      // unless cloned/Alt+F2 (!launch)
      if (!launch) {
        setenv("CHERE_INVOKING", "mintty", true);
        // if cloned and then launched from Windows shortcut (!shortcut) 
        // (by sanitizing taskbar icon grouping, #784, mintty/wsltty#96) 
        // indicate to set proper directory
        if (shortcut)
          setenv("MINTTY_PWD", set_dir, true);
      }

      if (support_wsl)
        delete(set_dir);
    }

#ifdef add_child_parameters
    // add child parameters
    int newparams = 0;
    char * * newargv = malloc((sd->argc + newparams + 1) * sizeof(char *));
    int i = 0, j = 0;
    bool addnew = true;
    while (1) {
      if (addnew && (! argv[i] || strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "-") == 0)) {
        addnew = false;
        // insert additional parameters here
        newargv[j++] = "-o";
        static char parbuf1[28];  // static to prevent #530
        sprintf(parbuf1, "Rows=%d", cterm->rows);
        newargv[j++] = parbuf1;
        newargv[j++] = "-o";
        static char parbuf2[31];  // static to prevent #530
        sprintf(parbuf2, "Columns=%d", cterm->cols);
        newargv[j++] = parbuf2;
      }
      newargv[j] = argv[i];
      if (! argv[i])
        break;
      i++;
      j++;
    }
    argv = newargv;
#else
#endif

    // provide environment to clone size
    if (!config_size) {
      setenvi("MINTTY_ROWS", cterm->rows0);
      setenvi("MINTTY_COLS", cterm->cols0);
      // provide environment to maximise window
      if (win_is_fullscreen)
        setenvi("MINTTY_MAXIMIZE", 2);
      else if (IsZoomed(wnd))
        setenvi("MINTTY_MAXIMIZE", 1);
    }
    // provide environment to select monitor
    if (moni > 0)
      setenvi("MINTTY_MONITOR", moni);
    // propagate shortcut-inherited icon
    if (icon_is_from_shortcut)
      setenv("MINTTY_ICON", cs__wcstoutf(cfg.icon), true);

    //setenv("MINTTY_CHILD", "1", true);

    // /proc/self/exe isn't available before Cygwin 1.5, so use argv[0] instead.
    // Strip enclosing quotes if present.
    int na;
    for(na=0;argv[na];na++);
    char ** new_argv = newn(char *, na+2);
    new_argv[0]=main_sd.cmd;
    for(na=0;argv[na];na++)new_argv[na+1]=argv[na];
    execvp(main_sd.cmd, new_argv);
    exit(mexit);
  }
  //reset_fork_mode();
}

/*
  Called from Alt+F2.
 */
void
child_fork(SessDef*sd, int moni, bool config_size)
{
  do_child_fork(sd, moni, false, config_size);
}
/*
  Called from session launcher.
 */
void
child_launch(int n, SessDef*sd, int moni)
{
  int argc=sd->argc;
  char **argv=sd->argv;
  if (*cfg.session_commands) {
    char * cmds = cs__wcstombs(cfg.session_commands);
    char * cmdp = cmds;
    char sepch = ';';
    if ((uchar)*cmdp <= (uchar)' ')
      sepch = *cmdp++;

    char * paramp;
    while (n >= 0 && (paramp = strchr(cmdp, ':'))) {
      paramp++;
      char * sepp = strchr(paramp, sepch);
      if (sepp) *sepp = '\0';
      if (n == 0) {
        argc = 1;
        char ** new_argv = newn(char *, argc + 1);
        new_argv[0] = argv[0];
        // prepare launch parameters from config string
        while (*paramp) {
          while (*paramp == ' ')
            paramp++;
          if (*paramp) {
            new_argv = renewn(new_argv, argc + 2);
            new_argv[argc] = paramp;
            argc++;
            while (*paramp && *paramp != ' ')
              paramp++;
            if (*paramp == ' ')
              *paramp++ = '\0';
          }
        }
        new_argv[argc] = 0;
        SessDef nsd={argc,0,sd->cmd,new_argv};
        do_child_fork(&nsd, moni, true, true);
        free(new_argv);
        break;
      }
      n--;

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
    free(cmds);
  }
}
//============================
void
child_free(STerm* pterm)
{
  if (pterm->child.pty_fd >= 0)
    close(pterm->child.pty_fd);
  pterm->child.pty_fd = -1;
}
