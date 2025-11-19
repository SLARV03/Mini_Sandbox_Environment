#ifndef SANDBOX_SECCOMP_H
#define SANDBOX_SECCOMP_H

/* Apply seccomp filter.
 * mode: "open" | "restricted" | "locked"
 * Returns 0 on success, -1 on error.
 */
int apply_seccomp_filter(const char *mode);

#endif /* SANDBOX_SECCOMP_H */
