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
#include "seccomp.h" 

// Child process function for the sandbox 
// It sets up the isolated environment and executes the command
// sets up new rootfs, mounts /proc, applies seccomp filter, and execs the command
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
    fprintf(stderr, "[sandbox] Hostname set to 'sandbox'\n");

    if (chroot(rootfs) != 0) { perror("chroot"); _exit(1); }
    if (chdir("/") != 0) { perror("chdir"); _exit(1); }

    if (mount("proc", "/proc", "proc", 0, "") != 0) {
        perror("mount /proc");
        _exit(1);
    }

    if (seccomp_mode) {
        if (apply_seccomp_filter(seccomp_mode) != 0) {
            fprintf(stderr, "[!] Failed to apply seccomp filter\n");
            _exit(1);
        }
    }

    execvp(cmd[0], cmd);
    perror("execvp"); 
    _exit(1);
}

// Start the sandboxed process with the specified root filesystem and command
int start_sandbox(const char *rootfs, char **argv) {
    const int STACK_SIZE = 1024*1024;
    char *stack = malloc(STACK_SIZE);
    if (!stack) { perror("malloc"); return -1; }
    char *stackTop = stack + STACK_SIZE;
    int argc = 0;
    while (argv[argc]) argc++;
    
    const char *seccomp_mode = getenv("SANDBOX_SECCOMP_MODE"); 
    //if (!seccomp_mode) seccomp_mode = "permissive";
    if (!seccomp_mode) seccomp_mode = "strict";


    char **args = malloc((argc + 3) * sizeof(char *)); 
    args[0] = (char *)rootfs;
    args[1] = (char *)seccomp_mode;
    for (int i = 0; i < argc; i++) args[i+2] = argv[i];
    args[argc+2] = NULL;
    
    pid_t pid = clone(sandbox_child, stackTop,
                      CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWPID | SIGCHLD,
                      args);
    if (pid < 0) { perror("clone"); free(stack); free(args); return -1; }

    // Wait for the sandbox process to finish
    waitpid(pid, NULL, 0);

    free(stack);
    free(args);
    return 0;
}
