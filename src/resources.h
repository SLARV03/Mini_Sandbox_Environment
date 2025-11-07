#ifndef RESOURCES_H
#define RESOURCES_H

#include <sys/types.h>
#include <sys/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

int apply_rlimits_from_env(void);
int resources_try_setup_cgroup(const char *cgroup_name);

#ifdef __cplusplus
}
#endif

#endif // RESOURCES_H
