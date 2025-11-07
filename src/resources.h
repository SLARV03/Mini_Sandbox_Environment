// src/resources.h
#ifndef RESOURCES_H
#define RESOURCES_H

#include <sys/types.h>   // ✅ Add this line (for pid_t)

#ifdef __cplusplus
extern "C" {
#endif

int apply_rlimits_from_env(void);
int resources_try_setup_cgroup(const char *cgroup_name);
int resources_move_pid_to_cgroup(const char *cgroup_name, pid_t pid); // your new function

#ifdef __cplusplus
}
#endif

#endif // RESOURCES_H
