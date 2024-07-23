// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

// this code is used to generate a stack with auxv envv argv argc
// this stack will be given to ld-linux after our manual ELF mapping

#ifndef _FAKESTACK_H
#define _FAKESTACK_H

#include <stdint.h>

// utilities to build a fake "pristine stack" as if we were just loaded by the
// kernel

uint8_t *fake_stack(uint8_t *sp, int ac, char **av, char **env,
                    unsigned long *auxv);

#endif
