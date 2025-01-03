#ifndef CTRLS_H
#define CTRLS_H

#include "config.h"
#define MAXCOLS 20
/* This is the big union which defines a single control, of any type.
 * 
 * General principles:
 *  - _All_ pointers in this structure are expected to point to
 *    dynamically allocated things, unless otherwise indicated.
 *  - The `label' field can often be null, which will cause the control
 *    to not have a label at all. This doesn't apply to
 *    checkboxes and push buttons, in which the label is not
 *    separate from the control.
 */

enum {
  CTRL_EDITBOX,    /* label plus edit box */
  CTRL_INTEDITBOX, /* label plus edit box */
  CTRL_RADIO,      /* label plus radio buttons */
  CTRL_CHECKBOX,   /* checkbox (contains own label) */
  CTRL_BUTTON,     /* simple push button (no label) */
  CTRL_LISTBOX,    /* label plus list box */
  CTRL_LISTVIEW,   /* label plus list box */
  CTRL_COLUMNS,    /* divide window into columns */
  CTRL_FONTSELECT, /* label plus font selector */
  CTRL_FILESELECT,
  CTRL_LABEL,      /* static text/label only */
  CTRL_CLRBUTTON,  /* color push button (no label) */
  CTRL_HOTKEY,     /* hotkey edit*/
};

/* Each control has an `int' field specifying which columns it
 * occupies in a multi-column part of the dialog box. These macros
 * pack and unpack that field.
 * 
 * If a control belongs in exactly one column, just specifying the
 * column number is perfectly adequate.
 */
#define COLUMN_FIELD(start, span) ( (((span)-1) << 16) + (start) )
#define COLUMN_START(field) ( (field) & 0xFFFF )
#define COLUMN_SPAN(field) ( (((field) >> 16) & 0xFFFF) + 1 )

/* The number of event types is being deliberately kept small, on
 * the grounds that not all platforms might be able to report a
 * large number of subtle events. We have:
 *  - the special REFRESH event, called when a control's value needs setting
 *  - the ACTION event, called when the user does something that
 *    positively requests action (double-clicking a list box item,
 *    or pushing a push-button)
 *  - the VALCHANGE event, called when the user alters the setting
 *    of the control in a way that is usually considered to alter
 *    the underlying data (toggling a checkbox or radio button,
 *    moving the items around in a drag-list, editing an edit control)
 *  - the SELCHANGE event, called when the user alters the setting
 *    of the control in a more minor way (changing the selected
 *    item in a list box).
 *  - the CALLBACK event, which happens after the handler routine
 *    has requested a subdialog (file selector, font selector,
 *    colour selector) and it has come back with information.
 */
enum {
  EVENT_REFRESH,
  EVENT_ACTION,
  EVENT_VALCHANGE,
  EVENT_SELCHANGE,
  EVENT_UNFOCUS,
  EVENT_CALLBACK,
  EVENT_DROP
};

typedef struct {
  char buf[128];
  int ind;
  UINT *pflg;
}vhotkey;
extern wstring dragndrop;  // drop drag-and-drop contents here

typedef struct control control;

typedef void (* handler_fn)(control *,int cid,int event,LPARAM p);

struct control {
  int type;
  /* Every control except CTRL_COLUMNS has _some_ sort of label.
   * By putting it in the `generic' union as well as everywhere else,
   * we avoid having to have an irritating switch statement when we
   * go through and deallocate all the memory in a config-box structure.
   * 
   * Yes, this does mean that any non-null value in this field
   * is expected to be dynamically allocated and freeable.
   * 
   * For CTRL_COLUMNS, this field MUST be null.
   */
  const wchar * label;
  const wchar * tip;
  /* Indicate which column(s) this control occupies. This can be unpacked
   * into starting column and column span by the COLUMN macros above.
   */
  int column;
  /* Most controls need to provide a function which gets called
   * when that control's setting is changed,
   * or when the control's setting needs initialising.
   * 
   * The `data' parameter points to the writable data being modified
   * as a result of the configuration activity; for example,
   * the `Config' structure, although not necessarily.
   * 
   * The `dlg' parameter is passed back to the platform-specific routines
   * to read and write the actual control state.
   */
  handler_fn handler;
  /* Identify a drag-and-drop target control by its widget ("Window") 
   * as due to the obscure wisdom of Windows design, drag-and-drop events 
   * are handled completely differently from other events and particularly 
   * do not provide a "Control identifier".
   */
  void * widget;
  /* Almost all of the above functions will find it useful to
   * be able to store a piece of `void *' data.
   */
  uint base_id;
  void * context;
  struct _controlset*parent;
  int ind;
  union {
    struct {//20
      /* Percentage of the dialog-box width used by the edit box.
       * If this is set to 100, the label is on its own line;
       * otherwise the label is on the same line as the box itself.
       */
      int percentwidth;
      int password;       /* details of input are hidden */
      /* A special case of the edit box is the combo box, which has
       * a drop-down list built in. (Note that a _non_-editable
       * drop-down list is done as a special case of a list box.)
       */
      int has_list;
      /* for int val*/
      int imin,imax;
    } editbox;
    struct { //24
      /* There are separate fields for `ncolumns' and `nbuttons'
       * for several reasons.
       * Firstly, we sometimes want the last of a set of buttons
       * to have a longer label than the rest; we achieve this by
       * setting `ncolumns' higher than `nbuttons', and the
       * layout code is expected to understand that the final button
       * should be given all the remaining space on the line.
       * This sounds like a ludicrously specific special case
       * (if we're doing this sort of thing, why not have
       * the general ability to have a particular button span
       * more than one column whether it's the last one or not?)
       * but actually it's reasonably common for the sort of
       * three-way control we get a lot of:
       * `yes' versus `no' versus `some more complex way to decide'.
       * Secondly, setting `nbuttons' higher than `ncolumns' lets us
       * have more than one line of radio buttons for a single setting.
       * A very important special case of this is setting `ncolumns' to 1,
       * so that each button is on its own line.
       */
      int ncolumns;
      int nbuttons;
      /* This points to a dynamically allocated array of `char *' pointers,
       * each of which points to a dynamically allocated string.
       */
      wstring * labels;     /* `nbuttons' button labels */
      /* This points to a dynamically allocated array,
       * with the value corresponding to each button.
       */
      int * vals;       /* `nbuttons' entries; may be null */
    } radio;
    struct {//8
      /* At least Windows has the concept of a `default push button',
       * which gets implicitly pressed when you hit.
       * Return even if it doesn't have the input focus.
       */
      int isdefault;
      /* Also, the reverse of this: a default cancel-type button,
       * which is implicitly pressed when you hit Escape.
       */
      int iscancel;
    } button;
    struct {//20
      /* Height of the list box, in approximate number of lines.
       * If this is zero, the list is a drop-down list.
       */
      int height;       /* height in lines */
      /* Percentage of the dialog-box width used by the list box.
       * If this is set to 100, the label is on its own line;
       * otherwise the label is on the same line as the box itself.
       * Setting this to anything other than 100 is not guaranteed
       * to work on a _non_-drop-down list, so don't try it!
       */
      int percentwidth;
      /* Some list boxes contain strings that contain tab characters.
       * If `ncols' is greater than 0, then `percentages' is expected
       * to be non-zero and to contain the respective widths of
       * `ncols' columns, which together will exactly fit the width 
       * of the list box. Otherwise `percentages' must be null.
       */
      int ncols;  /* number of columns */
      int * percentages;   /* % width of each column */
    } listbox;
    struct {//20
      /* Height of the list box, in approximate number of lines.
       * If this is zero, the list is a drop-down list.
       */
      int height;       /* height in lines */
      /* Percentage of the dialog-box width used by the list box.
       * If this is set to 100, the label is on its own line;
       * otherwise the label is on the same line as the box itself.
       * Setting this to anything other than 100 is not guaranteed
       * to work on a _non_-drop-down list, so don't try it!
       */
      int percentwidth;
      /* Some list boxes contain strings that contain tab characters.
       * If `ncols' is greater than 0, then `percentages' is expected
       * to be non-zero and to contain the respective widths of
       * `ncols' columns, which together will exactly fit the width 
       * of the list box. Otherwise `percentages' must be null.
       */
      int ncols;  /* number of columns */
      wstring*cnames;
      int * percentages;   /* % width of each column */
    } listview;
    struct {//12
      /* In this variant, `label' MUST be null. */
      int ncols;                /* number of columns */
      int * percentages;        /* % width of each column */
      /* Every time this control type appears, exactly one of `ncols'
       * and the previous number of columns MUST be one.
       * Attempting to allow a seamless transition from a four-column
       * to a five-column layout, for example, would be way more
       * trouble than it was worth. If you must lay things out
       * like that, define eight unevenly sized columns and use
       * column-spanning a lot. But better still, just don't.
       * 
       * `percentages' may be null if ncols==1, to save space.
       */
    } columns;
  };

  /* Space for storing platform-specific control data */
  void * plat_ctrl;
};

#undef STANDARD_PREFIX

/* `controlset' is a container holding an array of `control' structures,
 * together with a panel name and a title for the whole set.
 * In Windows and any similar-looking GUI, each `controlset'
 * in the config will be a container box within a panel.
 */
typedef struct _controlset{
  const wchar * pathname;      /* panel path, e.g. "SSH/Tunnels" */
  const wchar * boxtitle;      /* title of container box */
  int ncolumns;         /* current no. of columns at bottom */
  int ncontrols;        /* number of `control' in array */
  int ctrlsize;         /* allocated size of array */
  control * * ctrls;    /* actual array */
} controlset;

typedef struct {
  string name;
  int val;
} opt_val;
/* This is the container structure which holds a complete set of
 * controls.
 */
typedef struct {
  int nctrlsets;                /* number of ctrlsets */
  int ctrlsetsize;              /* ctrlset size */
  controlset * * ctrlsets;      /* actual array of ctrlsets */
  int nfrees;
  int freesize;
  void * * frees;               /* array of aux data areas to free */
} controlbox;

extern controlbox * ctrl_new_box(void);
extern void ctrl_free_box(controlbox *);

/* Standard functions used for populating a controlbox structure.
*/

/* Create a controlset. */
extern controlset * ctrl_new_set(controlbox *, const wchar * path, 
                                 const wchar * panel, const wchar * title);
extern void ctrl_free_set(controlset *);

extern void ctrl_free(control *);

/* This function works like `malloc', but the memory it returns
 * will be automatically freed when the controlbox is freed.
 * Note that a controlbox is a dialog-box _template_, not an instance,
 * and so data allocated through this function is better not used
 * to hold modifiable per-instance things. It's mostly here for
 * allocating structures to be passed as control handler params.
 */
extern void * ctrl_alloc(controlbox *, size_t size);

/* Individual routines to create `control' structures in a controlset.
 * 
 * Most of these routines allow the most common fields to be set directly,
 * and put default values in the rest. Each one returns a pointer
 * to the `control' it created, so that final tweaks can be made.
 */

/* `ncolumns' is followed by that many percentages, as integers. */
extern control * ctrl_columns(controlset *, int ncolumns, ...);
extern control * ctrl_columnsa(controlset *s, int ncolumns,int *cols);

extern control * ctrl_label(controlset *, int col, const wchar * label,const wchar * tip);
extern control * ctrl_inteditbox(controlset *, int col, const wchar * label,const wchar * tip, int percentage,
                              handler_fn handler, void * context,int imin,int imax);
extern control * ctrl_editbox(controlset *, int col, const wchar * label,const wchar * tip, int percentage,
                              handler_fn handler, void * context);
extern control * ctrl_combobox(controlset *, int col, const wchar * label,const wchar * tip, int percentage,
                               handler_fn handler, void * context);
extern control * ctrl_listbox(controlset *, int col, const wchar * label,const wchar * tip, int lines, int percentage,
                              handler_fn handler, void * context);
control * ctrl_listview(controlset *s, int col, const wchar * label,const wchar * tip, int lines, int percentage,int ncols,opt_val*pv,
              handler_fn handler, void *context) ;
/* `ncolumns' is followed by (alternately) radio button titles and integers,
 * until a null in place of a title string is seen.
 */
extern control * ctrl_radiobuttons(controlset *s, int col, const wchar * label,const wchar * tip, const opt_val *pov, 
                                    handler_fn handler,int*context);
extern control * ctrl_pushbutton(controlset *, int col, const wchar * label,const wchar * tip,
                                 handler_fn handler, void * context);
extern control * ctrl_clrbutton(controlset *, int col, const wchar * label,const wchar * tip,
                                handler_fn handler, void * context);
extern control * ctrl_droplist(controlset *, int col, const wchar * label,const wchar * tip, int percentage,
                               handler_fn handler, void * context);
extern control * ctrl_fontsel(controlset *, int col, const wchar * label,const wchar * tip,
                              handler_fn handler, void * context);
extern control * ctrl_filesel(controlset *s, int col, const wchar * label,const wchar * tip,
             handler_fn handler, void *context);
extern control * ctrl_checkbox(controlset *, int col, const wchar * label,const wchar * tip,
                               handler_fn handler, void * context);
extern control * ctrl_hotkey(controlset *, int col, const wchar * label,const wchar * tip,int percentage,
                             handler_fn handler, void * context);

/* Standard handler routines to cover most of the common cases in
 * the config box.
 */

extern void dlg_stdcheckbox_handler(control *, int cid,int event,LPARAM p);
extern void dlg_stdstringbox_handler(control *, int cid,int event,LPARAM p);
extern void dlg_stdwstringbox_handler(control *, int cid,int event,LPARAM p);
extern void dlg_stwdstringbox_handler(control *, int cid,int event,LPARAM p);
extern void dlg_stdintbox_handler(control *, int cid,int event,LPARAM p);
extern void dlg_stdradiobutton_handler(control *, int cid,int event,LPARAM p);
extern void dlg_stdfontsel_handler(control *, int cid,int event,LPARAM p);
extern void dlg_stdfilesel_handler(control *ctrl, int cid,int event,LPARAM p);
extern void dlg_stdcolour_handler(control *, int cid,int event,LPARAM p);

/* Routines the platform-independent dialog code can call to read
 * and write the values of controls.
 */
extern void dlg_radiobutton_set(control *, int whichbutton);
extern int dlg_radiobutton_get(control *);
extern void dlg_checkbox_set(control *, UINT);
extern UINT dlg_checkbox_get(control *);
extern void dlg_label_setW(control *ctrl, wstring text);
extern void dlg_label_setA(control *ctrl, string text);
extern void dlg_label_getW(control *ctrl, wstring *text_p);
extern void dlg_label_getA(control *ctrl, string *text_p);
extern void dlg_editbox_setW(control *, wstring);
extern void dlg_editbox_setA(control *, string);
extern void dlg_editbox_getW(control *, wstring *);
extern void dlg_editbox_getA(control *, string *);
extern void dlg_hotkey_get(control *ctrl, string text,int size);
extern void dlg_hotkey_set(control *ctrl, string text);
/* The `listbox' functions also apply to combo boxes. */
extern void dlg_listbox_clear(control *);
extern void dlg_listbox_addA(control *, string);
extern void dlg_listbox_addW(control *, wstring);
extern int  dlg_listbox_getcur(control *);
extern void dlg_listview_add(control *ctrl, int ind,wstring text);
extern void dlg_listview_set(control *ctrl, int ind,int isub,wstring text);
extern void dlg_listview_get(control *ctrl, int ind,int isub,wstring *text);
extern void dlg_fontsel_set(control *, font_spec *);
extern void dlg_fontsel_get(control *, font_spec *);
/* Special for font sample */
extern void dlg_text_paint(control *ctrl);
/* Set input focus into a particular control.
*/
extern void dlg_set_focus(control *);
/* This function signals to the front end that the dialog's
 * processing is completed.
 */
extern void dlg_end(void);

/* Routines to manage a (per-platform) colour selector.
 * dlg_coloursel_start() is called in an event handler, and
 * schedules the running of a colour selector after the event
 * handler returns. The colour selector will send EVENT_CALLBACK to
 * the control that spawned it, when it's finished;
 * dlg_coloursel_results() fetches the results, as integers from 0
 * to 255; it returns nonzero on success, or zero if the colour
 * selector was dismissed by hitting Cancel or similar.
 * 
 * dlg_coloursel_start() accepts an RGB triple which is used to
 * initialise the colour selector to its starting value.
 */
extern void dlg_coloursel_start(colour);
extern int dlg_coloursel_results(colour *);

/* This routine is used by the platform-independent code to
 * indicate that the value of a particular control is likely to
 * have changed. It triggers a call of the handler for that control
 * with `event' set to EVENT_REFRESH.
 * 
 * If `ctrl' is null, _all_ controls in the dialog get refreshed
 * (for loading or saving entire sets of settings).
 */
extern void dlg_refresh(control *);
extern void dlg_refreshp(control *);

/* Standard helper functions for reading a controlbox structure.  */

/* Find the index of next controlset in a controlbox for a given
 * path, or -1 if no such controlset exists. If -1 is passed as
 * input, finds the first. Intended usage is something like
 * 
 *      for (index=-1; (index=ctrl_find_path(ctrlbox, index, path)) >= 0 ;) {
 *          ... process this controlset ...
 *      }
 */
extern int ctrl_find_path(controlbox *,const wchar * path, int index);

/* Return the number of matching path elements at the starts of p1 and p2,
 * or INT_MAX if the paths are identical. */
extern int ctrl_path_compare(const wchar * p1, const wchar * p2);

#endif
