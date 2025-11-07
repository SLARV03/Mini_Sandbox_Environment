#define _GNU_SOURCE
#include "resources.h"
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static int parse_rlim_from_env(const char *envname, rlim_t *out) {
    const char *s = getenv(envname);
    if (!s) return 0; // not set -> skip
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (end == s) return -1;
    *out = (rlim_t)v;
    return 1;
}

static void print_rlim(const char *name, int resource) {
    struct rlimit rl;
    if (getrlimit(resource, &rl) == 0) {
        unsigned long long cur = (unsigned long long)rl.rlim_cur;
        unsigned long long max = (unsigned long long)rl.rlim_max;
        fprintf(stderr, "[resources] %s -> soft=%llu hard=%llu\n", name, cur, max);
    } else {
        fprintf(stderr, "[resources] getrlimit(%s) failed: %s\n", name, strerror(errno));
    }
}

int apply_rlimits_from_env(void) {
    int rc = 0;
    rlim_t val;

    // === DEBUG: print detected environment values ===
    const char *val_cpu = getenv("SANDBOX_RLIMIT_CPU");
    const char *val_as = getenv("SANDBOX_RLIMIT_AS");
    const char *val_nofile = getenv("SANDBOX_RLIMIT_NOFILE");
    const char *val_nproc = getenv("SANDBOX_RLIMIT_NPROC");
    fprintf(stderr, "[resources] ENV -> CPU=%s AS=%s NOFILE=%s NPROC=%s\n",
            val_cpu ? val_cpu : "(unset)",
            val_as ? val_as : "(unset)",
            val_nofile ? val_nofile : "(unset)",
            val_nproc ? val_nproc : "(unset)");

    // RLIMIT_AS (virtual memory in bytes)
    if (parse_rlim_from_env("SANDBOX_RLIMIT_AS", &val) == 1) {
        struct rlimit rl = { .rlim_cur = val, .rlim_max = val };
        if (setrlimit(RLIMIT_AS, &rl) != 0) {
            fprintf(stderr, "[resources] setrlimit(RLIMIT_AS) failed: %s\n", strerror(errno));
            rc = -1;
        } else {
            fprintf(stderr, "[resources] Requested RLIMIT_AS = %llu bytes\n", (unsigned long long)val);
        }
        print_rlim("RLIMIT_AS", RLIMIT_AS);
    }

    // RLIMIT_DATA (optional; often reflected by shells)
    if (parse_rlim_from_env("SANDBOX_RLIMIT_DATA", &val) == 1) {
        struct rlimit rl = { .rlim_cur = val, .rlim_max = val };
        if (setrlimit(RLIMIT_DATA, &rl) != 0) {
            fprintf(stderr, "[resources] setrlimit(RLIMIT_DATA) failed: %s\n", strerror(errno));
            rc = -1;
        } else {
            fprintf(stderr, "[resources] Requested RLIMIT_DATA = %llu bytes\n", (unsigned long long)val);
        }
        print_rlim("RLIMIT_DATA", RLIMIT_DATA);
    }

    // CPU time (seconds)
    if (parse_rlim_from_env("SANDBOX_RLIMIT_CPU", &val) == 1) {
        struct rlimit rl = { .rlim_cur = val, .rlim_max = val };
        if (setrlimit(RLIMIT_CPU, &rl) != 0) {
            fprintf(stderr, "[resources] setrlimit(RLIMIT_CPU) failed: %s\n", strerror(errno));
            rc = -1;
        } else {
            fprintf(stderr, "[resources] Requested RLIMIT_CPU = %llu seconds\n", (unsigned long long)val);
        }
        print_rlim("RLIMIT_CPU", RLIMIT_CPU);
    }

    // Number of open files
    if (parse_rlim_from_env("SANDBOX_RLIMIT_NOFILE", &val) == 1) {
        struct rlimit rl = { .rlim_cur = val, .rlim_max = val };
        if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
            fprintf(stderr, "[resources] setrlimit(RLIMIT_NOFILE) failed: %s\n", strerror(errno));
            rc = -1;
        } else {
            fprintf(stderr, "[resources] Requested RLIMIT_NOFILE = %llu\n", (unsigned long long)val);
        }
        print_rlim("RLIMIT_NOFILE", RLIMIT_NOFILE);
    }

    // Number of processes (threads + forks)
    if (parse_rlim_from_env("SANDBOX_RLIMIT_NPROC", &val) == 1) {
        struct rlimit rl = { .rlim_cur = val, .rlim_max = val };
        if (setrlimit(RLIMIT_NPROC, &rl) != 0) {
            fprintf(stderr, "[resources] setrlimit(RLIMIT_NPROC) failed: %s\n", strerror(errno));
            rc = -1;
        } else {
            fprintf(stderr, "[resources] Requested RLIMIT_NPROC = %llu\n", (unsigned long long)val);
        }
        print_rlim("RLIMIT_NPROC", RLIMIT_NPROC);
    }

    // Show a few common ones for sanity
    print_rlim("RLIMIT_STACK", RLIMIT_STACK);
    print_rlim("RLIMIT_CORE", RLIMIT_CORE);

    return rc;
}

int resources_try_setup_cgroup(const char *cgroup_name) {
    (void)cgroup_name;
    fprintf(stderr, "[resources] cgroup setup not implemented here\n");
    return 0;
}
