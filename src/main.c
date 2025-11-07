#define _GNU_SOURCE
#include "isolation.h"
#include "utils.h"
#include "resources.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
Usage:
  ./build/sandbox_cli <rootfs> <mode> <cmd> [args...]
    Modes:
      open        - No syscall restrictions (debug)
      restricted  - Limited but network allowed
      locked      - Fully restricted (safe mode)
  Or fallback: ./build/sandbox_cli <rootfs> <cmd> [args...]
    Mode read from env or /etc/sandbox_policy (default: locked)
*/

static int is_valid_mode(const char *s) {
    return (s &&
            (!strcmp(s, "open") ||
             !strcmp(s, "restricted") ||
             !strcmp(s, "locked")));
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,
            "Usage:\n"
            "  %s <rootfs> <mode> <cmd> [args...]\n"
            "  %s <rootfs> <cmd> [args...] (mode via env/file; default locked)\n",
            argv[0], argv[0]);
        return 1;
    }

    const char *rootfs = argv[1];
    const char *mode = NULL;
    char **cmd = NULL;

    if (argc >= 4 && is_valid_mode(argv[2])) {
        mode = argv[2];
        cmd = &argv[3];
    } else {
        cmd = &argv[2];
        mode = getenv("SANDBOX_SECCOMP_MODE");

        if (!mode || !*mode) {
            char *file_mode = load_policy_mode_from_file("/etc/sandbox_policy");
            if (file_mode && is_valid_mode(file_mode)) {
                fprintf(stderr, "[utils] Loaded mode '%s' from /etc/sandbox_policy'\n", file_mode);
                setenv("SANDBOX_SECCOMP_MODE", file_mode, 1);
                mode = getenv("SANDBOX_SECCOMP_MODE");
                free(file_mode);
            } else {
                if (file_mode) free(file_mode);
            }
        }

        if (!mode || !*mode) mode = "open";
    }

    setenv("SANDBOX_SECCOMP_MODE", mode, 1);

    printf("[+] Starting sandbox with root: %s\n", rootfs);
    printf("    Security Mode: %s\n\n", mode);

    if (apply_rlimits_from_env() != 0) {
        fprintf(stderr, "[!] Failed to apply resource limits\n");
        return 1;
    }

    return start_sandbox(rootfs, cmd);
}
