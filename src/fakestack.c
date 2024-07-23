// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

// this code is used to generate a stack with auxv envv argv argc
// this stack will be given to ld-linux after our manual ELF mapping

#include "icrt/icrt.h"

#include "fakestack.h"
#include <stdint.h>

// utilities to build a fake "pristine stack" as if we were just loaded by the
// kernel

#define FSTACK_PUSH_STR(sp, s)                   \
    {                                            \
        unsigned long l = std_strlen(s) + 1;     \
        unsigned long a = (unsigned long)sp - l; \
        while ((a % sizeof(unsigned long)) != 0) \
            a -= 1;                              \
        std_memcpy((void *)a, s, l);             \
        sp = (void *)a;                          \
    }

#define FSTACK_PUSH_LONG(sp, n)                  \
    {                                            \
        unsigned long l = sizeof(unsigned long); \
        unsigned long v = n;                     \
        sp -= l;                                 \
        std_memcpy(sp, &v, l);                   \
    }

#define FSTACK_PUSH_AUXV(sp, auxv)      \
    {                                   \
        unsigned long *a = auxv;        \
        FSTACK_PUSH_LONG(sp, 0);        \
        FSTACK_PUSH_LONG(sp, 0);        \
        while (*a) {                    \
            FSTACK_PUSH_LONG(sp, a[1]); \
            FSTACK_PUSH_LONG(sp, a[0]); \
            a += 2;                     \
        }                               \
    }

uint8_t *fake_stack(uint8_t *sp, int ac, char **av, char **env,
                    unsigned long *auxv) {
    uint8_t *env_ptrs[256];
    uint8_t **arg_ptrs;
    int env_max = 0;
    char *av_0 = NULL;
    std_memset(env_ptrs, 0, sizeof(env_ptrs));
    arg_ptrs = (uint8_t **)std_malloc(sizeof(*arg_ptrs) * ac);
    std_memset(arg_ptrs, 0, sizeof(uint8_t *) * ac);

    // align stack
    FSTACK_PUSH_STR(sp, "");

    // copy original env
    while (*env && env_max < 254) {
        FSTACK_PUSH_STR(sp, *env);
        env_ptrs[env_max++] = sp;
        env++;
    }

    // add to envdata
    FSTACK_PUSH_STR(sp, "MANMAP=1");
    env_ptrs[env_max++] = sp;

    // argv data
    for (int i = 0; i < ac; i++) {
        FSTACK_PUSH_STR(sp, av[ac - i - 1]);
        arg_ptrs[ac - i - 1] = sp;
    }

    // auxv
    FSTACK_PUSH_AUXV(sp, auxv);

    // envp
    FSTACK_PUSH_LONG(sp, 0);
    for (int i = 0; i < env_max; i++)
        FSTACK_PUSH_LONG(sp, (unsigned long)env_ptrs[i]);

    // argp
    FSTACK_PUSH_LONG(sp, 0);
    for (int i = 0; i < ac; i++) {
        FSTACK_PUSH_LONG(sp, (unsigned long)arg_ptrs[ac - i - 1]);
    }
    // argc
    FSTACK_PUSH_LONG(sp, ac);

    return sp;
}
