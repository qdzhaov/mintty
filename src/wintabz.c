#include <windows.h>
#include <unistd.h>
#include <stdlib.h>
#include "term.h"
#include "child.h"
#include "winpriv.h"
#include <termios.h>
int cs_mbstowcs(wchar *ws, const char *s, size_t wlen);

#define lengthof(array) (sizeof(array) / sizeof(*(array)))
static unsigned int ntabs=0,mtabs=0 ,active_tab = 0;
static STab** tabs=NULL;
static bool tab_bar_visible = false;
static STab *stabs[16];
static int stabs_i=0;
static int tab_paint_width = 0;
static const char* g_home;

void win_tab_go(int index) ;
static void set_tab_bar_visibility() ;

static int gtab_font_size() { return cfg.tab_font_size * dpi/96; }
static int gtab_height() { return cfg.tab_font_size*3/2 * dpi/96; }
static unsigned int rel_index(int change) { return (active_tab + change + ntabs) % ntabs; }

STab**win_tabs() { return tabs; }
int win_tab_count() { return ntabs; }
STab*win_tab_active() { return tabs[active_tab]; }
bool win_tab_should_die() { return ntabs == 0; }

void win_tab_v(STab *tab){ if(tab){ cterm=tab->terminal; } }
void win_tab_save_title(STerm*pterm) { win_tab_title_push(pterm); }
void win_tab_restore_title(STerm*pterm) { win_tab_set_title(pterm, win_tab_title_pop(pterm)); }
void win_tab_change(int change) { win_tab_go(rel_index(change)); }
void win_tab_pop(){ if(stabs_i){ win_tab_v(stabs[--stabs_i]); } }
void win_tab_actv(){ if(tabs)win_tab_v( tabs[active_tab]); stabs_i=0; }
wchar_t* win_tab_get_title(unsigned int idx) { return tabs[idx]->titles[tabs[idx]->titles_i]; }
void win_tab_indicator(){ cfg.indicator=!cfg.indicator; set_tab_bar_visibility(); }
void win_tab_show(){ cfg.tab_bar_show=!cfg.tab_bar_show; set_tab_bar_visibility(); } 

static STab *tab_by_term(STerm* Term) {
  for(STab**p=tabs;*p; p++){
    if((*p)->terminal==Term)return *p;
  }
  return NULL;
}
STerm* win_tab_active_term() {
  if(ntabs) return tabs[active_tab]->terminal;
  return NULL;
}
void win_tab_set_title(STerm*pterm, const wchar_t* title) {
  STab* tab = tab_by_term(pterm);
  if (wcscmp(tab->titles[tab->titles_i] , title)) {
    wcsncpy(tab->titles[tab->titles_i] , title,TAB_LTITLE-1);
    win_invalidate_all(1);
  }
  if (pterm == win_tab_active_term()) {
    win_set_title((wchar *)tab->titles[tab->titles_i]);
  }
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
void win_tab_for_each(void (*cb)(STerm*pterm)) {
  for (STab** tab=tabs;*tab;tab++){
    win_tab_v(*tab);
    cb((*tab)->terminal);
  }
}

void win_tab_mouse_click(int x) {
  unsigned int itab = x / tab_paint_width;
  if (itab >= ntabs) return;
  win_tab_go(itab);
}

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
static void set_tab_bar_visibility() {
  int b=(ntabs > 1)&&cfg.tab_bar_show;
  if (b == tab_bar_visible) return;
  tab_bar_visible = b;
  OFFSET = tab_bar_visible ? gtab_height() : 0; 
  fix_window_size();
  win_invalidate_all(1);
}

static void update_window_state() {
  win_update_menus(0);
  if (cfg.title_settable)
    SetWindowTextW(wnd, win_tab_get_title(active_tab));
  win_adapt_term_size(0,0);
}
void win_tab_push(STab *tab){
  if(cterm){
    stabs[stabs_i++]=cterm->tab;
  }
  win_tab_v(tab);
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
  win_invalidate_all(1);
}
void win_tab_move(int amount) {
  int new_idx = rel_index(amount);
  STab*p;p=tabs[active_tab];
  tabs[active_tab]=tabs[new_idx];
  tabs[new_idx]=p;
  win_tab_go(new_idx);
}

static void tab_init(STab*tab){
  memset(tab,0,sizeof(*tab));
  tab->terminal=new(STerm);
  memset(tab->terminal, 0, sizeof(STerm));
}
static void tab_free(STab*tab){
  if (tab->terminal){
    child_free(tab->terminal);
    term_free(tab->terminal);
  }
  VFREE(tab->sd.title);
  VFREE(tab->sd.cmd  );
  for(int i=0;tab->sd.argv[i];i++){
    free(tab->sd.argv[i]);
  }
  VFREE(tab->sd.argv );
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
      tabs[i]=new(STab);
      tab_init(tabs[i]);
      ntabs++;
      return tabs[i];
    }
  }
  return NULL;
}
static void newtab(SessDef*sd,struct winsize *wsz, const char* cwd ){
  STab* tab = vtab();
  win_tab_v(tab);
  tab->sd.argc=sd->argc;
  if(sd->title)tab->sd.title=strdup(sd->title);
  else tab->sd.title=0;
  tab->sd.cmd=strdup(sd->cmd);
  tab->sd.argv=newn(char*,sd->argc+1);
  for(int i=0;sd->argv[i];i++){
    tab->sd.argv[i]=strdup(sd->argv[i]);
  }
  tab->sd.argv[sd->argc]=0;
  cterm->usepartline=cfg.partline!=0; 
  term_reset(1);
  term_resize(wsz->ws_row, wsz->ws_col);
  tab->terminal->child.cmd = sd->cmd;
  tab->terminal->child.home = g_home;
  const wchar * ws=NULL;
  wchar *ws1=NULL;
  int size;
  const char *st;
  if( sd->title&&*sd->title){
    st=sd->title;
  }else{
    st=sd->cmd;
  }
  size = cs_mbstowcs(NULL, st, 0) + 1;
  ws=ws1 = (wchar *)malloc(size * sizeof(wchar));  // includes terminating NUL
  cs_mbstowcs(ws1, st, size);
  win_tab_set_title(tab->terminal, ws);
  child_create(tab->terminal, sd, wsz, cwd);
  if(ws1)free(ws1);
}


void win_tab_init(const char* home,SessDef*sd,const  int width, int height) {
  g_home = home;
  struct winsize wsz={cfg.rows, cfg.cols, width, height};
  newtab(sd,&wsz, NULL);
  set_tab_bar_visibility();
}
void win_tab_create(SessDef*sd){
  STerm* t = tabs[active_tab]->terminal;
  char cwd_path[256];
  sprintf(cwd_path,"/proc/%d/cwd",child_get_pid(t) ); 
  char* cwd = realpath(cwd_path, 0);
  struct winsize wsz={t->rows, t->cols, t->cols * cell_width, t->rows * cell_height};
  newtab(sd,&wsz, cwd);
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
    win_invalidate_all(1);
  }
  if(*tabs==NULL){
    exit_mintty();
  }
}

//======== painting ======================
static wchar State[5]=W("    ");
static int Statel=4;

extern int tabctrling;
struct tabpaint{
  HGDIOBJ normfont,boldfont,fgpen,dcbuf,
          bgbrush,activebrush ,attentionbrush ;
  HGDIOBJ obrush,ofont,open,obuf;
  int otabw,otabh;
  int ofsize;
  HDC bdc;
};
static struct tabpaint tp={0};
static void InitGdi(HDC dc,int fsize,int width,int tabh){
  const colour bg = cfg.tab_bg_colour;
  const colour fg = cfg.tab_fg_colour;
  const colour active_bg = cfg.tab_active_bg_colour;
  const colour attention_bg = cfg.tab_attention_bg_colour;
  if(tp.bdc==NULL){
    tp.bdc = CreateCompatibleDC(dc);
    SetBkMode(tp.bdc, TRANSPARENT);
    SetTextColor(tp.bdc, fg);
    SetBkColor(tp.bdc,bg);
    SetTextAlign(tp.bdc, TA_CENTER);
  }
  if(tp.otabw<width||tp.otabh<tabh){
    if(tp.otabw){
      DeleteObject(tp.dcbuf);
    } 
    tp.dcbuf=CreateCompatibleBitmap(dc, width, tabh);
    tp.otabw=width; tp.otabh=tabh;
    tp.obuf = SelectObject(tp.bdc, tp.dcbuf);
  }
  if(tp.ofsize==0){
    tp.bgbrush = CreateSolidBrush(bg);
    tp.fgpen=CreatePen(PS_SOLID, 0, fg);
    tp.activebrush = CreateSolidBrush(active_bg);
    tp.attentionbrush = CreateSolidBrush(attention_bg);
    tp.obrush = SelectObject(tp.bdc, tp.bgbrush);
    tp.open = SelectObject(tp.bdc, tp.fgpen);
  }
  if(fsize!=tp.ofsize){
    if(tp.ofsize){
      DeleteObject(tp.normfont);
      DeleteObject(tp.boldfont);
    }
    tp.ofsize=fsize;
    tp.normfont=CreateFont(fsize     ,0,0,0,FW_NORMAL,0,0,0,1,0,0,CLEARTYPE_QUALITY,0,0);
    tp.boldfont=CreateFont(fsize*10/8,0,0,0,FW_BOLD  ,0,0,0,1,0,0,CLEARTYPE_QUALITY,0,0);
    tp.ofont  =SelectObject( tp.bdc, tp.normfont);
  }
  //  SelectObject(tp.bdc,tp.open  );
  //  SelectObject(tp.bdc,tp.ofont);
  //  SelectObject(tp.bdc,tp.obuf  );
  //  SelectObject(tp.bdc,tp.obrush);
}
static void paint_tab(HDC dc, int x0,int width, int tabh, const STab* tab) {
  MoveToEx(dc, x0, tabh, NULL);
  LineTo(dc, x0, 0);
  LineTo(dc, x0+width, 0);
  TextOutW(dc, x0+width/2, (tabh - gtab_font_size()) / 2, tab->titles[tab->titles_i], wcslen(tab->titles[tab->titles_i]));
}

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
  RECT r;
  GetClientRect(wnd, &r);
  int width = r.right-r.left - 2 * PADDING;
  int tabh=gtab_height();
  int tabfs=gtab_font_size();
  GState();
  InitGdi(dc,tabfs,width,tabh);
  if (tab_bar_visible){
    SelectObject(tp.bdc, tp.bgbrush);
    RECT tabrect;
    const unsigned int preferred_width = 200 * dpi/96;
    const int tabwidth = ((width) / (ntabs+1)) > preferred_width ? preferred_width : width / ntabs;
    const int loc_tabheight = tabh-2;
    tab_paint_width = tabwidth;
    SelectObject(tp.bdc, tp.fgpen);
    SelectObject(tp.bdc, tp.normfont);
    SetTextAlign(tp.bdc, TA_CENTER);
    int x0;
    for (size_t i = 0; i < ntabs; i++) {
      bool  active = i == active_tab;
      x0=i*tabwidth;
      SetRect(&tabrect, x0, 0, x0+tabwidth, loc_tabheight+1);
      if (active) {
        FillRect(tp.bdc, &tabrect, tp.activebrush);
      } else if (tabs[i]->attention) {
        FillRect(tp.bdc, &tabrect, tp.attentionbrush);
      } else {
        FillRect(tp.bdc, &tabrect, tp.bgbrush);
      }

      if (active) {
        SelectObject( tp.bdc, tp.boldfont);
        paint_tab(tp.bdc, x0,tabwidth, loc_tabheight, tabs[i]);
        SelectObject( tp.bdc, tp.normfont);
      } else {
        MoveToEx(tp.bdc, x0, loc_tabheight, NULL);
        LineTo(tp.bdc, x0+tabwidth, loc_tabheight);
        paint_tab(tp.bdc, x0,tabwidth, loc_tabheight, tabs[i]);
      }
    }
    x0=(int)ntabs*tabwidth;
    SetRect(&tabrect, x0, 0, width+1 , loc_tabheight+1);
    FillRect(tp.bdc, &tabrect, tp.bgbrush);
    SelectObject( tp.bdc, tp.boldfont);
    SetTextAlign(tp.bdc, TA_LEFT|TA_TOP);
    SIZE sw;
    GetTextExtentPoint32W(dc,State,4,&sw);
    TextOutW(tp.bdc, width - sw.cx, 0,State , 4);
    SelectObject( tp.bdc, tp.normfont);
    MoveToEx(tp.bdc, x0, 0, NULL);
    LineTo(tp.bdc, x0, loc_tabheight);
    LineTo(tp.bdc, width , loc_tabheight);
    SelectObject( tp.bdc, tp.normfont);
    BitBlt(dc, 0, 0, width, tabh, tp.bdc, 0, 0, SRCCOPY);
  }else{
    if(tp.ofsize==0){
      int fsize=tabfs;
      tp.ofsize=fsize;
      tp.normfont=CreateFont(fsize     ,0,0,0,FW_NORMAL,0,0,0,1,0,0,CLEARTYPE_QUALITY,0,0);
      tp.boldfont=CreateFont(fsize*10/8,0,0,0,FW_BOLD  ,0,0,0,1,0,0,CLEARTYPE_QUALITY,0,0);
    }
    HGDIOBJ ofont  =SelectObject( dc, tp.boldfont);
    SetTextColor(dc, cfg.tab_fg_colour);
    SetBkColor(dc,cfg.tab_bg_colour);
    SetBkMode(dc, OPAQUE);
    SetTextAlign(dc, TA_LEFT|TA_TOP);
    wchar s[20];
    wsprintfW(s,_W("  %d/%d %s"),active_tab,ntabs ,State);
    int sl=wcslen(s);
    SIZE sw;
    GetTextExtentPoint32W(dc,s,sl,&sw);
    TextOutW(dc,width - sw.cx, (tabh - tabfs) / 2,s, sl  );
    SelectObject( dc, ofont);
  } 
}

