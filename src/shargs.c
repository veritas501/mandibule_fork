// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

#include <stddef.h>

#include "icrt/icrt.h"

#include "shargs.h"

// code to build / parse / manipulate a shared arguments structure
// used to pass info from injector to injected code
// we hijack the ELF header (+ section header ...) to pass arguments to the
// injected copy of our code

// fwd declarations
void usage(const char *argv0, const char *msg);

// allocates memory & prepares structure
ashared_t *_ashared_new(size_t size) {
    ashared_t *a = (ashared_t *)std_malloc(size);
    if (!a)
        return NULL;

    std_memset(a, 0, size);
    a->base_addr = -1;
    a->size_max = size;
    a->size_used = sizeof(ashared_t);
    return a;
}

// add a string (either arg or env) to our shared data struct
int _ashared_add(ashared_t *a, char *arg, int is_arg_not_env) {
    // size of data to be added (including null byte)
    size_t l = std_strlen(arg) + 1;
    // trying to add an argument after we started adding env?
    if (is_arg_not_env && a->count_env)
        return -1;

    // check that we have the required space
    if (a->size_max - a->size_used < l)
        return -1;

    char *p = a->data + a->size_used - sizeof(ashared_t);
    std_memcpy(p, arg, l);
    a->size_used += l;
    if (is_arg_not_env)
        a->count_arg += 1;
    else
        a->count_env += 1;
    return 0;
}

// get an envvar or argval from index
char *_ashared_get(ashared_t *a, int ind, int is_arg_not_env) {
    char *p = a->data;

    // sanity checks
    if (is_arg_not_env && ind >= a->count_arg)
        return NULL;
    if (!is_arg_not_env && ind >= a->count_env)
        return NULL;

    // convert env index to string (arg + env) index in data
    if (!is_arg_not_env)
        ind += a->count_arg;

    // enumerate argvs
    for (int i = 0; i < a->count_arg + a->count_env; i++) {
        // found what we're looking for?
        if (i == ind)
            return p;

        // goto next string
        while (*p)
            p++;
        p++;
    }

    return NULL;
}

// print recap of settings
void _ashared_print(ashared_t *args) {
    std_printf("> target pid: %d\n", args->pid);

    for (int i = 0; i < args->count_arg; i++)
        std_printf("> arg[%d]: %s\n", i, _ashared_get(args, i, 1));
    for (int i = 0; i < args->count_env; i++)
        std_printf("> env[%d]: %s\n", i, _ashared_get(args, i, 0));
    if (args->base_addr != (unsigned long)-1)
        std_printf("> base address: 0x%lx\n", args->base_addr);

    std_printf("> args size: %d\n", args->size_used);
}

// parse argc & argv to fill info
ashared_t *_ashared_parse(int ac, char **av) {
    // allocate struct to pass info to injected code in remote process
    ashared_t *args = _ashared_new(0x1000);
    int optind = 1;
    if (!args)
        _ashared_error("> error allocating memory for arguments\n");

    // parse arguments
    if (ac < 3)
        usage(av[0], "at least 2 arguments are required (<elf> + <pid>)");

    // add argv[0] (path of elf to inject)
    if (_ashared_add(args, av[optind++], 1) < 0)
        _ashared_error("> error setting elf path (too long?)\n");

    // add arguments
    while (!std_strcmp(av[optind], "-a")) {
        if (optind + 1 + 1 >= ac)
            usage(av[0], "missing string for option -a");
        if (_ashared_add(args, av[optind + 1], 1) < 0)
            _ashared_error("> error setting argument '%s'\n", av[optind + 1]);
        optind += 2;
    }

    // add environment
    while (!std_strcmp(av[optind], "-e")) {
        if (optind + 1 + 1 >= ac)
            usage(av[0], "missing string for option -e");
        if (_ashared_add(args, av[optind + 1], 0) < 0)
            _ashared_error("> error setting envvar '%s'\n", av[optind + 1]);
        optind += 2;
    }

    // specify memory addr ?
    if (!std_strcmp(av[optind], "-m")) {
        if (optind + 1 + 1 >= ac)
            usage(av[0], "missing address for option -m");
        args->base_addr = std_strtoul(av[optind + 1], NULL, 0);
        optind += 2;
    }

    // and finally, the pid to inject into
    args->pid = std_strtoul(av[optind], NULL, 10);

    // print a recap now
    _ashared_print(args);

    // parsing finished - trim struct
    args->size_max = args->size_used;
    return args;
}
