#include "source.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "util.h"

typedef struct {
    char root[NG_PATH_LEN];
} LocalPriv;

typedef struct {
    FILE *fp;
} LocalFile;

static int
local_join(NgSource *source, const char *path, char *out, size_t out_size)
{
    LocalPriv *priv = source->priv;
    return ng_path_join_local(out, out_size, priv->root, path);
}

static int
local_connect(NgSource *source)
{
    char path[NG_PATH_LEN];
    DIR *dir;

    if (local_join(source, "/", path, sizeof(path)) != 0) return -1;
    dir = opendir(path);
    if (!dir) return -1;
    closedir(dir);
    return 0;
}

static void
local_disconnect(NgSource *source)
{
    (void)source;
}

static int
local_list(NgSource *source, const char *path, NgSourceList *list)
{
    char local_path[NG_PATH_LEN];
    DIR *dir;
    struct dirent *de;
    int rc = 0;

    if (local_join(source, path, local_path, sizeof(local_path)) != 0) return -1;
    dir = opendir(local_path);
    if (!dir) return -1;
    while ((de = readdir(dir)) != NULL) {
        char full[NG_PATH_LEN];
        struct stat st;
        NgSourceEntry entry;
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        if (strlen(de->d_name) >= sizeof(entry.name) ||
            snprintf(full, sizeof(full), "%s/%s", local_path, de->d_name) >= (int)sizeof(full)) {
            errno = ENAMETOOLONG;
            rc = -1;
            break;
        }
        if (lstat(full, &st) != 0 || (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))) continue;
        memset(&entry, 0, sizeof(entry));
        ng_strlcpy(entry.name, de->d_name, sizeof(entry.name));
        entry.is_dir = S_ISDIR(st.st_mode);
        entry.size = (uint64_t)st.st_size;
        entry.mtime = (uint64_t)st.st_mtime;
        if (ng_source_list_add(list, &entry) != 0) {
            rc = -1;
            break;
        }
    }
    closedir(dir);
    return rc;
}

static int
local_stat(NgSource *source, const char *path, NgSourceEntry *entry)
{
    char local_path[NG_PATH_LEN];
    struct stat st;

    if (local_join(source, path, local_path, sizeof(local_path)) != 0 || stat(local_path, &st) != 0) {
        return -1;
    }
    memset(entry, 0, sizeof(*entry));
    ng_strlcpy(entry->name, ng_path_basename(local_path), sizeof(entry->name));
    entry->is_dir = S_ISDIR(st.st_mode);
    entry->size = (uint64_t)st.st_size;
    entry->mtime = (uint64_t)st.st_mtime;
    return 0;
}

static void *
local_file_open(NgSource *source, const char *path, uint64_t *size)
{
    char local_path[NG_PATH_LEN];
    struct stat st;
    LocalFile *file;
    FILE *fp;

    if (local_join(source, path, local_path, sizeof(local_path)) != 0) return NULL;
    if (stat(local_path, &st) != 0 || !S_ISREG(st.st_mode)) {
        errno = EISDIR;
        return NULL;
    }
    fp = fopen(local_path, "rb");
    if (!fp) return NULL;
    file = calloc(1, sizeof(*file));
    if (!file) {
        fclose(fp);
        return NULL;
    }
    file->fp = fp;
    *size = (uint64_t)st.st_size;
    return file;
}

static ssize_t
local_file_read(NgSource *source, void *handle, void *buf, size_t size)
{
    LocalFile *file = handle;
    size_t n;
    (void)source;
    n = fread(buf, 1, size, file->fp);
    if (n == 0 && ferror(file->fp)) return -1;
    return (ssize_t)n;
}

static int
local_file_seek(NgSource *source, void *handle, uint64_t offset)
{
    LocalFile *file = handle;
    (void)source;
    return fseeko(file->fp, (off_t)offset, SEEK_SET);
}

static void
local_file_close(NgSource *source, void *handle)
{
    LocalFile *file = handle;
    (void)source;
    fclose(file->fp);
    free(file);
}

static const NgSourceOps local_ops = {
    .connect = local_connect,
    .disconnect = local_disconnect,
    .list = local_list,
    .stat = local_stat,
    .file_open = local_file_open,
    .file_read = local_file_read,
    .file_seek = local_file_seek,
    .file_close = local_file_close,
};

NgSource *
ng_source_local_create(const NgSourceConfig *config)
{
    NgSource *source = calloc(1, sizeof(*source));
    LocalPriv *priv = calloc(1, sizeof(*priv));

    if (!source || !priv) {
        free(source);
        free(priv);
        return NULL;
    }
    source->config = *config;
    source->ops = &local_ops;
    source->priv = priv;
    ng_strlcpy(priv->root, config->local_root, sizeof(priv->root));
    ng_strlcpy(source->status, "offline", sizeof(source->status));
    return source;
}
