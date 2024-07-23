// ======================================================================== //
// author:  ixty                                                       2018 //
// project: inline c runtime library                                        //
// licence: beerware                                                        //
// ======================================================================== //

// ixty malloc
// use mmap for every required block
// block header = [unsigned long user_size] [unsigned long allocd_size]

#include <stddef.h>

#include "icrt_mem.h"
#include "icrt_std.h"
#include "icrt_syscall.h"

// this realloc will use the current mapped page if it is big enough
void *std_realloc(void *addr, size_t size) {
    size_t alloc_size;
    unsigned long mem;

    if (!size)
        return NULL;

    if (addr && size < 0x1000 - IXTY_SIZE_HDR) {
        IXTY_SIZE_USER(addr) = size;
        return addr;
    }

    alloc_size = size + IXTY_SIZE_HDR;
    if (alloc_size % IXTY_PAGE_SIZE)
        alloc_size = ((alloc_size / IXTY_PAGE_SIZE) + 1) * IXTY_PAGE_SIZE;

    mem = (unsigned long)sys_mmap(NULL, alloc_size, PROT_READ | PROT_WRITE,
                                  MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mem < 0) {
        std_printf("> memory allocation error (0x%x bytes)\n", alloc_size);
        return NULL;
    }

    mem += IXTY_SIZE_HDR;
    IXTY_SIZE_USER(mem) = size;
    IXTY_SIZE_ALLOC(mem) = alloc_size;

    if (addr && IXTY_SIZE_USER(addr))
        std_memcpy((void *)mem, addr, IXTY_SIZE_USER(addr));
    if (addr)
        std_free(addr);

    return (void *)mem;
}

// wrapper around realloc
void *std_malloc(size_t len) { return std_realloc(NULL, len); }

// free mmapped page
void std_free(void *ptr) {
    char *page = (char *)(ptr)-IXTY_SIZE_HDR;

    if (!ptr)
        return;

    sys_munmap(page, IXTY_SIZE_ALLOC(ptr));
}
