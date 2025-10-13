#include "isolation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <rootfs> <cmd> [args...]\n", argv[0]);
        return 1;
    }

    const char *rootfs = argv[1];
    char **cmd = &argv[2];

    printf("[+] Starting sandbox with root: %s\n", rootfs);
    return start_sandbox(rootfs, cmd);
}
