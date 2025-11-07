#define _GNU_SOURCE
#include "isolation.h"
#include "seccomp.h"
#include "resources.h"
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/*
 * Child process setup:
 * - sets hostname
 * - mounts /proc
 * - chroots into rootfs
 * - applies seccomp policy (mode from env)
 * - executes command
 */
static int sandbox_child(void *arg) {
    char **argv = arg;
    const char *rootfs = argv[0];
    const char *seccomp_mode = argv[1];
    char **cmd = &argv[2];

    fprintf(stderr, "[sandbox] Changing root to: %s\n", rootfs);

    if (sethostname("sandbox", 7) != 0) {
        perror("sethostname");
        _exit(1);
    }

    if (chroot(rootfs) != 0) {
        perror("chroot");
        _exit(1);
    }
    if (chdir("/") != 0) {
        perror("chdir");
        _exit(1);
    }

    if (mount("proc", "/proc", "proc", 0, "") != 0) {
        perror("mount /proc");
        _exit(1);
    }

    // Apply resource limits before executing command
    if (apply_rlimits_from_env() != 0) {
        fprintf(stderr, "[sandbox] Failed to apply resource limits\n");
    } else {
        fprintf(stderr, "[sandbox] Resource limits applied successfully\n");
    }


    if (seccomp_mode) {
        fprintf(stderr, "[sandbox] Applying seccomp mode: %s\n", seccomp_mode);
        if (apply_seccomp_filter(seccomp_mode) != 0) {
            fprintf(stderr, "[sandbox] Failed to apply seccomp filter\n");
            _exit(1);
        }
    }
    

    execvp(cmd[0], cmd);
    perror("execvp");
    _exit(1);
}

/*
 * Create sandbox process:
 *  - new UTS, mount, and PID namespaces
 *  - passes rootfs + seccomp_mode + command
 */
int start_sandbox(const char *rootfs, char **argv) {
    const int STACK_SIZE = 1024 * 1024;
    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc");
        return -1;
    }
    char *stackTop = stack + STACK_SIZE;

    int argc = 0;
    while (argv[argc])
        argc++;

    const char *seccomp_mode = getenv("SANDBOX_SECCOMP_MODE");
    if (!seccomp_mode)
        seccomp_mode = "locked";  // default safest mode

    // Prepare args: [rootfs, mode, cmd...]
    char **args = malloc((argc + 3) * sizeof(char *));
    if (!args) {
        perror("malloc");
        free(stack);
        return -1;
    }

    args[0] = (char *)rootfs;
    args[1] = (char *)seccomp_mode;
    for (int i = 0; i < argc; i++)
        args[i + 2] = argv[i];
    args[argc + 2] = NULL;

    pid_t pid = clone(sandbox_child, stackTop,
                      CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWPID | SIGCHLD,
                      args);
    if (pid < 0) {
        perror("clone");
        free(stack);
        free(args);
        return -1;
    }

    waitpid(pid, NULL, 0);

    free(stack);
    free(args);
    return 0;
}
