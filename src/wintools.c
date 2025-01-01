//G #include "winpriv.h"
#include <mmsystem.h>  // PlaySound for MSys
//G #include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM
#include <shlwapi.h>  // PathIsNetworkPathW
#include <shellapi.h>
#include "wintools.h"
//G #include "charset.h"

#include <shlobj.h>

#include "jumplist.h"
//G #include "child.h"

#if CYGWIN_VERSION_API_MINOR < 74
#define getopt_long_only getopt_long
typedef UINT_PTR uintptr_t;
#endif


#if CYGWIN_VERSION_DLL_MAJOR >= 1007
#include <propkey.h> //PKEY_...
#endif

//G #include <fcntl.h>  // open flags,struct stat

#define dont_debug_guardpath

#ifdef debug_guardpath
#define trace_guard(p)	printf p
#else
#define trace_guard(p)	
#endif
HRESULT (WINAPI * pGetProcessDpiAwareness)(HANDLE hprocess, int * value) = 0;
HRESULT (WINAPI * pSetProcessDpiAwareness)(int value) = 0;
HRESULT (WINAPI * pGetDpiForMonitor)(HMONITOR mon, int type, uint * x, uint * y) = 0;
HRESULT (WINAPI * pDwmIsCompositionEnabled)(BOOL *) = 0;
HRESULT (WINAPI * pDwmExtendFrameIntoClientArea)(HWND, const MARGINS *) = 0;
HRESULT (WINAPI * pDwmEnableBlurBehindWindow)(HWND, void *) = 0;
HRESULT (WINAPI * pDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD) = 0;

HRESULT (WINAPI * pSetWindowCompositionAttribute)(HWND, void *) = 0;
BOOL (WINAPI * pSystemParametersInfo)(UINT, UINT, PVOID, UINT) = 0;

BOOLEAN (WINAPI * pShouldAppsUseDarkMode)(void) = 0; /* undocumented */
DWORD (WINAPI * pSetPreferredAppMode)(DWORD) = 0; /* undocumented */
HRESULT (WINAPI * pSetWindowTheme)(HWND, const wchar_t *, const wchar_t *) = 0;

#define HTHEME HANDLE
COLORREF (WINAPI * pGetThemeSysColor)(HTHEME hth, int colid) = 0;
HTHEME (WINAPI * pOpenThemeData)(HWND, LPCWSTR pszClassList) = 0;
HRESULT (WINAPI * pCloseThemeData)(HTHEME) = 0;
static BOOL (WINAPI * pGetLayeredWindowAttributes)(HWND, COLORREF *, BYTE *, DWORD *) = 0;
DPI_AWARENESS_CONTEXT (WINAPI * pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpic) = 0;
HRESULT (WINAPI * pEnableNonClientDpiScaling)(HWND win) = 0;
BOOL (WINAPI * pAdjustWindowRectExForDpi)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi) = 0;
INT (WINAPI * pGetSystemMetricsForDpi)(INT index, UINT dpi) = 0;
// Helper for loading a system library. Using LoadLibrary() directly is insecure
// because Windows might be searching the current working directory first.
static HMODULE
load_sys_library(string name)
{
  char path[MAX_PATH];
  uint len = GetSystemDirectoryA(path, MAX_PATH);
  if (len && len + strlen(name) + 1 < MAX_PATH) {
    path[len] = '\\';
    strcpy(&path[len + 1], name);
    return LoadLibraryA(path);
  }
  else
    return 0;
}

void
load_dwm_funcs(void)
{
  HMODULE dwm = load_sys_library("dwmapi.dll");
  HMODULE user32 = load_sys_library("user32.dll");
  HMODULE uxtheme = load_sys_library("uxtheme.dll");

  if (dwm) {
    pDwmIsCompositionEnabled =
      (void *)GetProcAddress(dwm, "DwmIsCompositionEnabled");
    pDwmExtendFrameIntoClientArea =
      (void *)GetProcAddress(dwm, "DwmExtendFrameIntoClientArea");
    pDwmEnableBlurBehindWindow =
      (void *)GetProcAddress(dwm, "DwmEnableBlurBehindWindow");
    pDwmSetWindowAttribute = 
      (void *)GetProcAddress(dwm, "DwmSetWindowAttribute");
  }
  if (user32) {
    pSetWindowCompositionAttribute =
      (void *)GetProcAddress(user32, "SetWindowCompositionAttribute");
    pSystemParametersInfo =
      (void *)GetProcAddress(user32, "SystemParametersInfoW");
    pGetLayeredWindowAttributes =
      (void *)GetProcAddress(user32, "GetLayeredWindowAttributes");
  }
  if (uxtheme) {
    DWORD win_version = GetVersion();
    uint build = HIWORD(win_version);
    win_version = ((win_version & 0xff) << 8) | ((win_version >> 8) & 0xff);
    //printf("Windows %d.%d Build %d\n", win_version >> 8, win_version & 0xFF, build);
    if (win_version >= 0x0A00 && build >= 17763) { // minimum version 1809
      pShouldAppsUseDarkMode = 
        (void *)GetProcAddress(uxtheme, MAKEINTRESOURCEA(132)); /* ordinal */
      pSetPreferredAppMode = 
        (void *)GetProcAddress(uxtheme, MAKEINTRESOURCEA(135)); /* ordinal */
        // this would be AllowDarkModeForApp before Windows build 18362
    }
    pSetWindowTheme = 
      (void *)GetProcAddress(uxtheme, "SetWindowTheme");

    pOpenThemeData =
      (void *)GetProcAddress(uxtheme, "OpenThemeData");
    pCloseThemeData =
      (void *)GetProcAddress(uxtheme, "CloseThemeData");
    if (pOpenThemeData && pCloseThemeData)
      pGetThemeSysColor =
        (void *)GetProcAddress(uxtheme, "GetThemeSysColor");
  }
}
void
load_dpi_funcs(void)
{
  HMODULE shc = load_sys_library("shcore.dll");
  HMODULE user = load_sys_library("user32.dll");
#ifdef debug_dpi
  printf("load_dpi_funcs shcore %d user32 %d\n", !!shc, !!user);
#endif
  if (shc) {
    pGetProcessDpiAwareness =
      (void *)GetProcAddress(shc, "GetProcessDpiAwareness");
    pSetProcessDpiAwareness =
      (void *)GetProcAddress(shc, "SetProcessDpiAwareness");
    pGetDpiForMonitor =
      (void *)GetProcAddress(shc, "GetDpiForMonitor");
  }
  if (user) {
    pSetThreadDpiAwarenessContext =
      (void *)GetProcAddress(user, "SetThreadDpiAwarenessContext");
    pEnableNonClientDpiScaling =
      (void *)GetProcAddress(user, "EnableNonClientDpiScaling");
    pAdjustWindowRectExForDpi =
      (void *)GetProcAddress(user, "AdjustWindowRectExForDpi");
    pGetSystemMetricsForDpi =
      (void *)GetProcAddress(user, "GetSystemMetricsForDpi");
  }
#ifdef debug_dpi
  printf("SetProcessDpiAwareness %d GetProcessDpiAwareness %d GetDpiForMonitor %d SetThreadDpiAwarenessContext %d EnableNonClientDpiScaling %d AdjustWindowRectExForDpi %d GetSystemMetricsForDpi %d\n", !!pSetProcessDpiAwareness, !!pGetProcessDpiAwareness, !!pGetDpiForMonitor, !!pSetThreadDpiAwarenessContext, !!pEnableNonClientDpiScaling, !!pAdjustWindowRectExForDpi, !!pGetSystemMetricsForDpi);
#endif
}

void *
load_library_func(string lib, string func)
{
  HMODULE hm = load_sys_library(lib);
  if (hm)
    return GetProcAddress(hm, func);
  return 0;
}

// WSL path conversion, using wsl.exe
static char *
wslwinpath(string path)
{
  char * wslpath(char * path)
  {
    char * wslcmd;
    // do the actual conversion with WSL wslpath -m
    // wslpath -w fails in some cases during pathname postprocessing
    // ~ needs to be unquoted to be expanded by sh
    // other paths should be quoted; pathnames with quotes are not handled
    if (*path == '~')
      wslcmd = asform("wsl -d %ls sh -c 'wslpath -m ~ 2>/dev/null'", wv.wslname);
    else
      wslcmd = asform("wsl -d %ls sh -c 'wslpath -m \"%s\" 2>/dev/null'", wv.wslname, path);
    FILE * wslpopen = popen(wslcmd, "r");
    char line[MAX_PATH + 1];
    char * got = fgets(line, sizeof line, wslpopen);
    pclose(wslpopen);
    free(wslcmd);
    if (!got)
      return 0;
    // adjust buffer
    int len = strlen(line);
    if (line[len - 1] == '\n')
      line[len - 1] = 0;
    // return path string
    if (*line)
      return strdup(line);
    else  // file does not exist
      return 0;
  }

  trace_guard(("wslwinpath %s\n", path));
  if (0 == strcmp("~", path))
    return wslpath("~");
  else if (0 == strncmp("~/", path, 2)) {
    char * wslhome = wslpath("~");
    if (!wslhome)
      return 0;
    char * ret = asform("%s/%s", wslhome, path + 2);
    free(wslhome);
    return ret;
  }
  else {
    char * abspath;
    if (*path != '/') {
      // if we have a relative pathname, let's prefix it with 
      // the current working directory if possible (and check again);
      // we cannot determine it via foreground_cwd through wslbridge, 
      // so let's check OSC 7 in this case
      if (term.child.child_dir && *term.child.child_dir)
        abspath = asform("%s/%s", term.child.child_dir, path);
      else
        abspath = strdup(path);
      trace_guard(("wslwinpath abspath %s\n", abspath));
      if (*abspath != '/') {
        // failed to determine an absolute path
        free(abspath);
        return 0;
      }
    }
    else
      abspath = strdup(path);
    char * winpath = wslpath(abspath);
    trace_guard(("wslwinpath -> %s\n", winpath));
    free(abspath);
    return winpath;
  }
}

// Safeguard checking path to guard against unexpected network access
char *
guardpath(string path, int level)
{
  if (!path)
    return 0;

  if (0 == strncmp(path, "file:", 5))
    path += 5;

  // path transformations
  char * expath;
  if (wv.support_wsl) {
    expath = wslwinpath(path);
    if (!expath)
      return 0;
  }
  else if (0 == strcmp("~", path))
    expath = strdup(wv.home);
  else if (0 == strncmp("~/", path, 2))
    expath = asform("%s/%s", wv.home, path + 2);
  else if (*path != '/' && !(*path && path[1] == ':')) {
    char * fgd = foreground_cwd(&term);
    if (fgd) {
      if (0 == strcmp("/", fgd))
        expath = asform("/%s", path);
      else
        expath = asform("%s/%s", fgd, path);
    }
    else
      return 0;
  }
  else
    expath = strdup(path);

  if (!(level & cfg.guard_path))
    // use case level is not in configured guarding bitmask
    return expath;

  wchar * wpath;

  if ((expath[0] == '/' || expath[0] == '\\') && (expath[1] == '/' || expath[1] == '\\')) {
    wpath = cs__mbstowcs(expath);
    // transform network path to Windows syntax (\ separators)
    for (wchar * p = wpath; *p; p++)
      if (*p == '/')
        *p = '\\';
  }
  else {
    // transform cygwin path to Windows drive path
    wpath = path_posix_to_win_w(expath);  // implies realpath()
  }
  trace_guard(("guardpath <%s>\n       ex <%s>\n        w <%ls>\n", path, expath ?: "(null)", wpath ?: W("(null)")));
  if (!wpath) {
    free(expath);
    return 0;
  }

  bool guard = false;

  // guard access if its target is a network path ...
  if (PathIsNetworkPathW(wpath))
    guard = true;
  else {
    char drive[] = "@:\\";
    *drive = *wpath;
    if (GetDriveTypeA(drive) == DRIVE_REMOTE)
      guard = true;
  }
  trace_guard(("   guard %d <%ls>\n", guard, wpath));
  int plen = wcslen(wpath);

  // ... but do not guard if it is in $HOME or $APPDATA
  if (guard) {
    void unguard(char * env) {
      if (env) {
        wchar * prepath = path_posix_to_win_w(env);
        if (prepath && *prepath) {
          int envlen = wcslen(prepath);
          if (0 == wcsncmp(prepath, wpath, envlen))
            if (prepath[envlen - 1] == '\\' || 
                plen <= envlen || wpath[envlen] == '\\'
               )
              guard = false;
        }
        trace_guard(("         %d <%s>\n        -> <%ls>\n", guard, env, prepath ?: W("(null)")));
        if (prepath)
          free(prepath);
      }
      else {
        trace_guard(("         null\n"));
      }
    }
    unguard(getenv("APPDATA"));
    if (wv.support_wsl) {
      //char * rootdir = path_win_w_to_posix(wsl_basepath);
      char * rootdir = wslwinpath("/");
      unguard(rootdir);
      free(rootdir);
      // in case WSL ~ is outside WSL /
      char * homedir = wslwinpath("~");
      if (homedir) {
        unguard(homedir);
        free(homedir);
      }
#ifdef consider_WSL_OSC7
#warning exemption from path guarding is not proper
      // if the WSL bridge/gateway could be used to transport the 
      // current working directory back to mintty, we could enable this
      if (term.child.child_dir && *term.child.child_dir) {
        char * cwd = wslwinpath(term.child.child_dir);
        if (cwd) {
          unguard(cwd);
          free(cwd);
        }
      }
#endif
    }
    else {
      unguard(getenv("HOME"));
      char * fg_cwd = foreground_cwd(&term);
      if (fg_cwd) {
        unguard(fg_cwd);
        free(fg_cwd);
      }
      else {
        // if tcgetpgrp / foreground_pid() / foreground_cwd() fails,
        // check for processes $p where /proc/$p/ctty is child_tty()
        // whether the checked filename is below their /proc/$p/cwd
#include <dirent.h>
        DIR * d = opendir("/proc");
        if (d) {
          char * tty = child_tty(&term);
          struct dirent * e;
          while (guard && (e = readdir(d))) {
            char * pn = e->d_name;
            int thispid = atoi(pn);
            if (thispid) {
              char * ctty = procres(thispid, "ctty");
              if (ctty) {
                if (0 == strcmp(ctty, tty)) {
                  // check cwd
                  char * fn = asform("/proc/%d/%s", thispid, "cwd");
                  char target [MAX_PATH + 1];
                  int ret = readlink (fn, target, sizeof (target) - 1);
                  free(fn);
                  if (ret >= 0) {
                    target [ret] = '\0';
                    unguard(target);
                  }
                }
                free(ctty);
              }
            }
          }
          closedir(d);
        }
      }
    }
  }
  delete(wpath);

  trace_guard(("   -> %d -> <%s>\n", guard, expath));
  if (guard) {
    free(expath);
    if (level & 0xF)  // could choose to beep or not to beep in future...
      win_bell(&cfg);
    return 0;
  }
  else
    return expath;
}
const int Process_System_DPI_Aware = 1;
const int Process_Per_Monitor_DPI_Aware = 2;
int
set_per_monitor_dpi_aware(void)
{
  int res = DPI_UNAWARE;
  // DPI handling V2: make EnableNonClientDpiScaling work, at last
  if (pSetThreadDpiAwarenessContext && cfg.handle_dpichanged == 2 &&
      pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
    res = DPI_AWAREV2;
  else if (cfg.handle_dpichanged == 1 &&
           pSetProcessDpiAwareness && pGetProcessDpiAwareness) {
    HRESULT hr = pSetProcessDpiAwareness(Process_Per_Monitor_DPI_Aware);
    // E_ACCESSDENIED:
    // The DPI awareness is already set, either by calling this API previously
    // or through the application (.exe) manifest.
    if (hr != E_ACCESSDENIED && !SUCCEEDED(hr))
      pSetProcessDpiAwareness(Process_System_DPI_Aware);

    int awareness = 0;
    if (SUCCEEDED(pGetProcessDpiAwareness(NULL, &awareness)) &&
        awareness == Process_Per_Monitor_DPI_Aware)
      res = DPI_AWAREV1;
  }
#ifdef debug_dpi
  printf("dpi_awareness %d\n", res);
#endif
  return res;
}
static HKEY
regopen(HKEY key, wstring subkey)
{
  HKEY hk = 0;
  RegOpenKeyW(key, subkey, &hk);
  return hk;
}

static void
regclose(HKEY key)
{
  if (key)
    RegCloseKey(key);
}
#if 0
#if CYGWIN_VERSION_API_MINOR >= 74
#ifdef not_used
bool
wexists(wstring fn)
{
  WIN32_FIND_DATAW ffd;
  HANDLE hFind = FindFirstFileW(fn, &ffd);
  bool ok = hFind != INVALID_HANDLE_VALUE;
  FindClose(hFind);
  return ok;
}
#endif
typedef struct SessionDef{
  int type;
  wchar*name;
  wchar*cmd;
}SessionDef;
static int
listwsl(int maxdn,SessionDef*pdn){
  static wstring lxsskeyname = W("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Lxss");
  HKEY lxss = regopen(HKEY_CURRENT_USER, lxsskeyname);
  if (!lxss) return 0;
  wchar * dd = getregstr(HKEY_CURRENT_USER, lxsskeyname, W("DefaultDistribution"));
  wchar * ddn = getregstr(lxss, dd, W("lxsskeyname"));
  VFREE(dd);
  DWORD nsubkeys = 0;
  DWORD maxlensubkey;
  DWORD ret;
  int ind=0;
  wchar * dn = getregstr(lxss, ddn, W("DistributionName"));
  pdn[ind].type=1;
  pdn[ind].name=wcsdup(_W("WSL"));
  pdn[ind++].cmd=dn;
  // prepare enumeration of distributions
  ret = RegQueryInfoKeyW(
    lxss, NULL, NULL, NULL, &nsubkeys, &maxlensubkey, 
    NULL, NULL, NULL, NULL, NULL, NULL);
  // enumerate the distribution subkeys
  DWORD keylen = maxlensubkey + 2;
  wchar subkey[keylen];
  for (uint i = 0; i < nsubkeys; i++) {
    ret = RegEnumKeyW(lxss, i, subkey, keylen);
    if (ret == ERROR_SUCCESS) {
      if (!wcscmp(subkey, ddn)) { 
        dn = getregstr(lxss, subkey, W("DistributionName"));
        wchar*s=awsform(W("Wsl:%s"),dn);
        pdn[ind].type=1;
        pdn[ind].name=s;
        pdn[ind++].cmd=dn;
        VFREE(s);
      }
      if(ind>=maxdn)break;
    }
  }
  regclose(lxss);
  VFREE(ddn);
  return ind;
}
#endif
#endif
int
getlxssinfo(bool list, wstring wslname, uint * wsl_ver,
            char ** wsl_guid, wstring * wsl_rootfs, wstring * wsl_icon)
{
  static wstring lxsskeyname = W("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Lxss");
  HKEY lxss = regopen(HKEY_CURRENT_USER, lxsskeyname);
  if (!lxss)
    return 1;

#ifdef use_wsl_getdistconf
  typedef enum
  {
    WSL_DISTRIBUTION_FLAGS_NONE = 0,
    //...
  } WSL_DISTRIBUTION_FLAGS;
  HRESULT (WINAPI * pWslGetDistributionConfiguration)
           (PCWSTR name, ULONG *distVersion, ULONG *defaultUID,
            WSL_DISTRIBUTION_FLAGS *,
            PSTR **defaultEnvVars, ULONG *defaultEnvVarCount
           ) =
    // this works only in 64 bit mode
    load_library_func("wslapi.dll", "WslGetDistributionConfiguration");
#endif

  wchar * legacy_icon()
  {
    // "%LOCALAPPDATA%/lxss/bash.ico"
    char * icf = getenv("LOCALAPPDATA");
    if (icf) {
      wchar * icon = cs__mbstowcs(icf);
      icon = renewn(icon, wcslen(icon) + 15);
      wcscat(icon, W("\\lxss\\bash.ico"));
      return icon;
    }
    return 0;
  }

  int getlxssdistinfo(bool list, HKEY lxss,const  wchar * guid)
  {
    wchar * rootfs;
    wchar * icon = 0;

    wchar * bp = getregstr(lxss, guid, W("BasePath"));
    if (!bp)
      return 3;

    wchar * pn = getregstr(lxss, guid, W("PackageFamilyName"));
    wchar * pfn = 0;
    if (pn) {  // look for installation directory and icon file
      rootfs = newn(wchar, wcslen(bp) + 8);
      wcscpy(rootfs, bp);
      wcscat(rootfs, W("\\rootfs"));
      HKEY appdata = regopen(HKEY_CURRENT_USER, W("Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\SystemAppData"));
      HKEY package = regopen(appdata, pn);
      pfn = getregstr(package, W("Schemas"), W("PackageFullName"));
      regclose(package);
      regclose(appdata);
      // "%ProgramW6432%/WindowsApps/<PackageFullName>/images/icon.ico"
      char * prf = getenv("ProgramW6432");
      if (prf && pfn) {
        icon = cs__mbstowcs(prf);
        icon = renewn(icon, wcslen(icon) + wcslen(pfn) + 30);
        wcscat(icon, W("\\WindowsApps\\"));
        wcscat(icon, pfn);
        wcscat(icon, W("\\images\\icon.ico"));
        // alternatively, icons can also be in Assets/*.png but those
        // are not in .ico file format, or in *.exe;
        // however, as the whole directory is not readable for non-admin,
        // mintty cannot check that here
      }
    }
    else {  // legacy
      rootfs = newn(wchar, wcslen(bp) + 8);
      wcscpy(rootfs, bp);
      wcscat(rootfs, W("\\rootfs"));

      char * rootdir = path_win_w_to_posix(rootfs);
      struct stat fstat_buf;
      if (stat (rootdir, & fstat_buf) == 0 && S_ISDIR (fstat_buf.st_mode)) {
        // non-app or imported deployment
      }
      else {
        // legacy Bash on Windows
        free(rootfs);
        rootfs = wcsdup(bp);
      }
      free(rootdir);

      icon = legacy_icon();
    }

    wchar * name = getregstr(lxss, guid, W("DistributionName"));
#ifdef use_wsl_getdistconf
    // this has currently no benefit, and it does not work in 32-bit cygwin
    if (pWslGetDistributionConfiguration) {
      ULONG ver, uid, varc;
      WSL_DISTRIBUTION_FLAGS flags;
      PSTR * vars;
      if (S_OK == pWslGetDistributionConfiguration(name, &ver, &uid, &flags, &vars, &varc)) {
        for (uint i = 0; i < varc; i++)
          CoTaskMemFree(vars[i]);
        CoTaskMemFree(vars);
        //printf("%d %ls %d uid %d %X\n", (int)res, name, (int)ver, (int)uid, (uint)flags);
      }
    }
#endif

    if (list) {
      printf("WSL distribution name [7m%ls[m\n", name);
      printf("-- guid %ls\n", guid);
      printf("-- flag %u\n", getregval(lxss, guid, W("Flags")));
      printf("-- root %ls\n", rootfs);
      if (pn)
        printf("-- pack %ls\n", pn);
      if (pfn)
        printf("-- full %ls\n", pfn);
      printf("-- icon %ls\n", icon);
    }

    *wsl_icon = icon;
    *wsl_ver = 1 + ((getregval(lxss, guid, W("Flags")) >> 3) & 1);
    *wsl_guid = cs__wcstoutf(guid);
    char * rootdir = path_win_w_to_posix(rootfs);
    struct stat fstat_buf;
    if (stat (rootdir, & fstat_buf) == 0 && S_ISDIR (fstat_buf.st_mode)) {
      *wsl_rootfs = rootfs;
    }
    else if (wslname) {
      free(rootfs);
      rootfs = newn(wchar, wcslen(wslname) + 8);
      wcscpy(rootfs, W("\\\\wsl$\\"));
      wcscat(rootfs, wslname);
      *wsl_rootfs = rootfs;
    }
    free(rootdir);
    return 0;
  }

  if (!list && (!wslname || !*wslname)) {
    wchar * dd = getregstr(HKEY_CURRENT_USER, lxsskeyname, W("DefaultDistribution"));
    int err;
    if (dd) {
      err = getlxssdistinfo(false, lxss, dd);
      free(dd);
    }
    else {  // Legacy "Bash on Windows" installed only, no registry info
#ifdef set_basepath_here
      // "%LOCALAPPDATA%\\lxss"
      char * icf = getenv("LOCALAPPDATA");
      if (icf) {
        wchar * rootfs = cs__mbstowcs(icf);
        rootfs = renewn(rootfs, wcslen(rootfs) + 6);
        wcscat(rootfs, W("\\lxss"));
        *wsl_rootfs = rootfs;
        *wsl_ver = 1;
        *wsl_guid = "";
        *wsl_icon = legacy_icon();
        err = 0;
      }
      else
        err = 7;
#else
      *wsl_ver = 1;
      *wsl_guid = "";
      *wsl_rootfs = W("");  // activate legacy tricks in winclip.c
      *wsl_icon = legacy_icon();
      err = 0;
#endif
    }
    regclose(lxss);
    return err;
  }
  else {
    DWORD nsubkeys = 0;
    DWORD maxlensubkey;
    DWORD ret;
    // prepare enumeration of distributions
    ret = RegQueryInfoKeyW(lxss,
                           NULL, NULL, // class
                           NULL,
                           &nsubkeys, &maxlensubkey, // subkeys
                           NULL,
                           NULL, NULL, NULL, // values
                           NULL, NULL);
    // enumerate the distribution subkeys
    for (uint i = 0; i < nsubkeys; i++) {
      DWORD keylen = maxlensubkey + 2;
      wchar subkey[keylen];
      ret = RegEnumKeyW(lxss, i, subkey, keylen);
      if (ret == ERROR_SUCCESS) {
          wchar * dn = getregstr(lxss, subkey, W("DistributionName"));
          if (list) {
            getlxssdistinfo(true, lxss, subkey);
          }
          else if (dn && 0 == wcscmp(dn, wslname)) {
            int err = getlxssdistinfo(false, lxss, subkey);
            regclose(lxss);
            return err;
          }
      }
    }
    regclose(lxss);
    return 9;
  }
}
static wstring * jumplist_title = 0;
static wstring * jumplist_cmd = 0;
static wstring * jumplist_icon = 0;
static int * jumplist_ii = 0;
static int jumplist_len = 0;

static void
cmd_enum(wstring label, wstring cmd, wstring icon, int icon_index)
{
  jumplist_title = renewn(jumplist_title, jumplist_len + 1);
  jumplist_cmd = renewn(jumplist_cmd, jumplist_len + 1);
  jumplist_icon = renewn(jumplist_icon, jumplist_len + 1);
  jumplist_ii = renewn(jumplist_ii, jumplist_len + 1);

  jumplist_title[jumplist_len] = label;
  jumplist_cmd[jumplist_len] = cmd;
  jumplist_icon[jumplist_len] = icon;
  jumplist_ii[jumplist_len] = icon_index;
  jumplist_len++;
}

static void
enum_commands(wstring commands, CMDENUMPROC cmdenum)
{
  char * cmds = cs__wcstoutf(commands);
  char * cmdp = cmds;
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

    wchar * params = cs__utftowcs(paramp);
    wstring icon = wslicon(params);  // default: 0 (no icon)
    //printf("	task <%s> args <%ls> icon <%ls>\n", cmdp, params, icon);
    cmdenum(_W(cmdp), params, icon, 0);

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
void
configure_taskbar(const wchar * app_id)
{
  if (*cfg.task_commands) {
    enum_commands(cfg.task_commands, cmd_enum);
    setup_jumplist(app_id, jumplist_len, jumplist_title, jumplist_cmd, jumplist_icon, jumplist_ii);
  }

#if CYGWIN_VERSION_DLL_MAJOR >= 1007
  // initial patch (issue #471) contributed by Johannes Schindelin
  wchar * relaunch_icon = (wchar *) cfg.icon;
  wchar * relaunch_display_name = (wchar *) cfg.app_name;
  wchar * relaunch_command = (wchar *) cfg.app_launch_cmd;

#define dont_debug_properties

  // Set the app ID explicitly, as well as the relaunch command and display name
  if (wv.prevent_pinning || (app_id && *app_id)) {
    HMODULE shell = load_sys_library("shell32.dll");
    HRESULT (WINAPI *pGetPropertyStore)(HWND hwnd, REFIID riid, void **ppv) =
      (void *)GetProcAddress(shell, "SHGetPropertyStoreForWindow");
#ifdef debug_properties
      printf("SHGetPropertyStoreForWindow linked %d\n", !!pGetPropertyStore);
#endif
    if (pGetPropertyStore) {
      IPropertyStore *pps;
      HRESULT hr;
      PROPVARIANT var;

      hr = pGetPropertyStore(wv.wnd, &IID_IPropertyStore, (void **) &pps);
#ifdef debug_properties
      printf("IPropertyStore found %d\n", SUCCEEDED(hr));
#endif
      if (SUCCEEDED(hr)) {
        // doc: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459%28v=vs.85%29.aspx
        // def: typedef struct tagPROPVARIANT PROPVARIANT: propidl.h
        // def: enum VARENUM (VT_*): wtypes.h
        // def: PKEY_*: propkey.h
        if (relaunch_command && *relaunch_command && wv.store_taskbar_properties) {
#ifdef debug_properties
          printf("AppUserModel_RelaunchCommand=%ls\n", relaunch_command);
#endif
          var.pwszVal = relaunch_command;
          var.vt = VT_LPWSTR;
          pps->lpVtbl->SetValue(pps,
              &PKEY_AppUserModel_RelaunchCommand, &var);
        }
        if (relaunch_display_name && *relaunch_display_name) {
#ifdef debug_properties
          printf("AppUserModel_RelaunchDisplayNameResource=%ls\n", relaunch_display_name);
#endif
          var.pwszVal = relaunch_display_name;
          var.vt = VT_LPWSTR;
          pps->lpVtbl->SetValue(pps,
              &PKEY_AppUserModel_RelaunchDisplayNameResource, &var);
        }
        if (relaunch_icon && *relaunch_icon) {
#ifdef debug_properties
          printf("AppUserModel_RelaunchIconResource=%ls\n", relaunch_icon);
#endif
          var.pwszVal = relaunch_icon;
          var.vt = VT_LPWSTR;
          pps->lpVtbl->SetValue(pps,
              &PKEY_AppUserModel_RelaunchIconResource, &var);
        }
        if (wv.prevent_pinning) {
          var.boolVal = VARIANT_TRUE;
#ifdef debug_properties
          printf("AppUserModel_PreventPinning=%d\n", var.boolVal);
#endif
          var.vt = VT_BOOL;
          // PreventPinning must be set before setting ID
          pps->lpVtbl->SetValue(pps,
              &PKEY_AppUserModel_PreventPinning, &var);
        }
#ifdef set_userpinned
DEFINE_PROPERTYKEY(PKEY_AppUserModel_StartPinOption, 0x9f4c2855,0x9f79,0x4B39,0xa8,0xd0,0xe1,0xd4,0x2d,0xe1,0xd5,0xf3,12);
#define APPUSERMODEL_STARTPINOPTION_USERPINNED 2
#warning needs Windows 8/10 to build...
        {
          var.uintVal = APPUSERMODEL_STARTPINOPTION_USERPINNED;
#ifdef debug_properties
          printf("AppUserModel_StartPinOption=%d\n", var.uintVal);
#endif
          var.vt = VT_UINT;
          pps->lpVtbl->SetValue(pps,
              &PKEY_AppUserModel_StartPinOption, &var);
        }
#endif
        if (app_id && *app_id) {
#ifdef debug_properties
          printf("AppUserModel_ID=%ls\n", app_id);
#endif
          var.pwszVal = (wchar*)app_id;
          var.vt = VT_LPWSTR;  // VT_EMPTY should remove but has no effect
          pps->lpVtbl->SetValue(pps,
              &PKEY_AppUserModel_ID, &var);
        }

        pps->lpVtbl->Commit(pps);
        pps->lpVtbl->Release(pps);
      }
    }
  }
#endif
}

void
win_update_glass(bool opaque)
{
  bool glass = !(opaque && term.has_focus)
               //&& !wv.win_is_fullscreen
               && cfg.transparency == TR_GLASS
               //&& cfg.glass // decouple glass mode from transparency setting
               ;

  if (pDwmExtendFrameIntoClientArea) {
    pDwmExtendFrameIntoClientArea(wv.wnd, &(MARGINS){glass ? -1 : 0, 0, 0, 0});
  }

  if (pSetWindowCompositionAttribute) {
    enum AccentState
    {
      ACCENT_DISABLED = 0,
      ACCENT_ENABLE_GRADIENT = 1,
      ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
      ACCENT_ENABLE_BLURBEHIND = 3,
      ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
      ACCENT_ENABLE_HOSTBACKDROP = 5,
      ACCENT_INVALID_STATE = 6
    };
    enum WindowCompositionAttribute
    {
      WCA_ACCENT_POLICY = 19,
      WCA_USEDARKMODECOLORS = 26, // does not yield the desired effect (#1005)
    };
    struct ACCENTPOLICY
    {
      //enum AccentState nAccentState;
      int nAccentState;
      int nFlags;
      int nColor;
      int nAnimationId;
    };
    struct WINCOMPATTRDATA
    {
      //enum WindowCompositionAttribute attribute;
      DWORD attribute;
      PVOID pData;
      ULONG dataSize;
    };
    struct ACCENTPOLICY policy = {
      glass ? ACCENT_ENABLE_BLURBEHIND : ACCENT_DISABLED,
      0,
      0,
      0
    };
    struct WINCOMPATTRDATA data = {
      WCA_ACCENT_POLICY,
      (PVOID)&policy,
      sizeof(policy)
    };

    //printf("SetWindowCompositionAttribute %d\n", policy.nAccentState);
    pSetWindowCompositionAttribute(wv.wnd, &data);
  }
}

void
win_dark_mode(HWND w)
{
  if (pShouldAppsUseDarkMode) {
    HIGHCONTRASTW hc;
    hc.cbSize = sizeof hc;
    pSystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof hc, &hc, 0);
    //printf("High Contrast scheme <%ls>\n", hc.lpszDefaultScheme);

    if (!(hc.dwFlags & HCF_HIGHCONTRASTON) && pShouldAppsUseDarkMode()) {
      pSetWindowTheme(w, W("DarkMode_Explorer"), NULL);

      // set DWMWA_USE_IMMERSIVE_DARK_MODE; needed for titlebar
      BOOL dark = 1;
      if (S_OK != pDwmSetWindowAttribute(w, 20, &dark, sizeof dark)) {
        // this would be the call before Windows build 18362
        pDwmSetWindowAttribute(w, 19, &dark, sizeof dark);
      }
    }
  }
}
LPARAM
screentoclient(HWND wnd, LPARAM lp)
{
  POINT wpos = {.x = GET_X_LPARAM(lp), .y = GET_Y_LPARAM(lp)};
  ScreenToClient(wnd, &wpos);
  return MAKELPARAM(wpos.x, wpos.y);
}

bool
in_client_area(HWND wnd, LPARAM lp)
{
  POINT wpos = {.x = GET_X_LPARAM(lp), .y = GET_Y_LPARAM(lp)};
  ScreenToClient(wnd, &wpos);
  int height, width;
  win_get_pixels(&height, &width, false);
  height += OFFSET + 2 * PADDING;
  width += 2 * PADDING;
  return wpos.y >= 0 && wpos.y < height && wpos.x >= 0 && wpos.x < width;
}
char * getlocalappdata(void)
{
  // get appx-redirected system dir, as investigated by Biswapriyo Nath
#ifndef KF_FLAG_FORCE_APP_DATA_REDIRECTION
#define KF_FLAG_FORCE_APP_DATA_REDIRECTION 0x00080000
#endif
  HMODULE shell = load_sys_library("shell32.dll");
  HRESULT (WINAPI *pSHGetKnownFolderPath)(const GUID*, DWORD, HANDLE, wchar**) =
      (void *)GetProcAddress(shell, "SHGetKnownFolderPath");
  if (!pSHGetKnownFolderPath)
    return 0;
  wchar * wlappdata;
  long hres = pSHGetKnownFolderPath(&FOLDERID_LocalAppData, KF_FLAG_FORCE_APP_DATA_REDIRECTION, 0, &wlappdata);
  if (hres)
    return 0;
  else
    return path_win_w_to_posix(wlappdata);
}
//(void)listwsl;
wstring
wslicon(const wchar * params)
{
  wstring icon = 0;  // default: no icon
#if CYGWIN_VERSION_API_MINOR >= 74
  wchar * wsl = wcsstr(params, W("--WSL"));
  if (wsl) {
    wsl += 5;
    if (*wsl == '=')
      wsl++;
    else if (*wsl <= ' ')
      ; // SP or NUL: no WSL distro specified
    else
      wsl = 0;
  }
  if (wsl) {
    wchar * sp = wcsstr(wsl, W(" "));
    int len;
    if (sp)
      len = sp - wsl;
    else
      len = wcslen(wsl);
    if (len) {
      wchar * wslname = newn(wchar, len + 1);
      wcsncpy(wslname, wsl, len);
      wslname[len] = 0;
      uint ver;
      char * guid;
      wstring basepath;
      int err = getlxssinfo(false, wslname, &ver, &guid, &basepath, &icon);
      free(wslname);
      if (!err) {
        delete(basepath);
        free(guid);
      }
    }
    if (!icon) {  // no WSL distro specified or failed to find icon
      char * wslico = get_resource_file("icon", "wsl.ico", false);
      if (wslico) {
        icon = path_posix_to_win_w(wslico);
        free(wslico);
      }
      else {
        char * lappdata = getenv("LOCALAPPDATA");
        if (lappdata && *lappdata) {
          wslico = asform("%s/wsltty/wsl.ico", lappdata);
          icon = cs__mbstowcs(wslico);
          free(wslico);
        }
      }
    }
  }
#else
  (void)params;
#endif
  return icon;
}
void SetAppID(const wchar_t*app_id)
{
  if (*app_id && wcscmp(app_id, W("@")) != 0) {
    HMODULE shell = load_sys_library("shell32.dll");
    HRESULT (WINAPI *pSetAppID)(PCWSTR) =
      (void *)GetProcAddress(shell, "SetCurrentProcessExplicitAppUserModelID");

    if (pSetAppID)
      pSetAppID(app_id);
  }
}
void
taskbar_progress(int i)
{
#if CYGWIN_VERSION_API_MINOR >= 74
int last_i = 0;
  if (i == last_i)
    return;
  //printf("taskbar_progress %d detect %d\n", i, term.detect_progress);

  ITaskbarList3 * tbl;
  HRESULT hres = CoCreateInstance(&CLSID_TaskbarList, NULL,
                                  CLSCTX_INPROC_SERVER,
                                  &IID_ITaskbarList3, (void **) &tbl);
  if (!SUCCEEDED(hres))
    return;

  hres = tbl->lpVtbl->HrInit(tbl);
  if (!SUCCEEDED(hres)) {
    tbl->lpVtbl->Release(tbl);
    return;
  }

  if (i >= 0)
    hres = tbl->lpVtbl->SetProgressValue(tbl, wv.wnd, i, 100);
  else if (i == -1)
    hres = tbl->lpVtbl->SetProgressState(tbl, wv.wnd, TBPF_NORMAL);
  else if (i == -2)
    hres = tbl->lpVtbl->SetProgressState(tbl, wv.wnd, TBPF_PAUSED);
  else if (i == -3)
    hres = tbl->lpVtbl->SetProgressState(tbl, wv.wnd, TBPF_ERROR);
  else if (i == -8)
    hres = tbl->lpVtbl->SetProgressState(tbl, wv.wnd, TBPF_INDETERMINATE);
  else if (i == -9)
    hres = tbl->lpVtbl->SetProgressState(tbl, wv.wnd, TBPF_NOPROGRESS);

  last_i = i;

  tbl->lpVtbl->Release(tbl);
#else
  (void)i;
#endif
}
wchar *
get_shortcut_icon_location(const wchar * iconfile, bool * wdpresent)
{
  IShellLinkW * shell_link;
  IPersistFile * persist_file;
  HRESULT hres = OleInitialize(NULL);
  if (hres != S_FALSE && hres != S_OK)
    return 0;

  hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IShellLinkW, (voidrefref) &shell_link);
  if (!SUCCEEDED(hres))
    return 0;

  hres = shell_link->lpVtbl->QueryInterface(shell_link, &IID_IPersistFile,
                                            (voidrefref) &persist_file);
  if (!SUCCEEDED(hres)) {
    shell_link->lpVtbl->Release(shell_link);
    return 0;
  }

  /* Load the shortcut.  */
  hres = persist_file->lpVtbl->Load(persist_file, iconfile, STGM_READ);

  wchar * result = 0;

  if (SUCCEEDED(hres)) {
    WCHAR wil[MAX_PATH + 1];
    * wil = 0;
    int index;
    hres = shell_link->lpVtbl->GetIconLocation(shell_link, wil, MAX_PATH, &index);
    if (!SUCCEEDED(hres) || !*wil)
      goto iconex;

    wchar * wicon = wil;

    /* Append ,icon-index if non-zero.  */
    wchar * widx = W("");
    if (index) {
      char idx[22];
      sprintf(idx, ",%d", index);
      widx = cs__mbstowcs(idx);
    }

    /* Resolve leading Windows environment variable component.  */
    wchar * wenv = W("");
    wchar * fin;
    if (wil[0] == '%' && wil[1] && wil[1] != '%' && (fin = wcschr(&wil[2], '%'))) {
      char var[fin - wil];
      char * cop = var;
      wchar * v;
      for (v = &wil[1]; *v != '%'; v++) {
        if (wil[2] == 'y' && *v >= 'a' && *v <= 'z')
          // capitalize %SystemRoot%
          *cop = *v - 'a' + 'A';
        else
          *cop = *v;
        cop++;
      }
      *cop = '\0';
      v ++;
      wicon = v;

      char * val = getenv(var);
      if (val) {
        wenv = cs__mbstowcs(val);
      }
    }

    result = newn(wchar, wcslen(wenv) + wcslen(wicon) + wcslen(widx) + 1);
    wcscpy(result, wenv);
    wcscpy(&result[wcslen(result)], wicon);
    wcscpy(&result[wcslen(result)], widx);
    if (* widx)
      free(widx);
    if (* wenv)
      free(wenv);

    // also retrieve working directory:
    if (wdpresent) {
      hres = shell_link->lpVtbl->GetWorkingDirectory(shell_link, wil, MAX_PATH);
      *wdpresent = SUCCEEDED(hres) && *wil;
    }
#ifdef use_shortcut_description
    // also retrieve shortcut description:
    static wchar * shortcut = 0;
    uint sdlen = 55;
    wchar * sd = newn(wchar, sdlen + 1);
    do {
      // Note: this is the "Comment:" field, not the shortcut name
      hres = shell_link->lpVtbl->GetDescription(shell_link, sd, sdlen);
      if (hres != S_OK)
        break;
      if (wcslen(sd) < sdlen - 1) {
        shortcut = wcsdup(sd);
        break;
      }
      sdlen += 55;
      sd = renewn(sd, sdlen + 1);
    } while (true);
    delete(sd);
#endif
  }
  iconex:

  /* Release the pointer to the IPersistFile interface. */
  persist_file->lpVtbl->Release(persist_file);

  /* Release the pointer to the IShellLink interface. */
  shell_link->lpVtbl->Release(shell_link);

  return result;
}
wchar *
get_shortcut_appid(const wchar * shortcut)
{
#if CYGWIN_VERSION_API_MINOR >= 74
  DWORD win_version = GetVersion();
  win_version = ((win_version & 0xff) << 8) | ((win_version >> 8) & 0xff);
  if (win_version < 0x0601)
    return 0;  // PropertyStore not supported on Windows XP

  HRESULT hres = OleInitialize(NULL);
  if (hres != S_FALSE && hres != S_OK)
    return 0;

  IShellLink * link;
  hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                          &IID_IShellLink, (voidrefref) &link);
  if (!SUCCEEDED(hres))
    return 0;

  wchar * res = 0;

  IPersistFile * file;
  hres = link->lpVtbl->QueryInterface(link, &IID_IPersistFile, (voidrefref) &file);
  if (!SUCCEEDED(hres))
    goto rel1;

  hres = file->lpVtbl->Load(file, (LPCOLESTR)shortcut, STGM_READ | STGM_SHARE_DENY_NONE);
  if (!SUCCEEDED(hres))
    goto rel2;

  IPropertyStore * store;
  hres = link->lpVtbl->QueryInterface(link, &IID_IPropertyStore, (voidrefref) &store);
  if (!SUCCEEDED(hres))
    goto rel3;

  PROPVARIANT pv;
  hres = store->lpVtbl->GetValue(store, &PKEY_AppUserModel_ID, &pv);
  if (!SUCCEEDED(hres))
    goto rel3;

  if (pv.vt == VT_LPWSTR)
    res = wcsdup(pv.pwszVal);

  PropVariantClear(&pv);
rel3:
  store->lpVtbl->Release(store);
rel2:
  file->lpVtbl->Release(file);
rel1:
  link->lpVtbl->Release(link);

  return res;
#else
  (void)shortcut;
  return 0;
#endif
}
