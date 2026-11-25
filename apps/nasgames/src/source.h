#ifndef NASGAMES_SOURCE_H
#define NASGAMES_SOURCE_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "common.h"

typedef struct {
    char name[NG_FILE_NAME_LEN];
    int is_dir;
    uint64_t size;
    uint64_t mtime;
} NgSourceEntry;

typedef struct {
    NgSourceEntry *items;
    size_t count;
    size_t capacity;
} NgSourceList;

#define NG_MAX_SHARES 64

typedef struct {
    char name[NG_NAME_LEN];
    char remark[NG_TITLE_LEN];
    uint32_t type;
} NgShareEntry;

typedef struct {
    NgShareEntry items[NG_MAX_SHARES];
    size_t count;
} NgShareList;

typedef struct NgSource NgSource;
typedef struct NgSourceFile NgSourceFile;

typedef struct {
    int (*connect)(NgSource *source);
    void (*disconnect)(NgSource *source);
    int (*list)(NgSource *source, const char *path, NgSourceList *list);
    int (*stat)(NgSource *source, const char *path, NgSourceEntry *entry);
    void *(*file_open)(NgSource *source, const char *path, uint64_t *size);
    ssize_t (*file_read)(NgSource *source, void *handle, void *buf, size_t size);
    int (*file_seek)(NgSource *source, void *handle, uint64_t offset);
    void (*file_set_read_limit)(NgSource *source, void *handle, uint64_t limit);
    void (*file_close)(NgSource *source, void *handle);
} NgSourceOps;

struct NgSource {
    NgSourceConfig config;
    const NgSourceOps *ops;
    void *priv;
    pthread_mutex_t io_lock;
    pthread_mutex_t lock;
    pthread_mutex_t ref_lock;
    int refcount;
    int connected;
    char status[32];
    char error[NG_ERROR_LEN];
};

struct NgSourceFile {
    NgSource *source;
    void *handle;
    uint64_t size;
    uint64_t offset;
};

NgSource *ng_source_create(const NgSourceConfig *config);
void ng_source_acquire(NgSource *source);
void ng_source_release(NgSource *source);
void ng_source_list_init(NgSourceList *list);
void ng_source_list_free(NgSourceList *list);
int ng_source_list_add(NgSourceList *list, const NgSourceEntry *entry);
int ng_source_connect(NgSource *source);
int ng_source_list_dir(NgSource *source, const char *path, NgSourceList *list);
int ng_source_stat(NgSource *source, const char *path, NgSourceEntry *entry);
int ng_source_child_path(char *out, size_t out_size, const char *parent, const char *name);
int ng_source_measure_tree(NgSource *source, const char *path,
                           uint64_t *bytes, uint64_t *files, int (*canceled)(void));
void ng_source_set_error(NgSource *source, const char *format, ...);
void ng_source_error_snapshot(NgSource *source, const char *fallback,
                              char *out, size_t out_size);
void ng_source_reset_connection(NgSource *source);
int ng_source_list_shares(const NgSourceConfig *config, NgShareList *shares,
                          char *error, size_t error_size);
int ng_source_read_all(NgSource *source, const char *path, size_t max_bytes,
                       char **out, size_t *out_size);
NgSourceFile *ng_source_open_file(NgSource *source, const char *path);
ssize_t ng_source_file_read(NgSourceFile *file, void *buf, size_t size);
int ng_source_file_seek(NgSourceFile *file, uint64_t offset);
void ng_source_file_set_read_limit(NgSourceFile *file, uint64_t limit);
void ng_source_file_close(NgSourceFile *file);

#endif
