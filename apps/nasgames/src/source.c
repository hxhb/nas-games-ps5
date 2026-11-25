#include "source.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "util.h"

#ifdef NASGAMES_WITH_SMB
NgSource *ng_source_smb_create(const NgSourceConfig *config);
int ng_source_smb_list_shares(const NgSourceConfig *config, NgShareList *shares,
                              char *error, size_t error_size);
#endif
NgSource *ng_source_ftp_create(const NgSourceConfig *config);
NgSource *ng_source_local_create(const NgSourceConfig *config);

NgSource *
ng_source_create(const NgSourceConfig *config)
{
    NgSource *source = NULL;

    if (!config) return NULL;
    switch (config->type) {
    case NG_SOURCE_LOCAL:
        source = ng_source_local_create(config);
        break;
    case NG_SOURCE_FTP:
        source = ng_source_ftp_create(config);
        break;
#ifdef NASGAMES_WITH_SMB
    case NG_SOURCE_SMB:
        source = ng_source_smb_create(config);
        break;
#endif
    default:
        break;
    }
    if (!source) return NULL;
    if (pthread_mutex_init(&source->io_lock, NULL) != 0) goto fail;
    if (pthread_mutex_init(&source->lock, NULL) != 0) goto destroy_io_lock;
    if (pthread_mutex_init(&source->ref_lock, NULL) != 0) goto destroy_lock;
    source->refcount = 1;
    return source;

destroy_lock:
    pthread_mutex_destroy(&source->lock);
destroy_io_lock:
    pthread_mutex_destroy(&source->io_lock);
fail:
    free(source->priv);
    free(source);
    return NULL;
}

static void
source_free(NgSource *source)
{
    if (!source) return;
    pthread_mutex_lock(&source->io_lock);
    if (source->connected && source->ops && source->ops->disconnect) {
        source->ops->disconnect(source);
    }
    source->connected = 0;
    pthread_mutex_unlock(&source->io_lock);
    pthread_mutex_destroy(&source->ref_lock);
    pthread_mutex_destroy(&source->lock);
    pthread_mutex_destroy(&source->io_lock);
    free(source->priv);
    free(source);
}

void
ng_source_acquire(NgSource *source)
{
    if (!source) return;
    pthread_mutex_lock(&source->ref_lock);
    source->refcount++;
    pthread_mutex_unlock(&source->ref_lock);
}

void
ng_source_release(NgSource *source)
{
    int free_source = 0;

    if (!source) return;
    pthread_mutex_lock(&source->ref_lock);
    source->refcount--;
    free_source = source->refcount == 0;
    pthread_mutex_unlock(&source->ref_lock);
    if (free_source) source_free(source);
}

void
ng_source_set_error(NgSource *source, const char *format, ...)
{
    va_list args;

    pthread_mutex_lock(&source->lock);
    va_start(args, format);
    vsnprintf(source->error, sizeof(source->error), format, args);
    va_end(args);
    pthread_mutex_unlock(&source->lock);
}

void
ng_source_error_snapshot(NgSource *source, const char *fallback,
                         char *out, size_t out_size)
{
    pthread_mutex_lock(&source->lock);
    ng_strlcpy(out, source->error[0] ? source->error : fallback, out_size);
    pthread_mutex_unlock(&source->lock);
}

void
ng_source_reset_connection(NgSource *source)
{
    if (!source) return;
    pthread_mutex_lock(&source->io_lock);
    if (source->connected && source->ops && source->ops->disconnect) {
        source->ops->disconnect(source);
    }
    source->connected = 0;
    pthread_mutex_unlock(&source->io_lock);
}

void
ng_source_list_init(NgSourceList *list)
{
    memset(list, 0, sizeof(*list));
}

void
ng_source_list_free(NgSourceList *list)
{
    free(list->items);
    memset(list, 0, sizeof(*list));
}

int
ng_source_list_add(NgSourceList *list, const NgSourceEntry *entry)
{
    NgSourceEntry *items;
    size_t cap;

    if (list->count == list->capacity) {
        cap = list->capacity ? list->capacity * 2 : 64;
        items = realloc(list->items, cap * sizeof(*items));
        if (!items) return -1;
        list->items = items;
        list->capacity = cap;
    }
    list->items[list->count++] = *entry;
    return 0;
}

static int
source_connect_locked(NgSource *source)
{
    int rc;

    if (source->connected) return 0;
    pthread_mutex_lock(&source->lock);
    ng_strlcpy(source->status, "connecting", sizeof(source->status));
    source->error[0] = '\0';
    pthread_mutex_unlock(&source->lock);
    rc = source->ops->connect ? source->ops->connect(source) : 0;
    if (rc != 0) {
        source->connected = 0;
        pthread_mutex_lock(&source->lock);
        ng_strlcpy(source->status, "error", sizeof(source->status));
        if (!source->error[0]) {
            ng_strlcpy(source->error, strerror(errno ? errno : EIO), sizeof(source->error));
        }
        char message[NG_ERROR_LEN];
        ng_strlcpy(message, source->error, sizeof(message));
        pthread_mutex_unlock(&source->lock);
        ng_log_error("source %s: %s", source->config.id, message);
        return -1;
    }
    source->connected = 1;
    pthread_mutex_lock(&source->lock);
    ng_strlcpy(source->status, "online", sizeof(source->status));
    source->error[0] = '\0';
    pthread_mutex_unlock(&source->lock);
    return 0;
}

int
ng_source_connect(NgSource *source)
{
    int rc;
    if (!source) return -1;
    pthread_mutex_lock(&source->io_lock);
    rc = source_connect_locked(source);
    pthread_mutex_unlock(&source->io_lock);
    return rc;
}

int
ng_source_list_dir(NgSource *source, const char *path, NgSourceList *list)
{
    char norm[NG_PATH_LEN];
    int rc;

    if (!source || !list) return -1;
    if (ng_path_normalize(path, norm, sizeof(norm)) != 0) return -1;
    pthread_mutex_lock(&source->io_lock);
    if (source_connect_locked(source) != 0) {
        pthread_mutex_unlock(&source->io_lock);
        return -1;
    }
    rc = source->ops->list(source, norm, list);
    pthread_mutex_lock(&source->lock);
    if (rc == 0) {
        ng_strlcpy(source->status, "online", sizeof(source->status));
        source->error[0] = '\0';
    } else {
        ng_strlcpy(source->status, "error", sizeof(source->status));
        if (!source->error[0]) {
            ng_strlcpy(source->error, "目录读取失败", sizeof(source->error));
        }
    }
    pthread_mutex_unlock(&source->lock);
    pthread_mutex_unlock(&source->io_lock);
    return rc;
}

int
ng_source_stat(NgSource *source, const char *path, NgSourceEntry *entry)
{
    char norm[NG_PATH_LEN];
    int rc;

    if (!source || !entry) return -1;
    if (ng_path_normalize(path, norm, sizeof(norm)) != 0) return -1;
    pthread_mutex_lock(&source->io_lock);
    if (source_connect_locked(source) != 0) {
        pthread_mutex_unlock(&source->io_lock);
        return -1;
    }
    rc = source->ops->stat(source, norm, entry);
    pthread_mutex_unlock(&source->io_lock);
    return rc;
}

int
ng_source_child_path(char *out, size_t out_size, const char *parent, const char *name)
{
    if (!name || !name[0] || strchr(name, '/') || strchr(name, '\\') ||
        !strcmp(name, ".") || !strcmp(name, "..")) {
        errno = EINVAL;
        return -1;
    }
    const char *separator = !strcmp(parent, "/") ? "" : "/";
    if (snprintf(out, out_size, "%s%s%s", parent, separator, name) >= (int)out_size) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

static int
measure_tree_depth(NgSource *source, const char *path, int depth,
                   uint64_t *bytes, uint64_t *files, int (*canceled)(void))
{
    NgSourceList list;
    int rc = 0;

    if (depth > 64) {
        errno = ELOOP;
        return -1;
    }
    if (canceled && canceled()) {
        errno = ECANCELED;
        return -1;
    }
    ng_source_list_init(&list);
    rc = ng_source_list_dir(source, path, &list);
    if (rc != 0) goto done;
    for (size_t i = 0; i < list.count; i++) {
        char child[NG_PATH_LEN];
        if (ng_source_child_path(child, sizeof(child), path, list.items[i].name) != 0) {
            rc = -1;
            break;
        }
        if (list.items[i].is_dir) {
            if (measure_tree_depth(source, child, depth + 1, bytes, files, canceled) != 0) {
                rc = -1;
                break;
            }
        } else {
            if (list.items[i].size > UINT64_MAX - *bytes) {
                errno = EOVERFLOW;
                rc = -1;
                break;
            }
            *bytes += list.items[i].size;
            if (files) (*files)++;
        }
    }
done:
    ng_source_list_free(&list);
    return rc;
}

int
ng_source_measure_tree(NgSource *source, const char *path,
                       uint64_t *bytes, uint64_t *files, int (*canceled)(void))
{
    if (!source || !path || !bytes) return -1;
    return measure_tree_depth(source, path, 0, bytes, files, canceled);
}

int
ng_source_list_shares(const NgSourceConfig *config, NgShareList *shares,
                      char *error, size_t error_size)
{
    if (!config || !shares) return -1;
    memset(shares, 0, sizeof(*shares));
    if (config->type != NG_SOURCE_SMB) {
        snprintf(error, error_size, "共享枚举仅支持 SMB");
        return -1;
    }
#ifdef NASGAMES_WITH_SMB
    return ng_source_smb_list_shares(config, shares, error, error_size);
#else
    snprintf(error, error_size, "当前构建未包含 SMB 支持");
    return -1;
#endif
}

NgSourceFile *
ng_source_open_file(NgSource *source, const char *path)
{
    char norm[NG_PATH_LEN];
    NgSourceFile *file;
    uint64_t size = 0;
    void *handle;

    if (!source || !path || !path[0]) return NULL;
    if (ng_path_normalize(path, norm, sizeof(norm)) != 0) return NULL;
    file = calloc(1, sizeof(*file));
    if (!file) return NULL;
    pthread_mutex_lock(&source->io_lock);
    if (source_connect_locked(source) != 0) {
        pthread_mutex_unlock(&source->io_lock);
        free(file);
        return NULL;
    }
    handle = source->ops->file_open(source, norm, &size);
    if (!handle) {
        pthread_mutex_unlock(&source->io_lock);
        free(file);
        return NULL;
    }
    ng_source_acquire(source);
    file->source = source;
    file->handle = handle;
    file->size = size;
    pthread_mutex_unlock(&source->io_lock);
    return file;
}

ssize_t
ng_source_file_read(NgSourceFile *file, void *buf, size_t size)
{
    ssize_t rc;
    if (!file || !file->handle) return -1;
    pthread_mutex_lock(&file->source->io_lock);
    rc = file->source->ops->file_read(file->source, file->handle, buf, size);
    pthread_mutex_unlock(&file->source->io_lock);
    if (rc > 0) file->offset += (uint64_t)rc;
    return rc;
}

int
ng_source_file_seek(NgSourceFile *file, uint64_t offset)
{
    int rc;
    if (!file || !file->handle) return -1;
    pthread_mutex_lock(&file->source->io_lock);
    rc = file->source->ops->file_seek(file->source, file->handle, offset);
    pthread_mutex_unlock(&file->source->io_lock);
    if (rc == 0) file->offset = offset;
    return rc;
}

void
ng_source_file_set_read_limit(NgSourceFile *file, uint64_t limit)
{
    if (!file || !file->handle || !file->source->ops->file_set_read_limit) return;
    pthread_mutex_lock(&file->source->io_lock);
    file->source->ops->file_set_read_limit(file->source, file->handle, limit);
    pthread_mutex_unlock(&file->source->io_lock);
}

void
ng_source_file_close(NgSourceFile *file)
{
    NgSource *source;

    if (!file) return;
    pthread_mutex_lock(&file->source->io_lock);
    if (file->handle && file->source->ops->file_close) {
        file->source->ops->file_close(file->source, file->handle);
    }
    pthread_mutex_unlock(&file->source->io_lock);
    source = file->source;
    free(file);
    ng_source_release(source);
}

int
ng_source_read_all(NgSource *source, const char *path, size_t max_bytes,
                   char **out, size_t *out_size)
{
    NgSourceFile *file;
    char *buf;
    size_t cap;
    size_t len = 0;

    *out = NULL;
    if (out_size) *out_size = 0;
    file = ng_source_open_file(source, path);
    if (!file) return -1;
    if (file->size && file->size > max_bytes) {
        ng_source_file_close(file);
        errno = EFBIG;
        return -1;
    }
    cap = file->size ? (size_t)file->size + 1 : 65536;
    if (cap > max_bytes + 1) cap = max_bytes + 1;
    buf = malloc(cap);
    if (!buf) {
        ng_source_file_close(file);
        return -1;
    }
    while (len < max_bytes) {
        ssize_t n;
        if (len + 1 >= cap) {
            size_t next = cap * 2;
            char *tmp;
            if (next > max_bytes + 1) next = max_bytes + 1;
            if (next <= cap) break;
            tmp = realloc(buf, next);
            if (!tmp) {
                free(buf);
                ng_source_file_close(file);
                return -1;
            }
            buf = tmp;
            cap = next;
        }
        n = ng_source_file_read(file, buf + len, cap - len - 1);
        if (n < 0) {
            free(buf);
            ng_source_file_close(file);
            return -1;
        }
        if (n == 0) break;
        len += (size_t)n;
    }
    if (file->size && len != file->size) {
        ng_source_file_close(file);
        free(buf);
        errno = EIO;
        return -1;
    }
    if (!file->size && len == max_bytes) {
        char extra;
        ssize_t count = ng_source_file_read(file, &extra, 1);
        if (count != 0) {
            int error = count > 0 ? EFBIG : errno;
            ng_source_file_close(file);
            free(buf);
            errno = error;
            return -1;
        }
    }
    ng_source_file_close(file);
    buf[len] = '\0';
    *out = buf;
    if (out_size) *out_size = len;
    return (int)len;
}
