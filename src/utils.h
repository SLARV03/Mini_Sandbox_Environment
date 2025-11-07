#ifndef UTILS_H
#define UTILS_H

/* Load the policy mode string (e.g. "open", "restricted", "locked")
 * from the given path. Returns a heap-allocated string which must be freed
 * by the caller, or NULL on error.
 */
char *load_policy_mode_from_file(const char *path);

#endif /* UTILS_H */
