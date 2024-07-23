// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

// only c file of our code injector
// it includes directly all other code to be able to wrap all generated code
// between start/end boundaries

#include <linux/unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

// #define DEBUG 0

#include "icrt/icrt_asm.h"

// define injected code entry point - which calls payload_main()
INJ_ENTRY(_start, payload_main);

#define BUFFER_SIZE 0x400

const uint64_t g_buf[];

// include minimal inline c runtime + support code
#include "icrt/icrt.h"
#include "icrt/icrt_code.h"

#include "elfload.h"
#include "fakestack.h"
#include "ptinject.h"
#include "shargs.h"

#include "elfload.c"
#include "fakestack.c"
#include "ptinject.c"
#include "shargs.c"

void payload_loadelf(ashared_t *args);

// small macro for print + exit
#define error(...)               \
    do {                         \
        std_printf(__VA_ARGS__); \
        sys_exit(1);             \
    } while (0)

// show program usage & exit
void usage(const char *argv0, const char *msg) {
    if (msg)
        std_printf("error: %s\n\n", msg);

    std_printf("usage: %s <elf> [-a arg]* [-e env]* [-m addr] <pid>\n", argv0);
    std_printf("\n");
    std_printf("loads an ELF binary into a remote process.\n");
    std_printf("\n");
    std_printf("arguments:\n");
    std_printf("    - elf: path of binary to inject into <pid>\n");
    std_printf("    - pid: pid of process to inject into\n");
    std_printf("\n");
    std_printf("options:\n");
    std_printf(
        "    -a arg: argument to send to injected program - can be repeated\n");
    std_printf(
        "    -e env: environment value sent to injected program - can be "
        "repeated\n");
    std_printf("    -m mem: base address at which program is loaded in remote "
               "process, default=AUTO\n");
    std_printf("\n");
    std_printf("Note: order of arguments must be respected (no getopt sry)\n");
    std_printf("\n");
    sys_exit(1);
}

void payload_loadelf(ashared_t *args) {
    char pids[24];
    char path[256];
    uint8_t *auxv_buf;
    size_t auxv_len;
    char **av;
    char **env;
    uint8_t fakestack[4096 * 16];
    uint8_t *stackptr = fakestack + sizeof(fakestack);
    unsigned long eop;
    unsigned long base_addr;

    // convert pid to string
    if (fmt_num(pids, sizeof(pids), args->pid, 10) < 0)
        return;

    // read auxv
    std_memset(path, 0, sizeof(path));
    std_strlcat(path, "/proc/", sizeof(path));
    std_strlcat(path, pids, sizeof(path));
    std_strlcat(path, "/auxv", sizeof(path));
    if (read_file(path, &auxv_buf, &auxv_len) < 0)
        return;
    std_printf("> auxv len: %d\n", auxv_len);

    // build argv from args
    av = std_malloc((args->count_arg + 1) * sizeof(char *));
    std_memset(av, 0, (args->count_arg + 1) * sizeof(char *));
    for (int i = 0; i < args->count_arg; i++)
        av[i] = _ashared_get(args, i, 1);

    // build envp from args
    env = std_malloc((args->count_env + 1) * sizeof(char *));
    for (int i = 0; i < args->count_env; i++)
        env[i] = _ashared_get(args, i, 0);

    // autodetect binary mapping address?
    base_addr = args->base_addr == -1 ? get_mapmax(args->pid) : args->base_addr;
    std_printf("> mapping '%s' into memory at 0x%lx\n", av[0], base_addr);

    // load the elf into memory!
    if (map_elf(av[0], base_addr, (unsigned long *)auxv_buf, &eop) < 0)
        error("> failed to load elf\n");

    // build a stack for loader entry
    std_printf("ptr: %x\n", fakestack);
    std_memset(fakestack, 0, sizeof(fakestack));
    std_printf("stackptr: %x\n", stackptr);
    stackptr = fake_stack(stackptr, args->count_arg, av, env,
                          (unsigned long *)auxv_buf);
    if (((long)stackptr & 0x0f) != 0) {
        std_printf("realign stack: %x\n", stackptr);
        std_memset(fakestack, 0, sizeof(fakestack));
        stackptr = fakestack + sizeof(fakestack);
        stackptr -= 8;
        stackptr = fake_stack(stackptr, args->count_arg, av, env,
                              (unsigned long *)auxv_buf);
    }

    // all done
    std_printf("> starting ...\n\n");
    //   asm("int3");
    FIX_SP_JMP(stackptr, eop);

    // never reached if everything goes well
    std_printf("> returned from loader\n");
    std_free(auxv_buf);
    std_free(av);
    std_free(env);
    sys_exit(1);
}

// main function for injected code
// executed in remote process
void payload_main(void) {
    // get the arguments from memory
    // we overwrite the ELF header with the arguments for the injected copy of
    // our code

    ashared_t *args = (ashared_t *)g_buf;

    _ashared_print(args);

    // load elf into memory
    payload_loadelf(args);
    sys_exit(0);
}

const uint64_t g_buf[BUFFER_SIZE / sizeof(*g_buf)] = {0xdeadbeef13371337};
