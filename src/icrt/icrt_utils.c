// ======================================================================== //
// author:  ixty                                                       2018 //
// project: inline c runtime library                                        //
// licence: beerware                                                        //
// ======================================================================== //

// utilities (read_file, read /proc/pid/maps, ...)

#include <stddef.h>
#include <stdint.h>

#include "icrt_asm.h"
#include "icrt_mem.h"
#include "icrt_std.h"
#include "icrt_syscall.h"
#include "icrt_utils.h"

// get file size, supports fds with no seek_end (such as /proc/pid/maps
// apparently...)
long _get_file_size(int fd) {
    long fs, r;
    char tmp[0x1000];

    // can seek to end?
    fs = sys_lseek(fd, 0, SEEK_END);
    if (fs > 0) {
        sys_lseek(fd, 0, SEEK_SET);
        return fs;
    }

    fs = 0;
    do {
        r = sys_read(fd, tmp, 0x1000);
        if (r < 0)
            return -1;
        else
            fs += r;
    } while (r > 0);

    sys_lseek(fd, 0, SEEK_SET);
    return fs;
}

// allocate memory + read a file
int read_file(char *path, uint8_t **out_buf, size_t *out_len) {
    long fs, r, n = 0;
    int fd;

    *out_buf = 0;
    *out_len = 0;

    // open file
    if ((fd = sys_open(path, O_RDONLY, 0)) < 0)
        return -1;

    // get file size
    if (!(fs = _get_file_size(fd)))
        return -1;
    *out_len = fs;

    // allocate mem
    if ((*out_buf = (uint8_t *)std_realloc(NULL, fs)) == NULL)
        goto exit;

    // read file
    while (n < fs) {
        r = sys_read(fd, *out_buf + n, fs);
        if (r < 0)
            goto exit;
        n += r;
        if (r == 0 && n != fs)
            goto exit;
    }
    sys_close(fd);
    return 0;

exit:
    if (*out_buf) {
        std_free(*out_buf);
        *out_buf = 0;
    }
    sys_close(fd);
    return -1;
}

int get_memmaps(int pid, uint8_t **maps_buf, size_t *maps_len) {
    char path[256];
    char pids[24];

    // convert pid to string
    if (fmt_num(pids, sizeof(pids), pid, 10) < 0)
        return -1;

    // build /proc/pid/maps string
    std_memset(path, 0, sizeof(path));
    std_strlcat(path, "/proc/", sizeof(path));
    std_strlcat(path, pids, sizeof(path));
    std_strlcat(path, "/maps", sizeof(path));

    // read file
    if (read_file(path, maps_buf, maps_len) < 0)
        return -1;

    return 0;
}

// returns the address & size of the first section that matches _filter_
int get_section(int pid, const char *filter, unsigned long *sec_start,
                size_t *sec_size) {
  uint8_t *maps_buf = NULL;
  size_t maps_len = 0;
  size_t off = 0;
  size_t min_len = *sec_size;
  char *p;

  // read /proc/pid/maps
  if (get_memmaps(pid, &maps_buf, &maps_len) < 0)
    return -1;

  // find first line that matches _filter_
  do {
    if (!(p = std_memmem(maps_buf + off, maps_len - off, filter,
                         std_strlen(filter))))
      goto fail;
    off = (size_t)p - (size_t)maps_buf + 1;

    while (*p && p >= (char *)maps_buf && *p != '\n')
      p--;
    p++;

    // extract section addr & size
    *sec_start = std_strtoul(p, &p, 16);
    p++;
    *sec_size = std_strtoul(p, &p, 16) - (size_t)*sec_start;

  } while (*sec_size < min_len);

  std_free(maps_buf);
  return 0;

fail:
  std_free(maps_buf);
  return -1;
}

// returns max - low address (doesnt start with F or 7F) used by the process
unsigned long get_mapmax(int pid) {
    char *maps_buf = NULL;
    size_t maps_len = 0;
    char *p;
    unsigned long max = 0;

    // read /proc/pid/maps
    if (get_memmaps(pid, (uint8_t **)&maps_buf, &maps_len) < 0)
        return -1;

    // parse all maps
    for (p = (char *)maps_buf; p >= maps_buf && p < maps_buf + maps_len;) {
        char *endline = std_memmem(p, maps_len - (p - maps_buf), "\n", 1);
        char *tmp = NULL;
        unsigned long e;
        unsigned long t;

        std_strtoul(p, &tmp, 16);
        if (!tmp)
            continue;
        tmp++;
        e = std_strtoul(tmp, NULL, 16);
        t = e;

        while (t > 0xff)
            t >>= 8;
        if (t != 0xff && t != 0x7f)
            max = e;

        p = endline + 1;
    }
    std_free(maps_buf);
    std_printf("> auto-detected manual mapping address 0x%lx\n",
               MAPS_ADDR_ALIGN(max));
    return MAPS_ADDR_ALIGN(max);
}
