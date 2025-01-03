// wintip.c (part of mintty)
// Copyright 2008-10 Andy Koppe
// Adapted from code from PuTTY-0.60 by Simon Tatham and team.
// Licensed under the terms of the GNU General Public License v3 or later.

//G #include "winpriv.h"

static HWND tip_wnd = 0;
static ATOM tip_class = 0;
static HFONT tip_font;
static COLORREF tip_bg;
static COLORREF tip_text;
static int tipshow=0;
static wchar sizetip[32] = W("");
static wchar *tipstr=NULL;

static LRESULT CALLBACK
tip_proc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
  switch (nMsg) {
    when WM_DESTROY:
      DeleteObject(tip_font);
      tip_font = null;
    when WM_ERASEBKGND: return true;
    when WM_NCHITTEST:  return HTTRANSPARENT;
    when WM_SETTEXT: {
      //LPCTSTR str = (LPCTSTR) lParam;
      string str = (string) lParam;
      HDC dc = CreateCompatibleDC(null);
      SelectObject(dc, tip_font);
      SIZE sz;
      GetTextExtentPoint32A(dc, str, strlen(str), &sz);
      SetWindowPos(hWnd, null, 0, 0, sz.cx + 6, sz.cy + 6,
                   SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
      InvalidateRect(hWnd, null, false);
      DeleteDC(dc);
    }
    when WM_PAINT: {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(hWnd, &ps);

      SelectObject(dc, tip_font);
      SelectObject(dc, GetStockObject(BLACK_PEN));

      HBRUSH hbr = CreateSolidBrush(tip_bg);
      HGDIOBJ holdbr = SelectObject(dc, hbr);

      RECT cr;
      GetClientRect(hWnd, &cr);
      Rectangle(dc, cr.left, cr.top, cr.right, cr.bottom);

      SetTextColor(dc, tip_text);
      SetBkColor(dc, tip_bg);
      int wtlen = GetWindowTextLengthA(hWnd);
      char wt[wtlen + 1];
      GetWindowTextA(hWnd, wt, wtlen + 1);
      TextOutA(dc, cr.left + 3, cr.top + 3, wt, wtlen);

      SelectObject(dc, holdbr);
      DeleteObject(hbr);

      EndPaint(hWnd, &ps);
      return 0;
    }
  }
  return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

void
win_show_tip(int x, int y, wstring text)
{
  if (!tip_wnd) {
    NONCLIENTMETRICS nci;

   /* First make sure the window class is registered */
    if (!tip_class) {
      WNDCLASSA wc;
      wc.style = CS_HREDRAW | CS_VREDRAW;
      wc.lpfnWndProc = tip_proc;
      wc.cbClsExtra = 0;
      wc.cbWndExtra = 0;
      wc.hInstance = wv.inst;
      wc.hIcon = null;
      wc.hCursor = null;
      wc.hbrBackground = null;
      wc.lpszMenuName = null;
      wc.lpszClassName = "SizeTipClass";
      tip_class = RegisterClassA(&wc);
    }

   /* Prepare other GDI objects and drawing info */
    tip_bg = win_get_sys_colour(COLOR_INFOBK);
    tip_text = win_get_sys_colour(COLOR_INFOTEXT);
    memset(&nci, 0, sizeof(NONCLIENTMETRICS));
    nci.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS),
                         &nci, 0);
    tip_font = CreateFontIndirect(&nci.lfStatusFont);
    tip_wnd =
      CreateWindowExA(WS_EX_TOOLWINDOW,  // don't include WS_EX_TOPMOST
                      MAKEINTRESOURCEA(tip_class), null, WS_POPUP, x, y, 1, 1,
                      null, null, wv.inst, null);
    ShowWindow(tip_wnd, SW_SHOWNOACTIVATE);
  }
  else {
    if(!tipshow) ShowWindow(tip_wnd, SW_SHOWNOACTIVATE);
    SetWindowPos(tip_wnd, null, x, y, 0, 0,
                 SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
  }
  tipshow=1;
  if(tipstr){
    if(wcscmp(tipstr,text)){
      delete(tipstr);
      tipstr=wcsdup(text);
    SetWindowTextW(tip_wnd, tipstr);
    }
  }else {
    tipstr=wcsdup(text);
    SetWindowTextW(tip_wnd, tipstr);
  }
}
void
win_show_tip_size(int x, int y, int cols, int rows)
{
  swprintf(sizetip,32, W("%dx%d"), cols, rows);
  win_show_tip(x, y, sizetip);
}

void
win_hide_tip(void)
{
  if (tip_wnd) {
    tipshow=0;
    ShowWindow(tip_wnd, SW_HIDE);
  }
}
void
win_destroy_tip(void)
{
  if (tip_wnd) {
    DestroyWindow(tip_wnd);
    tip_wnd = null;
    tipshow=0;
  }
}
