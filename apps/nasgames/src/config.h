#ifndef NASGAMES_CONFIG_H
#define NASGAMES_CONFIG_H

#include <stddef.h>

#include "common.h"
#include "util.h"

void ng_config_defaults(NgConfig *cfg, const char *data_dir);
int ng_config_load(NgConfig *cfg, const char *path);
int ng_config_save(const NgConfig *cfg, const char *path);
int ng_config_to_json(const NgConfig *cfg, int include_secrets, NgStrBuf *out);
int ng_config_from_json(const char *json, const NgConfig *base, NgConfig *out,
                        char *error, size_t error_size);
int ng_config_source_from_json(const char *json, const NgConfig *base,
                               NgSourceConfig *out, char *error,
                               size_t error_size);
NgSourceConfig *ng_config_find_source(NgConfig *cfg, const char *id);
const NgSourceConfig *ng_config_find_source_const(const NgConfig *cfg, const char *id);

#endif
