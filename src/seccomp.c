#define _GNU_SOURCE
#include <seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "seccomp.h"

/*
 * Portable Seccomp filter that works across distros and kernels.
 * Resolves syscall names at runtime — so it never fails to compile
 * even if certain SCMP_SYS() macros are missing.
 */

static int allow_syscall(scmp_filter_ctx ctx, const char *name) {
    int nr = seccomp_syscall_resolve_name(name);
    if (nr == __NR_SCMP_ERROR || nr < 0)
        return 0; // syscall not supported, skip
    return seccomp_rule_add(ctx, SCMP_ACT_ALLOW, nr, 0);
}

static int deny_syscall(scmp_filter_ctx ctx, const char *name, int err) {
    int nr = seccomp_syscall_resolve_name(name);
    if (nr == __NR_SCMP_ERROR || nr < 0)
        return 0;
    return seccomp_rule_add(ctx, SCMP_ACT_ERRNO(err), nr, 0);
}

int apply_seccomp_filter(const char *mode) {
    scmp_filter_ctx ctx;

    if (!mode || strcmp(mode, "open") == 0) {
        fprintf(stderr, "[seccomp] Mode: open (no syscall restrictions)\n");
        return 0;
    }

    ctx = seccomp_init(SCMP_ACT_ERRNO(EPERM)); // default: kill unlisted syscalls
    if (!ctx) {
        perror("seccomp_init");
        return -1;
    }

    seccomp_attr_set(ctx, SCMP_FLTATR_CTL_LOG, 1); // log denied syscalls

    const char *common_syscalls[] = {
        // File + I/O
        "read", "write", "pread64", "pwrite64", "close",
        "fstat", "newfstatat", "fstatat", "fstatat64", "statx", "lseek",
        "open", "openat", "readlink", "access", "ioctl",

        // Memory
        "mmap", "mprotect", "munmap", "brk",

        // Process
        "clone", "fork", "vfork", "execve", "execveat",
        "wait4", "exit", "exit_group", "set_tid_address",
        "set_robust_list", "getpid", "getppid", "gettid", "futex",

        // Signals
        "rt_sigaction", "rt_sigprocmask", "sigaltstack", "rt_sigreturn",

        // Time + Info
        "clock_gettime", "gettimeofday", "nanosleep", "uname", "prlimit64",

        // Identity
        "getuid", "geteuid", "getgid", "getegid",

        // Misc
        "arch_prctl", "prctl", "rseq", "getrandom",

        // IO multiplexing
        "poll", "ppoll", "epoll_create1", "epoll_wait", "epoll_ctl",

        // Device & node
        "mknod", NULL
    };

    for (int i = 0; common_syscalls[i]; i++)
        allow_syscall(ctx, common_syscalls[i]);

    if (strcmp(mode, "restricted") == 0) {
        fprintf(stderr, "[seccomp] Mode: restricted (network allowed)\n");
        const char *netcalls[] = {
            "socket", "connect", "bind", "listen",
            "accept", "accept4", "sendto", "recvfrom",
            "sendmsg", "recvmsg", "socketpair", "dup", "dup2", "dup3",NULL
        };
        for (int i = 0; netcalls[i]; i++)
            allow_syscall(ctx, netcalls[i]);
    } else {
        fprintf(stderr, "[seccomp] Mode: locked (deny network + privileged syscalls)\n");
        deny_syscall(ctx, "mount", EPERM);
        deny_syscall(ctx, "umount2", EPERM);
        deny_syscall(ctx, "ptrace", EPERM);
        deny_syscall(ctx, "reboot", EPERM);
        deny_syscall(ctx, "kexec_load", EPERM);
    }

        /* --- Fallback: alias fstatat64 → newfstatat if missing --- */
    int fs64 = seccomp_syscall_resolve_name("fstatat64");
    int newfs = seccomp_syscall_resolve_name("newfstatat");
    if (fs64 < 0 && newfs >= 0)
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, newfs, 0);
    else if (fs64 >= 0)
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, fs64, 0);


    if (seccomp_load(ctx) < 0) {
        perror("seccomp_load");
        seccomp_release(ctx);
        return -1;
    }

    seccomp_release(ctx);
    return 0;
}
