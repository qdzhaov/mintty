// base64.c (part of mintty)
// Copyright 2016 Jianbin Kang
// Licensed under the terms of the GNU General Public License v3 or later.

#include <errno.h>
#include "base64.h"
#if CYGWIN_VERSION_API_MINOR >= 74
#include <stdint.h>
#else
#define uint32_t uint
#endif
#ifdef BASE64_TEST
#include "std.h"
#include <stdlib.h>
#endif

#define INVALID_CHAR	(-1)
static const char base64_table[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/',
};
static char base64_tabler[256]={
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, // 0
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, //10
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63, //20
  52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1, //30
  -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14, //40
  15,16,17,18, 19,20, 21,22,23,24,25,-1, -1,-1,-1,-1, //50
  -1,26,27,28, 29,30, 31,32,33,34,35,36, 37,38,39,40, //60
  41,42,43,44, 45,46, 47,48,49,50,51,-1, -1,-1,-1,-1, //70
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, //80
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, //90
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, //a0
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, //b0
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, //c0
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, //d0
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, //e0
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, //b0
};


static inline char encode(uint32_t v)
{
  return base64_table[v];
}
static inline int decode(char v)
{
  return base64_tabler[(unsigned char)v];
}

char * base64(char * s)
{
  int ilen = strlen(s);
  int olen = (ilen + 2) / 3 * 4;
  char * out = malloc(olen + 1);
  int out_len = base64_encode((unsigned char *)s, ilen, out, olen);
  if (out_len < 0)
    return 0;
  out[out_len] = '\0';
#ifdef BASE64_TEST
  printf("%d %d %s %s\n", olen, out_len, s, out);
#endif
  return out;
}

int base64_encode(const unsigned char * input, int ilen, char * output, int olen)
{
  int calc_len = (ilen + 2) / 3 * 4;
  int i = 0;

  if (olen < calc_len) {
    return B64_OVERFLOW;
  }
  while (ilen >= 3) {
    uint32_t v = (((uint32_t)input[0]) << 16) +
      (((uint32_t)input[1]) << 8) + input[2];
    output[i] = encode(v >> 18);
    output[i + 1] = encode((v >> 12) & 0x3f);
    output[i + 2] = encode((v >> 6) & 0x3f);
    output[i + 3] = encode(v & 0x3f);
    i += 4;
    input += 3;
    ilen -= 3;
  }
  if (ilen > 0) {
    uint32_t v = ((uint32_t)input[0]) << 16;
    if (ilen == 2) {
      v += ((uint32_t)input[1]) << 8;
    }
    output[i] = encode(v >> 18);
    output[i + 1] = encode((v >> 12) & 0x3f);
    if (ilen == 1) {
      output[i + 2] = '=';
    } else {
      output[i + 2] = encode((v >> 6) & 0x3f);
    }
    output[i + 3] = '=';
    i += 4;
  }
  return i;
}

static int decode_chars(const char *input, int num)
{
  int i;
  int dec_v = 0;
  int step = 18;

  for (i = 0; i < num; i += 1, step -= 6) {
    int v = decode(input[i]);
    if (v == INVALID_CHAR) {
      return B64_INVALID_CHAR;
    }
    dec_v += v << step;
  }
  return dec_v;
}

static int do_decode(const char *input, int ilen, char *out)
{
  int i = 0;
  int dec_v;

  while (ilen >= 4) {
    dec_v = decode_chars(input, 4);
    if (dec_v < 0) {
      return dec_v;
    }
    out[i] = dec_v >> 16;
    out[i + 1] = (dec_v >> 8) & 0xff;
    out[i + 2] = dec_v & 0xff;
    i += 3;
    ilen -= 4;
    input += 4;
  }
  if (ilen >= 2) {
    dec_v = decode_chars(input, ilen);
    out[i] = dec_v >> 16;
    i += 1;
    if (ilen == 3) {
      out[i] = dec_v >> 8;
      i += 1;
    }
  } else if (ilen == 1) {
    /* It is a bug */
    return B64_INTERNAL_ERROR;
  }
  return i;
}

int base64_decode_clip(const char *input, int ilen, char *out, int olen)
{
  if ((ilen % 4) != 0) {
    ilen = ilen / 4 * 4;
  }
  return base64_decode(input, ilen, out, olen);
}

int base64_decode(const char *input, int ilen, char *out, int olen)
{
  int dec_len;
  int encode_len;
  int out_len;

  if (ilen == 0) {
    return 0;
  }
  if ((ilen % 4) != 0) {
    return B64_INVALID_LEN;
  }
  dec_len = ilen / 4 * 3;
  encode_len = ilen;
  if (input[ilen - 1] == '=') {
    dec_len -= 1;
    encode_len -= 1;
  }
  if (input[ilen - 2] == '=') {
    dec_len -= 1;
    encode_len -= 1;
  }
  if (olen < dec_len) {
    return B64_INVALID_LEN;
  }
  out_len = do_decode(input, encode_len, out);
  if (out_len != dec_len) {
    return B64_INTERNAL_INVALID_LEN;
  }
  return out_len;
}


#ifdef BASE64_TEST

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "base64.h"

struct base64_test {
  char *orig;
  char *encode;
};

static struct base64_test test_sets[] = {
  { "", "", },
  { "A", "QQ==", },
  { "AA", "QUE=", },
  { "AAA", "QUFB", },
  { "AB", "QUI=", },
  { "ABC", "QUJD", },
  { "ABCD", "QUJDRA==" },
  { "ABCDE", "QUJDREU=" },
  { "ABCDEF", "QUJDREVG" },
  { "ABCDEFG", "QUJDREVGRw==" },
  { "ABCDEFGH", "QUJDREVGR0g=" },
  { "ABCDEFGHI", "QUJDREVGR0hJ" },
  { "bö", "YsO2" },
  { "e€", "ZeKCrA==" },
  { "oeœ", "b2XFkw==" },
  { "", "a,b" }
};

#define ARRAY_SIZE(a)		(sizeof(a) / sizeof(a[0]))

#warning compiling test code
#define error(fmt, ...)	fprintf(stderr, fmt, ##__VA_ARGS__); exit(1)

static void encode_string(const char *s, char *out, int len)
{
  int out_len;

  out_len = base64_encode(s, strlen(s), out, len - 1);
  if (out_len < 0) {
    error("Encode %s with len %d return %d\n", s, len, out_len);
  }
  if (out_len > len - 1) {
    error("Encode %s with len %d return invalid len %d\n", s, len, out_len);
  }
  out[out_len] = '\0';
}

static void decode_string(const char *s, char *out, int len)
{
  int out_len;

  out_len = base64_decode(s, strlen(s), out, len - 1);
  if (out_len < 0) {
    error("Decode %s with len %d return %d\n", s, len, out_len);
  }
  if (out_len > len - 1) {
    error("Decode %s with len %d return invalid len %d\n", s, len, out_len);
  }
  out[out_len] = '\0';
}

static void test_encode(void)
{
  char buf[1024];
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(test_sets) - 1; i += 1) {
    encode_string(test_sets[i].orig, &buf[0], sizeof(buf));
    if (strcmp(buf, test_sets[i].encode) != 0) {
      error("Encode %s return %s, expect %s\n",
            test_sets[i].orig, buf, test_sets[i].encode);
    }
    base64(test_sets[i].orig);
  }
  printf("Encode PASSED\n");
}

static void test_decode(void)
{
  char buf[1024];
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(test_sets); i += 1) {
    decode_string(test_sets[i].encode, &buf[0], sizeof(buf));
    if (strcmp(buf, test_sets[i].orig) != 0) {
      error("Decode %s return %s, expect %s\n",
            test_sets[i].encode, buf, test_sets[i].orig);
    }
  }
  printf("Decode PASSED\n");
}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  test_encode();
  test_decode();

  return 0;
}

#endif
