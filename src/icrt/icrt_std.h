// ======================================================================== //
// author:  ixty                                                       2018 //
// project: inline c runtime library                                        //
// licence: beerware                                                        //
// ======================================================================== //

// standard utilities (mem*, str*, ...)
#ifndef _ICRT_STD_H
#define _ICRT_STD_H

#include <stddef.h>

void std_memset(const void *dst, unsigned char c, unsigned int len);

int std_memcmp(const void *dst, const void *src, unsigned int len);

void std_memcpy(const void *dst, const void *src, unsigned int len);

void *std_memmem(const void *haystack, size_t n, const void *needle, size_t m);

int std_strlen(const char *str);

int std_strnlen(const char *str, int maxlen);

int std_strncmp(const char *s1, const char *s2, size_t l);

int std_strcmp(const char *s1, const char *s2);

size_t std_strlcat(char *dst, char *src, size_t maxlen);

#ifndef ULONG_MAX
#define ULONG_MAX ((unsigned long)(~0L))
#endif
#ifndef LONG_MAX
#define LONG_MAX ((long)(ULONG_MAX >> 1))
#endif
#ifndef LONG_MIN
#define LONG_MIN ((long)(~LONG_MAX))
#endif

#define ISSPACE(c) (c == ' ' || c == '\r' || c == '\t' || c == '\n')
#define ISDIGIT(c) (c >= '0' && c <= '9')
#define ISALPHA(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define ISUPPER(c) (c >= 'A' && c <= 'Z')

unsigned long std_strtoul(const char *nptr, char **endptr, int base);

int fmt_num(char *buf, size_t len, unsigned long val, int base);

// ugly AF but i dont want to include snprintf which has tons of globals / code
// / etc. only supports %x %d %s some modifiers are accepted but arent taken
// into account
void std_printf(const char *str, ...);

#endif
