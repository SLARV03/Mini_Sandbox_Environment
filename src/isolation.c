#define _GNU_SOURCE
#include "isolation.h"
#include "resources.h"     // ✅ Added for resource limits
#include "seccomp.h"
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

// =====================================================
// Child process: sets up the isolated sandbox environment
// =====================================================
static int sandbox_child(void *arg) {
    char **argv = arg;
    const char *rootfs = argv[0];
    const char *seccomp_mode = argv[1];
    char **cmd = &argv[2];

    fprintf(stderr, "[sandbox] Changing root to: %s\n", rootfs);

    // 1. Set unique hostname
    if (sethostname("sandbox", 7) != 0) {
        perror("sethostname");
        _exit(1);
    }
    fprintf(stderr, "[sandbox] Hostname set to 'sandbox'\n");

    // 2. Apply resource limits (moved inside child)
    if (apply_rlimits_from_env() != 0) {
        fprintf(stderr, "[sandbox] Failed to apply resource limits inside sandbox\n");
        // continue anyway (not fatal)
    }

    // 3. Change root to the isolated filesystem
    if (chroot(rootfs) != 0) {
        perror("chroot");
        _exit(1);
    }
    if (chdir("/") != 0) {
        perror("chdir");
        _exit(1);
    }

    // 4. Mount /proc inside sandbox (for ulimit and process info)
    if (mount("proc", "/proc", "proc", 0, "") != 0) {
        perror("mount /proc");
        _exit(1);
    }

    // 5. Apply seccomp restrictions based on mode
    if (seccomp_mode) {
        if (apply_seccomp_filter(seccomp_mode) != 0) {
            fprintf(stderr, "[!] Failed to apply seccomp filter\n");
            _exit(1);
        }
    }

    // 6. Execute user command
    execvp(cmd[0], cmd);
    perror("execvp");
    _exit(1);
}

// =====================================================
// Parent process: clones and manages the sandbox process
// =====================================================
int start_sandbox(const char *rootfs, char **argv) {
    const int STACK_SIZE = 1024 * 1024;
    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc");
        return -1;
    }
    char *stackTop = stack + STACK_SIZE;

    // Count arguments
    int argc = 0;
    while (argv[argc])
        argc++;

    // Get seccomp mode from environment
    const char *seccomp_mode = getenv("SANDBOX_SECCOMP_MODE");
    if (!seccomp_mode)
        seccomp_mode = "locked";

    // Prepare argument list for sandbox_child()
    char **args = malloc((argc + 3) * sizeof(char *));
    args[0] = (char *)rootfs;
    args[1] = (char *)seccomp_mode;
    for (int i = 0; i < argc; i++)
        args[i + 2] = argv[i];
    args[argc + 2] = NULL;

    // Clone new process with isolated namespaces
    pid_t pid = clone(sandbox_child, stackTop,
                      CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWPID | SIGCHLD,
                      args);

    if (pid < 0) {
        perror("clone");
        free(stack);
        free(args);
        return -1;
    }

    // Wait for the sandboxed process to finish
    waitpid(pid, NULL, 0);

    free(stack);
    free(args);
    return 0;
}
