// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

// code to inject code in a remote process using ptrace
// entry point is:
// int pt_inject(pid_t pid, uint8_t * sc_buf, size_t sc_len, size_t
// start_offset);

#ifndef _PTRACE_H
#define _PTRACE_H

#include <asm/ptrace.h>
#include <stdint.h>
#include <sys/uio.h>
#include <sys/user.h>

// ======================================================================== //
// arch specific defines
// ======================================================================== //
#if defined(__i386__)
#define REG_TYPE user_regs_struct
#define REG_PC eip
#define PC_OFF 2
#define REG_SYSCALL orig_eax

#elif defined(__x86_64__)
#define REG_TYPE user_regs_struct
#define REG_PC rip
#define PC_OFF 2
#define REG_SYSCALL orig_rax

#elif defined(__arm__)
#define REG_TYPE user_regs
#define REG_PC uregs[15]
#define PC_OFF 4
#define REG_SYSCALL uregs[7]

#elif defined(__aarch64__)
#define REG_TYPE user_regs_struct
#define REG_PC pc
#define PC_OFF 4
#define REG_SYSCALL regs[8]

#else
#error "unknown arch"
#endif

// ======================================================================== //
// tools
// ======================================================================== //

// returns the first executable segment of a process
int _pt_getxzone(pid_t pid, unsigned long *addr, size_t *size);

// read remote memory lword by lword
int _pt_read(int pid, void *addr, void *dst, size_t len);

// write remote memory lword by lword
int _pt_write(int pid, void *addr, void *src, size_t len);

// arm64 doesnt support PTRACE_GETREGS, so we use getregset
int _pt_getregs(int pid, struct REG_TYPE *regs);

// arm64 doesnt support PTRACE_GETREGS, so we use getregset
int _pt_setregs(int pid, struct REG_TYPE *regs);

int _pt_cancel_syscall(int pid);

// inject shellcode via ptrace into remote process
int pt_inject(pid_t pid, uint8_t *sc_buf, size_t sc_len, size_t start_offset);

#endif
