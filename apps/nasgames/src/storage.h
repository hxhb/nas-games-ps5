#ifndef NASGAMES_STORAGE_H
#define NASGAMES_STORAGE_H

#include <stdint.h>

typedef struct {
    int known;
    uint64_t free_bytes;
    uint64_t total_bytes;
} NgStorageInfo;

int ng_storage_probe(const char *path, NgStorageInfo *out);

#endif
