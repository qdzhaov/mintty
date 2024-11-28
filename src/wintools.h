//G #include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM
#if WINVER < 0x600

typedef struct {
  int cxLeftWidth;
  int cxRightWidth;
  int cyTopHeight;
  int cyBottomHeight;
} MARGINS;

#else

#include <uxtheme.h>

#endif

#if CYGWIN_VERSION_DLL_MAJOR >= 1005
typedef void * * voidrefref;
#else
typedef void * voidrefref;
#define STARTF_TITLEISLINKNAME 0x00000800
#define STARTF_TITLEISAPPID 0x00001000
#endif
#define DPI_UNAWARE 0
#define DPI_AWAREV1 1
#define DPI_AWAREV2 2
typedef void (* CMDENUMPROC)(wstring label, wstring cmd, wstring icon, int icon_index);
extern HRESULT (WINAPI * pGetProcessDpiAwareness)(HANDLE hprocess, int * value) ;
extern HRESULT (WINAPI * pSetProcessDpiAwareness)(int value) ;
extern HRESULT (WINAPI * pGetDpiForMonitor)(HMONITOR mon, int type, uint * x, uint * y) ;
extern HRESULT (WINAPI * pDwmIsCompositionEnabled)(BOOL *) ;
extern HRESULT (WINAPI * pDwmExtendFrameIntoClientArea)(HWND, const MARGINS *) ;
extern HRESULT (WINAPI * pDwmEnableBlurBehindWindow)(HWND, void *) ;
extern HRESULT (WINAPI * pDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD) ;

extern HRESULT (WINAPI * pSetWindowCompositionAttribute)(HWND, void *) ;
extern BOOL (WINAPI * pSystemParametersInfo)(UINT, UINT, PVOID, UINT) ;

extern BOOLEAN (WINAPI * pShouldAppsUseDarkMode)(void) ; /* undocumented */
extern DWORD (WINAPI * pSetPreferredAppMode)(DWORD) ; /* undocumented */
extern HRESULT (WINAPI * pSetWindowTheme)(HWND, const wchar_t *, const wchar_t *) ;

#define HTHEME HANDLE
extern COLORREF (WINAPI * pGetThemeSysColor)(HTHEME hth, int colid) ;
extern HTHEME (WINAPI * pOpenThemeData)(HWND, LPCWSTR pszClassList) ;
extern HRESULT (WINAPI * pCloseThemeData)(HTHEME) ;
extern DPI_AWARENESS_CONTEXT (WINAPI * pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpic) ;
extern HRESULT (WINAPI * pEnableNonClientDpiScaling)(HWND win) ;
extern BOOL (WINAPI * pAdjustWindowRectExForDpi)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi) ;
extern INT (WINAPI * pGetSystemMetricsForDpi)(INT index, UINT dpi) ;
LPARAM screentoclient(HWND wnd, LPARAM lp);
bool in_client_area(HWND wnd, LPARAM lp);
int select_WSL(const char * wsl);
char * getlocalappdata(void);
void load_dwm_funcs(void);
void load_dpi_funcs(void);
void configure_taskbar(const wchar * app_id);
void win_update_glass(bool opaque);
wstring wslicon(const wchar * params);
int set_per_monitor_dpi_aware(void);
void SetAppID(const wchar_t*app_id);
int getlxssinfo(bool list, wstring wslname, uint * wsl_ver,
            char ** wsl_guid, wstring * wsl_rootfs, wstring * wsl_icon);
void taskbar_progress(int i);
wchar * get_shortcut_icon_location(const wchar * iconfile, bool * wdpresent);
wchar * get_shortcut_appid(const wchar * shortcut);
