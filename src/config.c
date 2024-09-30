// config.c (part of mintty)
// Copyright 2008-2023 Andy Koppe, 2015-2024 Thomas Wolff
// Based on code from PuTTY-0.60 by Simon Tatham and team.
// Licensed under the terms of the GNU General Public License v3 or later.

// Internationalization approach:
// instead of refactoring a lot of framework functions (here, *ctrls.c)
// to use Unicode strings, the API is simply redefined to use UTF-8;
// non-ASCII strings are converted before being passed to the platform
// (using UTF-16 on Windows)

#include "term.h"
#include "ctrls.h"
#include "print.h"
#include "charset.h"
#include "win.h"
#include <fcntl.h>  // open+flags, mkdir

#include <windows.h>  // registry handling
#include "winpriv.h"  // support_wsl, load_library_func

#ifdef __CYGWIN__
#include <sys/cygwin.h>  // cygwin_internal
#endif




string config_dir = 0;
static wstring rc_filename = 0;

#define CLRFGSCPY(d,s) memcpy(d,s,16*sizeof(colourfg))
#define CLRFGSCMP(d,s) memcmp(d,s,16*sizeof(colourfg))
static colourfg def_colours[16]={0};
static colourfg cfg_colours[16]={0};
static colourfg theme_colours[16]={0};
static char theme_file[128]={0};

#define HASHTS 257
static inline int HASHS(const void*s){
  const unsigned char*p=(const unsigned char*)s;
  if(p==0||*p==0)return 0;
  if(p[1]==0||p[2]==0) return ((*(unsigned short*)s)|0x2020)%(HASHTS);
  return ((*(unsigned int*)s)|0x20202020)%(HASHTS);
}
// all entries need initialisation in options[] or crash...
#define CFGDEFT  DEFVAL
const config default_cfg = {
#include "configdef.h"
};
  // Text
//  .font = {.name = W("Lucida Console"), .size = 16, .weight = 400, .isbold = false},
config cfg, new_cfg, file_cfg;

typedef enum {
  OPT_BOOL=0,     OPT_MOD,        OPT_TRANS,      OPT_CURSOR, 
  OPT_FONTST,     OPT_FONTRD,     OPT_MDCLICK,    OPT_RTCLICK, 
  OPT_SCRLBAR,    OPT_BORDER,     OPT_WINDOW,     OPT_HOLD,
  OPT_CHARWD,     OPT_EMOJIS,     OPT_COMPKEY,    OPT_EM_PLACE,
  //0x10
  OPT_CPYHTML,    OPT_BOLD,       OPT_BELL   ,    OPT_FLASH,
  OPT_LOG    ,    OPT_SPBRK,      OPT_PCTRL,      OPT_ALEAD,
  OPT_LIGS   ,    OPT_DPIA,       OPT_PRSC,       
  OPT_CMT,        OPT_ARR ,       OPT_INT,        OPT_INTP,       
  //0x20 
  OPT_STR,        OPT_WSTR,       OPT_STRK,       OPT_WSTRK,      
  OPT_CLR,        OPT_CLRFG,      OPT_FONT,       OPT_STRS,
  OPT_BFILE,
  OPT_TYPE_MASK = 0x3F,
  OPT_LG = 0x40,
  OPT_THM  = 0x80,
} opt_type;

enum {
  OPF_H=0  ,OPF_PANE ,OPF_GRP  ,OPF_COL  ,
  OPF_LSTR ,OPF_LSTC ,OPF_CHK  ,OPF_ACLR ,
  OPF_CLR  ,OPF_CLRFG,OPF_INT  ,OPF_WSIZE,
  OPF_STR  ,OPF_WSTR ,OPF_TRANS,OPF_FONT ,
  OPF_BACKF,
  OPF_THEME,OPF_LCL  ,OPF_CHSET,OPF_CURS ,
  OPF_TERM ,OPF_BELL ,OPF_BELLF,OPF_PRINT,
  OPF_END  ,OPF_MASK=0x3f,
};
#define offcfg(option) offsetof(config, option)
typedef const struct {
  string name;//config name
  ushort offset;//var offset of config
  uchar type;//var type
  uchar flag;//dialog type
  union{
    struct{
      int   v0,v1;//v0<=int var<=v1
    }i;
    void *s;
  };
  uchar level; //
  uchar column;//column 参数
  uchar size;  //ratio of combo control
  string label;//dialog label
  string tip;//dialog label
}cfg_option;
#define CFGDEFT  DEFCFG
static cfg_option options[]= {
#include "configdef.h"
  {0}
};


//__ terminal theme / colour scheme
static const opt_val * lopt_vals[OPT_CMT];
static opt_val * loptvals=NULL;
static const opt_val * const opt_vals[] = {
  [OPT_BOOL] = (opt_val[]) {
    {__("no"), false},
    {__("yes"), true},
    {__("false"), false},
    {__("true"), true},
    {__("off"), false},
    {__("on"), true},
    {0, 0}
  },
  [OPT_MOD] = (opt_val[]) {
    {__("off"), 0},
    {__("shift"), MDK_SHIFT},
    {__("alt"), MDK_ALT},
    {__("ctrl"), MDK_CTRL},
    {__("win"), MDK_WIN},
    {__("super"), MDK_SUPER},
    {__("hyper"), MDK_HYPER},
    {__("capslock"), MDK_CAPSLOCK},
    {0, 0}
  },
  [OPT_TRANS] = (opt_val[]) {
    {__("off"), TR_OFF},
    {__("low"), TR_LOW},
    {__("medium"), TR_MEDIUM},
    {__("high"), TR_HIGH},
    {__("glass"), TR_GLASS},
    {0, 0}
  },
  [OPT_CURSOR] = (opt_val[]) {
    {__("line"), CUR_LINE},
    {__("block"), CUR_BLOCK},
    {__("box"), CUR_BOX},
    {__("underscore"), CUR_UNDERSCORE},
    {0, 0}
  },
  [OPT_FONTST] = (opt_val[]) {
    {__("Default"), FS_DEFAULT},
    {__("None"), FS_NONE},
    {__("Partial"), FS_PARTIAL},
    {__("Full"), FS_FULL},
    {0, 0}
  },
  [OPT_FONTRD] = (opt_val[]) {
    {__("textout"), FR_TEXTOUT},
    {__("uniscribe"), FR_UNISCRIBE},
    {0, 0}
  },
  [OPT_MDCLICK] = (opt_val[]) {
    {__("enter"), MC_ENTER},
    {__("paste"), MC_PASTE},
    {__("extend"), MC_EXTEND},
    {__("void"), MC_VOID},
    {0, 0}
  },
  [OPT_RTCLICK] = (opt_val[]) {
    {__("enter"), RC_ENTER},
    {__("paste"), RC_PASTE},
    {__("extend"), RC_EXTEND},
    {__("menu"), RC_MENU},
    {0, 0}
  },
  [OPT_SCRLBAR] = (opt_val[]) {
    {__("left"), -1},
    {__("right"), 1},
    {__("none"), 0},
    {0, 0}
  },
  [OPT_BORDER] = (opt_val[]) {
    {"normal", BORDER_NORMAL},
    {"frame", BORDER_FRAME},
    {"void", BORDER_VOID},
    {0, 0}
  },
  [OPT_WINDOW] = (opt_val[]){
    {__("hide"), 0},   // SW_HIDE
    {__("normal"), 1}, // SW_SHOWNORMAL
    {__("min"), 2},    // SW_SHOWMINIMIZED
    {__("max"), 3},    // SW_SHOWMAXIMIZED
    {__("FULL"), -1},
    {0, 0}
  },
  [OPT_HOLD] = (opt_val[]) {
    {__("never"), HOLD_NEVER},
    {__("start"), HOLD_START},
    {__("error"), HOLD_ERROR},
    {__("always"), HOLD_ALWAYS},
    {0, 0}
  },
  [OPT_CHARWD] = (opt_val[]) {
    {__("locale"), 0},
    {__("unicode"), 1},
    {__("ambig-wide"), 2},
    {__("ambig-narrow"), 3},
    {__("single"), 10},
    {__("single-unicode"), 11},
    {0, 0}
  },
  [OPT_EMOJIS] = (opt_val[]) {
    {__("none"), EMOJIS_NONE},
    {__("openmoji"), EMOJIS_OPENMOJI},
    {__("noto"), EMOJIS_NOTO},
    {__("joypixels"), EMOJIS_JOYPIXELS},
    {__("emojione"), EMOJIS_ONE},
    {__("apple"), EMOJIS_APPLE},
    {__("google"), EMOJIS_GOOGLE},
    {__("twitter"), EMOJIS_TWITTER},
    {__("facebook"), EMOJIS_FB},
    {__("samsung"), EMOJIS_SAMSUNG},
    {__("windows"), EMOJIS_WINDOWS},
    {__("zoom"), EMOJIS_ZOOM},
    {0, 0}
  },
  [OPT_COMPKEY] = (opt_val[]) {
    {__("off"), 0},
    {__("shift"), MDK_SHIFT},
    {__("alt"), MDK_ALT},
    {__("ctrl"), MDK_CTRL},
    {__("super"), MDK_SUPER},
    {__("hyper"), MDK_HYPER},
    {0, 0}
  },
  [OPT_EM_PLACE] = (opt_val[]) {
    {__("stretch"), EMPL_STRETCH},
    {__("align"), EMPL_ALIGN},
    {__("middle"), EMPL_MIDDLE},
    {__("full"), EMPL_FULL},
    {0, 0}
  },
  [OPT_CPYHTML] = (opt_val[]) {
    {__("none"), 0},
    {__("partial"), 1},
    {__("default"), 2},
    {__("full"), 3},
    {0, 0}
  },
  [OPT_BOLD] = (opt_val[]) {
    {__("as font"), 1},
    {__("as colour"), 2},
    {__("as font & as colour"), 3},
    {__("xterm"), 0},
    {0, 0}
  },
  [OPT_BELL] = (opt_val[]) {
    {__("simple beep"),   -1},
    {__("no beep"),       0},
    {__("Default Beep"),	1},
    {__("Critical Stop"),	2},
    {__("Question"),	    3},
    {__("Exclamation"),	  4},
    {__("Asterisk"),	    5},
    {0, 0}
  },
  [OPT_FLASH] = (opt_val[]) {
    {__("Flash Frame  ") , 1}, 
    {__("Flash Border ") , 2}, 
    {__("Flash Full   ") , 4}, 
    {__("Flash Reverse") , 8},
    {0, 0}
  },
  [OPT_LOG] = (opt_val[]) {
    {__("No Log   ") , 0},
    {__("Input Log") , 1}, 
    {__("Output Log") , 2}, 
    {__("I/O Log") , 3}, 
    {0, 0}
  },
  [OPT_SPBRK] = (opt_val[]) {
    {__("No Split Bracketed") , 0},
    {__("Split no Alt Screen") , 1}, 
    {__("Split Bracketed All") , 2}, 
    {0, 0}
  },
  [OPT_PCTRL] = (opt_val[]) {
    {__("No Print Controls") , 0},
    {__("Print High Controls") , 1}, 
    {__("Print All Controls") , 2}, 
    {0, 0}
  },
  [OPT_ALEAD] = (opt_val[]) {
    {__("No Auto Leading") , 0},
    {__("Auto Leading 1") , 1}, 
    {__("Auto Leading 2") , 2}, 
    {0, 0}
  },
  [OPT_LIGS] = (opt_val[]) {
    {__("No ligatures support") , 0},
    {__("ligatures support 1") , 1}, 
    {__("ligatures support 2") , 2}, 
    {0, 0}
  },
  [OPT_DPIA] = (opt_val[]) {
    {__("DPI Unaware") , 0},
    {__("DPI Aware ver 1") , 1}, 
    {__("DPI Aware ver 2") , 2}, 
    {0, 0}
  },
  [OPT_PRSC] = (opt_val[]) {
    {__("No Progress Scan") , 0},
    {__("Progress Scan on current cursor line") , 1}, 
    {__("Progress Scan on all line") , 2}, 
    {0, 0}
  }
};
static void init_loptvals(){
  int i,n,m=0;
  if(loptvals)return ;
  for(i=0;i<OPT_CMT;i++){
    const opt_val *o = opt_vals[i];
    for(n=0;o[n].name;n++);
    m+=n+1;
  }
  loptvals=(opt_val*)malloc(m*sizeof(opt_val));
  opt_val *od=loptvals;
  for(i=0;i<OPT_CMT;i++){
    const opt_val *o = opt_vals[i];
    lopt_vals[i]=od;
    for(n=0;o[n].name;n++){
      od[n].val=o[n].val;
      od[n].name=_(o[n].name);
    }
    od[n].name=NULL;
    od[n].val=0;
    od+=n+1;
  }
}

#ifdef debug_theme
#define trace_theme(params)	printf params
#else
#define trace_theme(params)
#endif
static struct {
  uchar r, g, b;
  char * name;
} xcolours[] = {
#include "rgb.t"
};

bool parse_colour(string s, colour *cp)
{
  uint r, g, b ,ok=0;
  float c, m, y, k = 0;
  switch(s[0]){
    when '#':
        if (sscanf(s, "#%2x%2x%2x", &r, &g, &b) == 3) ok=1;
        else if (sscanf(s, "#%4x%4x%4x", &r, &g, &b) == 3) {
          r >>= 8;
          g >>= 8;
          b >>= 8;
          ok=2;
        }
    when 'r':
        if (sscanf(s, "rgb:%2x/%2x/%2x", &r, &g, &b) == 3) ok=3;
        else if (sscanf(s, "rgb:%4x/%4x/%4x", &r, &g, &b) == 3)
          r >>= 8, g >>= 8, b >>= 8,ok=4;
    when 'c':
        if (sscanf(s, "cmy:%f/%f/%f", &c, &m, &y) == 3
          || sscanf(s, "cmyk:%f/%f/%f/%f", &c, &m, &y, &k) == 4
          ){
          if (c >= 0 && c <= 1 && m >= 0 && m <= 1 && y >= 0 && y <= 1 && k >= 0 && k <= 1) {
            r = (1 - c) * (1 - k) * 255;
            g = (1 - m) * (1 - k) * 255;
            b = (1 - y) * (1 - k) * 255;
            ok=5;
          }
        }
    when '0' ... '9':
        if (sscanf(s, "%u,%u,%u", &r, &g, &b) == 3) ok=6;
  }
  if(!ok){
    int coli = -1;
    int len = strlen(s);
    while (len && s[len - 1] == ' ') len--;
    for (uint i = 0; i < lengthof(xcolours); i++)
      if (0 == strncasecmp(s, xcolours[i].name, len)) {
        r = xcolours[i].r;
        g = xcolours[i].g;
        b = xcolours[i].b;
        coli = i;
        break;
      }
    if (coli < 0)
      return false;
    ok=7;
  }

  *cp = make_colour(r, g, b);
  return true;
}
static int GetOptVal(int type,const char*val_str){
  int len = strlen(val_str);
  if (!len)return -1;
  for (const opt_val *o = lopt_vals[type]; o->name; o++) {
    if (!strncasecmp(val_str, o->name, len)) {
      return o->val;
    }
  }
  for (const opt_val *o = opt_vals[type]; o->name; o++) {
    if (!strncasecmp(val_str, o->name, len)) {
      return o->val;
    }
  }
  // Value not found: try interpreting it as a number.
  char *val_end;
  int val = strtol(val_str, &val_end, 0);
  if (val_end != val_str) {
    return val;
  }
  return -1;
}
static void validcfgtype(opt_type type,const char *tag){
  if((type& OPT_TYPE_MASK)>=OPT_CMT){
    printf("In %s, Error opt_type %d>%d\n",tag,type,OPT_CMT);
  }
}
static int set_opt(int type,void*val_p,const char*val_str,bool from_file){
  int res=1;
  switch (type& OPT_TYPE_MASK) {
    when OPT_INT: {
      char *val_end;
      int val = strtol(val_str, &val_end, 0);
      if (val_end != val_str) {
        *(int *)val_p = val;
      }
    }
    when OPT_INTP: {
      char c,*val_end;
      int val = strtol(val_str, &val_end, 0);
      if (val_end != val_str) {
        ((intpair *)val_p)->x = val;
      }
      for(val_str=val_end+1;(c=*val_str);val_str++){
        if(!strchr(", ;:x",c))break;
      }
      val = strtol(val_str, &val_end, 0);
      if (val_end != val_str) {
        ((intpair *)val_p)->y = val;
      }
    }
    when OPT_STR:{
      ((char *)val_str)[strcspn(val_str, "\r")] = 0;
      strset(val_p, val_str);
    }
    when OPT_STRK:
        strset(val_p, val_str);
    when OPT_STRS:{
      ((char *)val_str)[strcspn(val_str, "\r")] = 0;
      strsadd(val_p,val_str);
    }
    when OPT_WSTR: {
      wchar * ws;
      ((char *)val_str)[strcspn(val_str, "\r")] = 0;
      if (from_file)
        ws = cs__utforansitowcs(val_str);
      else
        ws = cs__mbstowcs(val_str);
      wstrset(val_p, ws);
      delete(ws);
    }
    when OPT_WSTRK: {
      wchar * ws;
      if (from_file)
        ws = cs__utforansitowcs(val_str);
      else
        ws = cs__mbstowcs(val_str);
      wstrset(val_p, ws);
      delete(ws);
    }
    when OPT_CLR:
#ifdef debug_theme
        printf("set_option <%s> <%s>\n", name, val_str);
#endif
    if (!parse_colour(val_str, val_p))res=0;
    when OPT_CLRFG: {
#ifdef debug_theme
      printf("set_option <%s> <%s>\n", name, val_str);
#endif
      colourfg *pair = val_p;
      if (parse_colour(val_str, &pair->fg)) {
        const char *sep = strchr(val_str, ';');
        if (!sep) pair->bg = pair->fg;
        else if (!parse_colour(sep + 1, &pair->bg)) res=0;
      }else res=0;
    }
    when OPT_FONT: {
      wchar ws[64];
      char bold[64];
      font_spec *p=(font_spec*)val_p;
      bold[0]=0;
      sscanf(val_str,"%64l[^,],%d,%d,%64s",ws,&p->size,&p->weight,bold);
      wstrset(&p->name, ws);
      if(bold[0]){
        int val=GetOptVal(OPT_BOOL,bold);
        if(val>=0) p->isbold=(val>0);
      }
    }
    when OPT_BFILE: {
      backg_analyse(val_str,(bg_file*)val_p);
    }
    when OPT_CMT: ;
    when OPT_ARR: ;
    otherwise: {
      validcfgtype(type, "Set_Option ");
      int val=GetOptVal(type,val_str);
      if(val>=0){
        *(CTYPE*)val_p = val;
      }else res=0;
    }
  }
  return res;
}

static void copy_opt(int type,void*dst_val_p ,void*src_val_p ){
  switch (type & OPT_TYPE_MASK) {
    when OPT_INT:
        *(int *)dst_val_p = *(int *)src_val_p;
    when OPT_INTP:
        *(intpair *)dst_val_p = *(intpair *)src_val_p;
    when OPT_STR or OPT_STRK:
        strset(dst_val_p, *(string *)src_val_p);
    when OPT_STRS:
        strscpy(dst_val_p, src_val_p);
    when OPT_WSTR or OPT_WSTRK:
        wstrset(dst_val_p, *(wstring *)src_val_p);
    when OPT_CLR:
        *(colour *)dst_val_p = *(colour *)src_val_p;
    when OPT_CLRFG:
        *(colourfg *)dst_val_p = *(colourfg *)src_val_p;
    when OPT_FONT:{ 
      font_spec *pd=(font_spec*)dst_val_p;
      font_spec *ps=(font_spec*)src_val_p;
      wstrset(&pd->name, ps->name);
      pd->size=ps->size;
      pd->weight=ps->weight;
      pd->isbold=ps->isbold;
    }
    when OPT_BFILE: {
      bg_file *pd=(bg_file*)dst_val_p;
      bg_file *ps=(bg_file*)src_val_p;
      pd->type=ps->type;
      if(pd->type){
        if(ps->fn){
          wstrset(&pd->fn,ps->fn);
        }else{
          if(pd->fn)free((void*)pd->fn);
          pd->fn=NULL;
        }
        pd->alpha=ps->alpha;
        pd->update=1;
      }
    }
    when OPT_CMT: ;
    when OPT_ARR: ;
        otherwise:
        validcfgtype(type, "copy_config");
    *(CTYPE *)dst_val_p = *(CTYPE *)src_val_p;
  }
}
static int ischg_opt(int type,const void*val_p,const void*new_val_p){
  int changed=0;
  switch (type & OPT_TYPE_MASK) {
    when OPT_INT:
        changed = (*(int *)val_p != *(int *)new_val_p);
    when OPT_INTP:
        changed = ((((intpair *)val_p)->x != ((intpair *)new_val_p)->x)||
                   (((intpair *)val_p)->y != ((intpair *)new_val_p)->y));
    when OPT_STR or OPT_STRK:
        changed = strcmp(*(string *)val_p, *(string *)new_val_p);
    when OPT_STRS:{
      strings*p=(strings*)val_p;
      strings*q=(strings*)new_val_p;
      if(p->n==q->n){
        changed=0;
        for(int i=0;i<p->n;i++){
          if(strcmp(p->s[i],q->s[i])){
            changed=1;
            break;
          }
        }
      }else changed=1;
    }
    when OPT_WSTR or OPT_WSTRK:
        changed = wcscmp(*(wstring *)val_p, *(wstring *)new_val_p);
    when OPT_CLR:
        changed = (*(colour *)val_p != *(colour *)new_val_p);
    when OPT_CLRFG:
        changed = memcmp(val_p, new_val_p, sizeof(colourfg));
    when OPT_FONT:{ 
      font_spec *pd=(font_spec*)val_p;
      font_spec *ps=(font_spec*)new_val_p;
      changed=pd->size!=ps->size|| 
          pd->weight!=ps->weight||
          pd->isbold!=ps->isbold||
          wcscmp(pd->name, ps->name);
    }
    when OPT_BFILE: {
      bg_file *pd=(bg_file*)val_p;
      bg_file *ps=(bg_file*)new_val_p;
      changed=pd->type!=ps->type||
          pd->alpha!=ps->alpha||
          wcscmp(pd->fn,ps->fn);
    }
    when OPT_CMT: ;
    when OPT_ARR: ;
    otherwise:
      validcfgtype(type, "apply_config");
      changed = (*(CTYPE *)val_p != *(CTYPE *)new_val_p);
  }
  return changed;
}
static void printOptVar(FILE*file,const char*name,int type,const void*val_p){
  fprintf(file, "%s=", name);
  switch (type & OPT_TYPE_MASK) {
    when OPT_INT:
        fprintf(file, "%i", *(int *)val_p);
    when OPT_INTP:
        fprintf(file, "%i,%i", ((intpair *)val_p)->x,((intpair *)val_p)->y);
    when OPT_STR or OPT_STRK:
        fprintf(file, "%s", *(string *)val_p);
    when OPT_STRS:{
      strings*p=(strings*)val_p;
      fprintf(file, "%s", p->s[0]);
      for(int i=1;i<p->n;i++){
        fprintf(file, "\n%s=%s",name, *(string *)val_p);
      }
    }
    when OPT_WSTR or OPT_WSTRK: {
      char * s = cs__wcstoutf(*(wstring *)val_p);
      fprintf(file, "%s", s);
      delete(s);
    }
    when OPT_CLR: {
      colour c = *(colour *)val_p;
      fprintf(file, "%u,%u,%u", red(c), green(c), blue(c));
    }
    when OPT_CLRFG: {
      colourfg p = *(colourfg *)val_p;
      fprintf(file, "%u,%u,%u", red(p.fg), green(p.fg), blue(p.fg));
      if (p.fg != p.bg)
        fprintf(file, ";%u,%u,%u", red(p.bg), green(p.bg), blue(p.bg));
    }
    when OPT_FONT:{ 
      font_spec *pd=(font_spec*)val_p;
      fprintf(file,"%ls,%d,%d,%d",pd->name,pd->size,pd->weight,pd->isbold);
    }
    when OPT_BFILE: {
      bg_file *ps=(bg_file*)val_p;
      if(ps->type){
        if(ps->type=='='){
          fprintf(file,"%c",ps->type);
        }else{
          if(ps->fn&&*ps->fn){
            if(ps->alpha==255){
              fprintf(file,"%c%ls",ps->type,ps->fn);
            }else{
              fprintf(file,"%c%ls,%d",ps->type,ps->fn,ps->alpha);
            }
          }
        }
      }
    }
    when OPT_CMT: ;
    when OPT_ARR: ;
    otherwise: {
      validcfgtype(type, "printOptVar");
      int val = *(CTYPE *)val_p;
      const opt_val *o = lopt_vals[type];
      for (; o->name; o++) {
        if (o->val == val)
          break;
      }
      if (o->name) fputs(o->name, file);
      else fprintf(file, "%i", val);
    }
  }
  fputc('\n', file);
}
char *
save_filename(const char * suf)
{
  char * pat = cs__wcstombs(cfg.save_filename);

  // expand initial ~ or $variable
  char * sep;
  if (*pat == '~' && pat[1] == '/') {
    char * pat1 = asform("%s%s", wv.home, pat + 1);
    delete(pat);
    pat = pat1;
  }
  else if (*pat == '$' && (sep = strchr(pat, '/'))) {
    *sep = 0;
    if (getenv(pat + 1)) {
      char * pat1 = asform("%s/%s", getenv(pat + 1), sep + 1);
      delete(pat);
      pat = pat1;
    }
  }
  wchar * wpat = cs__mbstowcs(pat);
  delete(pat);
  pat = path_win_w_to_posix(wpat);
  delete(wpat);
  //printf("save_filename pat %ls -> %s\n", cfg.save_filename, pat);

  struct timeval now;
  gettimeofday(& now, 0);
  char * fn = newn(char, MAX_PATH + 1 + strlen(suf));
  strftime(fn, MAX_PATH, pat, localtime(& now.tv_sec));
  //printf("save_filename [%s] (%s) -> %s%s\n", pat, suf, fn, suf);
  delete(pat);
  strcat(fn, suf);

  // make sure directory exists
  char * basesep = strrchr(fn, '/');
  if (basesep) {
    *basesep = 0;
    if (access(fn, X_OK | W_OK) != 0) {
      mkdir(fn, 0755);
    }
    *basesep = '/';
  }

  return fn;
}


#define dont_debug_opterror

static void
opterror(string msg, bool utf8params, string p1, string p2)
{
  print_opterror(stderr, msg, utf8params, p1, p2);
}
#define USEHASHOPT
#ifdef USEHASHOPT
typedef struct {
  int n,m;
  int*t;
}s_co_hashtab;
static s_co_hashtab  hopts[HASHTS] = {0};
static int initedhopts=0;
static void inithopt()
{
  string nm;
  if(initedhopts)return;
  initedhopts=1;
  for (uint i = 0; (nm=options[i].name); i++) {
    s_co_hashtab *pt=&hopts[HASHS(nm)];
    renewm(pt->t,pt->n, pt->m);
    pt->t[pt->n]=i;
    pt->n++;
  }
}
static int
find_option(bool from_file, string name){
  if(initedhopts==0)inithopt();
  s_co_hashtab *pt=&hopts[HASHS(name)];
  if(pt){
    for (int i = 0; i<pt->n; i++) {
      if (!strcasecmp(name, options[pt->t[i]].name))
        return pt->t[i];
    }
  }
  //__ %s: unknown option name
  opterror(_("Ignoring unknown option '%s'"), from_file, name, 0);
  return -1;
}
#else
static int
find_option(bool from_file, string name)
{
  for (int i = 0; options[i].name; i++) {
    if (!strcasecmp(name, options[i].name))
      return i;
  }
  //__ %s: unknown option name
  opterror(_("Ignoring unknown option '%s'"), from_file, name, 0);
  return -1;
}
#endif
typedef union{
  char * comment;
  uintptr_t opti;
} cfg_file_opt;
static cfg_file_opt *file_opts=NULL;
static ushort *arg_opts=NULL;
static uint file_opts_num = 0,maxfileopt=0;
static uint arg_opts_num=0,maxargopt=0;

static void
clear_opts(void)
{
  for (uint n = 0; n < file_opts_num; n++)
    if (file_opts[n].opti>0x10000){
      delete(file_opts[n].comment);
      file_opts[n].opti=0;
    }
  file_opts_num = 0;
  arg_opts_num = 0;
}

static bool
seen_file_option(uint i)
{
//  return memchr(file_opts, i, file_opts_num);
  for (uint n = 0; n < file_opts_num; n++)
    if (file_opts[n].opti == i)
      return true;
  return false;
}

static bool
seen_arg_option(uint i)
{
  if(!arg_opts)return 0;
  for(uint j=0;j<arg_opts_num;j++){
    if(arg_opts[j]==i)return 1;
  }
  return 0;
}

static void
remember_file_option(const char * tag, uint i)
{
  (void)tag;
  trace_theme(("[%s] remember_file_option (file %d arg %d) %d %s\n", tag, seen_file_option(i), seen_arg_option(i), i, options[i].name));

  if (!seen_file_option(i)) {
    renewm(file_opts,file_opts_num,maxfileopt);
    file_opts[file_opts_num].opti = i;
    file_opts_num++;
  }
}

static void
remember_file_comment(const char * comment)
{
  trace_theme(("[] remember_file_comment <%s>\n", comment));
  renewm(file_opts,file_opts_num,maxfileopt);
  file_opts[file_opts_num].comment = strdup(comment);
  file_opts_num++;
}

static void
remember_arg_option(const char * tag, uint i)
{
  (void)tag;
  trace_theme(("[%s] remember_arg_option (file %d arg %d) %d %s\n", tag, seen_file_option(i), seen_arg_option(i), i, options[i].name));

  if (!seen_arg_option(i)){
    renewm(arg_opts,arg_opts_num,maxargopt);
    arg_opts[arg_opts_num++] = i;
  }
}

static void
check_legacy_options(void (*remember_option)(const char * tag, uint))
{
  if (cfg.use_system_colours) {
    // Translate 'UseSystemColours' to colour settings.
    cfg.colour.fg = cfg.cursor_colour = win_get_sys_colour(COLOR_WINDOWTEXT);
    cfg.colour.bg = win_get_sys_colour(COLOR_WINDOW);
    cfg.use_system_colours = false;

    // Make sure they're written to the config file.
    // This assumes that the colour options are the first three in options[].
    remember_option("legacy", 0);
    remember_option("legacy", 1);
    remember_option("legacy", 2);
  }
}

static int
set_option(config * p,string name, string val_str, bool from_file)
{
  int i = find_option(from_file, name);
  if (i < 0) return i;
  uint type = options[i].type ;
  if(set_opt(type ,(void *)p + options[i].offset,val_str,from_file))return i;
  //__ %2$s: option name, %1$s: invalid value
  if (!wv.wnd)  // report errors only during initialisation
    opterror(_("Ignoring invalid value '%s' for option '%s'"), 
             from_file, val_str, name);
  return -1;
}

static int
parse_option(config * p,string option, bool from_file)
{
  const char *eq = strchr(option, '=');
  if (!eq) {
    ((char *)option)[strcspn(option, "\r")] = 0;
    //__ %s: option name
    opterror(_("Ignoring option '%s' with missing value"), 
             from_file, option, 0);
    return -1;
  }

  const char *name_end = eq;
  while (isspace((uchar)name_end[-1]))
    name_end--;

  uint name_len = name_end - option;
  char name[name_len + 1];
  memcpy(name, option, name_len);
  name[name_len] = 0;

  const char *val = eq + 1;
  while (isspace((uchar)*val))
    val++;

  return set_option(p,name, val, from_file);
}

static void
check_arg_option(int i)
{
  if (i >= 0) {
    remember_arg_option("chk_arg", i);
    check_legacy_options(remember_arg_option);
  }
}

void
set_arg_option(string name, string val)
{
  check_arg_option(set_option(&cfg,name, val, false));
}

void
parse_arg_option(string option)
{
  check_arg_option(parse_option(&cfg,option, false));
}


/*
   In a configuration parameter list, map tag to value.
 */
char *
matchconf(char * conf,const char * item)
{
  char * cmdp = conf;
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

    if (!strcmp(cmdp, item))
      return paramp;

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
  return 0;
}


static string * config_dirs = 0;
static int last_config_dir = -1;

static void
init_config_dirs(void)
{
  if (config_dirs)
    return;

  int ncd = 3;
  char * appdata = getenv("APPDATA");
  if (appdata)
    ncd++;
  if (config_dir)
    ncd++;
  config_dirs = newn(string, ncd);

  // /usr/share/mintty , $APPDATA/mintty , ~/.config/mintty , ~/.mintty
  config_dirs[++last_config_dir] = "/usr/share/mintty";
  if (appdata) {
    appdata = newn(char, strlen(appdata) + 8);
    sprintf(appdata, "%s/mintty", getenv("APPDATA"));
    config_dirs[++last_config_dir] = appdata;
  }
  if (!wv.support_wsl && access(wv.home, X_OK) == 0) {
    char * xdgconf = newn(char, strlen(wv.home) + 16);
    sprintf(xdgconf, "%s/.config/mintty", wv.home);
    config_dirs[++last_config_dir] = xdgconf;
    char * homeconf = newn(char, strlen(wv.home) + 9);
    sprintf(homeconf, "%s/.mintty", wv.home);
    config_dirs[++last_config_dir] = homeconf;
  }
  if (config_dir) {
    config_dirs[++last_config_dir] = config_dir;
  }
}

char *
get_resource_file(string sub, string res, bool towrite)
{
  init_config_dirs();
  int fd;
  for (int i = last_config_dir; i >= 0; i--) {
    char * resfn = asform(("%s/%s/%s"), config_dirs[i], sub,res);
    fd = open(resfn, towrite ? O_CREAT |  O_WRONLY | O_BINARY : O_RDONLY | O_BINARY, 0644);
#if CYGWIN_VERSION_API_MINOR >= 194
    if (towrite && fd < 0 && errno == ENOENT) {
      // try to create resource subdirectories
      int dd = open(config_dirs[i], O_RDONLY | O_DIRECTORY);
      if (dd) {
        mkdirat(dd, "themes", 0755);
        mkdirat(dd, "sounds", 0755);
        mkdirat(dd, "lang", 0755);
        close(dd);
      }
      // retry
      fd = open(resfn, O_CREAT | O_WRONLY | O_BINARY, 0644);
    }
#endif
    if (fd >= 0) {
      close(fd);
      return resfn;
    }
    delete(resfn);
    if (errno == EACCES || errno == EEXIST)
      break;
  }
  return null;
}

#define dont_debug_messages 1

typedef struct {
  string msg;
  string locmsg;
  wstring wmsg;
}s_message;
static void setmsg(s_message*pm,string msg, string locmsg){
  pm->msg = msg;
  pm->locmsg = locmsg;
  pm->wmsg = null;
}
static void freemsg(s_message *pm){
    VFREE((void*)pm->msg);
    VFREE((void*)pm->locmsg);
    if  (pm->wmsg) VFREE((void*)pm->wmsg);
}
#define usehashtab
#ifdef usehashtab
typedef struct {
  int n,m;
  s_message *t;
}s_hashtab;
static s_hashtab  hmsgt[HASHTS] = {0};
static s_message*findmsg(string msg){
  s_hashtab *pt=&hmsgt[HASHS(msg)];
  for (int i = 0; i < pt->n; i++) {
    if (strcmp(msg, pt->t[i].msg) == 0) { return &pt->t[i]; }
  }
  return NULL;
}
static void clear_messages()
{
  for (int j = 0; j < HASHTS; j++) { 
    s_hashtab *pt=&hmsgt[j];
    for (int i = 0; i < pt->n; i++) { freemsg(pt->t+i); }
    VFREE(pt->t);
    pt->t=0;pt->n=0;pt->m=0;
  }
}
static s_message *add_message(string msg, string locmsg)
{
  s_hashtab *pt=&hmsgt[HASHS(msg)];
  renewm(pt->t,pt->n, pt->m);
  setmsg(& pt->t[pt->n], msg, locmsg);
  pt->n++;
  return & pt->t[pt->n-1];
}
#else
static s_message * messages = {0};
int nmessages = 0;
int maxmessages = 0;
static s_message*findmsg(string msg){
  for (int i = 0; i < nmessages; i++) {
    if (strcmp(msg, messages[i].msg) == 0) { return &messages[i]; }
  }
  return NULL;
}
static void clear_messages()
{
  for (int i = 0; i < nmessages; i++) { freemsg(messages+i); }
  nmessages = 0;
}
static s_message *add_message(string msg, string locmsg)
{
  renewm(messages,nmessages , maxmessages);
#if defined(debug_messages) && debug_messages > 3
  printf("add %d <%s> <%s>\n", nmessages, msg, locmsg);
#endif
  setmsg(& messages[nmessages], msg, locmsg);
  nmessages ++;
  return & messages[nmessages];
}
#endif

const char*loctext(string msg)
{
  s_message *pm=findmsg(msg);
  if(pm)return (char*)pm->locmsg;
  return  (char*)msg;
}

const wchar *wloctext(string msg)
{
  s_message *pm=findmsg(msg);
  if(!pm)pm=add_message(strdup(msg), strdup(msg));
  if (pm->wmsg == null)
    pm->wmsg = cs__utftowcs(pm->locmsg);
  return (wchar*)pm->wmsg;
}

static char *
readtext(char * buf, int len, FILE * file)
{
  char * unescape(char * s)
  {
    char * t = s;
    while (*s && *s != '"') {
      if (*s == '\\') {
        s++;
        switch (*s) {
          when 't': *t = '\t';
          when 'n': *t = '\n';
          otherwise: *t = *s;
        }
      }
      else
        *t = *s;
      t++;
      s++;
    }
    *t = '\0';
    return t;
  }

  char * p = buf;
  while (*p != ' ')
    p++;
  while (*p == ' ')
    p++;
  if (strncmp(p, "\"\"", 2) == 0) {
    // scan multi-line text
    char * str = newn(char, 1);
    *str = '\0';
    while (fgets(buf, len, file) && *buf == '"') {
      p = buf + 1;
      char * f = unescape(p);
      if (!*f) {
        str = renewn(str, strlen(str) + strlen(p) + 1);
        strcat(str, p);
      }
      else {
        delete(str);
        return null;
      }
    }
    return str;
  }
  else {
    // scan single-line text
    p++;
    char * f = unescape(p);
    if (!*f) {
      char * str = strdup(p);
      fgets(buf, len, file);
      return str;
    }
  }
  return null;
}

static void
load_messages_file(const char * textdbf)
{
  char linebuf[444];
  FILE * file = fopen(textdbf, "r");
  int nmsgs=0;
  if (file) {
    clear_messages();

    while (fgets(linebuf, sizeof linebuf, file)) {
      linebuf[strcspn(linebuf, "\r\n")] = 0;  /* trim newline */
      if (strncmp(linebuf, "msgid ", 6) == 0) {
        char * msg = readtext(linebuf, sizeof linebuf, file);
        if (strncmp(linebuf, "msgstr ", 7) == 0) {
          char * locmsg = readtext(linebuf, sizeof linebuf, file);
          if (msg && *msg && locmsg && *locmsg){
            nmsgs++; add_message(msg, locmsg);
          }
        }
      }
      else {
      }
    }
    fclose(file);
  }
#ifdef debug_messages
  printf("read %d messages\n", nmsgs);
#endif
}

static bool
load_messages_lang(string lang, bool fallback)
{

  if (lang) {
    char * wl = newn(char, strlen(lang) + 4);
    strcpy(wl, lang);
    if (fallback) {
      char * _ = strchr(wl, '_');
      if (_) {
        *_ = '\0';
      }
      else
        return false;
    }
    strcat(wl, ".po");
    const char * textdbf = get_resource_file("lang", wl, false);
    delete(wl);
#ifdef debug_messages
    printf("Trying to load messages from <%ls>: <%s>\n", lang, textdbf);
#endif
    if (textdbf) {
      load_messages_file(textdbf);
      delete(textdbf);
      return true;
    }
  }
  return false;
}

static void
load_messages(config * cfg_p)
{
  if (cfg_p->lang) for (int fallback = false; fallback <= true; fallback++) {
#ifdef debug_messages
    printf("Loading localization <%ls> (fallback %d)\n", cfg_p->lang, fallback);
#endif
    clear_messages();
    if (strcmp(cfg_p->lang, ("=")) == 0) {
      if (load_messages_lang(cfg_p->locale, fallback))
        return;
    }
    else if (strcmp(cfg_p->lang, ("@")) == 0) {
      // locale_menu[1] is transformed from GetUserDefaultUILanguage()
      if (load_messages_lang(locale_menu[1], fallback))
        return;
    }
    else if (strcmp(cfg_p->lang, ("*")) == 0) {
      // determine UI language from environment
      const char * lang = getenv("LANGUAGE");
      if (lang) {
        lang = strdup(lang);
        while (lang && *lang) {
          char * sep = strchr(lang, ':');
          if (sep)
            *sep = '\0';
          if (load_messages_lang(lang, fallback))
            return;
          lang = sep;
          if (lang)
            lang++;
        }
        delete(lang);
      }
      lang = getlocenvcat("LC_MESSAGES");
      if (lang && *lang) {
        lang = strdup(lang);
        char * dot = strchr(lang, '.');
        if (dot)
          *dot = '\0';
        if (load_messages_lang(lang, fallback))
          return;
        delete(lang);
      }
    }
    else {
      if (load_messages_lang(cfg_p->lang, fallback))
        return;
    }
  }
}

static int 
load_configr(config * p,string filename,int to_save)
{
  char linebuf[444];
  bool free_filename = false;
  if (*filename == '~' && filename[1] == '/') {
    filename = asform("%s%s", wv.home, filename + 1);
    free_filename = true;
  }
  FILE * file = fopen(filename, "r");

  if (free_filename) delete(filename);
  if(!file)return 0;

  while (fgets(linebuf, sizeof linebuf, file)) {
    char * lbuf = linebuf;
    int len;
    while (len = strlen(lbuf),
      (len && lbuf[len - 1] != '\n') ||
      (len > 1 && lbuf[len - 1] == '\n' && lbuf[len - 2] == '\\')
      )
    {
      if (lbuf == linebuf) {
        // make lbuf dynamic
        lbuf = strdup(lbuf);
      }
      // append to lbuf
      len = strlen(lbuf);
      lbuf = renewn(lbuf, len + sizeof linebuf);
      if (!fgets(&lbuf[len], sizeof linebuf, file))
        break;
    }

    if (lbuf[len - 1] == '\n')
      lbuf[len - 1] = 0;

    if (lbuf[0] == '#' || lbuf[0] == '\0') {
      // preserve comment lines and empty lines
      if (to_save)
        remember_file_comment(lbuf);
    }
    else {
      // apply config options
      int i = parse_option(p,lbuf, true);
      // remember config options for saving
      if (to_save) {
        if (i >= 0)
          remember_file_option("load", i);
        else
          // preserve unknown options as comment lines
          remember_file_comment(lbuf);
      }
    }
    if (lbuf != linebuf)
      delete(lbuf);
  }
  fclose(file);
  return 1;
}

// to_save:
// 0 read config from filename
// 1 use filename for saving if file exists and is writable
// 2 use filename for saving if none was previously determined
// 3 use filename for saving (override)
void
load_config(string filename, int to_save)
{
  int rok;
  trace_theme(("load_config <%s> %d\n", filename, to_save));
  if (!to_save) {
    // restore base configuration, without theme mix-ins
    copy_config("load", &cfg, &file_cfg);
  }

  bool free_filename = false;
  if (*filename == '~' && filename[1] == '/') {
    filename = asform("%s%s", wv.home, filename + 1);
    free_filename = true;
  }
  rok=access(filename, R_OK) == 0;

  if (rok && access(filename, W_OK) < 0)
    to_save = false;

  // prevent saving to /etc/minttyrc
  if (strstr(filename, "/etc/") == filename)
    to_save = false;

  if (wv.report_config)
    printf("loading config <%s>\n", filename);

  if (to_save) {
    if (rok|| (!rc_filename && to_save == 2) || to_save == 3) {
      clear_opts();

      delete(rc_filename);
      rc_filename = path_posix_to_win_w(filename);
      if (wv.report_config)
        printf("save to config <%ls>\n", rc_filename);
    }
  }

  if(load_configr(&cfg,filename, to_save)){
    CLRFGSCPY(cfg_colours,cfg.ansi_colours);
  }
  if (free_filename) delete(filename);
  check_legacy_options(remember_file_option);
  if (to_save) {
    copy_config("after load", &file_cfg, &cfg);
  }
}

void
copy_config(string tag, config * dst_p, const config * src_p)
{
#ifdef debug_theme
  char * _cfg(const config * p) {
    return p == &new_cfg ? "new" : p == &file_cfg ? "file" : p == &cfg ? "cfg" : "?";
  }
  printf("[%s] copy_config %s <- %s\n", tag, _cfg(dst_p), _cfg(src_p));
#else
  (void)tag;
#endif
  for (uint i = 0; options[i].name; i++) {
    //printf("cpycfg %d:%s\n",i,options[i].name);
    opt_type type = options[i].type;
    if (!(type & OPT_LG)) {
      uint offset = options[i].offset;
      copy_opt(type,(void *)dst_p + offset,(void *)src_p + offset);
    }
  }
}

void
init_config(void)
{
  init_loptvals();
  copy_config("init", &cfg, &default_cfg);
  CLRFGSCPY(def_colours,default_cfg.ansi_colours);
}

static void
fix_config(void)
{
  // Avoid negative sizes.
  cfg.winsize.y = max(1, cfg.winsize.y);
  cfg.winsize.x = max(1, cfg.winsize.x);
  cfg.scrollback_lines = max(0, cfg.scrollback_lines);

  // Limit size of scrollback buffer.
  cfg.scrollback_lines = min(cfg.scrollback_lines, cfg.max_scrollback_lines);
}

void
finish_config(void)
{
  if (*cfg.lang && (strcmp(cfg.lang, ("=")) != 0 || *cfg.locale))
    load_messages(&cfg);
#if defined(debug_messages) && debug_messages > 1
  else
    (void)load_messages_lang("messages",0);
#endif
#ifdef debug_opterror
  opterror("Tast L %s %s", false, "bh", "bh");
  opterror("Tast U %s %s", true, "bh", "bh");
#endif

  fix_config();

  // Ignore charset setting if we haven't got a locale.
  if (!*cfg.locale)
    strset(&cfg.charset, "");

  // bold_as_font used to be implied by !bold_as_colour.
  //printf("finish_config bd %d\n", cfg.bold_as_font);
#ifdef previous_patch_for_242
  // This tweak was added in commit/964b3097e4624d4b5a3231389d34c00eb5cd1d6d
  // to support bold display as both font and colour (#242)
  // but it does not seem necessary anymore with the current code and options
  // handling, and it confuses option initialization (mintty/wsltty#103),
  // so it's removed.
  if (cfg.bold_as_font == -1) {
    cfg.bold_as_font = !cfg.bold_as_colour;
    remember_file_option("finish", find_option(true, "BoldAsFont"));
  }
#endif

  if (0 < cfg.transparency && cfg.transparency <= 3)
    cfg.transparency *= 16;
  //printf("finish_config bd %d\n", cfg.bold_as_font);
}

static void
save_config(void)
{
  string filename;

  filename = path_win_w_to_posix(rc_filename);

  FILE *file = fopen(filename, "w");
  delete(filename);

  if (!file) {
    // Should we report the failed Windows or POSIX path? (see mintty/wsltty#42)
    // In either case, we must transform to Unicode.
    // For WSL, it's probably not a good idea to report a POSIX path 
    // because it would be mistaken for a WSL path.
    char *msg;
    char * up = cs__wcstoutf(rc_filename);
    //__ %1$s: config file name, %2$s: error message
    int len = asprintf(&msg, _("Could not save options to '%s':\n%s."),
                       up, strerror(errno));
    delete(up);
    if (len > 0) {
      win_show_error(msg);
      delete(msg);
    }
    return ;
  }
  void *cfg_p = &file_cfg;
  for (uint j = 0; j < file_opts_num; j++) {
    if (file_opts[j].opti>0x10000) {
      fprintf(file, "%s\n", file_opts[j].comment);
      continue;
    }
    uint i = file_opts[j].opti;
    opt_type type = options[i].type;
    if (!(type & OPT_LG)) {
      printOptVar(file, options[i].name,type,cfg_p+options[i].offset);
    }
  }
  fclose(file);
}

static int 
load_themer(config *p,string theme)
{
  char * thf=NULL;
  if (!*theme) return 0;
  if (strchr(theme, '/') || strchr(theme, '\\')) 
    thf = path_win_to_posix(theme);
  else 
    thf = get_resource_file("themes", theme, false);
  if(!thf)return 0;
  int r=load_configr(p,thf, false);
  delete(thf);
  if(!r)return 0;
  CLRFGSCPY(theme_colours,p->ansi_colours);
  strncpy(theme_file,theme,128);
  return 1;
}
void 
load_theme(string theme){
  load_themer(&cfg,theme);
}

static void
load_schemer(config*p,string cs)
{
  //copy_config("scheme", &cfg, &file_cfg);

  // analyse scheme description
  char * scheme = strdup(cs);
  char * sch = scheme;
  char * param = scheme;
  char * value = null;
  while (*sch) {
    if (*sch == '=') {
      *sch++ = '\0';
      value = sch;
    }
    else if (*sch == ';') {
      *sch++ = '\0';
      if (value) {
        set_option(p,param, value, false);
      }
      param = sch;
      value = null;
    }
    else
      sch++;
  }
  delete(scheme);
}
void
load_scheme(string cs){
  load_schemer(&cfg,cs);
}
void
apply_config(bool save)
{
  // Record what's changed
  for (uint i = 0; options[i].name; i++) {
    opt_type type = options[i].type;
    uint offset = options[i].offset;
    //void *val_p = (void *)&cfg + offset;
    if (!(type & OPT_LG)) {
      if(ischg_opt(type ,(void *)&file_cfg + offset,(void *)&new_cfg + offset)){
        remember_file_option("apply", i);
      }
    }
  }

  copy_config("apply", &file_cfg, &new_cfg);
  if (strcmp(new_cfg.lang, cfg.lang) != 0
      || (strcmp(new_cfg.lang, ("=")) == 0 && new_cfg.locale != cfg.locale)
     ){
    load_messages(&new_cfg);
  }
  if(CLRFGSCMP(theme_colours,file_cfg.ansi_colours)==0){
    CLRFGSCPY(file_cfg.ansi_colours,cfg_colours);
  }else{
    *(char*)file_cfg.theme_file=0;
  }
  if(ischg_opt(OPT_BFILE,&cfg.backgfile,&new_cfg.backgfile)){
    new_cfg.backgfile.update=1;
  }
  win_reconfig();  // copy_config(&cfg, &new_cfg);
  fix_config();
  if (save)
    save_config();
  bool had_theme = !!*cfg.theme_file;

  if (*cfg.colour_scheme) {
    load_scheme(cfg.colour_scheme);
    win_reset_colours();
    win_invalidate_all(false);
  }
  else if (*cfg.theme_file) {
    load_theme(cfg.theme_file);
    win_reset_colours();
    win_invalidate_all(false);
  }
  else if (had_theme) {
    win_reset_colours();
    win_invalidate_all(false);
  }
  //printf("apply_config %d bd %d\n", save, cfg.bold_as_font);
}


// Registry handling (for retrieving localized sound labels)

HKEY
regopen(HKEY key,const char * subkey)
{
  HKEY hk = 0;
  RegOpenKeyA(key, subkey, &hk);
  return hk;
}

static HKEY
getmuicache()
{
  HKEY hk = regopen(HKEY_CURRENT_USER, "Software\\Classes\\Local Settings\\MuiCache");
  if (!hk)
    return 0;

  char sk[256];
  if (RegEnumKeyA(hk, 0, sk, 256) != ERROR_SUCCESS)
    return 0;

  HKEY hk1 = regopen(hk, sk);
  RegCloseKey(hk);
  if (!hk1)
    return 0;

  if (RegEnumKeyA(hk1, 0, sk, 256) != ERROR_SUCCESS)
    return 0;

  hk = regopen(hk1, sk);
  RegCloseKey(hk1);
  if (!hk)
    return 0;

  return hk;
}

static HKEY muicache = 0;
static HKEY evlabels = 0;

static void
retrievemuicache()
{
  muicache = getmuicache();
  if (muicache) {
    evlabels = regopen(HKEY_CURRENT_USER, "AppEvents\\EventLabels");
    if (!evlabels) {
      RegCloseKey(muicache);
      muicache = 0;
    }
  }
}

static void
closemuicache()
{
  if (muicache) {
    RegCloseKey(evlabels);
    RegCloseKey(muicache);
  }
}

wchar *
getregstr(HKEY key, wstring subkey, wstring attribute)
{
#if CYGWIN_VERSION_API_MINOR < 74
  (void)key;
  (void)subkey;
  (void)attribute;
  return 0;
#else
  // RegGetValueW is easier but not supported on Windows XP
  HKEY sk = 0;
  if(subkey){
    RegOpenKeyW(key, subkey, &sk);
    if (!sk) return 0;
  }else sk=key;
  DWORD type;
  DWORD len;
  int res = RegQueryValueExW(sk, attribute, 0, &type, 0, &len);
  if (res)
    return 0;
  if (!(type == REG_SZ || type == REG_EXPAND_SZ || type == REG_MULTI_SZ))
    return 0;
  wchar * val = malloc (len);
  res = RegQueryValueExW(sk, attribute, 0, &type, (void *)val, &len);
  if(subkey){
    RegCloseKey(sk);
  }
  if (res) {
    delete(val);
    return 0;
  }
  return val;
#endif
}

uint
getregval(HKEY key, wstring subkey, wstring attribute)
{
#if CYGWIN_VERSION_API_MINOR < 74
  (void)key;
  (void)subkey;
  (void)attribute;
  return 0;
#else
  // RegGetValueW is easier but not supported on Windows XP
  HKEY sk = 0;
  RegOpenKeyW(key, subkey, &sk);
  if (!sk)
    return 0;
  DWORD type;
  DWORD len;
  int res = RegQueryValueExW(sk, attribute, 0, &type, 0, &len);
  if (res)
    return 0;
  if (type == REG_DWORD) {
    DWORD val;
    len = sizeof(DWORD);
    res = RegQueryValueExW(sk, attribute, 0, &type, (void *)&val, &len);
    RegCloseKey(sk);
    if (!res)
      return (uint)val;
  }
  return 0;
#endif
}

static wchar *
muieventlabel(const wchar * event)
{
  // HKEY_CURRENT_USER\AppEvents\EventLabels\SystemAsterisk
  // DispFileName -> "@mmres.dll,-5843"
  wchar * rsr = getregstr(evlabels, event, W("DispFileName"));
  if (!rsr)
    return 0;
  // HKEY_CURRENT_USER\Software\Classes\Local Settings\MuiCache\N\M
  // "@mmres.dll,-5843" -> "Sternchen"
  wchar * lbl = getregstr(muicache, 0, rsr);
  delete(rsr);
  return lbl;
}


// Options dialog handlers

static void
ok_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION) {
    apply_config(true);
    dlg_end();
  }
}

static void
cancel_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    dlg_end();
}

static void
apply_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    apply_config(false);
}

static void
about_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    win_show_about();
}


#if CYGWIN_VERSION_API_MINOR < 74
#define use_findfile
#endif

#ifdef use_findfile
static void
do_file_resources(control *ctrl, string pattern, bool list_dirs, str_fn fnh)
{
  static wstring *lst=NULL;
  static int nl=0,ml=0;
  void clrlist(){
    for (int i=0;i<nl;i++){
      delete(lst[i]);
    }
    nl=0;ml=0;
    delete(lst);
    lst=NULL;
  }
  int addlist(wstring s){
    for (int i=0;i<nl;i++){
      if(wcscasecmp(lst[i],s)==0)return i;
    }
    renewm(lst,nl,ml);
    lst[nl++]=wcsdup(s);
    return -nl;
  }
  clrlist();
  init_config_dirs();


  for (int i = last_config_dir; i >= 0; i--) {
    string suf = strrchr(pattern, '.');
    int sufl = suf ? strlen(suf) : 0;
    char * rc = asform(W("%s/%s"), config_dirs[i], pattern);
    wchar * rcpat = path_posix_to_win_w(rc);
    char *rt=strrchr(rc,L'/');
    int len=0;
    if(rt)len=rt-rc+1;

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(rcpat, &ffd);
    int ok = hFind != INVALID_HANDLE_VALUE;
    rcpat[len]=0;
    delete(rcpat);
    if (ok) {
      while (ok) {
        if (list_dirs && (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
          if (ffd.cFileName[0] != '.' && !!wcscmp(ffd.cFileName, W("common")))
            // exclude the [0-7] links left over by the `getemojis` script
            if (wcslen(ffd.cFileName) > 1)
              if(addlist(ffd.cFileName)<0){
                if (ctrl){
                  dlg_listbox_add_w(ctrl, ffd.cFileName);
                }
              }
        }
        else if (!list_dirs) {
          //LARGE_INTEGER filesize = {.LowPart = ffd.nFileSizeLow, .HighPart = ffd.nFileSizeHigh};
          //long s = filesize.QuadPart;

          // strip suffix
          int len = wcslen(ffd.cFileName);
          if (ffd.cFileName[0] != '.' && ffd.cFileName[len - 1] != '~') {
            ffd.cFileName[len - sufl] = 0;
            if(addlist(ffd.cFileName)<0){
              if (ctrl)
                dlg_listbox_add_w(ctrl, ffd.cFileName);
              else if(fnh){
                char * fn= asform(("%ls/%ls"), rc, ffd.cFileName);
                fnh(fn);  // CYGWIN_VERSION_API_MINOR < 74
                delete(fn);
              }
            }
          }
        }
        ok = FindNextFileW(hFind, &ffd);
      }
      FindClose(hFind);

      //break;
    }
    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
      // empty valid dir
      //break;
    }
  }
  clrlist();
}
#else
static void
do_file_resources(control *ctrl, string pattern, bool list_dirs, str_fn fnh)
{
  static string *lst=NULL;
  static int nl=0,ml=0;
  void clrlist(){
    for (int i=0;i<nl;i++){
      delete(lst[i]);
    }
    nl=0;ml=0;
    delete(lst);
    lst=NULL;
  }
  int addlist(string s){
    for (int i=0;i<nl;i++){
      if(strcasecmp(lst[i],s)==0)return i;
    }
    renewm(lst,nl,ml);
    lst[nl++]=strdup(s);
    return -nl;
  }
  init_config_dirs();
  //printf("add_file_resources <%s> dirs %d\n", pattern, list_dirs);
  clrlist();


#include <dirent.h>
  for (int i = last_config_dir; i >= 0; i--) {
    char*pat=strdup(pattern);
    char * patsuf = strrchr(pat, '.');
    int patsuflen = patsuf ? strlen(patsuf) : 0;
    char * patbase = strrchr(pat, '/');
    if (patbase)
      *patbase = 0;
    char * rcpat = asform("%s/%s", config_dirs[i], pat);

    DIR * dir = opendir(rcpat);
    if (dir) {
      struct dirent * direntry;
      while ((direntry = readdir(dir)) != 0) {
        if (patsuf && !strstr(direntry->d_name, patsuf))
          continue;

        if (list_dirs && direntry->d_type == DT_DIR) {
          if (direntry->d_name[0] != '.' && !!strcmp(direntry->d_name, "common"))
            // exclude the [0-7] links left over by the `getemojis` script
            if (strlen(direntry->d_name) > 1) {
              if(addlist(direntry->d_name)<0){
                if (ctrl)
                  dlg_listbox_add(ctrl, direntry->d_name);
              }
            }
        }
        else if (!list_dirs) {
          // strip suffix
          int len = strlen(direntry->d_name);
          if (direntry->d_name[0] != '.' && direntry->d_name[len - 1] != '~') {
            direntry->d_name[len - patsuflen] = 0;
            if(addlist(direntry->d_name)<0){
              if (ctrl)
                dlg_listbox_add(ctrl, direntry->d_name);
              else {
                char * fn = asform(("%s/%s"), rcpat, direntry->d_name);
                if(fnh)fnh(fn);
                delete(fn);
              }
            }
          }
        }
      }
      closedir(dir);
    }
    delete(rcpat);
    delete(pat);
  }
  clrlist();
}
#endif

static void
add_file_resources(control *ctrl, string pattern, bool list_dirs)
{
  do_file_resources(ctrl, pattern, list_dirs, 0);
}

void
handle_file_resources(string pattern, str_fn fn_handler)
{
  do_file_resources(0, pattern, false, fn_handler);
}


static void
current_size_handler(control *ctrl, int event)
{
  intpair* p = ctrl->context;
  if (event == EVENT_ACTION) {
    p->x = term.cols;
    p->y = term.rows;
    dlg_refreshp(ctrl);
  }
}

static void
printer_handler(control *ctrl, int event)
{
  const wstring NONE = _W("◇ None (printing disabled) ◇");  // ♢◇
  const wstring CFG_NONE = W("");
  const wstring DEFAULT = _W("◆ Default printer ◆");  // ♦◆
  const wstring CFG_DEFAULT = W("*");
  wstring printer = new_cfg.printer;
  if (event == EVENT_REFRESH) {
    dlg_listbox_clear(ctrl);
    dlg_listbox_add_w(ctrl, NONE);
    dlg_listbox_add_w(ctrl, DEFAULT);
    uint num = printer_start_enum();
    for (uint i = 0; i < num; i++)
      dlg_listbox_add_w(ctrl, printer_get_name(i));
    printer_finish_enum();
    if (*printer == '*')
      dlg_editbox_set_w(ctrl, DEFAULT);
    else
      dlg_editbox_set_w(ctrl, *printer ? printer : NONE);
  }
  else if (event == EVENT_VALCHANGE || event == EVENT_SELCHANGE) {
    int n = dlg_listbox_getcur(ctrl);
    if (n == 0)
      wstrset(&printer, CFG_NONE);
    else if (n == 1)
      wstrset(&printer, CFG_DEFAULT);
    else
      dlg_editbox_get_w(ctrl, &printer);

    new_cfg.printer = printer;
  }
}

static control *locale_box=NULL, *charset_box=NULL;
static control *transparency_valbox=NULL, *transparency_selbox=NULL;
static void
set_charset(string charset)
{
  strset(&new_cfg.charset, charset);
  dlg_editbox_set(charset_box, charset);
}

static void
locale_handler(control *ctrl, int event)
{
  string locale = new_cfg.locale;
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      string l;
      for (int i = 0; (l = locale_menu[i]); i++)
        dlg_listbox_add(ctrl, l);
      dlg_editbox_set(ctrl, locale);
    when EVENT_UNFOCUS:
      dlg_editbox_set(ctrl, locale);
      if (!*locale)
        set_charset("");
    when EVENT_VALCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.locale);
    when EVENT_SELCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.locale);
      if (*locale == '(')
        strset(&locale, "");
      if (!*locale)
        set_charset("");
#if HAS_LOCALES
      else if (!*new_cfg.charset)
        set_charset("UTF-8");
#endif
      new_cfg.locale = locale;
  }
}

static void
check_locale(void)
{
  if (!*new_cfg.locale) {
    strset(&new_cfg.locale, "C");
    dlg_editbox_set(locale_box, "C");
  }
}

static void
charset_handler(control *ctrl, int event)
{
  string charset = new_cfg.charset;
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      string cs;
      for (int i = 0; (cs = charset_menu[i]); i++)
        dlg_listbox_add(ctrl, cs);
      dlg_editbox_set(ctrl, charset);
    when EVENT_UNFOCUS:
      dlg_editbox_set(ctrl, charset);
      if (*charset)
        check_locale();
    when EVENT_VALCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.charset);
    when EVENT_SELCHANGE:
      dlg_editbox_get(ctrl, &charset);
      if (*charset == '(')
        strset(&charset, "");
      else {
        *strchr(charset, ' ') = 0;
        check_locale();
      }
      new_cfg.charset = charset;
  }
}


static void
cursor_handler(control *ctrl, int event)
{
  wstring *cs=(wstring*)ctrl->context;
  switch (event) {
    when EVENT_REFRESH:{
      dlg_listbox_clear(ctrl);
      for (uint i = 0; cursorstyles[i].name; i++){
        dlg_listbox_add(ctrl, cursorstyles[i].name);
      }
      dlg_editbox_set_w(ctrl, *cs);
    }
    when EVENT_VALCHANGE or EVENT_SELCHANGE:
      dlg_editbox_get_w(ctrl, cs);
  }
}
static void
term_handler(control *ctrl, int event)
{
  bool terminfo_exists(const char * ti) {
    bool terminfo_exists_in(const char * dir, const char * sub, const char * ti) {
      char * terminfo = asform("%s%s/%x/%s", dir, sub ?: "", *ti, ti);
      bool exists = !access(terminfo, R_OK);
      delete(terminfo);
      if (wv.support_wsl && !exists) {
        terminfo = asform("%s%s/%c/%s", dir, sub ?: "", *ti, ti);
        exists = !access(terminfo, R_OK);
        //printf("exists %d <%s>\n", exists, terminfo);
        delete(terminfo);
      }
      return exists;
    }
    if (wv.support_wsl) {
      char * wslroot;
      if (wv.wslname) {
        char * wslnamec = cs__wcstombs(wv.wslname);
        wslroot = asform("//wsl$/%s", wslnamec);
        delete(wslnamec);
      }
      else if (*wv.wsl_basepath)
        wslroot = path_win_w_to_posix(wv.wsl_basepath);
      else
        wslroot = strdup("");
      bool ex = terminfo_exists_in(wslroot, "/usr/share/terminfo", ti);
      delete(wslroot);
      return ex;
    }
    else
      return terminfo_exists_in("/usr/share/terminfo", 0, ti)
          || terminfo_exists_in(wv.home, "/.terminfo", ti)
           ;
  }
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      dlg_listbox_add(ctrl, "xterm");
      dlg_listbox_add(ctrl, "xterm-256color");
      if (terminfo_exists("xterm-direct"))
        dlg_listbox_add(ctrl, "xterm-direct");
      dlg_listbox_add(ctrl, "xterm-vt220");
      dlg_listbox_add(ctrl, "vt100");
      dlg_listbox_add(ctrl, "vt220");
      dlg_listbox_add(ctrl, "vt340");
      dlg_listbox_add(ctrl, "vt420");
      dlg_listbox_add(ctrl, "vt525");
      if (terminfo_exists("mintty"))
        dlg_listbox_add(ctrl, "mintty");
      if (terminfo_exists("mintty-direct"))
        dlg_listbox_add(ctrl, "mintty-direct");
      dlg_editbox_set(ctrl, new_cfg.Term);
    when EVENT_VALCHANGE or EVENT_SELCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.Term);
  }
}

//  1 -> 0x00000000 MB_OK              Default Beep
//  2 -> 0x00000010 MB_ICONSTOP        Critical Stop
//  3 -> 0x00000020 MB_ICONQUESTION    Question
//  4 -> 0x00000030 MB_ICONEXCLAMATION Exclamation
//  5 -> 0x00000040 MB_ICONASTERISK    Asterisk
// -1 -> 0xFFFFFFFF                    Simple Beep
static struct {
  string name;
  wchar * event;
} beeps[] = {
  {__("simple beep"), null},
  {__("no beep"), null},
  {__("Default Beep"),	W(".Default")},
  {__("Critical Stop"),	W("SystemHand")},
  {__("Question"),	W("SystemQuestion")},
  {__("Exclamation"),	W("SystemExclamation")},
  {__("Asterisk"),	W("SystemAsterisk")},
};

static void
bell_handler(control *ctrl, int event)
{
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      retrievemuicache();
      for (uint i = 0; i < lengthof(beeps); i++) {
        string beepname = _(beeps[i].name);
        if (beepname == beeps[i].name) {
          // no localization entry, try to retrieve system localization
          if (muicache && beeps[i].event) {
            wchar * lbl = muieventlabel(beeps[i].event);
            if (lbl) {
              dlg_listbox_add_w(ctrl, lbl);
              if ((int)i == new_cfg.bell_type + 1)
                dlg_editbox_set_w(ctrl, lbl);
              beepname = null;
              delete(lbl);
            }
          }
        }
        if (beepname) {
          dlg_listbox_add(ctrl, beepname);
          if ((int)i == new_cfg.bell_type + 1)
            dlg_editbox_set(ctrl, beepname);
        }
      }
      closemuicache();
    when EVENT_VALCHANGE or EVENT_SELCHANGE: {
      new_cfg.bell_type = dlg_listbox_getcur(ctrl) - 1;
      win_bell(&new_cfg);
    }
  }
}

static void
bellfile_handler(control *ctrl, int event)
{
  const string NONE = ("◇ None (system sound) ◇");  // ♢◇
  const string CFG_NONE = ("");
  string *cs=(string*)ctrl->context;
  if (event == EVENT_REFRESH) {
    dlg_listbox_clear(ctrl);
    dlg_listbox_add(ctrl, NONE);
    add_file_resources(ctrl, ("sounds/*.wav"), false);
    // strip std dir prefix...
    dlg_editbox_set(ctrl, **cs? *cs: NONE);
  }
  else if (event == EVENT_VALCHANGE || event == EVENT_SELCHANGE) {
    if (dlg_listbox_getcur(ctrl) == 0)
      strset(cs, CFG_NONE);
    else
      dlg_editbox_get(ctrl, cs);

    // add std dir prefix?
    win_bell(&new_cfg);
  }
  else if (event == EVENT_DROP) {
    dlg_editbox_set_w(ctrl, dragndrop);
    wstrset2a(cs, dragndrop);
    win_bell(&new_cfg);
  }
}

//static control * theme = null;
static control * store_button = null;

static void
enable_widget(control * ctrl, bool enable)
{
  if (!ctrl)
    return;

  HWND wid = ctrl->widget;
  EnableWindow(wid, enable);
}

#define dont_debug_scheme 2

/*
   Load scheme from URL or file, convert .itermcolors and .json formats
 */
static char *
download_scheme(const char * url)
{
  if (strchr(url, '\''))
    return null;  // Insecure link

  char linebuf[444];
  FILE * sf = 0;
  char * sfn = 0;
  if (url[1] == ':') {
    sf = fopen(url, "r");
  }
  else {
#ifdef use_curl
    static string cmdpat = "curl '%s' -o - 2> /dev/null";
    char * cmd = newn(char, strlen(cmdpat) -1 + strlen(url));
    sprintf(cmd, cmdpat, url);
    sf = popen(cmd, "r");
#else
    HRESULT (WINAPI * pURLDownloadToFile)(void *, LPCSTR, LPCSTR, DWORD, void *) = 0;
    pURLDownloadToFile = load_library_func("urlmon.dll", "URLDownloadToFileA");
    bool ok = false;
    sfn = asform("%s/.mintty-scheme.%d", tmpdir(), getpid());
    if (pURLDownloadToFile) {
# ifdef __CYGWIN__
      /* Need to sync the Windows environment */
      cygwin_internal(CW_SYNC_WINENV);
# endif
      char * wfn = path_posix_to_win_a(sfn);
      ok = S_OK == pURLDownloadToFile(NULL, url, wfn, 0, NULL);
      delete(wfn);
    }
    if (!ok)
      delete(sfn);
    sf = fopen(sfn, "r");
    //printf("URL <%s> file <%s> OK %d\n", url, sfn, !!sf);
    if (!sf) {
      remove(sfn);
      delete(sfn);
    }
#endif
  }
  if (!sf)
    return null;
#ifdef debug_scheme
  printf("URL <%s> OK %d\n", url, !!sf);
#endif

  // colour scheme string
  char * sch = null;
  // colour modifications, common for .itermcolors and .json (unused there)
  colour ansi_colours[16] = 
  {(colour)-1, (colour)-1, (colour)-1, (colour)-1, 
    (colour)-1, (colour)-1, (colour)-1, (colour)-1, 
    (colour)-1, (colour)-1, (colour)-1, (colour)-1, 
    (colour)-1, (colour)-1, (colour)-1, (colour)-1};
  colour fg_colour = (colour)-1, bg_colour = (colour)-1;
  colour bold_colour = (colour)-1, blink_colour = (colour)-1;
  colour cursor_colour = (colour)-1, sel_fg_colour = (colour)-1, sel_bg_colour = (colour)-1;
  colour underl_colour = (colour)-1, hover_colour = (colour)-1;
  // construct a ColourScheme string
  void schapp(const char * opt, colour c)
  {
#if defined(debug_scheme) && debug_scheme > 1
    printf("schapp %s %06X\n", opt, c);
#endif
    if (c != (colour)-1) {
      char colval[strlen(opt) + 14];
      sprintf(colval, "%s=%u,%u,%u;", opt, red(c), green(c), blue(c));
      int len = sch ? strlen(sch) : 0;
      sch = renewn(sch, len + strlen(colval) + 1);
      strcpy(&sch[len], colval);
    }
  }
  // collect all modified colours in a colour scheme string
  void schappall()
  {
    schapp("ForegroundColour", fg_colour);
    schapp("BackgroundColour", bg_colour);
    schapp("BoldColour", bold_colour);
    schapp("BlinkColour", blink_colour);
    schapp("CursorColour", cursor_colour);
    schapp("UnderlineColour", underl_colour);
    schapp("HoverColour", hover_colour);
    schapp("HighlightBackgroundColour", sel_bg_colour);
    schapp("HighlightForegroundColour", sel_fg_colour);
    schapp("Black", ansi_colours[BLACK_I]);
    schapp("Red", ansi_colours[RED_I]);
    schapp("Green", ansi_colours[GREEN_I]);
    schapp("Yellow", ansi_colours[YELLOW_I]);
    schapp("Blue", ansi_colours[BLUE_I]);
    schapp("Magenta", ansi_colours[MAGENTA_I]);
    schapp("Cyan", ansi_colours[CYAN_I]);
    schapp("White", ansi_colours[WHITE_I]);
    schapp("BoldBlack", ansi_colours[BOLD_BLACK_I]);
    schapp("BoldRed", ansi_colours[BOLD_RED_I]);
    schapp("BoldGreen", ansi_colours[BOLD_GREEN_I]);
    schapp("BoldYellow", ansi_colours[BOLD_YELLOW_I]);
    schapp("BoldBlue", ansi_colours[BOLD_BLUE_I]);
    schapp("BoldMagenta", ansi_colours[BOLD_MAGENTA_I]);
    schapp("BoldCyan", ansi_colours[BOLD_CYAN_I]);
    schapp("BoldWhite", ansi_colours[BOLD_WHITE_I]);
  }

  char * urlsuf = strrchr(url, '.');
  if (urlsuf && !strcmp(urlsuf, ".itermcolors")) {
    int level = 0;
    colour * key = 0;
    int component = -1;
    while (fgets(linebuf, sizeof(linebuf) - 1, sf)) {
      if (strstr(linebuf, "<dict>"))
        level++;
      else if (strstr(linebuf, "</dict>"))
        level--;
      else {
        char * entity = strstr(linebuf, "<key>");
        if (entity) {
          entity += 5;
          char * fini = strchr(entity, '<');
          if (fini)
            *fini = 0;
          if (level == 2) {
            if (0 == strcmp(entity, "Blue Component"))
              component = 2;
            else if (0 == strcmp(entity, "Green Component"))
              component = 1;
            else if (0 == strcmp(entity, "Red Component"))
              component = 0;
            else if (0 == strcmp(entity, "Alpha Component"))
              ;
            else if (0 == strcmp(entity, "Color Space"))
              ;
            else {
              component = -1;
            }
          }
          else if (level == 1) {
#if defined(debug_scheme) && debug_scheme > 1
            printf("iterm entity <%s>\n", entity);
#endif
            int coli;
            if (0 == strcmp(entity, "Foreground Color"))
              key = &fg_colour;
            else if (0 == strcmp(entity, "Bold Color"))
              key = &bold_colour;
            else if (0 == strcmp(entity, "Background Color"))
              key = &bg_colour;
            else if (0 == strcmp(entity, "Cursor Color"))
              key = &cursor_colour;
            //else if (0 == strcmp(entity, "Cursor Text Color"))
            else if (0 == strcmp(entity, "Selected Text Color"))
              key = &sel_fg_colour;
            else if (0 == strcmp(entity, "Selection Color"))
              key = &sel_bg_colour;
            else if (0 == strcmp(entity, "Underline Color"))
              key = &underl_colour;
            else if (0 == strcmp(entity, "Link Color"))
              key = &hover_colour;  // ?
                                    //else if (0 == strcmp(entity, "Cursor Guide Color"))
                                    //else if (0 == strcmp(entity, "Tab Color"))
                                    //else if (0 == strcmp(entity, "Badge Color"))
            else if (sscanf(entity, "Ansi %d Color", &coli) == 1 && coli >= 0 && coli < 16) {
              key = &ansi_colours[coli];
            }
            else
              key = 0;
          }
        }
        else if (level == 2 && key) {
#if defined(debug_scheme) && debug_scheme > 1
          printf("iterm value <%s>\n", linebuf);
#endif
          entity = strstr(linebuf, "<real>");
          double val;
          if (entity && sscanf(entity, "<real>%lf<", &val) == 1 && val >= 0.0 && val <= 1.0) {
            int ival = val * 255.0 + 0.5;
            switch (component) {
              when 0:  // red
                  *key = (*key & 0xFFFF00) | ival;
              when 1:  // green
                  *key = (*key & 0xFF00FF) | ival << 8;
              when 2:  // blue
                  *key = (*key & 0x00FFFF) | ival << 16;
            }
          }
        }
      }
    }
    // collect modified colours into colour scheme string
    schappall();
  }
  else if (urlsuf && !strcmp(urlsuf, ".json")) {
    // support .json theme files in either vscode or Windows terminal format
    while (fgets(linebuf, sizeof(linebuf) - 1, sf)) {
      char * scan = strchr(linebuf, '"');
      char * key;
      char * val;
      if (scan) {
        scan++;
        // strip vscode prefixes
        if (strncmp(scan, "terminal.", 9) == 0) {
          scan += 9;
          if (strncmp(scan, "ansi", 4) == 0)
            scan += 4;
        }
        key = scan;
        scan = strchr(scan, '"');
      }
      if (scan) {
        *scan = 0;
        scan++;
        scan = strchr(scan, '"');
        if (scan) {
          scan++;
          val = scan;
          scan = strchr(scan, '"');
          if (scan) {
            *scan = 0;
          }
        }
      }
      if (scan) {
#if defined(debug_scheme) && debug_scheme > 1
        printf("<%s> <%s> (%s)\n", key, val, linebuf);
#endif
        // transform .json colour names
        void schapp(const char * jname, const char * name)
        {
          if (strcasecmp(key, jname) == 0) {
#if defined(debug_scheme) && debug_scheme > 1
            printf("%s=%s\n", name, val);
#endif
            int len = sch ? strlen(sch) : 0;
            sch = renewn(sch, len + strlen(name) + strlen(val) + 3);
            sprintf(&sch[len], "%s=%s;", name, val);
          }
        }
        schapp("black", "Black");
        schapp("red", "Red");
        schapp("green", "Green");
        schapp("yellow", "Yellow");
        schapp("blue", "Blue");
        schapp("magenta", "Magenta");
        schapp("purple", "Magenta");
        schapp("cyan", "Cyan");
        schapp("white", "White");
        schapp("brightblack", "BoldBlack");
        schapp("brightred", "BoldRed");
        schapp("brightgreen", "BoldGreen");
        schapp("brightyellow", "BoldYellow");
        schapp("brightblue", "BoldBlue");
        schapp("brightmagenta", "BoldMagenta");
        schapp("brightpurple", "BoldMagenta");
        schapp("brightcyan", "BoldCyan");
        schapp("brightwhite", "BoldWhite");
        schapp("foreground", "ForegroundColour");
        schapp("background", "BackgroundColour");
      }
    }
  }
  else {
    int l = 0;
    char linebuf[22222];  // in case of json, pull in the whole stuff
    while (fgets(linebuf, sizeof(linebuf) - 1, sf)) {
#if defined(debug_scheme) && debug_scheme > 1
      printf("linebuf <%s>\n", linebuf);
#endif

      if (!l++ && *linebuf == '{') {
        // handle drag-and-drop json formats that contain colour specs like 
        // "Red=190,70,120" (https://github.com/mskyaxl/wsl-terminal) or
        // "Red=220,50,47\r" (https://github.com/oumu/mintty-color-schemes)
        void schapp(char * name)
        {
          char specbuf[30];
          sprintf(specbuf, "\"%s=", name);
          char * colspec = strstr(linebuf, specbuf);
          if (!colspec)
            return;
          colspec++;
          char * cpoi = colspec + strlen(name) + 1;
          while (isdigit((uchar)*cpoi) || *cpoi == ',')
            cpoi++;
          int collen = cpoi - colspec;
          int len = sch ? strlen(sch) : 0;
          sch = renewn(sch, len + collen + 2);
          snprintf(&sch[len], collen + 1, "%s", colspec);
          sprintf(&sch[len + collen], ";");
#if defined(debug_scheme) && debug_scheme > 1
          printf("%s\n", &sch[len]);
#endif
        }
        schapp("ForegroundColour");
        schapp("BackgroundColour");
        schapp("BoldColour");
        schapp("BlinkColour");
        schapp("CursorColour");
        schapp("UnderlineColour");
        schapp("HoverColour");
        schapp("HighlightBackgroundColour");
        schapp("HighlightForegroundColour");
        schapp("Black");
        schapp("Red");
        schapp("Green");
        schapp("Yellow");
        schapp("Blue");
        schapp("Magenta");
        schapp("Cyan");
        schapp("White");
        schapp("BoldBlack");
        schapp("BoldRed");
        schapp("BoldGreen");
        schapp("BoldYellow");
        schapp("BoldBlue");
        schapp("BoldMagenta");
        schapp("BoldCyan");
        schapp("BoldWhite");
        break;
      }else{
        char * eq = linebuf;
        while ((eq = strchr(++eq, '='))) {
          int dum;
          if (sscanf(eq, "= %d , %d , %d", &dum, &dum, &dum) == 3) {
            char *cp = eq;
            while (strchr("=0123456789, ", *cp))
              cp++;
            *cp++ = ';';
            *cp = '\0';
            cp = eq;
            if (cp != linebuf)
              cp--;
            while (strchr("BCFGMRWYacdeghiklnorstuwy ", *cp)) {
              eq = cp;
              if (cp == linebuf)
                break;
              else
                cp--;
            }
            while (*eq == ' ')
              eq++;
            if (*eq != '=') {
              // squeeze white space
              char * src = eq;
              char * dst = eq;
              while (*src) {
                if (*src != ' ' && *src != '\t')
                  *dst++ = *src;
                src++;
              }
              *dst = '\0';

              int len = sch ? strlen(sch) : 0;
              sch = renewn(sch, len + strlen(eq) + 1);
              strcpy(&sch[len], eq);
            }
            break;
          }
        }
      }
    }
  }

#ifdef use_curl
  pclose(sf);
#else
  fclose(sf);
  if (sfn) {
    remove(sfn);
    delete(sfn);
  }
#endif

#if defined(debug_scheme) && debug_scheme > 1
  printf("download_scheme -> <%s>\n", sch);
#endif
  return sch;
}
static int themechanged(){
  string theme_name = new_cfg.theme_file;
  if(!*theme_name)return 0;
  if(strchr(theme_name, '/') || strchr(theme_name, '\\'))return 0;
  if(CLRFGSCMP(theme_colours,new_cfg.ansi_colours))return 1;
  if(strcmp(theme_name,theme_file))return 1;
  return 0;
}

static void
theme_handler(control *ctrl, int event)
{
  //__ terminal theme / colour scheme
  const string NONE = ("◇ None ◇");  // ♢◇
  const string CFG_NONE = ("");
  //__ indicator of unsaved downloaded colour scheme
  const string DOWNLOADED = _("downloaded / give me a name!");
  // downloaded theme indicator must contain a slash
  // to steer enabled state of Store button properly
  const string CFG_DOWNLOADED = ("@/@");
  string theme_name = new_cfg.theme_file;
  int upd=0;
  static int ued=0;
  switch(event){
    when EVENT_REFRESH: {
      ued=0;
      dlg_listbox_clear(ctrl);
      dlg_listbox_add(ctrl, NONE);
      add_file_resources(ctrl, ("themes/*"), false);
#ifdef attempt_to_keep_scheme_hidden
      if (*new_cfg.colour_scheme)
        // don't do this, rather keep previously entered name to store scheme
        // scheme string will not be entered here anyway
        dlg_editbox_set_w(ctrl, W(""));
      else
#endif
        dlg_editbox_set(ctrl, !strcmp(theme_name, CFG_DOWNLOADED) ? DOWNLOADED : *theme_name ? theme_name : NONE);
    }
    when EVENT_SELCHANGE: {  // pull-down selection
      if (dlg_listbox_getcur(ctrl) == 0)
        strset(&theme_name, CFG_NONE);
      else
        dlg_editbox_get(ctrl, &theme_name);

      new_cfg.theme_file = theme_name;
      // clear pending colour scheme
      strset(&new_cfg.colour_scheme, "");
      enable_widget(store_button, false);
      upd=1;
      ued=0;
    }
    when EVENT_VALCHANGE: {  // pasted or typed-in
      ued=1;
    }
    when EVENT_UNFOCUS: {  // edit end
      if(ued){
        ued=0;
        dlg_editbox_get(ctrl, &theme_name);
        new_cfg.theme_file = theme_name;
        enable_widget(store_button,themechanged());
        upd=1;
      }
    }
    when EVENT_DROP: {
      ued=0;
#ifdef debug_scheme
      printf("EVENT_DROP <%ls>\n", dragndrop);
#endif
      char * url = cs__wcstoutf(dragndrop);
      if (wcsncmp(W("data:text/plain,"), dragndrop, 16) == 0) {
        // indicate availability of downloaded scheme to be stored
        dlg_editbox_set(ctrl, DOWNLOADED);
        strset(&new_cfg.theme_file, CFG_DOWNLOADED);
        // un-URL-escape scheme description
        char * scheme = cs__wcstoutf(&dragndrop[16]);
        char * purl = scheme;
        char * sch = scheme;
        while (*purl) {
          int c;
          if (sscanf(purl, "%%%02X", &c) == 1) {
            purl += 3;
          }
          else
            c = *purl++;
          if (c == '\n')
            *sch++ = ';';
          else if (c != '\r')
            *sch++ = c;
        }
        *sch = '\0';
        strset(&new_cfg.colour_scheme, scheme);
        delete(scheme);
        enable_widget(store_button, false);
      }
      else if (wcsncmp(W("http:"), dragndrop, 5) == 0
        || wcsncmp(W("https:"), dragndrop, 6) == 0
        || wcsncmp(W("ftp:"), dragndrop, 4) == 0
        || wcsncmp(W("ftps:"), dragndrop, 5) == 0
        || wcsncmp(W("file:"), dragndrop, 5) == 0
#if CYGWIN_VERSION_API_MINOR >= 74
        || (dragndrop[1] == ':' &&
            (wcsstr(dragndrop, W(".itermcolors")) ||
             wcsstr(dragndrop, W(".json"))
            )
           )
#endif
        )
      {
        char * sch = download_scheme(url);
        if (sch) {
          char * urlpoi = strchr(url, '?');
          if (urlpoi)
            *urlpoi = 0;
          // find URL basename
          urlpoi = strrchr(url, '/');
          if (urlpoi) {
            // set theme name proposal to url base name
            urlpoi++;
            dlg_editbox_set(ctrl, urlpoi);
            strset(&new_cfg.theme_file, urlpoi);
            // set scheme
            strset(&new_cfg.colour_scheme, sch);

            enable_widget(store_button, true);
          }
          delete(sch);
        }
        else {
          win_bell(&new_cfg);  // Could not load web theme
          win_show_warning(_("Could not load web theme"));
        }
      }
      else {
        dlg_editbox_set(ctrl, url);
        strset(&new_cfg.theme_file, url);
        enable_widget(store_button, false);
      }
      delete(url);
    }
  }
  // apply changed theme immediately
  if (strcmp(new_cfg.colour_scheme, cfg.colour_scheme) ){
    load_schemer(&new_cfg,cfg.colour_scheme);
    upd=1;
  }else if( strcmp(theme_name, theme_file)){
    upd=load_themer(&new_cfg,theme_name);
  }
  else if(!*theme_name) { 
    if(upd){
      CLRFGSCPY(new_cfg.ansi_colours,cfg_colours); 
      upd=1;
    }
  }
  if(upd)InvalidateRect(GetParent((HWND)ctrl->widget),NULL,true);
  //if(upd)InvalidateRect((HWND)ctrl->widget,NULL,true);
}
static void
backgfsel_handler(control *ctrl, int event)
{
  bg_file *bf=&new_cfg.backgfile;
  control **cs;
  cs=ctrl->parent->ctrls+ctrl->ind;
  if(ctrl->type==CTRL_FILESELECT){ //0,1:2-8:9-10
    dlg_stdfilesel_handler(ctrl,event);
    if(event== EVENT_VALCHANGE){// EVENT_SELCHANGE: 
      if(bf->fn&&bf->fn){
        if(bf->type==0) bf->type='_';
        if(bf->alpha==0) bf->alpha=255;
      }else{
        bf->type=0;
      }
    }
  }else if(ctrl->type==CTRL_RADIO){
    cs-=1;
    dlg_stdradiobutton_handler(ctrl,event);
    if(event== EVENT_VALCHANGE){// EVENT_SELCHANGE: 
      if(bf->alpha==0) bf->alpha=255;
      if(bf->type==0||bf->type=='='){
        if(bf->fn){ free((void*)bf->fn);bf->fn=NULL; }
      }
    }
  }else if(ctrl->type==CTRL_INTEDITBOX){
    cs-=2;
    dlg_stdintbox_handler(ctrl,event);
  }
  if(event== EVENT_VALCHANGE){// EVENT_SELCHANGE: 
    dlg_stdfilesel_handler(cs[0],EVENT_REFRESH);
    dlg_stdradiobutton_handler(cs[1],EVENT_REFRESH);
    dlg_stdintbox_handler(cs[2],EVENT_REFRESH);
  }
}

#define dont_debug_dragndrop
static void
theme_saver(control *ctrl, int event)
{
  string theme_name =  new_cfg.theme_file;
  if (event == EVENT_REFRESH) {
    enable_widget(ctrl,themechanged());
  }
  else if (event == EVENT_ACTION) {
#ifdef debug_dragndrop
    printf("%ls <- <%s>\n", new_cfg.theme_file, new_cfg.colour_scheme);
#endif
    if(!themechanged()) return;
    char * sn = get_resource_file("themes", theme_name, true);
    if (!sn) {
      win_bell(&new_cfg);  // Cannot store theme file
      win_show_warning(_("Cannot store theme file"));
      return;
    }
    // save colour_scheme to theme_file
    FILE * file= fopen(sn, "w");
    delete(sn);
    if (!file) {
      win_bell(&new_cfg);  // Cannot store theme file
      win_show_warning(_("Cannot store theme file"));
      return;
    }
    void *cfg_p = &new_cfg;
    for (uint i = 0; options[i].name; i++) {
      opt_type type = options[i].type;
      if (!(type & OPT_THM)) continue;
      printOptVar(file, options[i].name,type,cfg_p+options[i].offset);
    }
    fclose(file);
    enable_widget(ctrl,false);
  }
}

static void
bell_tester(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    win_bell(&new_cfg);
}

struct fontlist {
  wstring fn;
  struct weight {
    int weight;
    wstring style;
  } * weights;
  uint weightsn;
};
static struct fontlist * fontlist = 0;
static uint fontlistn = 0;


/* Windows LOGFONT values
	100	FW_THIN
	200	FW_EXTRALIGHT
	200	FW_ULTRALIGHT
	300	FW_LIGHT
	400	FW_NORMAL
	400	FW_REGULAR
	500	FW_MEDIUM
	600	FW_SEMIBOLD
	600	FW_DEMIBOLD
	700	FW_BOLD
	800	FW_EXTRABOLD
	800	FW_ULTRABOLD
	900	FW_HEAVY
	900	FW_BLACK
   Other weight names (http://www.webtype.com/info/articles/fonts-weights/)
    100    Extra Light or Ultra Light
    200    Light or Thin
    300    Book or Demi
    400    Normal or Regular
    500    Medium
    600    Semibold, Demibold
    700    Bold
    800    Black, Extra Bold or Heavy
    900    Extra Black, Fat, Poster or Ultra Black
 */
static wstring weights[] = {
  // the first 9 weight names are used for display and name filtering
  W("Thin"),    	// 100, 200
  W("Extralight"),	// 200, 100
  W("Light"),   	// 300, 200
  W("Regular"), 	// 400
  W("Medium"),  	// 500
  W("Semibold"),	// 600
  W("Bold"),    	// 700
  W("Extrabold"),	// 800
  W("Heavy"),   	// 900, 800
  // the remaining weight names are only used for name filtering
  W("Ultralight"),	// 200, 100
  W("Normal"),  	// 400
  W("Demibold"),	// 600
  W("Ultrabold"),	// 800
  W("Black"),   	// 900, 800
  W("Book"),    	// 300, 400
  W("Demi"),    	// 300
  W("Extrablack"),	// 900
  W("Fat"),     	// 900
  W("Poster"),  	// 900
  W("Ultrablack"),	// 900
};

static void
enterfontlist(const wchar * fn, int weight, const wchar * style)
{
  if (*fn == '@') {
    delete(fn);
    // ignore vertical font
    return;
  }

  bool found = false;
  uint fi = 0;
  while (fi < fontlistn) {
    int cmp = wcscmp(fn, fontlist[fi].fn);
    if (cmp <= 0) {
      if (cmp == 0)
        found = true;
      break;
    }
    fi++;
  }

  if (found) {
    delete(fn);

    bool found = false;
    uint wi = 0;
    while (wi < fontlist[fi].weightsn) {
      int cmp = weight - fontlist[fi].weights[wi].weight;
      if (cmp <= 0) {
        if (cmp == 0)
          found = true;
        break;
      }
      wi++;
    }
    if (found)
      delete(style);
    else {
      fontlist[fi].weightsn++;
      fontlist[fi].weights = renewn(fontlist[fi].weights, fontlist[fi].weightsn);
      for (uint j = fontlist[fi].weightsn - 1; j > wi; j--)
        fontlist[fi].weights[j] = fontlist[fi].weights[j - 1];
      fontlist[fi].weights[wi].weight = weight;
      fontlist[fi].weights[wi].style = style;
    }
  }
  else {
    if (fontlist) {
      fontlistn++;
      fontlist = renewn(fontlist, fontlistn);
      for (uint j = fontlistn - 1; j > fi; j--)
        fontlist[j] = fontlist[j - 1];
    }
    else
      fontlist = newn(struct fontlist, 1);

    fontlist[fi].fn = fn;

    fontlist[fi].weightsn = 1;
    fontlist[fi].weights = newn(struct weight, 1);
    fontlist[fi].weights[0].weight = weight;
    fontlist[fi].weights[0].style = style;
  }
}



struct data_fontenum {
  HDC dc;
  bool report;
  bool outer;
};

static int CALLBACK
fontenum(const ENUMLOGFONTW *lpelf, const NEWTEXTMETRICW *lpntm, DWORD fontType, LPARAM lParam)
{
  const LOGFONTW * lfp = &lpelf->elfLogFont;
  struct data_fontenum * pdata = (struct data_fontenum *)lParam;
  (void)lpntm, (void)fontType;

  if (pdata->outer) {
    // here we recurse into the fonts of one font family
    struct data_fontenum rdata = {
      .dc = pdata->dc, .report = pdata->report, .outer = false
    };
    if ((lfp->lfPitchAndFamily & 3) == FIXED_PITCH && !lfp->lfCharSet)
      EnumFontFamiliesW(pdata->dc, lfp->lfFaceName, (FONTENUMPROCW)fontenum, (LPARAM)&rdata);
  }
  else if (!lfp->lfItalic && !lfp->lfCharSet) {
    if (lfp->lfFaceName[0] == '@')
      // skip vertical font families
      return 1;

    const wchar * tagsplit(const wchar * fn, wstring style)
    {
#if CYGWIN_VERSION_API_MINOR >= 74
      wchar * tag = wcsstr(fn, style);
      if (tag) {
        int n = wcslen(style);
        if (tag[n] <= ' ' && tag != fn && tag[-1] == ' ') {
          tag[-1] = 0;
          tag[n] = 0;
          return tag;
        }
      }
#else
      (void)fn; (void)style;
#endif
      return 0;
    }

    /**
       Courier|
       FreeMono|Medium
       Inconsolata|Medium
       Source Code Pro ExtraLight|ExtraLight
       @BatangChe|Regular
       Iosevka Term Slab Medium Obliqu|Regular
       Lucida Sans Typewriter|Bold
       TIFAX|Alpha
       HanaMinA|Regular
       DejaVu Sans Mono|Book
       */
    wchar * fn = wcsdup(lfp->lfFaceName);
    const wchar * st = tagsplit(fn, W("Oblique"));
    if ((st = tagsplit(fn, lpelf->elfStyle))) {
      //   Source Code Pro ExtraLight|ExtraLight
      //-> Source Code Pro|ExtraLight
    }
    else {
      const wchar * fnst = fn;
#if CYGWIN_VERSION_API_MINOR >= 74
      int digsi = wcscspn(fn, W("0123456789"));
      int nodigsi = wcsspn(&fn[digsi], W("0123456789"));
      if (nodigsi)
        fnst = &fn[digsi + nodigsi];
#endif
      for (uint i = 0; i < lengthof(weights); i++)
        if ((st = tagsplit(fnst, weights[i]))) {
          //   Iosevka Term Slab Medium Obliqu|Regular
          //-> Iosevka Term Slab|Medium
          break;
        }
    }
    if (!st || !*st)
      st = lpelf->elfStyle;
    if (!*st)
      st = W("Regular");
    st = wcsdup(st);
    fn = renewn(fn, wcslen(fn) + 1);

    if (pdata->report)
      printf("%03ld %ls|%ls [2m[%ls|%ls][0m\n", (long int)lfp->lfWeight, fn, st, lfp->lfFaceName, lpelf->elfStyle);
    else
      enterfontlist(fn, lfp->lfWeight, st);
  }

  return 1;
}

void
list_fonts(bool report)
{
  struct data_fontenum data = {
    .dc = GetDC(0),
    .report = report,
    .outer = true
  };

  EnumFontFamiliesW(data.dc, 0, (FONTENUMPROCW)fontenum, (LPARAM)&data);
  ReleaseDC(0, data.dc);
}




static void
opt_handlern(control *ctrl, int event)
{
  cfg_option *co=(cfg_option *)ctrl->context;
  int *popt=(int*)(((char*)&new_cfg)+co->offset);
  const opt_val *ovals=lopt_vals[co->type];
  switch (event) {
    when EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      while (ovals->name) {
        dlg_listbox_add(ctrl, _(ovals->name));
        if (*popt == ovals->val)
          dlg_editbox_set(ctrl, _(ovals->name));
        ovals++;
      }
    when EVENT_VALCHANGE or EVENT_SELCHANGE: {
      int i = 0;
      while (ovals->name) {
        if (dlg_listbox_getcur(ctrl) == i++)
          *popt = ovals->val;
        ovals++;
      }
    }
  }
}
static void
ansicolour_handler(control *ctrl, int event)
{
  colour c = *(colour *)ctrl->context;
  dlg_stdcolour_handler(ctrl, event);
  if (event == EVENT_CALLBACK) {
    colour cn = *(colour *)ctrl->context;
    if(c!=cn){
      InvalidateRect(ctrl->widget,NULL,1);
      enable_widget(store_button,themechanged());
      //InvalidateRect(GetParent(ctrl->widget),NULL,1);
    }
  }
}

void
transparency_valhandler(control *ctrl, int event)
{
  if (event == EVENT_VALCHANGE) {
    string val = 0;
    dlg_editbox_get(ctrl, &val);
    new_cfg.transparency = atoi(val);
    delete(val);
  }
  else if (event == EVENT_UNFOCUS) {
    string val = 0;
    dlg_editbox_get(ctrl, &val);
    int transp = atoi(val);
    delete(val);
    // adjust value if out of range
    if (transp < 4)
      transp = 0;
    else if (transp > 254)
      transp = 254;
    new_cfg.transparency = transp;
    // refresh box in case we changed it
    char buf[16];
    sprintf(buf, "%i", transp);
    dlg_editbox_set(ctrl, buf);
    // update radio buttons
    if(transparency_selbox)
      dlg_stdradiobutton_handler(transparency_selbox, EVENT_REFRESH);
  }
  else if (event == EVENT_REFRESH) {
    char buf[16];
    sprintf(buf, "%i", (uchar)new_cfg.transparency);
    dlg_editbox_set(ctrl, buf);
  }
}

void
transparency_selhandler(control *ctrl, int event)
{
  dlg_stdradiobutton_handler(ctrl, event);
  if (event == EVENT_VALCHANGE) {
    if(transparency_valbox)
      transparency_valhandler(transparency_valbox, EVENT_REFRESH);
  }
}
static char *backgtag="_%*+=";
  //_:1 Scale image to term size
  //%:2 Scale term to image ration
  //*:3 use Image as titled texture
  //+:4 Scale image to term size,keep ratio,then titles
  //=:5 Use desktop background
int backg_type(int c){
  if(!c||c==' ')return 0;
  char *r=strchr(backgtag,c);
  if(!r)return '_';
  return c;
}
void backg_analyse(string pbf,bg_file *backgfile){
  backgfile->type=backg_type(*pbf);
  backgfile->update=1;
  if(backgfile->type==0)return;
  if(backgfile->type)pbf++;
  if(backgfile->fn)free((void*)backgfile->fn);
  char*p=strrchr(pbf,',');
  if(p){
    *p=0; p++;
    if (sscanf(p, "%u%c", &backgfile->alpha, &(char){0}) != 1)
      backgfile->alpha = 255;
  }else{
    backgfile->alpha = 255;
  }
  backgfile->fn= cs__mbstowcs(pbf);
}
void
setup_config_box(controlbox * b)
{
  controlset *s;
  control *c;
  copy_config("dialog", &new_cfg, &file_cfg);
  if(*theme_file)CLRFGSCPY(new_cfg.ansi_colours,theme_colours );
  /*
   * The standard panel that appears at the bottom of all panels:
   * Open, Cancel, Apply etc.
   */
  s = ctrl_new_set(b, W(""), W(""), W(""));
  ctrl_columns(s, 5, 20, 20, 20, 20, 20);
  c = ctrl_pushbutton(s,0, _W("About..."),0, about_handler, 0);
  c = ctrl_pushbutton(s,2, _W("&Save"),0, ok_handler, 0);
  c->button.isdefault = true;
  c = ctrl_pushbutton(s,3, _W("&Cancel"),0, cancel_handler, 0);
  c->button.iscancel = true;
  c = ctrl_pushbutton(s,4, _W("&Apply"),0, apply_handler, 0);
  char*p=(char*)&new_cfg;
  int ic=-1,nc=0;
  int cols[MAXCOLS];
  cfg_option *co=options;
  for (; (co->name); co++) {
    char*pv=p+co->offset;
    int nnc=co->size*10;
    if(co->flag == OPF_END)break;
    if(co->level<cfg.optlevel)continue;
    if(nc){
      if(co->column){
        ic++; if(ic>=nc){ic=0;}
      }else{
        ic=-1;
      }
    }
#define WCMT _W(co->label)
    switch(co->flag){
      when OPF_H:;
      when OPF_PANE: ctrl_new_set(b,_W(co->name),WCMT,null);nc=0;
      when OPF_GRP: s = ctrl_new_set(b,_W(co->name),null,WCMT);nc=0;
      when OPF_COL: {
        int j,mc=0,sc=0;
        cfg_option *cp=co+1;
        nc=co->column;
        int cw=0;
        for(;cp->name&&mc<nc;cp++){
          if(cp->flag==OPF_H)continue;
          if(cp->column&&cp->flag!=OPF_COL){
            sc+=(cols[mc]=cw=cp->column);mc++;
          }else{
            for(;mc<nc;mc++){
              sc+=(cols[mc]=cw);
            }
          }
        }
        if(!sc)sc=1;
        for(j=0;j<mc;j++)
          cols[j]=(cols[j]*100)/sc;
        ctrl_columnsa(s,nc,cols);
        ic=-1;
      }
      when OPF_LSTR:{
        ctrl_radiobuttonsa(s,ic,WCMT,0,dlg_stdradiobutton_handler,(int*)pv,opt_vals[co->type&OPT_TYPE_MASK]);
      }
      when OPF_LSTC :{
        ctrl_combobox(s,ic,WCMT,0,nnc,opt_handlern,(void*)co);
      }
      when OPF_CHK:{
        ctrl_checkbox(s,ic,WCMT,0,dlg_stdcheckbox_handler,pv);
      }
      when OPF_ACLR:{
        colourfg *c=(colourfg *)(pv);
        ctrl_columns(s,9,15,10,10,10,10,10,10,10,10);
        ctrl_label    (s,0,_W("Ansi:"),0);
        int j;
        char *sn[]={
          "Black"       , "Red"         , "Green"       , "Yellow"      ,
          "Blue"        , "Magenta"     , "Cyan"        , "White"       ,
          "BoldBlack"   , "BoldRed"     , "BoldGreen"   , "BoldYellow"  ,
          "BoldBlue"    , "BoldMagenta" , "BoldCyan"    , "BoldWhite"   ,
        };
        for(j=0;j<8;j++)
          ctrl_clrbutton(s,j+1,0,_W(sn[j]),ansicolour_handler,&c[ j]);
        ctrl_columns(s,9,15,10,10,10,10,10,10,10,10);
        ctrl_label    (s,0,W(""),0);
        for(j=0;j<8;j++)
          ctrl_clrbutton(s,j+1,_W(sn[8+j]),_W(sn[8+j]),ansicolour_handler,&c[8+j]);
      }
      when OPF_CLR:{
        ctrl_pushbutton(s,ic,WCMT,0,dlg_stdcolour_handler,pv);
      }
      when OPF_CLRFG:{
        ctrl_pushbutton(s,ic,WCMT,0,dlg_stdcolour_handler,pv);
      }
      when OPF_INT  :{
        ctrl_inteditbox(s,ic,WCMT,0,nnc,dlg_stdintbox_handler,pv,co->i.v0,co->i.v1);
      }
      when OPF_WSIZE:{
        ctrl_pushbutton( s,ic, _W("C&urrent size"),0, current_size_handler, pv);
      }
      when OPF_STR  :{
        ctrl_editbox(s,ic,WCMT,0,nnc,dlg_stdstringbox_handler,pv);
      }
      when OPF_WSTR  :{
        ctrl_editbox(s,ic,WCMT,0,nnc,dlg_stdwstringbox_handler,pv);
      }
      when OPF_TRANS:{
        transparency_selbox = ctrl_radiobuttonsa(s,ic,WCMT,0,transparency_selhandler,(int*)pv,opt_vals[co->type&OPT_TYPE_MASK]);
      }
      when OPF_FONT :{
        ctrl_fontsel(s,-1,WCMT,0,dlg_stdfontsel_handler,pv);
      }
      when OPF_BACKF:{
        bg_file*bf=(bg_file*)pv;
        ctrl_filesel(s,-1,WCMT,0,backgfsel_handler,&bf->fn);
        ctrl_radiobuttons( s,-1, _W(""),0, 2,
                           backgfsel_handler, &bf->type,
                           _W("none"), 0,// ˜
                           _W("Scale image to term size"), '_',//_
                           _W("Scale term to image ration"), '%',//%
                           _W("use Image as titled texture"), '*',//*
                           _W("Scale image to term size,keep ratio,then titles"), '+',//+
                           _W("Use desktop background"), '=',//=
                           null
                         );
        ctrl_inteditbox(s,-1,_W("alpha"),0,80,backgfsel_handler,&bf->alpha,0,255);
      }
      when OPF_THEME:{
        ctrl_columns(s, 2, 70,30);
        ctrl_combobox(s,0,WCMT,0,70,theme_handler,pv);
        store_button = ctrl_pushbutton(s,1,_W("T&heme Save"),0,theme_saver,0) ;
      }
      when OPF_LCL  :{
        locale_box = ctrl_combobox(s,ic,WCMT,0,nnc,locale_handler,pv);
      }
      when OPF_CHSET:{
        charset_box = ctrl_combobox(s,ic,WCMT,0,nnc,charset_handler,pv);
      }
      when OPF_CURS:{
        ctrl_combobox(s,ic,WCMT,0,nnc,cursor_handler,pv);
      }
      when OPF_TERM :{
        ctrl_combobox(s,ic,WCMT,0,nnc,term_handler,pv);
      }
      when OPF_BELL :{
        ctrl_columns(s, 2, 73, 27);
        ctrl_combobox( s,0, null,0, 100, bell_handler, pv);
        ctrl_pushbutton( s,1, _W("► &Play"),0, bell_tester, pv);
      }
      when OPF_BELLF:{
        ctrl_combobox(s,ic,WCMT,0,nnc,bellfile_handler,pv);
      }
      when OPF_PRINT:{
        ctrl_combobox( s,ic, WCMT,0, nnc, printer_handler, pv);
      }
    }
  }
}
