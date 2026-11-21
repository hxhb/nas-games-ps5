#ifndef NASGAMES_INSTALL_STATE_H
#define NASGAMES_INSTALL_STATE_H

#include "common.h"

typedef struct {
    int title_installed;
    int package_installed;
    int package_known;
    int package_partial;
    int library;
    int homebrew;
    char title_version[64];
} NgInstallState;

int ng_install_state_probe(const NgConfig *config, const char *title_id,
                           NgPkgKind kind, const char *package_version,
                           const char *content_id, NgInstallState *state);
int ng_install_state_find_asset(const char *title_id, const char *name,
                                char *out, size_t out_size);

#endif
