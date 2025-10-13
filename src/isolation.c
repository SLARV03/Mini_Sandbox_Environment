#define _GNU_SOURCE
#include "isolation.h"
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

static int child_func(void *arg) {
    char **argv = arg;

    if (sethostname("sandbox", 7) != 0) {
        perror("sethostname");
        exit(1);
    }

    if (chroot(argv[0]) != 0 || chdir("/") != 0) {
        perror("chroot/chdir");
        exit(1);
    }

    mount("proc", "/proc", "proc", 0, "");
    execvp(argv[1], &argv[1]);
    perror("exec");
    return 1;
}

int start_sandbox(const char *rootfs, char **argv) {
    const int STACK_SIZE = 1024*1024;
    char *stack = malloc(STACK_SIZE);
    if (!stack) return -1;

    char *stackTop = stack + STACK_SIZE;

    char *args[3] = {(char *)rootfs, argv[0], NULL};
    pid_t pid = clone(child_func, stackTop, CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD, args);
    if (pid < 0) {
        perror("clone");
        return -1;
    }
    waitpid(pid, NULL, 0);
    free(stack);
    return 0;
}
