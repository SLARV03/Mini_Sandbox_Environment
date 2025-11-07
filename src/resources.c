//Phase 3 Add resource limits (CPU, memory, file descriptors, process limits).
//setrlimit, getrlimit
// src/resources.c
#define _GNU_SOURCE
#include "resources.h"
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Simple Resource limits manager.
 *
 * This applies POSIX RLIMIT_* limits via setrlimit().
 * These limits are inherited by children, so call these
 * before start_sandbox() so sandbox child inherits them.
 *
 * You can configure limits via environment variables:
 *   SANDBOX_RLIMIT_AS       (bytes, e.g. 536870912 for 512MB)
 *   SANDBOX_RLIMIT_CPU      (seconds)
 *   SANDBOX_RLIMIT_NOFILE   (max open fds)
 *   SANDBOX_RLIMIT_NPROC    (max processes)
 *
 * Returns 0 on success, -1 on error.
 */

static int parse_rlim_from_env(const char *envname, rlim_t *out, int is_bytes) {
    const char *s = getenv(envname);
    if (!s) return 0; // not set -> skip
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (end == s) return -1;
    if (is_bytes == 0) {
        *out = (rlim_t)v;
    } else {
        *out = (rlim_t)v;
    }
    return 1;
}

int apply_rlimits_from_env(void) {
    int rc = 0;
    rlim_t val;

    // Address space (virtual memory) limit in bytes
    if (parse_rlim_from_env("SANDBOX_RLIMIT_AS", &val, 1) == 1) {
        struct rlimit rl = { .rlim_cur = val, .rlim_max = val };
        if (setrlimit(RLIMIT_AS, &rl) != 0) {
            perror("[resources] setrlimit(RLIMIT_AS)");
            rc = -1;
        } else {
            fprintf(stderr, "[resources] Applied RLIMIT_AS = %llu bytes\n", (unsigned long long)val);
        }
    }

    // CPU time (seconds)
    if (parse_rlim_from_env("SANDBOX_RLIMIT_CPU", &val, 0) == 1) {
        struct rlimit rl = { .rlim_cur = val, .rlim_max = val };
        if (setrlimit(RLIMIT_CPU, &rl) != 0) {
            perror("[resources] setrlimit(RLIMIT_CPU)");
            rc = -1;
        } else {
            fprintf(stderr, "[resources] Applied RLIMIT_CPU = %llu seconds\n", (unsigned long long)val);
        }
    }

    // Number of open files
    if (parse_rlim_from_env("SANDBOX_RLIMIT_NOFILE", &val, 0) == 1) {
        struct rlimit rl = { .rlim_cur = val, .rlim_max = val };
        if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
            perror("[resources] setrlimit(RLIMIT_NOFILE)");
            rc = -1;
        } else {
            fprintf(stderr, "[resources] Applied RLIMIT_NOFILE = %llu\n", (unsigned long long)val);
        }
    }

    // Number of processes (threads + forks)
    if (parse_rlim_from_env("SANDBOX_RLIMIT_NPROC", &val, 0) == 1) {
        struct rlimit rl = { .rlim_cur = val, .rlim_max = val };
        if (setrlimit(RLIMIT_NPROC, &rl) != 0) {
            perror("[resources] setrlimit(RLIMIT_NPROC)");
            rc = -1;
        } else {
            fprintf(stderr, "[resources] Applied RLIMIT_NPROC = %llu\n", (unsigned long long)val);
        }
    }

    return rc;
}

/* Optional helper stub for cgroups (Phase 3 extension)
 *
 * Implementing cgroups properly requires writing to /sys/fs/cgroup
 * or using libcgroup / cgcreate tools. This function is a placeholder
 * to show where you could create a dedicated cgroup for the sandbox,
 * configure memory/cpu/pids, and then move the child's PID into it.
 *
 * Return 0 for success (not implemented), -1 otherwise.
 */
int resources_try_setup_cgroup(const char *cgroup_name) {
    (void)cgroup_name;
    // For now, we do not implement cgroup creation here.
    // You can use system("cgcreate ...") or write to /sys/fs/cgroup directly.
    // Return 0 to indicate "not an error", but no action performed.
    fprintf(stderr, "[resources] cgroup setup not implemented in resources_try_setup_cgroup()\n");
    return 0;
}
