// ======================================================================== //
// author:  ixty                                                       2018 //
// project: inline c runtime library                                        //
// licence: beerware                                                        //
// ======================================================================== //

// ixty malloc
// use mmap for every required block
// block header = [unsigned long user_size] [unsigned long allocd_size]

#ifndef _ICRT_MEM_H
#define _ICRT_MEM_H

#include <stddef.h>

// some defines
#define IXTY_PAGE_SIZE 0x1000
#define IXTY_SIZE_USER(ptr) \
    (*(unsigned long *)((unsigned long)(ptr)-2 * sizeof(unsigned long)))
#define IXTY_SIZE_ALLOC(ptr) \
    (*(unsigned long *)((unsigned long)(ptr)-1 * sizeof(unsigned long)))
#define IXTY_SIZE_HDR (2 * sizeof(unsigned long))

// this realloc will use the current mapped page if it is big enough
void *std_realloc(void *addr, size_t size);

// wrapper around realloc
void *std_malloc(size_t len);

// free mmapped page
void std_free(void *ptr);

#endif
