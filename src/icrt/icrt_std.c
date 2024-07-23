// ======================================================================== //
// author:  ixty                                                       2018 //
// project: inline c runtime library                                        //
// licence: beerware                                                        //
// ======================================================================== //

// standard utilities (mem*, str*, ...)

#include <stdarg.h>
#include <stdint.h>

#include "icrt_std.h"
#include "icrt_syscall.h"

void std_memset(const void *dst, unsigned char c, unsigned int len) {
    unsigned char *p = (unsigned char *)dst;

    while (len--)
        *p++ = c;
}

int std_memcmp(const void *dst, const void *src, unsigned int len) {
    unsigned char *d = (unsigned char *)dst;
    unsigned char *s = (unsigned char *)src;

    while (len-- > 0)
        if (*d++ != *s++)
            return 1;

    return 0;
}

void std_memcpy(const void *dst, const void *src, unsigned int len) {
    unsigned char *d = (unsigned char *)dst;
    unsigned char *s = (unsigned char *)src;

    while (len--)
        *d++ = *s++;
}

void *std_memmem(const void *haystack, size_t n, const void *needle, size_t m) {
    for (int i = 0; i < n - m; i++)
        if (!std_memcmp((uint8_t *)haystack + i, needle, m))
            return (uint8_t *)haystack + i;
    return NULL;
}

int std_strlen(const char *str) {
    int n = 0;
    while (*str++)
        n++;
    return n;
}

int std_strnlen(const char *str, int maxlen) {
    int l = std_strlen(str);
    return l > maxlen ? maxlen : l;
}

int std_strncmp(const char *s1, const char *s2, size_t l) {
    for (size_t i = 0; i < l && *s1 && *s2; i++)
        if (s1[i] != s2[i])
            return -1;
    return 0;
}

int std_strcmp(const char *s1, const char *s2) {
    int l1 = std_strlen(s1);
    int l2 = std_strlen(s2);

    if (l1 == l2 && !std_strncmp(s1, s2, l1))
        return 0;
    return -1;
}

size_t std_strlcat(char *dst, char *src, size_t maxlen) {
    size_t srclen = std_strlen(src);
    size_t dstlen = std_strnlen(dst, maxlen);
    if (dstlen == maxlen)
        return maxlen + srclen;
    if (srclen < maxlen - dstlen)
        std_memcpy(dst + dstlen, src, srclen + 1);
    else {
        std_memcpy(dst + dstlen, src, maxlen - 1);
        dst[dstlen + maxlen - 1] = 0;
    }
    return dstlen + srclen;
}

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

unsigned long std_strtoul(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    unsigned long acc;
    unsigned long cutoff;
    int neg = 0, any, cutlim, c;

    do {
        c = *s++;
    } while (ISSPACE(c));

    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+')
        c = *s++;
    if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;
    cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
    cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
    for (acc = 0, any = 0;; c = *s++) {
        if (ISDIGIT(c))
            c -= '0';
        else if (ISALPHA(c))
            c -= ISUPPER(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0)
        acc = ULONG_MAX;
    else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *)(any ? s - 1 : nptr);
    return (acc);
}

int fmt_num(char *buf, size_t len, unsigned long val, int base) {
    unsigned long v = val;
    size_t n = 0;

    std_memset(buf, 0, len);

    // validate base first
    if (base != 0 && base != 8 && base != 10 && base != 16)
        return -1;
    if (base == 0)
        base = 10;

    // count required number of chars
    while (v) {
        v /= base;
        n += 1;
    }
    if (!n)
        n = 1;

    // check that we have enought room for string + null byte
    if (len < n + 1)
        return -1;

    for (int i = n - 1; i >= 0; i--) {
        if (base != 16 || val % 16 < 10)
            buf[i] = '0' + (val % base);
        else
            buf[i] = 'a' + ((val % 16) - 10);

        val /= base;
    }
    return 0;
}

#if (DEBUG)

// ugly AF but i dont want to include snprintf which has tons of globals / code
// / etc. only supports %x %d %s some modifiers are accepted but arent taken
// into account
void std_printf(const char *str, ...) {
    char tmp[32];
    const char *s = str;
    const char *p = str;
    va_list args;
    va_start(args, str);

    while (1) {
        // end of string reached, output everything left to display
        if (!*p) {
            if (p - s > 0)
                sys_write(1, s, p - s);
            goto exit;
        }

        // format string?
        if (*p == '%' && *(p + 1) != '%') {
            // output current string
            sys_write(1, s, p - s);

            // handle (and ignore :)) format string modifiers
            p += 1;
            while (ISDIGIT(*p) || *p == '.')
                p++;
            while (*p == 'l' || *p == 'h' || *p == 'b')
                p++;

            // we ignore sign aswell, unsigned only :)
            if (*p == 'i' || *p == 'I' || *p == 'd' || *p == 'D') {
                fmt_num(tmp, sizeof(tmp), va_arg(args, unsigned long), 10);
                sys_write(1, tmp, std_strlen(tmp));
            } else if (*p == 'x' || *p == 'X') {
                fmt_num(tmp, sizeof(tmp), va_arg(args, unsigned long), 16);
                sys_write(1, tmp, std_strlen(tmp));
            } else if (*p == 's') {
                char *fstr = va_arg(args, char *);
                sys_write(1, fstr, std_strlen(fstr));
            } else
                sys_write(1, "<?>", 3);

            p++;
            s = p;
        }
        // nope, skip
        else if (*p == '%')
            p++;

        p++;
    }

exit:
    va_end(args);
    return;
}

#else

void std_printf(const char *str, ...) { return; }

#endif
