#ifndef STD_H
#define STD_H
#define UNICODE
#ifdef __CYGWIN__
#include <cygwin/version.h>
#else
#define CYGWIN_VERSION_DLL_MAJOR 1007
#define CYGWIN_VERSION_API_MINOR 201
#endif

//unhide some definitions
#define _GNU_SOURCE

#include <sys/cygwin.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <wchar.h>
#include <errno.h>

//#define WINVER 0x0500	// Windows 2000
#define WINVER 0x0501	// Windows XP
//#define WINVER 0x0601	// Windows 7
//#define WINVER 0x0A00	// Windows 10

#ifdef __CYGWIN__
#define _WIN32_WINNT WINVER
#endif
#define _WIN32_IE WINVER

#include <windef.h>


#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <winbase.h>
#include <windows.h>
#include <windowsx.h>

#define always_inline __attribute__((always_inline)) inline
#define unused(arg) unused_##arg __attribute__((unused))
#define no_return __attribute__((noreturn)) void

#if CYGWIN_VERSION_API_MINOR >= 74
#include <wctype.h>
#else
extern int iswalnum(wint_t);
extern int iswalpha(wint_t);
extern int iswspace(wint_t);
#endif

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef void (*void_fn)(void);

typedef uint xchar;     // UTF-32
typedef WCHAR wchar;    // UTF-16

typedef const char *string;
typedef const wchar *wstring;


#if CYGWIN_VERSION_API_MINOR >= 91
#include <argz.h>
#else
extern int argz_create (char *const argv[], char **argz, size_t *argz_len);
extern void argz_stringify (char *argz, size_t argz_len, int sep);
#endif

static inline void *
_realloc(void *aptr, size_t nbytes)
{
  if (aptr)
    return realloc(aptr, nbytes);
  else
    return malloc(nbytes);
}

#define new(type) ((type *) malloc(sizeof(type)))
#define newn(type, n) ((type *) calloc((n), sizeof(type)))
#define renewn(p, n) ((typeof(p)) _realloc((p), sizeof(*p) * (n)))
#define renewm(p, n,m) if(n>=m){m+=16;if(n>=m)m=n+16;p=renewn(p,m);}
static inline void delete(const void *p) { if(p)free((void *)p); }


extern char * tmpdir(void);



#if CYGWIN_VERSION_API_MINOR < 53
#define strlcpy(dst, src, len) snprintf(dst, len, "%s", src)
#endif


#if CYGWIN_VERSION_API_MINOR < 70
extern int asprintf(char **, const char *, ...);
extern int vasprintf(char **, const char *, va_list);
#endif

extern char *asform(const char *fmt, ...);



#ifdef DMALLOC
#include <dmalloc.h>
#endif


typedef struct _strings{
  int n,m;
  const char **s;
}strings;
void strsadd(strings*a,string s);
void strscpy(strings*a,strings*b);
void strsclr(strings*b);
#define null ((void *) 0)

#define strappend(s0, s1) s0 = _strappend(s0, s1)

static inline char *
_strappend(char * s0, const char * s1)
{
  s0 = renewn(s0, strlen(s0) + strlen(s1) + 1);
  strcat(s0, s1);
  return s0;
}


// UTF-16 literals:
#if __GNUC__ >= 5
#define __W(s) u##s
#else
#define __W(s) L##s
#endif
#define W(s) __W(s)

// localized string/wstring lookup:
extern const char * loctext(string msg);
extern const wchar * wloctext(string msg);
extern const wchar * wtext(string msg);
#define _(s) loctext(s)
#define _W(s) wloctext(s)
#define _w(s) wtext(s)
// dummy marker for xgettext:
#define __(s) s


#define lengthof(array) (sizeof(array) / sizeof(*(array)))
#define endof(array) (&(array)[lengthof(array)])


#ifdef __CYGWIN__
extern void strset(string *sp, string s);
#endif
extern void stradd(string *sp, string s);
extern void wstrset(wstring *sp, wstring s);
extern void wstradd(wstring *sp, wstring s);
extern int  vwasprintf(wchar **buf, const wchar *fmt, va_list va);
extern wchar*awsform(const wchar *fmt, ...);


#define when break; case
#define or : case
#define otherwise break; default

#ifdef TRACE
#define trace(xs...) \
    printf("%s:%u:%s:", __FILE__, __LINE__, __func__); \
    printf(" " xs); \
    putchar('\n')
#else
#define trace(f, xs...) {}
#endif


#define sgn(x) ({ typeof(x) x_ = (x); (x_ > 0) - (x_ < 0); })
#define sqr(x) ({ typeof(x) x_ = (x); x_ * x_; })

#define VFREE(p) delete(p)

#include "winpriv.h"
#include "charset.h"
#include "child.h"
#endif
