#ifndef NASGAMES_PKG_META_H
#define NASGAMES_PKG_META_H

#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "source.h"

typedef struct {
    char title_id[16];
    char title_name[NG_TITLE_LEN];
    char version[64];
    char category[32];
    char content_id[64];
    NgPkgKind kind;
    uint64_t icon_offset;
    uint32_t icon_size;
    uint64_t pic0_offset;
    uint32_t pic0_size;
    uint64_t pic1_offset;
    uint32_t pic1_size;
    int has_icon;
    int has_pic0;
    int has_pic1;
} NgPkgMeta;

int ng_app_meta_from_json(const char *json, NgPkgMeta *meta);
int ng_app_meta_from_sfo(const void *data, size_t size, NgPkgMeta *meta);
int ng_pkg_probe(NgSource *source, const char *path, uint64_t file_size,
                 NgPkgMeta *meta, char *error, size_t error_size);
int ng_pkg_read_range(NgSource *source, const char *path, uint64_t offset,
                      uint32_t size, void **out, size_t *out_size);

#endif
