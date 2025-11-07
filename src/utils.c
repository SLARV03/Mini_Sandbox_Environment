#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Reads the sandbox policy mode from a file.
 * Expected file contents: one line containing "open", "restricted", or "locked".
 * Example path: /etc/sandbox_policy
 *
 * Returns: dynamically allocated string (must be freed by caller)
 *          or NULL on error.
 */
char *load_policy_mode_from_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;

    char buf[128];
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return NULL;
    }
    fclose(f);

    buf[strcspn(buf, "\n")] = '\0'; // remove newline if present
    return strdup(buf); // caller must free()
}
