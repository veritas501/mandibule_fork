// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

#ifndef _ARGS_H
#define _ARGS_H

#include <stddef.h>

#include "icrt/icrt.h"

// code to build / parse / manipulate a shared arguments structure
// used to pass info from injector to injected code
// we hijack the ELF header (+ section header ...) to pass arguments to the
// injected copy of our code

// small error macro
#define _ashared_error(...)      \
    do {                         \
        std_printf(__VA_ARGS__); \
        sys_exit(1);             \
    } while (0)

// shared arguments between injector & injectee
typedef struct ashared_s {
    // internals
    size_t size_max;  // total size in bytes of header + data
    size_t size_used; // currently used memory

    // payload
    unsigned long pid;       // pid to inject into
    unsigned long base_addr; // address where we want to load binary in memory -
                             // 0 for auto
    int count_arg;           // number of arguments
    int count_env;           // number of env values
    char data[]; // data where args & envs are stored, list of null terminated
                 // strings
} ashared_t;

// allocates memory & prepares structure
ashared_t *_ashared_new(size_t size);

// add a string (either arg or env) to our shared data struct
int _ashared_add(ashared_t *a, char *arg, int is_arg_not_env);

// get an envvar or argval from index
char *_ashared_get(ashared_t *a, int ind, int is_arg_not_env);

// print recap of settings
void _ashared_print(ashared_t *args);

// parse argc & argv to fill info
ashared_t *_ashared_parse(int ac, char **av);

#endif
