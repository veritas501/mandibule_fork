// ======================================================================== //
// author:  ixty                                                       2018 //
// project: inline c runtime library                                        //
// licence: beerware                                                        //
// ======================================================================== //

// utilities (read_file, read /proc/pid/maps, ...)

#ifndef _ICRT_UTILS_H
#define _ICRT_UTILS_H

#include <stddef.h>
#include <stdint.h>

// get file size, supports fds with no seek_end (such as /proc/pid/maps
// apparently...)
long _get_file_size(int fd);

// allocate memory + read a file
int read_file(char *path, uint8_t **out_buf, size_t *out_len);

int get_memmaps(int pid, uint8_t **maps_buf, size_t *maps_len);

// returns the address & size of the first section that matches _filter_
int get_section(int pid, const char * filter, unsigned long * sec_start,
size_t * sec_size);

// returns max - low address (doesnt start with F or 7F) used by the process
unsigned long get_mapmax(int pid);

#endif
