// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

// code to manually map an executable ELF in local memory
// uses mmap & mprotect to map segments in memory
// this code does not process relocations
// this code loads the loader to handle dependencies

#include <elf.h>
#include <stdint.h>
#include <sys/types.h>

#include "icrt/icrt.h"

#include "elfload.h"

// utility to set AUX values
int set_auxv(unsigned long *auxv, unsigned long at_type, unsigned long at_val) {
    int i = 0;
    while (auxv[i] && auxv[i] != at_type)
        i += 2;

    if (!auxv[i]) {
        std_printf("> error setting auxv[%d] to 0x%llx\n", at_type, at_val);
        return -1;
    }

    auxv[i + 1] = at_val;
    std_printf("> set auxv[%d] to 0x%llx\n", at_type, at_val);
    return 0;
}

// load a single segment in memory & set perms
int load_segment(uint8_t *elf, elf_ehdr *ehdr, elf_phdr *phdr,
                 unsigned long base_off) {
    unsigned long seg_addr = phdr->p_vaddr + base_off;
    unsigned long seg_len = phdr->p_memsz;
    unsigned long seg_off = seg_addr & (PAGE_SIZE - 1);
    int prot = 0;
    long addr;

    // align segment
    ALIGN_PAGE_UP(seg_len);

    // get memory at fixed addr
    addr = (unsigned long)sys_mmap(
        (void *)(seg_addr - seg_off), seg_len + seg_off, PROT_READ | PROT_WRITE,
        MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // all good?
    std_printf("> load segment addr 0x%llx len 0x%llx => 0x%llx\n", seg_addr,
               seg_len, addr);
    if (addr != seg_addr - seg_off)
        return -1;

    // load up the segment with code/data
    std_memcpy((void *)seg_addr, elf + phdr->p_offset, phdr->p_filesz);

    // set correct permission
    if (phdr->p_flags & PF_R)
        prot |= PROT_READ;
    if (phdr->p_flags & PF_W)
        prot |= PROT_WRITE;
    if (phdr->p_flags & PF_X)
        prot |= PROT_EXEC;
    if (sys_mprotect((void *)(seg_addr - seg_off), seg_len + seg_off, prot) < 0)
        return -1;

    // all good
    return 0;
}

// load an elf in memory at specified base address
// will load and run interpreter if any
int map_elf(char *path, unsigned long base_mod, unsigned long *auxv,
            unsigned long *out_eop) {
    uint8_t *elf_buf = NULL;
    size_t elf_len = 0;
    unsigned long base_off = 0;
    unsigned long base_next = 0;
    unsigned long base_seg = 0;
    unsigned long base_intp = 0;
    unsigned long eop_elf = 0;
    unsigned long eop_ldr = 0;
    elf_ehdr *ehdr;
    elf_phdr *phdr;

    std_printf("> reading elf file '%s'\n", path);
    if (read_file(path, &elf_buf, &elf_len)) {
        std_printf("> read_file failed\n");
        return -1;
    }

    std_printf("> loading elf at: 0x%llx\n", base_mod);
    ehdr = (elf_ehdr *)elf_buf;
    if (ehdr->e_type == ET_DYN)
        base_off = base_mod;

    eop_elf = base_off + ehdr->e_entry;

    // load segments
    for (int i = 0; i < ehdr->e_phnum; i++) {
        phdr = (elf_phdr *)(elf_buf + ehdr->e_phoff + i * ehdr->e_phentsize);
        // printf("> seg[%d] load: %d addr 0x%llx size 0x%llx\n", i,
        // phdr->p_type == PT_LOAD, phdr->p_vaddr, phdr->p_memsz);
        if (phdr->p_type == PT_LOAD &&
            load_segment(elf_buf, ehdr, phdr, base_off))
            return -1;
        if (!base_seg)
            base_seg = phdr->p_vaddr;
        base_next = phdr->p_vaddr + phdr->p_memsz > base_next
                        ? phdr->p_vaddr + phdr->p_memsz
                        : base_next;
    }

    ALIGN_PAGE_DOWN(base_seg);

    if (ehdr->e_type == ET_DYN)
        base_seg += base_off;
    // printf("> program base: 0x%llx\n", base_seg);

    base_next += MOD_OFFSET_NEXT;
    ALIGN_PAGE_UP(base_next);

    std_printf("> max vaddr 0x%llx\n", base_mod + base_next);

    // load interp
    for (int i = 0; i < ehdr->e_phnum; i++) {
        char *interp;

        phdr = (elf_phdr *)(elf_buf + ehdr->e_phoff + i * ehdr->e_phentsize);

        if (phdr->p_type != PT_INTERP)
            continue;

        base_intp = base_mod + base_next;
        interp =
            (char *)(ehdr->e_type == ET_DYN ? base_seg : 0) + phdr->p_vaddr;

        std_printf("> loading interp '%s'\n", interp);
        if (map_elf(interp, base_intp, NULL, &eop_ldr))
            return -1;
    }

    // set auxv phdr / phent / phnum / pagesz / entry
    if (auxv) {
        std_printf("> setting auxv\n");
        set_auxv(auxv, AT_PHDR, base_seg + ehdr->e_phoff);
        set_auxv(auxv, AT_PHNUM, ehdr->e_phnum);
        set_auxv(auxv, AT_ENTRY, eop_elf);
        set_auxv(auxv, AT_BASE, base_seg);
    }

    *out_eop = eop_ldr ? eop_ldr : eop_elf;
    std_printf("> eop 0x%llx\n", *out_eop);
    return 0;
}
