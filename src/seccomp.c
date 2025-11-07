#define _GNU_SOURCE
#include <seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "seccomp.h"

/*
 * Allow essential syscalls for normal program operation.
 */
static int allow_basic_syscalls(scmp_filter_ctx ctx) {
    int r = 0;

    // Existing syscalls ...
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(futex), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clone), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(execve), 0);

    // New additions: required for BusyBox and glibc
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getrandom), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigprocmask), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(prctl), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rseq), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(set_tid_address), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(set_robust_list), 0);

    // Identity syscalls
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getuid), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(geteuid), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getgid), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getegid), 0);

    // Filesystem-related
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(openat), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(statx), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(newfstatat), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(readlinkat), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(uname), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(prlimit64), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(arch_prctl), 0);
    r |= seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clock_gettime), 0);

    return r;
}




/*
 * Apply seccomp filter based on mode:
 *  - open       → all syscalls allowed
 *  - restricted → allows IO + network, denies mounts/ptrace
 *  - locked     → only basic syscalls, no network or system modifications
 */
int apply_seccomp_filter(const char *mode) {
    scmp_filter_ctx ctx;
    int rc;

    if (mode && strcmp(mode, "open") == 0) {
        fprintf(stderr, "[seccomp] Mode: open (no syscall restrictions)\n");
        return 0; // no filtering
    }

    ctx = seccomp_init(SCMP_ACT_ERRNO(EPERM)); // default: deny
    if (!ctx) {
        fprintf(stderr, "[seccomp] seccomp_init failed\n");
        return -1;
    }

    if (allow_basic_syscalls(ctx) != 0) {
        fprintf(stderr, "[seccomp] Failed to add basic syscall rules\n");
        seccomp_release(ctx);
        return -1;
    }

    // Always allow some harmless utility calls
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(ioctl), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getpid), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(gettid), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clock_gettime), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(gettimeofday), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(readlink), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(kill), 0);

       const int essential_syscalls[] = {
        /* process creation / exec */
        SCMP_SYS(execve),
        SCMP_SYS(execveat),
        SCMP_SYS(clone),
        SCMP_SYS(fork),
        SCMP_SYS(vfork),
        SCMP_SYS(clone3),
   

        /* memory / stack setup */
        SCMP_SYS(mmap),
        SCMP_SYS(mprotect),
        SCMP_SYS(brk),
        SCMP_SYS(munmap),
        SCMP_SYS(arch_prctl),

        /* threading / futex / tid handling */
        SCMP_SYS(futex),
        SCMP_SYS(set_tid_address),
        SCMP_SYS(set_robust_list),

        /* signals */
        SCMP_SYS(rt_sigaction),
        SCMP_SYS(rt_sigprocmask),
        SCMP_SYS(rt_sigreturn),
        SCMP_SYS(sigaltstack),

        /* file and descriptor basics (if your default policy is deny by default,
           allow these; if default is allow you can remove them) */
        SCMP_SYS(open),
        SCMP_SYS(openat),
        SCMP_SYS(close),
        SCMP_SYS(read),
        SCMP_SYS(write),
        SCMP_SYS(lseek),
        SCMP_SYS(fstat),
        SCMP_SYS(fcntl),
        SCMP_SYS(pipe),
        SCMP_SYS(dup),
        SCMP_SYS(dup2),
        SCMP_SYS(pread64),
        SCMP_SYS(writev),

        /* time/wait / process metadata */
        SCMP_SYS(wait4),
        SCMP_SYS(getpid),
        SCMP_SYS(getppid),
        SCMP_SYS(getuid),
        SCMP_SYS(geteuid),
        SCMP_SYS(gettid),
        SCMP_SYS(kill),

        /* resource limits, prctl */
        SCMP_SYS(prlimit64),
        SCMP_SYS(prctl),

        /* misc / directory handling */
        SCMP_SYS(chdir),
        SCMP_SYS(stat),
        SCMP_SYS(lstat),
        SCMP_SYS(access),
        SCMP_SYS(newfstatat)
    };
    const size_t n_essential = sizeof(essential_syscalls) / sizeof(essential_syscalls[0]);

    if (mode && strcmp(mode, "restricted") == 0) {
        fprintf(stderr, "[seccomp] Mode: restricted (network + file access allowed)\n");

        /* Allow networking syscalls you already had */
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(socket), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(connect), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sendto), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(recvfrom), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(bind), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(listen), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(accept), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sendmsg), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(recvmsg), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(socketpair), 0);

        /* Allow the essential runtime syscalls */
        for (size_t i = 0; i < n_essential; ++i) {
            /* ignore errors in case a syscall is unavailable on this kernel/libseccomp */
            seccomp_rule_add(ctx, SCMP_ACT_ALLOW, essential_syscalls[i], 0);
        }
    } else {
        fprintf(stderr, "[seccomp] Mode: locked (no network or privileged syscalls)\n");

        /* locked: keep your explicit denials for network and privileged syscalls */
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(socket), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(connect), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(sendto), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(recvfrom), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(bind), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(listen), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(accept), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(sendmsg), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(recvmsg), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(socketpair), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(ptrace), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(mount), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(umount2), 0);
        seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(mknod), 0);

        /* Even in locked mode, allow essential runtime syscalls (process startup needs them).
           We still deny network/privileged calls above. */
        for (size_t i = 0; i < n_essential; ++i) {
            seccomp_rule_add(ctx, SCMP_ACT_ALLOW, essential_syscalls[i], 0);
        }
    }
    fprintf(stderr, "[seccomp] Debug: logging blocked syscalls to dmesg\n");
seccomp_attr_set(ctx, SCMP_FLTATR_CTL_LOG, 1);

    rc = seccomp_load(ctx);
    if (rc < 0) {
        fprintf(stderr, "[seccomp] seccomp_load failed: %s\n", strerror(-rc));
        seccomp_release(ctx);
        return -1;
    }

    seccomp_release(ctx);
    return 0;
}
