#include <windows.h>

#include <unistd.h>
#include <stdlib.h>

#include "term.h"
#include "child.h"

typedef unsigned int uint;
typedef unsigned short ushort;
typedef wchar_t wchar;
typedef unsigned char uchar;
typedef const char *string;
typedef const wchar *wstring;


#include <d2d1.h>

#include "winpriv.h"
int cs_mbstowcs(wchar *ws, const char *s, size_t wlen);

#define lengthof(array) (sizeof(array) / sizeof(*(array)))



static unsigned int ntabs=0,mtabs=0 ,active_tab = 0;
static STab** tabs=NULL;

static void invalidate_tabs() {
    win_invalidate_all(1);
}

STerm* win_tab_active_term() {
    if(ntabs) return tabs[active_tab]->terminal;
    return NULL;
}

int win_tab_count() { return ntabs; }
STab*win_tab_active() { return tabs[active_tab]; }

static void update_window_state() {
    win_update_menus(0);
    if (cfg.title_settable)
        SetWindowTextW(wnd, win_tab_get_title(active_tab));
    win_adapt_term_size(0,0);
}
static STab *stabs[16];
static int stabs_i=0;
void win_tab_v(STab *tab){
  if(tab){
    cterm=tab->terminal;
  }
}
void win_tab_push(STab *tab){
  if(cterm){
    stabs[stabs_i++]=cterm->tab;
  }
  win_tab_v(tab);
}
void win_tab_pop(){
  if(stabs_i){
    win_tab_v(stabs[--stabs_i]);
  }
}
void win_tab_actv(){
    if(tabs)win_tab_v( tabs[active_tab]);
    stabs_i=0;
}

void win_tab_go(int index) {
    active_tab = index;
    STab* active = tabs[active_tab];
    for (STab**tab=tabs;*tab ; tab++) {
        cterm=(*tab)->terminal;
        term_set_focus(*tab == active,1);
    }
    win_tab_actv();
    active->attention = false;
    update_window_state();
    invalidate_tabs();
}

static unsigned int rel_index(int change) {
    return (active_tab + change + ntabs) % ntabs;
}

void win_tab_change(int change) {
   win_tab_go(rel_index(change));
}
void win_tab_move(int amount) {
    int new_idx = rel_index(amount);
    STab*p;p=tabs[active_tab];
    tabs[active_tab]=tabs[new_idx];
    tabs[new_idx]=p;
    win_tab_go(new_idx);
}

static STab *tab_by_term(STerm* Term) {
    for(STab**p=tabs;*p; p++){
        if((*p)->terminal==Term)return *p;
    }
    return NULL;
}

static const char* g_home;
#define ZNEW(T) (T*)malloc(sizeof(T))
static void tab_init(STab*tab){
    memset(tab,0,sizeof(*tab));
    tab->terminal=ZNEW(STerm);
    memset(tab->terminal, 0, sizeof(STerm));
}
static void tab_free(STab*tab){
    if (tab->terminal){
        child_free(tab->terminal);
        term_free(tab->terminal);
    }
    free(tab);
}

static STab*vtab(){
    if(ntabs<=mtabs){
        mtabs+=16;
        tabs=(STab**)realloc(tabs,(mtabs+1)*sizeof(*tabs));
        memset(tabs+mtabs-16,0,17*sizeof(*tabs));
    } 
    for(uint i=0;i<mtabs;i++){
        if(tabs[i]==NULL){
            tabs[i]=ZNEW(STab);
            tab_init(tabs[i]);
            ntabs++;
            return tabs[i];
        }
    }
    return NULL;
}
static void newtab(SessDef*sd,
    unsigned short rows, unsigned short cols,
    unsigned short width, unsigned short height, 
    const char* cwd,const char* title) {
    STab* tab = vtab();
    win_tab_v(tab);
    tab->sd.argc=sd->argc;
    tab->sd.cmd=strdup(sd->cmd);
    tab->sd.argv=newn(char*,sd->argc+1);
    for(int i=0;sd->argv[i];i++){
      tab->sd.argv[i]=strdup(sd->argv[i]);
    }
    tab->sd.argv[sd->argc]=0;
    term_reset(1);
    term_resize(rows, cols);
    tab->terminal->child.cmd = sd->cmd;
    tab->terminal->child.home = g_home;
    const wchar * ws=NULL;
    wchar *ws1=NULL;
    int size;
    const char *st=title;
    if(st){
      size = cs_mbstowcs(NULL, st, 0) + 1;
      ws=ws1 = (wchar *)malloc(size * sizeof(wchar));  // includes terminating NUL
      cs_mbstowcs(ws1, st, size);
    }else if( cfg.title&&*cfg.title){
      ws=cfg.title;
    }else{
      st=sd->cmd;
      size = cs_mbstowcs(NULL, st, 0) + 1;
      ws=ws1 = (wchar *)malloc(size * sizeof(wchar));  // includes terminating NUL
      cs_mbstowcs(ws1, st, size);
    }
    win_tab_set_title(tab->terminal, ws);
    struct winsize wsz={rows, cols, width, height};
    child_create(tab->terminal, sd, &wsz, cwd);
    if(ws1)free(ws1);
}

static void set_tab_bar_visibility();

void win_tab_set_argv(char** argv) {
  (void)argv;
}

void win_tab_init(const char* home,SessDef*sd,const  int width, int height, const char* title) {
    g_home = home;
    newtab(sd,cfg.rows, cfg.cols, width, height, NULL, title);
    set_tab_bar_visibility();
}
void win_tab_create(SessDef*sd){
    STerm* t = tabs[active_tab]->terminal;
    char cwd_path[256];
    sprintf(cwd_path,"/proc/%d/cwd",child_get_pid(t) ); 
    char* cwd = realpath(cwd_path, 0);
    newtab(sd,t->rows, t->cols, t->cols * cell_width, t->rows * cell_height, cwd, NULL);
    free(cwd);
    win_tab_go(ntabs - 1);
    set_tab_bar_visibility();
}

void win_tab_clean() {
    bool invalidate = false;
    STab**p,**pd;
    for(pd=p=tabs;*p;p++){
        if(child_get_pid((*p)->terminal)==0){
            tab_free(*p); *p=NULL;
            invalidate = true;
            ntabs--;
        }else{
            if(pd!=p) *pd=*p;
            pd++;
        } 
    }
    for(;pd<p;pd++)*pd=NULL;
    if (invalidate && ntabs > 0) {
        if (active_tab >= ntabs)
            win_tab_go(ntabs - 1);
        else
            win_tab_go(active_tab);
        set_tab_bar_visibility();
        invalidate_tabs();
    }
    if(*tabs==NULL){
        exit_mintty();
    }
}

void win_tab_attention(STerm*pterm) {
    tab_by_term(pterm)->attention = true;
    invalidate_tabs();
}

void win_tab_set_title(STerm*pterm, const wchar_t* title) {
    STab* tab = tab_by_term(pterm);
    if (wcscmp(tab->titles[tab->titles_i] , title)) {
        wcsncpy(tab->titles[tab->titles_i] , title,TAB_LTITLE-1);
        invalidate_tabs();
    }
    if (pterm == win_tab_active_term()) {
        win_set_title((wchar *)tab->titles[tab->titles_i]);
    }
}

wchar_t* win_tab_get_title(unsigned int idx) {
    return tabs[idx]->titles[tabs[idx]->titles_i];
}

void win_tab_title_push(STerm*pterm) {
    STab* tab = tab_by_term(pterm);
    int oi=tab->titles_i;
    tab->titles_i++;
    if (tab->titles_i == TAB_NTITLE)
        tab->titles_i = 0;
    wcsncpy(tab->titles[tab->titles_i] , tab->titles[oi],TAB_LTITLE-1);
}

wchar_t* win_tab_title_pop(STerm*pterm) {
    STab* tab = tab_by_term(pterm);
    if (!tab->titles_i)
        tab->titles_i = TAB_NTITLE;
    else
        tab->titles_i--;
    return win_tab_get_title(active_tab);
}

/*
 * Title stack (implemented as fixed-size circular buffer)
 */
    void
win_tab_save_title(STerm*pterm)
{
    win_tab_title_push(pterm);
}

    void
win_tab_restore_title(STerm*pterm)
{
    win_tab_set_title(pterm, win_tab_title_pop(pterm));
}

bool win_tab_should_die() { return ntabs == 0; }


static void fix_window_size() {
    // doesn't work fully when you put fullscreen and then show or hide
    // tab bar, but it's not too terrible (just looks little off) so I
    // don't care. Maybe fix it later?
    if (win_is_fullscreen) {
        win_adapt_term_size(0,0);
    } else {
        STerm* t = tabs[active_tab]->terminal;
        win_set_chars(t->rows, t->cols);
    }
}
static bool tab_bar_visible = false;
static int gtab_font_size() {
    return cfg.tab_font_size * dpi/96;
}
static int gtab_height() {
    return cfg.tab_font_size*3/2 * dpi/96;
}
static void set_tab_bar_visibility() {
    int b=(ntabs > 1)&&cfg.tab_bar_show;
    if (b == tab_bar_visible) return;
    tab_bar_visible = b;
    OFFSET = tab_bar_visible ? gtab_height() : 0; 
    fix_window_size();
    invalidate_tabs();
}
void win_tab_indicator(){
  cfg.indicator=!cfg.indicator;
  set_tab_bar_visibility();
}
void win_tab_show(){
  cfg.tab_bar_show=!cfg.tab_bar_show;
  set_tab_bar_visibility();
} 


static HGDIOBJ new_norm_font(int size) {
    return CreateFont(size,0,0,0,FW_NORMAL,0,0,0,1,0,0,CLEARTYPE_QUALITY,0,0);
}

static HGDIOBJ new_fold_font(int size) {
    return CreateFont(size,0,0,0,FW_BOLD,0,0,0,1,0,0,CLEARTYPE_QUALITY,0,0);
}

// paint a tab to dc (where dc draws to buffer)
static void paint_tab(HDC dc, int x0,int width, int tabh, const STab* tab) {
    MoveToEx(dc, x0, tabh, NULL);
    LineTo(dc, x0, 0);
    LineTo(dc, x0+width, 0);
    TextOutW(dc, x0+width/2, (tabh - gtab_font_size()) / 2, tab->titles[tab->titles_i], wcslen(tab->titles[tab->titles_i]));
}

static int tab_paint_width = 0;
extern int tabctrling;
static wchar State[5]=W("    ");
static int Statel=4;
static const wchar*GState(){
  if (tab_bar_visible||cfg.indicator){
    State[0]=' ';
    State[1]=cterm->selection_pending?'S':' ';
    State[2]=tabctrling>2?'C':' ';
    State[3]=cfg.partline&&cterm->usepartline?'P':' ';
    State[4]=0 ;
    Statel=4;
  }else{
    if(tabctrling){
      State[0]='C';
      State[1]=0 ;
      Statel=1;
    }else{
      State[0]=0 ;
      Statel=0;
    }
  }
  return State;
} 
void win_tab_paint(HDC dc) {
  // the sides of drawable area are not visible, so we really should draw to
  // coordinates 1..(width-2)
  RECT r;
  GetClientRect(wnd, &r);
  int width = r.right-r.left - 2 * PADDING;
  int tabh=gtab_height();
  int tabfs=gtab_font_size();

  const colour bg = cfg.tab_bg_colour;
  const colour fg = cfg.tab_fg_colour;
  const colour active_bg = cfg.tab_active_bg_colour;
  const colour attention_bg = cfg.tab_attention_bg_colour;
  GState();
  HGDIOBJ ffont = new_fold_font(tabfs*10/8);
  if (tab_bar_visible){
    HDC bufdc = CreateCompatibleDC(dc);
    SetBkMode(bufdc, TRANSPARENT);
    SetTextColor(bufdc, fg);
    SetBkColor(bufdc,bg);
    HGDIOBJ brush = CreateSolidBrush(bg);
    VSELGDIOBJ(obrush , bufdc, brush);
    RECT tabrect;
    HGDIOBJ ofont ;
    HGDIOBJ nfont = new_norm_font(tabfs);
    const unsigned int preferred_width = 200 * dpi/96;
    const int tabwidth = ((width) / (ntabs+1)) > preferred_width ? preferred_width : width / ntabs;
    const int loc_tabheight = tabh-2;
    tab_paint_width = tabwidth;
    VSELGDIOBJ(open   , bufdc, CreatePen(PS_SOLID, 0, fg));
    VSELGDIOBJ(obuf   , bufdc, CreateCompatibleBitmap(dc, width, tabh));
    SetTextAlign(bufdc, TA_CENTER);
    ofont  =SelectObject( bufdc, nfont);
    int x0;
    for (size_t i = 0; i < ntabs; i++) {
      bool  active = i == active_tab;
      x0=i*tabwidth;
      SetRect(&tabrect, x0, 0, x0+tabwidth, loc_tabheight+1);
      if (active) {
        HGDIOBJ activebrush = CreateSolidBrush(active_bg);
        FillRect(bufdc, &tabrect, activebrush);
        DeleteObject(activebrush);
      } else if (tabs[i]->attention) {
        HGDIOBJ activebrush = CreateSolidBrush(attention_bg);
        FillRect(bufdc, &tabrect, activebrush);
        DeleteObject(activebrush);
      } else {
        FillRect(bufdc, &tabrect, brush);
      }

      if (active) {
        SelectObject( bufdc, ffont);
        paint_tab(bufdc, x0,tabwidth, loc_tabheight, tabs[i]);
        SelectObject( bufdc, nfont);
      } else {
        MoveToEx(bufdc, x0, loc_tabheight, NULL);
        LineTo(bufdc, x0+tabwidth, loc_tabheight);
        paint_tab(bufdc, x0,tabwidth, loc_tabheight, tabs[i]);
      }

    }

    x0=(int)ntabs*tabwidth;
    SetRect(&tabrect, x0, 0, width+1 , loc_tabheight+1);
    FillRect(bufdc, &tabrect, brush);
    SelectObject( bufdc, ffont);
    SetTextAlign(bufdc, TA_LEFT|TA_TOP);
    TextOutW(bufdc, width - tabfs*2, 0,State , 4);
    SelectObject( bufdc, nfont);
    MoveToEx(bufdc, x0, 0, NULL);
    LineTo(bufdc, x0, loc_tabheight);
    LineTo(bufdc, width , loc_tabheight);
    BitBlt(dc, 0, 0, width, tabh, bufdc, 0, 0, SRCCOPY);
    VDELGDIOBJ(open  );
    SelectObject( bufdc, ofont);
    DeleteObject(nfont);
    VDELGDIOBJ(obuf  );
    VDELGDIOBJ(obrush);
    DeleteObject(ffont);
    DeleteDC(bufdc);
  }else{
    HGDIOBJ ofont  =SelectObject( dc, ffont);
    SetTextColor(dc, fg);
    SetBkColor(dc,bg);
    SetBkMode(dc, OPAQUE);
    SetTextAlign(dc, TA_LEFT|TA_TOP);
    wchar s[20];
    wsprintfW(s,_W("%d/%d %s"),active_tab,ntabs ,State);
    int sl=wcslen(s);
    SIZE sw;
    GetTextExtentPoint32W(dc,s,sl,&sw);
    TextOutW(dc,width - sw.cx, (tabh - tabfs) / 2,s, sl  );
    SelectObject( dc, ofont);
  } 
  DeleteObject(ffont);
}

void win_tab_for_each(void (*cb)(STerm*pterm)) {
    for (STab** tab=tabs;*tab;tab++){
      win_tab_v(*tab);
      cb((*tab)->terminal);
    }
}

void win_tab_mouse_click(int x) {
    unsigned int itab = x / tab_paint_width;
    if (itab >= ntabs)
        return;
    win_tab_go(itab);
}
STab**   win_tabs() {
    return tabs;
}
