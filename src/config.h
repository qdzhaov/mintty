#ifndef CONFIG_H
#define CONFIG_H

#define support_blurred

// Enums for various options.

typedef enum {  MDK_SHIFT   = 1, MDK_ALT  = 2, MDK_CTRL = 4,  
                MDK_WIN     = 8, MDK_SUPER=16, MDK_HYPER=32, 
                MDK_CAPSLOCK=64, MDK_EXT  =128 
} mod_keys;
typedef enum { SMDK_SHIFT   = 0,SMDK_ALT  = 1,SMDK_CTRL = 2, 
               SMDK_WIN     = 3,SMDK_SUPER= 4,SMDK_HYPER= 5,
               SMDK_CAPSLOCK= 6,SMDK_EXT  = 7 
} smod_keys;

enum { HOLD_NEVER, HOLD_START, HOLD_ERROR, HOLD_ALWAYS };
enum { CUR_BLOCK, CUR_UNDERSCORE, CUR_LINE, CUR_BOX };
enum { FS_DEFAULT, FS_PARTIAL, FS_NONE, FS_FULL };
enum { FR_TEXTOUT, FR_UNISCRIBE };
enum { MC_VOID, MC_PASTE, MC_EXTEND, MC_ENTER };
enum { RC_MENU, RC_PASTE, RC_EXTEND, RC_ENTER };
enum { BORDER_NORMAL, BORDER_FRAME, BORDER_VOID };
enum { TR_OFF = 0, TR_LOW = 16, TR_MEDIUM = 32, TR_HIGH = 48, TR_GLASS = -1 };
enum { FLASH_FRAME = 1, FLASH_BORDER = 2, FLASH_FULL = 4, FLASH_REVERSE = 8 };
enum { EMOJIS_NONE = 0, EMOJIS_ONE = 1, EMOJIS_NOTO = 2, EMOJIS_APPLE = 3, 
       EMOJIS_GOOGLE = 4, EMOJIS_TWITTER = 5, EMOJIS_FB = 6, 
       EMOJIS_SAMSUNG = 7, EMOJIS_WINDOWS = 8, EMOJIS_JOYPIXELS = 9, 
       EMOJIS_OPENMOJI = 10, EMOJIS_ZOOM = 11 };
enum { EMPL_STRETCH = 0, EMPL_ALIGN = 1, EMPL_MIDDLE = 2, EMPL_FULL = 3 };

// Colour values.

typedef uint colour;
typedef struct { colour fg, bg; } colourfg;
typedef struct { int x, y; } intpair;

enum { DEFAULT_COLOUR = UINT_MAX };

static inline colour make_colour(uchar r, uchar g, uchar b) { return r | g << 8 | b << 16; }
static inline uchar red(colour c) { return c; }
static inline uchar green(colour c) { return c >> 8; }
static inline uchar blue(colour c) { return c >> 16; }
extern bool parse_colour(string, colour *);

// Font properties.
typedef struct {
  wstring name;
  int size;
  int weight;
  bool isbold;
} font_spec;
typedef struct {
  wstring fn;
  int type;
  int alpha;
  int update;
}bg_file;
#define CTYPE int
#define CBOOL CTYPE

// Configuration data.
#define CFGDEFT  DEFVAR
typedef struct {
#include "cdef.t"
} config;
#ifdef CFGDEFT  
#undef CFGDEFT  
#endif
struct HKDef{
    int mode;
    unsigned char flg,key;
};
struct function_def {
  int level;
  string name;
  int type;
  union {
    WPARAM cmd;
    void *fv;
    void (*fct)(void);
    int  (*fctv)(void);
    void (*fct_key)(uint key, mod_keys mods);
    int  (*fct_keyv)(uint key, mod_keys mods);
    void (*fct_par1)(int p0);
    void (*fct_par2)(int p0,int p1);
  };
  uint (*fct_status)(void);
  char*tip;
  //int p0,p1;
  struct HKDef kd[4];
  struct HKDef kr[4];
  /*type :
   * 1: cmd;
   * 2: fct();
   * 3: fct_key(key,mods)
   * 4: fct_par(p0,)
   * 5: fct_par(p0,p1)
   */
};
extern struct function_def cmd_defs[] ;
typedef void (* str_fn)(char *);

extern string config_dir;
extern config cfg, new_cfg, file_cfg;

extern void init_config(void);
extern void list_fonts(bool report);
extern void load_config(string filename, int to_save);
extern void load_theme(string theme);
extern char * get_resource_file(string sub, string res, bool towrite);
extern void handle_file_resources(string pattern, str_fn fnh);
extern void load_scheme(string colour_scheme);
extern void set_arg_option(string name, string val);
extern void parse_arg_option(string);
extern void remember_arg(string);
extern void finish_config(void);
extern void copy_config(const char * tag, config * dst, const config * src);
extern void apply_config(bool save);
extern wchar * getregstr(HKEY key, wstring subkey, wstring attribute);
extern uint getregval(HKEY key, wstring subkey, wstring attribute);
extern char * save_filename(const char * suf);
// In a configuration parameter list, map tag to value
extern char * matchconf(char * conf,const char * item);
extern int backg_type(int c);
extern void backg_analyse(string pbf,bg_file *backgfile);

#endif
