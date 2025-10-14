//Phase 3 WORK: load policy mode from file
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *load_policy_mode_from_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    char buf[128];
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return NULL; }
    fclose(f);
    // trim
    char *nl = strchr(buf, '\n'); if (nl) *nl = '\0';
    return strdup(buf);
}
