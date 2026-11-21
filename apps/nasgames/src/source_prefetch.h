#ifndef NASGAMES_SOURCE_PREFETCH_H
#define NASGAMES_SOURCE_PREFETCH_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "source.h"

#define NG_PREFETCH_THRESHOLD (256ull * 1024ull * 1024ull)
#define NG_PREFETCH_SLOT_BYTES (8u * 1024u * 1024u)
#define NG_PREFETCH_SLOT_COUNT 3

typedef struct NgPrefetcher NgPrefetcher;

int ng_prefetch_enabled(uint64_t file_size);
int ng_prefetch_is_active(const NgPrefetcher *prefetch);
size_t ng_prefetch_chunk_size(const NgPrefetcher *prefetch);
NgPrefetcher *ng_prefetch_create(NgSourceFile *file, uint64_t limit);
ssize_t ng_prefetch_peek(NgPrefetcher *prefetch, const void **data);
void ng_prefetch_advance(NgPrefetcher *prefetch, size_t size);
void ng_prefetch_destroy(NgPrefetcher *prefetch);

#endif
