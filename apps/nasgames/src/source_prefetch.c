#include "source_prefetch.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

typedef struct {
    uint8_t *data;
    size_t length;
    size_t position;
} PrefetchSlot;

struct NgPrefetcher {
    NgSourceFile *file;
    PrefetchSlot slots[NG_PREFETCH_SLOT_COUNT];
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t ready;
    size_t head;
    size_t tail;
    size_t count;
    int enabled;
    int thread_started;
    int eof;
    int error;
    int stop;
    uint64_t remaining;
};

int
ng_prefetch_enabled(uint64_t file_size)
{
    return file_size >= NG_PREFETCH_THRESHOLD;
}

size_t
ng_prefetch_chunk_size(const NgPrefetcher *prefetch)
{
    return prefetch && prefetch->enabled
               ? NG_PREFETCH_SLOT_BYTES
               : 1024u * 1024u;
}

int
ng_prefetch_is_active(const NgPrefetcher *prefetch)
{
    return prefetch && prefetch->enabled && prefetch->thread_started;
}

static void *
prefetch_main(void *opaque)
{
    NgPrefetcher *prefetch = opaque;

    for (;;) {
        PrefetchSlot *slot;
        size_t length = 0;
        int failed = 0;
        int eof = 0;

        pthread_mutex_lock(&prefetch->lock);
        while (prefetch->count == NG_PREFETCH_SLOT_COUNT && !prefetch->stop) {
            pthread_cond_wait(&prefetch->ready, &prefetch->lock);
        }
        if (prefetch->stop) {
            pthread_mutex_unlock(&prefetch->lock);
            break;
        }
        slot = &prefetch->slots[prefetch->tail];
        pthread_mutex_unlock(&prefetch->lock);

        while (length < NG_PREFETCH_SLOT_BYTES) {
            size_t want = NG_PREFETCH_SLOT_BYTES - length;
            ssize_t n;

            if (prefetch->remaining == 0) {
                eof = 1;
                break;
            }
            if (prefetch->remaining < want) want = (size_t)prefetch->remaining;
            n = ng_source_file_read(prefetch->file, slot->data + length, want);
            if (n < 0) {
                failed = 1;
                break;
            }
            if (n == 0) {
                eof = 1;
                break;
            }
            length += (size_t)n;
            prefetch->remaining -= (uint64_t)n;
            if (prefetch->remaining == 0 ||
                prefetch->file->offset >= prefetch->file->size) {
                eof = 1;
                break;
            }
        }

        pthread_mutex_lock(&prefetch->lock);
        if (prefetch->stop) {
            pthread_mutex_unlock(&prefetch->lock);
            break;
        }
        if (length > 0) {
            slot->length = length;
            slot->position = 0;
            prefetch->tail = (prefetch->tail + 1) % NG_PREFETCH_SLOT_COUNT;
            prefetch->count++;
        }
        if (failed) prefetch->error = 1;
        if (eof || failed) prefetch->eof = 1;
        pthread_cond_broadcast(&prefetch->ready);
        pthread_mutex_unlock(&prefetch->lock);
        if (failed || eof) break;
    }
    return NULL;
}

NgPrefetcher *
ng_prefetch_create(NgSourceFile *file, uint64_t limit)
{
    NgPrefetcher *prefetch;

    if (!file) return NULL;
    prefetch = calloc(1, sizeof(*prefetch));
    if (!prefetch) return NULL;
    prefetch->file = file;
    prefetch->remaining = limit;
    if (pthread_mutex_init(&prefetch->lock, NULL) != 0) {
        free(prefetch);
        return NULL;
    }
    if (pthread_cond_init(&prefetch->ready, NULL) != 0) {
        pthread_mutex_destroy(&prefetch->lock);
        free(prefetch);
        return NULL;
    }
    if (limit < 2u * NG_PREFETCH_SLOT_BYTES || !ng_prefetch_enabled(file->size)) return prefetch;

    for (size_t i = 0; i < NG_PREFETCH_SLOT_COUNT; i++) {
        if (posix_memalign((void **)&prefetch->slots[i].data, 4096,
                           NG_PREFETCH_SLOT_BYTES) != 0) {
            prefetch->slots[i].data = NULL;
        }
        if (!prefetch->slots[i].data) {
            ng_log_info("预读缓冲不足，%s 使用直接读取",
                        file->size ? "当前文件" : "文件");
            return prefetch;
        }
    }
    if (pthread_create(&prefetch->thread, NULL, prefetch_main, prefetch) != 0) {
        ng_log_info("无法创建预读线程，%s 使用直接读取", "当前文件");
        return prefetch;
    }
    prefetch->enabled = 1;
    prefetch->thread_started = 1;
    return prefetch;
}

ssize_t
ng_prefetch_peek(NgPrefetcher *prefetch, const void **data)
{
    PrefetchSlot *slot;

    if (!ng_prefetch_is_active(prefetch) || !data) return -1;
    pthread_mutex_lock(&prefetch->lock);
    while (prefetch->count == 0 && !prefetch->eof && !prefetch->stop) {
        pthread_cond_wait(&prefetch->ready, &prefetch->lock);
    }
    if (prefetch->count == 0) {
        int error = prefetch->error;
        pthread_mutex_unlock(&prefetch->lock);
        if (error) return -1;
        return 0;
    }
    slot = &prefetch->slots[prefetch->head];
    *data = slot->data + slot->position;
    {
        ssize_t available = (ssize_t)(slot->length - slot->position);
        pthread_mutex_unlock(&prefetch->lock);
        return available;
    }
}

void
ng_prefetch_advance(NgPrefetcher *prefetch, size_t size)
{
    PrefetchSlot *slot;

    if (!ng_prefetch_is_active(prefetch) || size == 0) return;
    pthread_mutex_lock(&prefetch->lock);
    if (prefetch->count > 0) {
        slot = &prefetch->slots[prefetch->head];
        if (size > slot->length - slot->position) {
            size = slot->length - slot->position;
        }
        slot->position += size;
        if (slot->position >= slot->length) {
            prefetch->head = (prefetch->head + 1) % NG_PREFETCH_SLOT_COUNT;
            prefetch->count--;
            pthread_cond_broadcast(&prefetch->ready);
        }
    }
    pthread_mutex_unlock(&prefetch->lock);
}

void
ng_prefetch_destroy(NgPrefetcher *prefetch)
{
    if (!prefetch) return;
    if (prefetch->thread_started) {
        pthread_mutex_lock(&prefetch->lock);
        prefetch->stop = 1;
        pthread_cond_broadcast(&prefetch->ready);
        pthread_mutex_unlock(&prefetch->lock);
        pthread_join(prefetch->thread, NULL);
    }
    for (size_t i = 0; i < NG_PREFETCH_SLOT_COUNT; i++) {
        free(prefetch->slots[i].data);
    }
    pthread_cond_destroy(&prefetch->ready);
    pthread_mutex_destroy(&prefetch->lock);
    free(prefetch);
}
