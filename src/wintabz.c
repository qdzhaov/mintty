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
static Tab** tabs=NULL;

static float g_xscale, g_yscale;

static void init_scale_factors() {
    static ID2D1Factory* d2d_factory = NULL;
    if (d2d_factory == NULL) {
        //D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,__uuidof(ID2D1Factory),NULL,&d2d_factory);
    }
    float xdpi=96, ydpi=96;
    //d2d_factory->ReloadSystemMetrics();
    //d2d_factory->GetDesktopDpi(&xdpi, &ydpi);
    g_xscale = xdpi / 96.0f;
    g_yscale = ydpi / 96.0f;
}

static void invalidate_tabs() {
    win_invalidate_all(1);
}

STerm* win_tab_active_term() {
    if(ntabs) return tabs[active_tab]->terminal;
    return NULL;
}

int win_tab_count() { return ntabs; }
int win_tab_active() { return active_tab; }

static void update_window_state() {
    win_update_menus(0);
    if (cfg.title_settable)
        SetWindowTextW(wnd, win_tab_get_title(active_tab));
    win_adapt_term_size(0,0);
}

static void set_active_tab(unsigned int index) {
    active_tab = index;
    Tab* active = tabs[active_tab];
    for (Tab**tab=tabs;*tab ; tab++) {
        cterm=(*tab)->terminal;
        term_set_focus(*tab == active,1);
    }
    cterm=active->terminal;
    cchild=active->chld;
    active->info.attention = false;
    update_window_state();
    invalidate_tabs();
}

static unsigned int rel_index(int change) {
    return (active_tab + change + ntabs) % ntabs;
}

void win_tab_change(int change) {
    set_active_tab(rel_index(change));
}
void win_tab_move(int amount) {
    int new_idx = rel_index(amount);
    Tab*p;p=tabs[active_tab];
    tabs[active_tab]=tabs[new_idx];
    tabs[new_idx]=p;
    set_active_tab(new_idx);
}

static Tab *tab_by_term(STerm* Term) {
    for(Tab**p=tabs;*p; p++){
        if((*p)->terminal==Term)return *p;
    }
    return NULL;
}

static char* g_home;
static char* g_cmd;
static char** g_argv;
#define VFREE(p) if(p){free(p);p=NULL;}
#define ZNEW(T) (T*)malloc(sizeof(T))
static void tab_init(Tab*tab){
    memset(tab,0,sizeof(*tab));
    tab->terminal=ZNEW(STerm);
    tab->chld=ZNEW(SChild) ;
    memset(tab->terminal, 0, sizeof(STerm));
    memset(tab->chld, 0, sizeof(SChild));
}
static void tab_free(Tab*tab){
    if (tab->terminal)
        term_free(tab->terminal);
    if (tab->chld)
        child_free(tab->chld);
    free(tab);
}

static Tab*vtab(){
    if(ntabs<=mtabs){
        mtabs+=16;
        tabs=(Tab**)realloc(tabs,(mtabs+1)*sizeof(*tabs));
        memset(tabs+mtabs-16,0,17*sizeof(*tabs));
    } 
    for(uint i=0;i<mtabs;i++){
        if(tabs[i]==NULL){
            tabs[i]=ZNEW(Tab);
            tab_init(tabs[i]);
            ntabs++;
            return tabs[i];
        }
    }
    return NULL;
}
static void newtab(
                   unsigned short rows, unsigned short cols,
                   unsigned short width, unsigned short height, const char* cwd, char* title) {
    Tab* tab = vtab();
    tab->terminal->child = tab->chld;
    cterm=tab->terminal;
    cchild=tab->chld;
    term_reset(1);
    term_resize(rows, cols);
    tab->chld->cmd = g_cmd;
    tab->chld->home = g_home;
    struct winsize wsz={rows, cols, width, height};
    child_create(tab->chld, tab->terminal, g_argv, &wsz, cwd);
    wchar * ws;
    char *st=title;
    if(!st)st=g_cmd;
    int size = cs_mbstowcs(NULL, st, 0) + 1;
    ws = (wchar *)malloc(size * sizeof(wchar));  // includes terminating NUL
    cs_mbstowcs(ws, st, size);
    win_tab_set_title(tab->terminal, ws);
    free(ws);
}

static void set_tab_bar_visibility();

void win_tab_set_argv(char** argv) {
    g_argv = argv;
}

void win_tab_init(char* home, char* cmd, char** argv, int width, int height, char* title) {
    g_home = home;
    g_cmd = cmd;
    g_argv = argv;
    newtab(cfg.rows, cfg.cols, width, height, NULL, title);
    set_tab_bar_visibility();
}
void win_tab_create() {
    STerm* t = tabs[active_tab]->terminal;
    char cwd_path[256];
    sprintf(cwd_path,"/proc/%d/cwd",t->child->_pid ); 
    char* cwd = realpath(cwd_path, 0);
    newtab(t->rows, t->cols, t->cols * cell_width, t->rows * cell_height, cwd, NULL);
    free(cwd);
    set_active_tab(ntabs - 1);
    set_tab_bar_visibility();
}

void win_tab_clean() {
    bool invalidate = false;
    Tab**p,**pd;
    for(pd=p=tabs;*p;p++){
        if((*p)->chld->_pid==0){
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
            set_active_tab(ntabs - 1);
        else
            set_active_tab(active_tab);
        set_tab_bar_visibility();
        invalidate_tabs();
    }
    if(*tabs==NULL){
        exit_mintty();
    }
}

void win_tab_attention(STerm*pterm) {
    tab_by_term(pterm)->info.attention = true;
    invalidate_tabs();
}

void win_tab_set_title(STerm*pterm, wchar_t* title) {
    Tab* tab = tab_by_term(pterm);
    if (wcscmp(tab->info.titles[tab->info.titles_i] , title)) {
        wcsncpy(tab->info.titles[tab->info.titles_i] , title,TAB_LTITLE);
        invalidate_tabs();
    }
    if (pterm == win_tab_active_term()) {
        win_set_title((wchar *)tab->info.titles[tab->info.titles_i]);
    }
}

wchar_t* win_tab_get_title(unsigned int idx) {
    return tabs[idx]->info.titles[tabs[idx]->info.titles_i];
}

void win_tab_title_push(STerm*pterm) {
    Tab* tab = tab_by_term(pterm);
    if (tab->info.titles_i == TAB_NTITLE)
        tab->info.titles_i = 0;
    else
        tab->info.titles_i++;
}

wchar_t* win_tab_title_pop(STerm*pterm) {
    Tab* tab = tab_by_term(pterm);
    if (!tab->info.titles_i)
        tab->info.titles_i = TAB_NTITLE;
    else
        tab->info.titles_i--;
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


static bool tab_bar_visible = false;
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
    int b=ntabs > 1;
    if (b == tab_bar_visible) return;
    tab_bar_visible = b;
    OFFSET = win_tab_height();
    fix_window_size();
    invalidate_tabs();
}

static int tab_font_size() {
    return 24 * g_yscale;
}
static int tabheight() {
    init_scale_factors();
    return 36 * g_yscale;
}
int win_tab_height() { return tab_bar_visible ? tabheight() : 0; }

static HGDIOBJ new_tab_font() {
    return CreateFont(tab_font_size(),0,0,0,FW_NORMAL,0,0,0,1,0,0,CLEARTYPE_QUALITY,0,0);
}

static HGDIOBJ new_active_tab_font() {
    return CreateFont(tab_font_size(),0,0,0,FW_BOLD,0,0,0,1,0,0,CLEARTYPE_QUALITY,0,0);
}

// paint a tab to dc (where dc draws to buffer)
static void paint_tab(HDC dc, int width, int tabh, const Tab* tab) {
    MoveToEx(dc, 0, tabh, NULL);
    LineTo(dc, 0, 0);
    LineTo(dc, width, 0);
    TextOutW(dc, width/2, (tabh - tab_font_size()) / 2, tab->info.titles[tab->info.titles_i], wcslen(tab->info.titles[tab->info.titles_i]));
}

static int tab_paint_width = 0;
void win_tab_paint(HDC dc) {
    if (!tab_bar_visible) return;
    // the sides of drawable area are not visible, so we really should draw to
    // coordinates 1..(width-2)
    RECT r;
    GetClientRect(wnd, &r);
    int width = r.right-r.left - 2 * PADDING;
    int tabh=tabheight();

    const colour bg = cfg.tab_bg_colour;
    const colour fg = cfg.tab_fg_colour;
    const colour active_bg = cfg.tab_active_bg_colour;
    const colour attention_bg = cfg.tab_attention_bg_colour;

    const unsigned int preferred_width = 200 * g_xscale;

    const int tabwidth = (width / ntabs) > preferred_width ? preferred_width : width / ntabs;
    const int loc_tabheight = tabh-2;
    tab_paint_width = tabwidth;
    RECT tabrect;
    SetRect(&tabrect, 0, 0, tabwidth, loc_tabheight+1);
    HDC bufdc = CreateCompatibleDC(dc);
    SetBkMode(bufdc, TRANSPARENT);
    SetTextColor(bufdc, fg);
    SetTextAlign(bufdc, TA_CENTER);
    {
        HGDIOBJ brush  = CreateSolidBrush(bg);
        VSELGDIOBJ(obrush , bufdc, brush);
        VSELGDIOBJ(open   , bufdc, CreatePen(PS_SOLID, 0, fg));
        VSELGDIOBJ(obuf   , bufdc, CreateCompatibleBitmap(dc, tabwidth, tabh));
        VSELGDIOBJ(ofont  , bufdc, new_tab_font());

        for (size_t i = 0; i < ntabs; i++) {
            bool  active = i == active_tab;
            if (active) {
                HGDIOBJ activebrush = CreateSolidBrush(active_bg);
                FillRect(bufdc, &tabrect, activebrush);
                DeleteObject(activebrush);
            } else if (tabs[i]->info.attention) {
                HGDIOBJ activebrush = CreateSolidBrush(attention_bg);
                FillRect(bufdc, &tabrect, activebrush);
                DeleteObject(activebrush);
            } else {
                FillRect(bufdc, &tabrect, brush);
            }

            if (active) {
                VSELGDIOBJ( _f , bufdc, new_active_tab_font());
                paint_tab(bufdc, tabwidth, loc_tabheight, tabs[i]);
                VDELGDIOBJ(_f);
            } else {
                MoveToEx(bufdc, 0, loc_tabheight, NULL);
                LineTo(bufdc, tabwidth, loc_tabheight);
                paint_tab(bufdc, tabwidth, loc_tabheight, tabs[i]);
            }

            BitBlt(dc, i*tabwidth+PADDING, PADDING, tabwidth, tabh, bufdc, 0, 0, SRCCOPY);
        }

        if ((int)ntabs * tabwidth < width) {
            SetRect(&tabrect, 0, 0, width - (ntabs * tabwidth), loc_tabheight+1);
            VSELGDIOBJ(obuf   ,bufdc, CreateCompatibleBitmap(dc, width - (ntabs * tabwidth), tabh));
            FillRect(bufdc, &tabrect, brush);
            MoveToEx(bufdc, 0, 0, NULL);
            LineTo(bufdc, 0, loc_tabheight);
            LineTo(bufdc, width - (ntabs * tabwidth), loc_tabheight);
            BitBlt(dc, ntabs*tabwidth+PADDING, PADDING, width - (ntabs * tabwidth), tabh, bufdc, 0, 0, SRCCOPY);
            VDELGDIOBJ(obuf  );
        }
        VDELGDIOBJ(obrush);
        VDELGDIOBJ(open  );
        VDELGDIOBJ(obuf  );
        VDELGDIOBJ(ofont );
    }
    DeleteDC(bufdc);
}

void win_tab_for_each(void (*cb)(STerm*pterm)) {
    for (Tab** tab=tabs;*tab;tab++)
        cb((*tab)->terminal);
}

void win_tab_mouse_click(int x) {
    unsigned int itab = x / tab_paint_width;
    if (itab >= ntabs)
        return;
    set_active_tab(itab);
}
Tab**   win_tabs() {
    return tabs;
}
