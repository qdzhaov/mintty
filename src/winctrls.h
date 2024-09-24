#ifndef WINCTRLS_H
#define WINCTRLS_H

#include "ctrls.h"

extern HINSTANCE inst;
typedef struct {
  HWND wnd;
  WPARAM font;
  int dlu4inpix;
  int ypos, width;
  int xoff;
  int boxystart, boxid;
  const wchar *boxtext;
} ctrlpos;

enum {
  IDCX_0703      =  703,
  IDCX_0704      =  704,
  IDCX_0705      =  705,
  IDCX_0706      =  706,
  IDCX_0707      =  707,
  IDCX_0708      =  708,
  IDCX_0712      =  712,
  IDCX_0719      =  719,
  IDCX_0723      =  723,
  IDCX_0724      =  724,
  IDCX_0725      =  725,
  IDCX_0726      =  726,
  IDCX_0727      =  727,
  IDCX_0728      =  728,
  IDCX_0730      =  730,
  IDCX_0731      =  731,
  IDCX_TVSTATIC  = 1001,
  IDCX_TREEVIEW,
  IDCX_STDBASE,
  IDCX_APPLY     = IDCX_STDBASE  + 23,//1026
  IDCX_PANELBASE = IDCX_STDBASE  + 32,//1035
  IDCX_FONTLSP   = IDCX_PANELBASE+ 38,//1073
  IDCX_FONTL     = IDCX_PANELBASE+ 53,//1088
  IDCX_FONTLST   = IDCX_PANELBASE+ 54,//1089
  IDCX_FONTLSZ   = IDCX_PANELBASE+ 55,//1090
  IDCX_FONTSP    = IDCX_PANELBASE+ 57,//1092
  IDCX_1094      = IDCX_PANELBASE+ 59,//1094
  IDCX_1136      = IDCX_PANELBASE+101,//1136
  IDCX_1137      = IDCX_PANELBASE+102,//1137
  IDCX_1138      = IDCX_PANELBASE+103,//1138
  IDCX_1140      = IDCX_PANELBASE+105,//1140
  IDCX_1592      = IDCX_PANELBASE+557,//1592
};

extern int scale_dialog(int x);
extern WPARAM diafont(void);

/*
 * Private structure for prefslist state. Only in the header file
 * so that we can delegate allocation to callers.
 */
void ctrlposinit(ctrlpos * cp, HWND wnd, int leftborder, int rightborder,
                int topborder);

#define MAX_SHORTCUTS_PER_CTRL 16

/*
 * This structure is what's stored for each `union control' in the
 * portable-dialog interface.
 */
typedef struct winctrl {
  control *ctrl;
 /*
  * The control may have several components at the Windows
  * level, with different dialog IDs. We impose the constraint that
  * those IDs must be in a contiguous block.
  */
  int base_id;
  int num_ids;
 /*
  * Some controls need a piece of allocated memory in which to
  * store temporary data about the control.
  */
  void *data;
  struct winctrl *next;
} winctrl;

/*
 * And this structure holds a set of the above
 */
typedef struct {
  winctrl *first, *last;
} winctrls;

/*
 * This describes a dialog box.
 */
typedef struct {
  HWND wnd;    /* the wnd of the dialog box */
  HWND ctlwnd;    /* the wnd of the dialog box */
  winctrls *controltrees[8];    /* can have several of these */
  int nctrltrees;
  control *focused; /* which ctrl has focus now/before */
  int coloursel_wanted; /* has an event handler asked for
                         * a colour selector? */
  colour coloursel_result;  /* 0-255 */
  bool coloursel_ok;
  int ended;            /* has the dialog been ended? */
} windlg;

extern windlg dlg;

void windlg_init(void);
void windlg_add_tree(winctrls *);

void winctrl_init(winctrls *);
void winctrl_cleanup(winctrls *);
void winctrl_layout(winctrls *, ctrlpos *, controlset *, int *id);
int winctrl_handle_command(HWND hwnd,UINT msg, WPARAM wParam, LPARAM lParam);

#endif
