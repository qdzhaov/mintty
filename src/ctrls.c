// ctrls.c (part of mintty)
// Copyright 2008-11 Andy Koppe
// Adapted from code from PuTTY-0.60 by Simon Tatham and team.
// (corresponds to putty:dialog.c)
// Licensed under the terms of the GNU General Public License v3 or later.

#include "ctrls.h"
/*
 * ctrls.c - a reasonably platform-independent mechanism for
 * describing window controls.
 */

/* Return the number of matching path elements at the starts of p1 and p2,
 * or INT_MAX if the paths are identical. */
int ctrl_path_compare(const wchar *p1, const wchar *p2) {
  int i = 0;
  while (*p1 || *p2) {
    if ((*p1 == '/' || *p1 == '\0') && (*p2 == '/' || *p2 == '\0'))
      i++;      /* a whole element matches, ooh */
    if (*p1 != *p2)
      return i; /* mismatch */
    p1++, p2++;
  }
  return -1;       /* exact match */
}

controlbox * ctrl_new_box(void) {
  controlbox *ret = new(controlbox);

  ret->nctrlsets = ret->ctrlsetsize = 0;
  ret->ctrlsets = null;
  ret->nfrees = ret->freesize = 0;
  ret->frees = null;

  return ret;
}

void ctrl_free_box(controlbox * b) {
  for (int i = 0; i < b->nctrlsets; i++)
    ctrl_free_set(b->ctrlsets[i]);
  for (int i = 0; i < b->nfrees; i++)
    delete(b->frees[i]);
  delete(b->ctrlsets);
  delete(b->frees);
  delete(b);
}

void ctrl_free_set(controlset *s) {
  delete(s->pathname);
  delete(s->boxtitle);
  for (int i = 0; i < s->ncontrols; i++) {
    ctrl_free(s->ctrls[i]);
  }
  delete(s->ctrls);
  delete(s);
}

/*
 * Find the index of first controlset in a controlbox for a given path.
 * If that path doesn't exist, return the index where it should be inserted.
 */
static int ctrl_find_set(controlbox * b, const wchar *path) {
#ifdef debug_layout
  printf("ctrl_find_set %d\n", b->nctrlsets);
#endif
  int last = 0;
  for (int i = 0; i < b->nctrlsets; i++) {
#ifdef debug_layout
    printf("  <%s> <%s>\n", path, b->ctrlsets[i]->pathname);
#endif
    int thisone = ctrl_path_compare(path, b->ctrlsets[i]->pathname);
   /*
    * If `start' is true and there exists a controlset with
    * exactly the path we've been given, we should return the
    * index of the first such controlset we find. Otherwise,
    * we should return the index of the first entry in which
    * _fewer_ path elements match than they did last time.
    */
    if ( thisone < last)
      return i;
    last = thisone;
  }
  return b->nctrlsets;  /* insert at end */
}

/*
 * Find the index of next controlset in a controlbox for a given path,
 * or -1 if no such controlset exists.
 * If -1 is passed as input, find the first.
 */
int ctrl_find_path(controlbox * b,const wchar *path, int index) {
  if (index < 0)
    index = ctrl_find_set(b, path);
  else
    index++;

  if (index < b->nctrlsets && !wcscmp(path, b->ctrlsets[index]->pathname))
    return index;
  else
    return -1;
}

static void insert_controlset(controlbox *b, int index, controlset *s) {
  if (b->nctrlsets >= b->ctrlsetsize) {
    b->ctrlsetsize = b->nctrlsets + 32;
    b->ctrlsets = renewn(b->ctrlsets, b->ctrlsetsize);
  }
  if (index < b->nctrlsets)
    memmove(&b->ctrlsets[index + 1], &b->ctrlsets[index],
            (b->nctrlsets - index) * sizeof(*b->ctrlsets));
  b->ctrlsets[index] = s;
  b->nctrlsets++;
}

/* Create a controlset. */
controlset * ctrl_new_set(controlbox *b, const wchar *path, const wchar *panel, const wchar *title) {
  // See whether this path exists already
  int index = ctrl_find_set(b, path);
  controlset *s=null;
  // If not, and it's not an empty path, set up a title.
  if (path&&*path&&index == b->nctrlsets ) {
    const wchar *title = panel;
    if (!title) {
      title = wcsrchr(path, '/');
      title = title ? title + 1 : path;
    }
    s = new(controlset);
    s->pathname = wcsdup(path);
    s->boxtitle = wcsdup(title);
    s->ncontrols = s->ctrlsize = 0;
    s->ncolumns = 0;      /* this is a title! */
    s->ctrls = null;
    insert_controlset(b, index, s);
    index++;
  }
  if(!title)return s;
  // Skip existing sets for the same path.
  while (index < b->nctrlsets && !wcscmp(b->ctrlsets[index]->pathname, path))
    index++;
  s = new(controlset);
  s->pathname = wcsdup(path);
  s->boxtitle = *title ? wcsdup(title) : null;
  s->ncolumns = 1;
  s->ncontrols = s->ctrlsize = 0;
  s->ctrls = null;
  insert_controlset(b, index, s);
  return s;
}

/* Allocate some private data in a controlbox. */
void * ctrl_alloc(controlbox * b, size_t size) {
  void *p;
  /*
  * This is an internal allocation routine, so it's allowed to
  * use malloc directly.
  */
  p = malloc(size);
  if (b->nfrees >= b->freesize) {
    b->freesize = b->nfrees + 32;
    b->frees = renewn(b->frees, b->freesize);
  }
  b->frees[b->nfrees++] = p;
  return p;
}
control *ctrl_next(control *ctrl,int n){
  return ctrl->parent->ctrls[ctrl->ind+n];
}
static control * ctrl_new(controlset *s,int col, int type,wstring label,wstring tip, handler_fn handler, void *context) {
  control *c = new(control);
  if (s->ncontrols >= s->ctrlsize) {
    s->ctrlsize = s->ncontrols + 32;
    s->ctrls = renewn(s->ctrls, s->ctrlsize);
  }
  s->ctrls[s->ncontrols++] = c;
  /*
  * Fill in the standard fields.
  */
  c->type = type;
  if(col<0) c->column = COLUMN_FIELD(0, s->ncolumns);
  else c->column=col;
  c->handler = handler;
  c->context = context;
  c->label = label ? wcsdup(label) : null;
  c->tip   = tip   ? wcsdup(tip  ) : null;
  c->plat_ctrl = null;
  c->parent=s;
  c->ind=s->ncontrols-1;
  return c;
}

/* `ncolumns' is followed by that many percentages, as integers. */
control * ctrl_columnsa(controlset *s, int ncolumns,int *cols) {
  control *c = ctrl_new(s,-1,CTRL_COLUMNS, 0,0,0,0);
  //assert(s->ncolumns == 1 || ncolumns == 1);
  c->columns.ncols = ncolumns;
  s->ncolumns = ncolumns;
  if (ncolumns <= 1)
    c->columns.percentages = null;
  else {
    c->columns.percentages = newn(int, ncolumns);
    for (int i = 0; i < ncolumns; i++)
      c->columns.percentages[i] = cols[i];
  }
  return c;
}
control * ctrl_columns(controlset *s, int ncolumns, ...) {
  int cols[MAXCOLS];
  if(ncolumns>1){
    va_list ap;
    va_start(ap, ncolumns);
    for (int i = 0; i < ncolumns; i++)
      cols[i] = va_arg(ap, int);
    va_end(ap);
  }
  return ctrl_columnsa(s,ncolumns,cols);
}

control * ctrl_inteditbox(controlset *s, int col, const wchar * label,const wchar * tip, int percentage,
             handler_fn handler, void *context,int imin,int imax) {
  control *c = ctrl_new(s,col, CTRL_INTEDITBOX, label,tip,handler, context);
  c->editbox.percentwidth = percentage;
  c->editbox.password = 0;
  c->editbox.has_list = 0;
  c->editbox.imin=imin;
  c->editbox.imax=imax;
  return c;
}
control * ctrl_editbox(controlset *s, int col, const wchar * label,const wchar * tip, int percentage,
             handler_fn handler, void *context) {
  control *c = ctrl_new(s,col, CTRL_EDITBOX, label,tip,handler, context);
  c->editbox.percentwidth = percentage;
  c->editbox.password = 0;
  c->editbox.has_list = 0;
  return c;
}

control * ctrl_combobox(controlset *s, int col, const wchar * label,const wchar * tip, int percentage,
              handler_fn handler, void *context) {
  control *c = ctrl_new(s,col, CTRL_EDITBOX, label,tip,handler, context);
  c->editbox.percentwidth = percentage;
  c->editbox.password = 0;
  c->editbox.has_list = 1;
  return c;
}

control * ctrl_listbox(controlset *s, int col, const wchar * label,const wchar * tip, int lines, int percentage,
              handler_fn handler, void *context) {
  control *c = ctrl_new(s,col, CTRL_LISTBOX, label,tip,handler, context);
  c->listbox.percentwidth = percentage;
  c->listbox.height = lines;
  c->listbox.ncols = 0;
  c->listbox.percentages = 0;
  return c;
}

control * ctrl_listview(controlset *s, int col, const wchar * label,const wchar * tip, int lines, int percentage,
                        int ncols,opt_val*pov,
                        handler_fn handler, void *context) {
  control *c = ctrl_new(s,col, CTRL_LISTVIEW, label,tip,handler, context);
  c->listview.percentwidth = percentage;
  c->listview.height = lines;
  c->listview.ncols = ncols;
  c->listview.cnames= newn(wstring, ncols);
  c->listview.percentages = newn(int, ncols);
  for(int i=0;i<ncols;i++){
    c->listview.cnames[i] = _W(pov[i].name);
    c->listview.percentages[i] = pov[i].val;
  }
  return c;
}
/*
 * `ncolumns' is followed by (alternately) radio button labels and values,
 * until a null in place of a title string is seen.
 */
control * ctrl_radiobuttons(controlset *s, int col, const wchar * label,const wchar * tip, const opt_val *pov, 
                  handler_fn handler,int*context) {
  int i;
  control *c = ctrl_new(s,col, CTRL_RADIO, label,tip,handler, context);
  for(i=0;pov[i].name;i++);
  c->radio.nbuttons = i;
  c->radio.ncolumns = i;
  if(c->radio.ncolumns>5)c->radio.ncolumns =5; 
  c->radio.labels = newn(wstring, c->radio.nbuttons);
  c->radio.vals = newn(int, c->radio.nbuttons);
  for (i = 0; i < c->radio.nbuttons; i++) {
    c->radio.labels[i] = _W(pov[i].name);
    c->radio.vals[i] = pov[i].val;
  }
  return c;
}
control * ctrl_pushbutton(controlset *s, int col, const wchar * label,const wchar * tip,
                handler_fn handler, void *context) {
  control *c = ctrl_new(s,col, CTRL_BUTTON, label,tip,handler, context);
  c->button.isdefault = 0;
  c->button.iscancel = 0;
  return c;
}
control * ctrl_clrbutton(controlset *s, int col, const wchar * label,const wchar * tip,
                handler_fn handler, void *context) {
  control *c = ctrl_new(s,col, CTRL_CLRBUTTON, label,tip,handler, context);
  c->button.isdefault = 0;
  c->button.iscancel = 0;
  return c;
}

control * ctrl_label(controlset *s, int col, const wchar * label,const wchar * tip) {
  control *c = ctrl_new(s,col, CTRL_LABEL, label, tip,0,0);
  return c;
}

control * ctrl_fontsel(controlset *s, int col, const wchar * label,const wchar * tip,
             handler_fn handler, void *context) {
  control *c = ctrl_new(s,col, CTRL_FONTSELECT, label,tip,handler, context);
  return c;
}

control * ctrl_filesel(controlset *s, int col, const wchar * label,const wchar * tip,
             handler_fn handler, void *context) {
  control *c = ctrl_new(s,col, CTRL_FILESELECT, label,tip,handler, context);
  return c;
}
control * ctrl_checkbox(controlset *s, int col, const wchar * label,const wchar * tip,
              handler_fn handler, void *context) {
  control *c = ctrl_new(s,col, CTRL_CHECKBOX, label,tip,handler, context);
  return c;
}

control * ctrl_hotkey(controlset *s, int col, const wchar * label,const wchar * tip, int percentage,
              handler_fn handler, void *context) {
  control *c = ctrl_new(s,col, CTRL_HOTKEY, label,tip,handler, context);
  c->editbox.percentwidth = percentage;
  return c;
}

void ctrl_free(control *ctrl) {
  delete(ctrl->label);
  switch (ctrl->type) {
    when CTRL_LISTVIEW:
      delete(ctrl->listview.cnames);
      delete(ctrl->listview.percentages);
    when CTRL_RADIO:
      delete(ctrl->radio.labels);
      delete(ctrl->radio.vals);
    when CTRL_COLUMNS:
      delete(ctrl->columns.percentages);
  }
  delete(ctrl);
}
void dlg_stdradiobutton_handler(control *ctrl,int cid, int event,LPARAM lp) {
  (void)cid;(void)lp;
  CTYPE*val_p = ctrl->context;
  if (event == EVENT_REFRESH) {
    int button;
    for (button = 0; button < ctrl->radio.nbuttons; button++) {
      if (ctrl->radio.vals[button] == *val_p) {
        dlg_radiobutton_set(ctrl, button);
        return;
      }
    }
    // clear all buttons if none selected
    dlg_radiobutton_set(ctrl, -1);
  }
  else if (event == EVENT_VALCHANGE) {
    int button = dlg_radiobutton_get(ctrl);
    *val_p = ctrl->radio.vals[button];
  }
}

void dlg_stdcheckbox_handler(control *ctrl,int cid, int event,LPARAM lp) {
  (void)cid;(void)lp;
  UINT *bp = ctrl->context;
  if (event == EVENT_REFRESH)
    dlg_checkbox_set(ctrl, *bp);
  else if (event == EVENT_VALCHANGE)
    *bp = dlg_checkbox_get(ctrl);
}

void dlg_stdfontsel_handler(control *ctrl, int cid,int event,LPARAM lp) {
  (void)cid;(void)lp;
  font_spec *fp = ctrl->context;
  if (event == EVENT_REFRESH)
    dlg_fontsel_set(ctrl, fp);
  else if (event == EVENT_VALCHANGE)
    dlg_fontsel_get(ctrl, fp);
}
void dlg_filesel_set(control *ctrl, wstring*fs);
void dlg_filesel_get(control *ctrl, wstring*fs);

void dlg_stdfilesel_handler(control *ctrl, int cid,int event,LPARAM lp) {
  (void)cid;(void)lp;
  wstring *vp = ctrl->context;
  if (event == EVENT_REFRESH)
    dlg_filesel_set(ctrl,vp);
  else if (event == EVENT_VALCHANGE){
    dlg_filesel_get(ctrl,vp);
  }
}
void dlg_stdwstringbox_handler(control *ctrl, int cid,int event,LPARAM lp) {
  (void)cid;(void)lp;
  wstring *sp = ctrl->context;
  if (event == EVENT_VALCHANGE)
    dlg_editbox_getW(ctrl, sp);
  else if (event == EVENT_REFRESH)
    dlg_editbox_setW(ctrl, *sp);
}

void dlg_stdstringbox_handler(control *ctrl, int cid,int event,LPARAM lp) {
  (void)cid;(void)lp;
  string *sp = ctrl->context;
  if (event == EVENT_VALCHANGE)
    dlg_editbox_getA(ctrl, sp);
  else if (event == EVENT_REFRESH)
    dlg_editbox_setA(ctrl, *sp);
}
void dlg_hotkey_get(control *ctrl, string text,int size);
void dlg_hotkey_set(control *ctrl, string text);
void dlg_stdhotkeybox_handler(control *ctrl, int cid,int event,LPARAM lp) {
  (void)cid;(void)lp;
  vhotkey*sp = ctrl->context;
  if (event == EVENT_VALCHANGE)
    dlg_hotkey_get(ctrl, sp->buf,sizeof(sp->buf));
  else if (event == EVENT_REFRESH)
    dlg_hotkey_set(ctrl, sp->buf);
}
void dlg_stdintbox_handler(control *ctrl, int cid,int event,LPARAM lp) {
  (void)cid;(void)lp;
  int *ip = ctrl->context;
  if (event == EVENT_VALCHANGE) {
    string val = 0;
    dlg_editbox_getA(ctrl, &val);
    *ip = max(0, atoi(val));
    delete(val);
  }
  else if (event == EVENT_REFRESH) {
    char buf[16];
    sprintf(buf, "%i", *ip);
    dlg_editbox_setA(ctrl, buf);
  }
}

void dlg_stdcolour_handler(control *ctrl, int cid,int event,LPARAM lp) {
  (void)cid;(void)lp;
  colour *cp = ctrl->context;
  if (event == EVENT_ACTION)
    dlg_coloursel_start(*cp);
  else if (event == EVENT_CALLBACK) {
    colour c;
    if (dlg_coloursel_results(&c)){
      *cp = c;
    }
  }
}
