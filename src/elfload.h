// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

// code to manually map an executable ELF in local memory
// uses mmap & mprotect to map segments in memory
// this code does not process relocations
// this code loads the loader to handle dependencies

#ifndef _ELFLOAD_H
#define _ELFLOAD_H

#include <elf.h>
#include <stdint.h>

#if defined(__i386__) || defined(__arm__)
// 32 bits elf
#define elf_ehdr Elf32_Ehdr
#define elf_phdr Elf32_Phdr

#elif defined(__x86_64__) || defined(__aarch64__)
// 64 bits elf
#define elf_ehdr Elf64_Ehdr
#define elf_phdr Elf64_Phdr
#endif

// some defines..
#define PAGE_SIZE 0x1000
#define MOD_OFFSET_NEXT 0x10000
#define MOD_OFFSET_BIN 0x100000

#define ALIGN_PAGE_UP(x)                            \
    do {                                            \
        if ((x) % PAGE_SIZE)                        \
            (x) += (PAGE_SIZE - ((x) % PAGE_SIZE)); \
    } while (0)
#define ALIGN_PAGE_DOWN(x)            \
    do {                              \
        if ((x) % PAGE_SIZE)          \
            (x) -= ((x) % PAGE_SIZE); \
    } while (0)

// utility to set AUX values
int set_auxv(unsigned long *auxv, unsigned long at_type, unsigned long at_val);

// load a single segment in memory & set perms
int load_segment(uint8_t *elf, elf_ehdr *ehdr, elf_phdr *phdr,
                 unsigned long base_off);

// load an elf in memory at specified base address
// will load and run interpreter if any
int map_elf(char *path, unsigned long base_mod, unsigned long *auxv,
            unsigned long *out_eop);

#endif
