#define _GNU_SOURCE
#include "isolation.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//./build/sandbox_cli sandbox_env /bin/sh 


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <rootfs> <cmd> [args...]\n", argv[0]);
        return 1;
    }
    // Phase 3 Part : user login authentication
    // if (check_auth() != 0) {
    //     fprintf(stderr, "Unauthorized: set SANDBOX_TOKEN env or create ~/.sandbox_token\n");
    //     return 1;
    // }
    const char *rootfs = argv[1]; 
    char **cmd = &argv[2];        

    printf("[+] Starting sandbox with root: %s\n", rootfs);
    return start_sandbox(rootfs, cmd);
}

/*
int check_auth(void) {
    // 1. Check for SANDBOX_TOKEN environment variable
    const char *env_token = getenv("SANDBOX_TOKEN");
    if (env_token && strlen(env_token) > 0)
        return 0; // authorized if token present

    // 2. Fallback: look for ~/.sandbox_token file
    char path[512];
    const char *home = getenv("HOME");
    if (!home)
        return 1;

    snprintf(path, sizeof(path), "%s/.sandbox_token", home);
    FILE *f = fopen(path, "r");
    if (!f)
        return 1;

    char token[256];
    if (!fgets(token, sizeof(token), f)) {
        fclose(f);
        return 1;
    }
    fclose(f);

    // Trim newline and verify token not empty
    char *nl = strchr(token, '\n');
    if (nl)
        *nl = '\0';

    return (strlen(token) == 0) ? 1 : 0;
}*/
