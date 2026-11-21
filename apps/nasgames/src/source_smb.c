#include "source.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#include <smb2/libsmb2-raw.h>
#include <smb2/libsmb2-dcerpc-srvsvc.h>
#include <smb2/smb2-errors.h>

#include "log.h"
#include "util.h"

#define SMB_READ_INFLIGHT 8
#define SMB_READ_CHUNK (1024u * 1024u)
#define SMB_TIMEOUT_SECONDS 12
#define SMB_SOCKET_BUFFER (512u * 1024u)
#define SMB_TRANSFER_LOG_BYTES (64ull * 1024ull * 1024ull)
#define SMB_PIPELINE_THRESHOLD (256ull * 1024ull * 1024ull)

typedef struct {
    struct smb2_context *ctx;
    uint64_t generation;
    char server[384];
    int socket_rcvbuf;
    int socket_sndbuf;
} SmbPriv;

static pthread_mutex_t smb_contexts_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct SmbFile SmbFile;

typedef struct {
    SmbFile *file;
    size_t slot;
} SmbReadJob;

typedef struct {
    int done;
    int status;
    void *data;
} SmbResult;

struct SmbFile {
    struct smb2fh *fh;
    struct smb2_context *ctx;
    uint64_t generation;
    uint64_t size;
    uint64_t read_offset;
    uint64_t next_submit;
    uint64_t bytes_read;
    uint64_t opened_ms;
    uint64_t reads_submitted;
    uint32_t chunk;
    uint16_t dialect;
    int socket_rcvbuf;
    int socket_sndbuf;
    int pipeline_enabled;
    int pipeline_failed;
    size_t inflight;
    size_t window;
    size_t head;
    size_t count;
    uint8_t *slot_buf[SMB_READ_INFLIGHT];
    uint32_t slot_want[SMB_READ_INFLIGHT];
    uint32_t slot_got[SMB_READ_INFLIGHT];
    uint32_t slot_used[SMB_READ_INFLIGHT];
    int slot_status[SMB_READ_INFLIGHT];
    int slot_done[SMB_READ_INFLIGHT];
    uint64_t slot_offset[SMB_READ_INFLIGHT];
    char path[NG_PATH_LEN];
};

static void smb_set_error(NgSource *source, const char *prefix);
static void smb_reset(NgSource *source);

static struct smb2_context *
smb_context_create(void)
{
    struct smb2_context *context;

    pthread_mutex_lock(&smb_contexts_lock);
    context = smb2_init_context();
    pthread_mutex_unlock(&smb_contexts_lock);
    return context;
}

static void
smb_context_destroy(struct smb2_context *context)
{
    pthread_mutex_lock(&smb_contexts_lock);
    smb2_destroy_context(context);
    pthread_mutex_unlock(&smb_contexts_lock);
}

static int
smb_wait_events(NgSource *source, uint64_t deadline)
{
    SmbPriv *priv = source->priv;
    struct pollfd event = {smb2_get_fd(priv->ctx), (short)smb2_which_events(priv->ctx), 0};

    if (event.fd >= 0 && ng_now_monotonic_ms() < deadline) {
        int ready = poll(&event, 1, 1000);
        if (ready < 0 && errno == EINTR) return 0;
        if (ready >= 0 && !(event.revents & POLLNVAL) &&
            smb2_service(priv->ctx, event.revents) == 0) return 0;
        if (ready >= 0) errno = ECONNRESET;
    } else {
        errno = event.fd < 0 ? ENOTCONN : ETIMEDOUT;
    }
    smb_set_error(source, "SMB 连接中断");
    smb_reset(source);
    return -1;
}

static void
smb_command_done(struct smb2_context *ctx, int status, void *data, void *opaque)
{
    SmbResult *result = opaque;
    (void)ctx;

    result->done = 1;
    result->status = status;
    result->data = data;
}

static int
smb_wait_result(NgSource *source, SmbResult *result, int submitted)
{
    uint64_t deadline = ng_now_monotonic_ms() + SMB_TIMEOUT_SECONDS * 1000u;

    if (submitted < 0) return submitted;
    while (!result->done) {
        if (smb_wait_events(source, deadline) != 0) return -ECONNRESET;
    }
    return result->status;
}

static int
smb_pipeline_drain(SmbFile *file, NgSource *source)
{
    uint64_t deadline = ng_now_monotonic_ms() + SMB_TIMEOUT_SECONDS * 1000u;

    while (file->inflight > 0) {
        if (smb_wait_events(source, deadline) != 0) return -1;
    }
    return 0;
}

static void
smb_pipeline_reset(SmbFile *file, uint64_t offset)
{
    file->head = 0;
    file->count = 0;
    file->next_submit = offset;
    file->read_offset = offset;
    for (size_t i = 0; i < SMB_READ_INFLIGHT; i++) {
        file->slot_done[i] = 0;
        file->slot_got[i] = 0;
        file->slot_used[i] = 0;
        file->slot_status[i] = 0;
    }
}

static void
smb_read_done(struct smb2_context *ctx, int status, void *data, void *opaque)
{
    SmbReadJob *job = opaque;
    SmbFile *file = job->file;
    size_t slot = job->slot;
    (void)ctx;
    (void)data;

    file->slot_status[slot] = status;
    file->slot_got[slot] = status > 0 ? (uint32_t)status : 0;
    file->slot_done[slot] = 1;
    if (status > 0) file->bytes_read += (uint64_t)status;
    if (file->inflight > 0) file->inflight--;
    free(job);
}

static int
smb_pipeline_fill(SmbFile *file, NgSource *source, uint64_t end)
{
    while (file->count < file->window && file->next_submit < end) {
        size_t slot = (file->head + file->count) % SMB_READ_INFLIGHT;
        uint64_t left = end - file->next_submit;
        uint32_t want = left < file->chunk ? (uint32_t)left : file->chunk;
        SmbReadJob *job;
        int rc;

        if (want == 0) break;
        job = calloc(1, sizeof(*job));
        if (!job) {
            ng_source_set_error(source, "SMB 读取任务内存不足");
            file->pipeline_failed = 1;
            return -1;
        }
        job->file = file;
        job->slot = slot;
        file->slot_offset[slot] = file->next_submit;
        file->slot_want[slot] = want;
        file->slot_got[slot] = 0;
        file->slot_used[slot] = 0;
        file->slot_status[slot] = 0;
        file->slot_done[slot] = 0;
        rc = smb2_pread_async(file->ctx, file->fh, file->slot_buf[slot], want,
                              file->next_submit, smb_read_done, job);
        if (rc < 0) {
            free(job);
            file->pipeline_failed = 1;
            smb_set_error(source, "SMB 提交异步读取失败");
            return -1;
        }
        file->reads_submitted++;
        file->inflight++;
        file->next_submit += want;
        file->count++;
    }
    return 0;
}

static int
smb_pipeline_recover(SmbFile *file, NgSource *source)
{
    SmbPriv *priv = source->priv;

    if (!priv->ctx || file->generation != priv->generation) return -1;
    if (smb_pipeline_drain(file, source) != 0) return -1;
    smb_pipeline_reset(file, file->read_offset);
    file->pipeline_enabled = 0;
    file->pipeline_failed = 0;
    return 0;
}

static ssize_t
smb_sync_read(SmbFile *file, NgSource *source, void *buf, size_t size)
{
    size_t done = 0;

    while (done < size && file->read_offset < file->size) {
        uint64_t left = file->size - file->read_offset;
        uint32_t want = left < file->chunk ? (uint32_t)left : file->chunk;
        uint32_t room = size - done;
        if (want > room) want = room;
        if (want == 0) break;
        SmbResult result = {0};
        int submitted = smb2_pread_async(file->ctx, file->fh, (uint8_t *)buf + done,
                                          want, file->read_offset, smb_command_done, &result);
        int rc = smb_wait_result(source, &result, submitted);
        if (rc < 0) {
            if (source->connected) smb_set_error(source, "SMB 同步读取失败");
            if (rc == -ETIMEDOUT || rc == -ENETRESET) smb_reset(source);
            return done > 0 ? (ssize_t)done : -1;
        }
        if (rc == 0) {
            errno = EIO;
            return done ? (ssize_t)done : -1;
        }
        done += (size_t)rc;
        file->read_offset += (uint64_t)rc;
        file->bytes_read += (uint64_t)rc;
        if ((uint32_t)rc < want) break;
    }
    return (ssize_t)done;
}

static int
smb_pipeline_wait_head(SmbFile *file, NgSource *source)
{
    uint64_t deadline = ng_now_monotonic_ms() + SMB_TIMEOUT_SECONDS * 1000u;

    while (!file->slot_done[file->head]) {
        if (smb_wait_events(source, deadline) != 0) return -1;
    }
    return 0;
}

static ssize_t
smb_pipeline_read(SmbFile *file, NgSource *source, void *buf, size_t size)
{
    uint8_t *out = buf;
    size_t done = 0;
    uint64_t remaining = file->size - file->read_offset;
    uint64_t end = file->read_offset + (remaining < size ? remaining : size);

    while (done < size) {
        size_t slot;
        size_t available;
        size_t take;

        if (file->count < file->window && file->next_submit < end) {
            if (smb_pipeline_fill(file, source, end) != 0) {
                return done ? (ssize_t)done : -1;
            }
        }
        if (file->count == 0) break;
        if (smb_pipeline_wait_head(file, source) != 0) return done ? (ssize_t)done : -1;

        slot = file->head;
        if (file->slot_status[slot] < 0) {
            file->pipeline_failed = 1;
            smb_set_error(source, "SMB 异步读取失败");
            if (file->slot_status[slot] == -ETIMEDOUT ||
                file->slot_status[slot] == -ENETRESET) smb_reset(source);
            return done ? (ssize_t)done : -1;
        }
        if (file->slot_got[slot] == 0) {
            file->pipeline_failed = 1;
            break;
        }
        if (file->slot_got[slot] < file->slot_want[slot] &&
            file->slot_offset[slot] + file->slot_got[slot] < file->size) {
            file->pipeline_failed = 1;
        }
        available = file->slot_got[slot] - file->slot_used[slot];
        take = size - done < available ? size - done : available;
        memcpy(out + done, file->slot_buf[slot] + file->slot_used[slot], take);
        done += take;
        file->read_offset += (uint64_t)take;
        if (take < available) {
            file->slot_used[slot] += (uint32_t)take;
        } else {
            file->head = (file->head + 1) % SMB_READ_INFLIGHT;
            file->count--;
        }
        if (file->pipeline_failed) break;
    }
    return (ssize_t)done;
}

static void
smb_server_address(const NgSourceConfig *config, char *out, size_t out_size)
{
    int port = config->port ? config->port : 445;
    if (port != 445) {
        if (strchr(config->host, ':')) {
            snprintf(out, out_size, "[%s]:%d", config->host, port);
        } else {
            snprintf(out, out_size, "%s:%d", config->host, port);
        }
    } else {
        ng_strlcpy(out, config->host, out_size);
    }
}

static void
smb_split_share(const char *configured, char *share, size_t share_size)
{
    char value[NG_TITLE_LEN];
    char *start;
    char *slash;

    ng_strlcpy(value, configured, sizeof(value));
    for (char *p = value; *p; p++) {
        if (*p == '\\') *p = '/';
    }
    start = value;
    while (*start == '/') start++;
    slash = strchr(start, '/');
    if (slash) *slash = '\0';
    ng_strlcpy(share, start, share_size);
}

static const char *
smb_api_path(const char *path, char *buffer, size_t buffer_size)
{
    while (*path == '/') path++;
    ng_strlcpy(buffer, path, buffer_size);
    return buffer;
}

static void
smb_status_text(uint32_t status, const char *share, char *out, size_t out_size)
{
    switch (status) {
    case SMB2_STATUS_BAD_NETWORK_NAME:
        snprintf(out, out_size,
                 "共享名“%s”不存在。共享名只填写 NAS 上显示的共享，例如 NasBackups，不要填写后面的子目录",
                 share && *share ? share : "(空)");
        break;
    case SMB2_STATUS_BAD_NETWORK_PATH:
        snprintf(out, out_size, "共享内的目录不存在，请检查扫描目录");
        break;
    case SMB2_STATUS_OBJECT_PATH_NOT_FOUND:
    case SMB2_STATUS_OBJECT_NAME_NOT_FOUND:
        snprintf(out, out_size, "路径或文件不存在");
        break;
    case SMB2_STATUS_LOGON_FAILURE:
    case SMB2_STATUS_WRONG_PASSWORD:
    case SMB2_STATUS_WRONG_PASSWORD_CORE:
        snprintf(out, out_size, "用户名或密码错误");
        break;
    case SMB2_STATUS_ACCESS_DENIED:
    case SMB2_STATUS_NETWORK_ACCESS_DENIED:
        snprintf(out, out_size, "账号已连接，但没有访问该共享的权限");
        break;
    case SMB2_STATUS_IO_TIMEOUT:
        snprintf(out, out_size, "SMB 请求超时");
        break;
    case SMB2_STATUS_INVALID_PARAMETER:
        snprintf(out, out_size, "SMB 路径参数无效，请检查共享内的目录是否存在");
        break;
    default:
        snprintf(out, out_size, "服务器拒绝连接 (NTSTATUS 0x%08X)", status);
        break;
    }
}

static void
smb_transport_text(int error_number, const char *server,
                   const char *detail, char *out, size_t out_size)
{
    switch (error_number) {
    case ECONNREFUSED:
        snprintf(out, out_size,
                 "无法连接 %s，端口被拒绝。请确认 NAS 已启用 SMB 服务且端口填写正确",
                 server);
        return;
    case ETIMEDOUT:
        snprintf(out, out_size,
                 "连接 %s 超时。请检查 IP、路由和 NAS 防火墙，并确认 PS5 所在网段可访问 SMB 端口",
                 server);
        return;
    case EHOSTUNREACH:
    case ENETUNREACH:
        snprintf(out, out_size,
                 "无法路由到 %s。请确认 PS5 与 NAS 在同一网络，或路由允许访问该地址",
                 server);
        return;
    case ECONNRESET:
        snprintf(out, out_size, "服务器 %s 重置了连接", server);
        return;
    case EACCES:
    case EPERM:
        snprintf(out, out_size,
                 "连接 %s 被拒绝，请检查 NAS 防火墙、共享权限与 SMB 版本设置", server);
        return;
    default:
        break;
    }
    if (detail && *detail) {
        snprintf(out, out_size, "%s（%s）", server, detail);
    } else if (error_number) {
        snprintf(out, out_size, "%s：%s", server, strerror(error_number));
    } else {
        snprintf(out, out_size, "无法连接 %s", server);
    }
}

static void
smb_set_error(NgSource *source, const char *prefix)
{
    SmbPriv *priv = source->priv;
    const char *message = priv && priv->ctx ? smb2_get_error(priv->ctx) : NULL;
    uint32_t status = priv && priv->ctx ? (uint32_t)smb2_get_nterror(priv->ctx) : 0;
    char error[NG_ERROR_LEN];

    if (status) {
        char reason[NG_ERROR_LEN];
        smb_status_text(status, source->config.share, reason, sizeof(reason));
        snprintf(error, sizeof(error), "%s：%s (NTSTATUS 0x%08X)", prefix, reason, status);
    } else {
        char reason[NG_ERROR_LEN];
        smb_transport_text(errno, priv && priv->server[0] ? priv->server : source->config.host,
                           message, reason, sizeof(reason));
        snprintf(error, sizeof(error), "%s：%s", prefix, reason);
    }
    ng_source_set_error(source, "%s", error);
    ng_log_error("SMB %s: %s", source->config.id, error);
}

static void
smb_reset(NgSource *source)
{
    SmbPriv *priv = source->priv;
    if (priv->ctx) {
        smb_context_destroy(priv->ctx);
        priv->ctx = NULL;
        priv->generation++;
    }
    source->connected = 0;
}

static void
smb_tune_socket(NgSource *source)
{
    SmbPriv *priv = source->priv;
    int fd = smb2_get_fd(priv->ctx);
    socklen_t length;
    int requested = SMB_SOCKET_BUFFER;
    uint32_t max_read;
    uint16_t dialect;

    priv->socket_rcvbuf = 0;
    priv->socket_sndbuf = 0;
    if (fd >= 0) {
        setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &requested, sizeof(requested));
        setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &requested, sizeof(requested));
        length = sizeof(priv->socket_rcvbuf);
        getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &priv->socket_rcvbuf, &length);
        length = sizeof(priv->socket_sndbuf);
        getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &priv->socket_sndbuf, &length);
    }
    max_read = smb2_get_max_read_size(priv->ctx);
    dialect = smb2_get_dialect(priv->ctx);
    ng_log_info("SMB %s: 传输参数 dialect=0x%04x maxRead=%u KiB "
                "inflight=%d socket=%d/%d KiB",
                source->config.id, dialect, max_read / 1024,
                SMB_READ_INFLIGHT,
                priv->socket_rcvbuf / 1024, priv->socket_sndbuf / 1024);
}

static int
smb_connect(NgSource *source)
{
    SmbPriv *priv = source->priv;
    struct smb2_context *ctx;
    const char *user = source->config.username;
    char share[NG_TITLE_LEN];

    smb_reset(source);
    smb_server_address(&source->config, priv->server, sizeof(priv->server));
    smb_split_share(source->config.share, share, sizeof(share));
    ng_log_info("SMB %s: 连接 %s，共享=%s，用户=%s，密码=%s",
                source->config.id, priv->server, share,
                user && *user ? user : "(guest)",
                source->config.password[0] ? "已设置" : "未设置");
    ctx = smb_context_create();
    if (!ctx) {
        ng_source_set_error(source, "smb2_init_context 失败");
        return -1;
    }
    priv->ctx = ctx;
    smb2_set_timeout(ctx, SMB_TIMEOUT_SECONDS);
    smb2_set_security_mode(ctx, SMB2_NEGOTIATE_SIGNING_ENABLED);
    if (user && *user) {
        smb2_set_user(ctx, user);
    }
    if (source->config.password[0]) {
        smb2_set_password(ctx, source->config.password);
    }
    if (source->config.domain[0]) {
        smb2_set_domain(ctx, source->config.domain);
    }
    if (smb2_connect_share(ctx, priv->server, share,
                           user && *user ? user : NULL) < 0) {
        smb_set_error(source, "无法连接 SMB 共享");
        smb_reset(source);
        return -1;
    }
    smb_tune_socket(source);
    ng_log_info("SMB %s: 已连接 %s/%s", source->config.id, priv->server, share);
    return 0;
}

static void
smb_disconnect(NgSource *source)
{
    smb_reset(source);
}

static void
smb_close_handle(NgSource *source, struct smb2fh *handle)
{
    SmbPriv *priv = source->priv;
    SmbResult result = {0};

    int submitted = smb2_close_async(priv->ctx, handle, smb_command_done, &result);
    int rc = smb_wait_result(source, &result, submitted);
    if (priv->ctx && (submitted < 0 || rc == -ETIMEDOUT || rc == -ENETRESET)) {
        smb_set_error(source, "SMB 无法关闭文件");
        smb_reset(source);
    }
}

static int
smb_list(NgSource *source, const char *path, NgSourceList *list)
{
    SmbPriv *priv = source->priv;
    struct smb2dir *dir;
    struct smb2dirent *entry;
    char remote[NG_PATH_LEN];
    SmbResult result = {0};
    int rc = 0;

    smb2_set_error(priv->ctx, "");
    int submitted = smb2_opendir_async(priv->ctx, smb_api_path(path, remote, sizeof(remote)),
                                       smb_command_done, &result);
    int status_code = smb_wait_result(source, &result, submitted);
    dir = result.data;
    if (status_code != 0 || !dir) {
        if (!priv->ctx) return -1;
        smb_set_error(source, "SMB 打开目录失败");
        uint32_t status = (uint32_t)smb2_get_nterror(priv->ctx);
        if (status == SMB2_STATUS_NETWORK_NAME_DELETED ||
            status == SMB2_STATUS_USER_SESSION_DELETED || status == SMB2_STATUS_IO_TIMEOUT ||
            status == SMB2_STATUS_CONNECTION_DISCONNECTED || smb2_get_fd(priv->ctx) < 0) {
            smb_reset(source);
        }
        return -1;
    }
    while ((entry = smb2_readdir(priv->ctx, dir)) != NULL) {
        NgSourceEntry item;
        if (!strcmp(entry->name, ".") || !strcmp(entry->name, "..")) continue;
        if (strlen(entry->name) >= sizeof(item.name)) {
            errno = ENAMETOOLONG;
            ng_source_set_error(source, "SMB 目录包含过长的文件名");
            rc = -1;
            break;
        }
        memset(&item, 0, sizeof(item));
        ng_strlcpy(item.name, entry->name, sizeof(item.name));
        item.is_dir = entry->st.smb2_type == SMB2_TYPE_DIRECTORY;
        item.size = entry->st.smb2_size;
        item.mtime = entry->st.smb2_mtime;
        if (ng_source_list_add(list, &item) != 0) {
            rc = -1;
            break;
        }
    }
    smb2_closedir(priv->ctx, dir);
    return rc;
}

static int
smb_stat(NgSource *source, const char *path, NgSourceEntry *item)
{
    SmbPriv *priv = source->priv;
    struct smb2_stat_64 st;
    char remote[NG_PATH_LEN];
    SmbResult result = {0};

    int submitted = smb2_stat_async(priv->ctx, smb_api_path(path, remote, sizeof(remote)), &st,
                                    smb_command_done, &result);
    if (smb_wait_result(source, &result, submitted) != 0) {
        if (priv->ctx) smb_set_error(source, "SMB stat 失败");
        return -1;
    }
    memset(item, 0, sizeof(*item));
    ng_strlcpy(item->name, ng_path_basename(path), sizeof(item->name));
    item->is_dir = st.smb2_type == SMB2_TYPE_DIRECTORY;
    item->size = st.smb2_size;
    item->mtime = st.smb2_mtime;
    return 0;
}

static void *
smb_file_open(NgSource *source, const char *path, uint64_t *size)
{
    SmbPriv *priv = source->priv;
    struct smb2_stat_64 st;
    struct smb2fh *fh;
    SmbFile *file;
    char remote[NG_PATH_LEN];
    uint32_t max_read;
    SmbResult opened = {0};
    SmbResult stat_result = {0};

    int submitted = smb2_open_async(priv->ctx, smb_api_path(path, remote, sizeof(remote)),
                                    O_RDONLY, smb_command_done, &opened);
    int rc = smb_wait_result(source, &opened, submitted);
    fh = opened.data;
    if (rc != 0 || !fh) {
        if (priv->ctx) smb_set_error(source, "SMB 打开文件失败");
        if (rc == -ETIMEDOUT || rc == -ENETRESET) smb_reset(source);
        return NULL;
    }
    submitted = smb2_fstat_async(priv->ctx, fh, &st, smb_command_done, &stat_result);
    rc = smb_wait_result(source, &stat_result, submitted);
    if (rc != 0) {
        if (!priv->ctx) return NULL;
        smb_set_error(source, "SMB 读取文件信息失败");
        if (rc == -ETIMEDOUT || rc == -ENETRESET) smb_reset(source);
        else smb_close_handle(source, fh);
        return NULL;
    }
    file = calloc(1, sizeof(*file));
    if (!file) {
        smb_close_handle(source, fh);
        return NULL;
    }
    file->fh = fh;
    file->ctx = priv->ctx;
    file->generation = priv->generation;
    file->size = st.smb2_size;
    file->dialect = smb2_get_dialect(priv->ctx);
    file->socket_rcvbuf = priv->socket_rcvbuf;
    file->socket_sndbuf = priv->socket_sndbuf;
    file->opened_ms = ng_now_monotonic_ms();
    file->window = file->size >= SMB_PIPELINE_THRESHOLD ? SMB_READ_INFLIGHT : 1;
    ng_strlcpy(file->path, path, sizeof(file->path));
    max_read = smb2_get_max_read_size(priv->ctx);
    if (max_read == 0 || max_read > SMB_READ_CHUNK) max_read = SMB_READ_CHUNK;
    if (file->dialect <= SMB2_VERSION_0202 && max_read > 64u * 1024u) {
        max_read = 64u * 1024u;
    }
    file->chunk = max_read;
    file->pipeline_enabled = 1;
    for (size_t i = 0; i < file->window; i++) {
        if (posix_memalign((void **)&file->slot_buf[i], 4096, file->chunk) != 0) {
            file->slot_buf[i] = NULL;
        }
        if (!file->slot_buf[i]) {
            file->pipeline_enabled = 0;
            file->pipeline_failed = 0;
            break;
        }
    }
    if (!file->pipeline_enabled) {
        for (size_t i = 0; i < SMB_READ_INFLIGHT; i++) {
            free(file->slot_buf[i]);
            file->slot_buf[i] = NULL;
        }
        ng_log_info("SMB %s: 异步读取缓冲不足，改用同步读取", source->config.id);
    }
    smb_pipeline_reset(file, 0);
    *size = st.smb2_size;
    return file;
}

static ssize_t
smb_file_read(NgSource *source, void *handle, void *buf, size_t size)
{
    SmbPriv *priv = source->priv;
    SmbFile *file = handle;
    ssize_t rc;

    if (!priv->ctx || file->generation != priv->generation) {
        errno = ENOTCONN;
        return -1;
    }
    if (!size || file->read_offset >= file->size) return 0;
    if (!file->pipeline_enabled) {
        return smb_sync_read(file, source, buf, size);
    }
    if (file->pipeline_failed) {
        if (smb_pipeline_recover(file, source) != 0) return -1;
        return smb_sync_read(file, source, buf, size);
    }
    rc = smb_pipeline_read(file, source, buf, size);
    if (rc <= 0 && file->pipeline_failed) {
        if (smb_pipeline_recover(file, source) == 0) {
            rc = smb_sync_read(file, source, buf, size);
        }
    }
    return rc;
}

static int
smb_file_seek(NgSource *source, void *handle, uint64_t offset)
{
    SmbPriv *priv = source->priv;
    SmbFile *file = handle;
    if (!priv->ctx || file->generation != priv->generation) {
        errno = ENOTCONN;
        return -1;
    }
    if (file->inflight > 0 && smb_pipeline_drain(file, source) != 0) return -1;
    file->pipeline_failed = 0;
    smb_pipeline_reset(file, offset);
    return 0;
}

static void
smb_file_close(NgSource *source, void *handle)
{
    SmbPriv *priv = source->priv;
    SmbFile *file = handle;
    uint64_t elapsed;
    if (priv->ctx && file->generation == priv->generation &&
        smb_pipeline_drain(file, source) == 0) {
        smb_close_handle(source, file->fh);
    }
    elapsed = ng_now_monotonic_ms() - file->opened_ms;
    if (file->bytes_read >= SMB_TRANSFER_LOG_BYTES) {
        double seconds = elapsed ? (double)elapsed / 1000.0 : 0.0;
        double mib = (double)file->bytes_read / (1024.0 * 1024.0);
        ng_log_info("SMB %s: %s 读取 %.1f MiB，耗时 %.2f s，%.1f MiB/s，"
                    "dialect=0x%04x，chunk=%u KiB，窗口=%zu，批量=%llu，socket=%d/%d KiB",
                    source->config.id, file->path, mib, seconds,
                    seconds > 0.0 ? mib / seconds : 0.0,
                    file->dialect, file->chunk / 1024, file->window,
                    (unsigned long long)file->reads_submitted,
                    file->socket_rcvbuf / 1024, file->socket_sndbuf / 1024);
    }
    for (size_t i = 0; i < SMB_READ_INFLIGHT; i++) free(file->slot_buf[i]);
    free(file);
}

typedef struct {
    int done;
    int status;
    struct srvsvc_NetrShareEnum_rep *reply;
} SmbShareEnumResult;

static void
smb_share_enum_cb(struct smb2_context *ctx, int status, void *data, void *opaque)
{
    SmbShareEnumResult *result = opaque;
    (void)ctx;
    result->status = status;
    result->reply = data;
    result->done = 1;
}

static void
smb_share_error(uint32_t status, int error_number, const char *server,
                const char *detail, char *error, size_t error_size)
{
    if (status) {
        char reason[NG_ERROR_LEN];
        smb_status_text(status, "IPC$", reason, sizeof(reason));
        snprintf(error, error_size, "%s (NTSTATUS 0x%08X)", reason, status);
    } else {
        char reason[NG_ERROR_LEN];
        smb_transport_text(error_number, server, detail, reason, sizeof(reason));
        snprintf(error, error_size, "共享枚举失败：%s", reason);
    }
}

static int
compare_share_entries(const void *a, const void *b)
{
    const NgShareEntry *left = a;
    const NgShareEntry *right = b;
    return strcasecmp(left->name, right->name);
}

int
ng_source_smb_list_shares(const NgSourceConfig *config, NgShareList *shares,
                          char *error, size_t error_size)
{
    struct smb2_context *ctx = NULL;
    SmbShareEnumResult result;
    char server[384];
    const char *user = config->username;
    const char *detail;
    uint32_t ntstatus;
    int status;
    time_t deadline;

    memset(&result, 0, sizeof(result));
    smb_server_address(config, server, sizeof(server));
    ng_log_info("SMB 共享枚举: 连接 %s，用户=%s，密码=%s", server,
                user && *user ? user : "(guest)",
                config->password[0] ? "已设置" : "未设置");
    ctx = smb_context_create();
    if (!ctx) {
        snprintf(error, error_size, "无法初始化 SMB 会话");
        return -1;
    }
    smb2_set_timeout(ctx, 12);
    smb2_set_security_mode(ctx, SMB2_NEGOTIATE_SIGNING_ENABLED);
    if (user && *user) smb2_set_user(ctx, user);
    if (config->password[0]) {
        smb2_set_password(ctx, config->password);
    }
    if (config->domain[0]) smb2_set_domain(ctx, config->domain);
    if (smb2_connect_share(ctx, server, "IPC$", user && *user ? user : NULL) < 0) {
        ntstatus = (uint32_t)smb2_get_nterror(ctx);
        detail = smb2_get_error(ctx);
        smb_share_error(ntstatus, errno, server, detail, error, error_size);
        ng_log_error("SMB 共享枚举连接失败: %s", error);
        smb_context_destroy(ctx);
        return -1;
    }

    status = smb2_share_enum_async(ctx, SHARE_INFO_1, smb_share_enum_cb, &result);
    deadline = time(NULL) + 15;
    while (!status && !result.done && time(NULL) <= deadline) {
        struct pollfd pfd;
        int ready;
        pfd.fd = smb2_get_fd(ctx);
        pfd.events = (short)smb2_which_events(ctx);
        pfd.revents = 0;
        ready = poll(&pfd, 1, 750);
        if (ready < 0) {
            if (errno == EINTR) continue;
            status = -errno;
            break;
        }
        status = smb2_service(ctx, pfd.revents);
    }
    if (!status) status = result.status;
    if (!status && !result.done) status = -ETIMEDOUT;
    if (!status && !result.reply) status = -EIO;

    if (status) {
        ntstatus = (uint32_t)smb2_get_nterror(ctx);
        detail = smb2_get_error(ctx);
        if (!status) status = -EIO;
        smb_share_error(ntstatus, status < 0 ? -status : errno, server,
                        detail, error, error_size);
        ng_log_error("SMB 共享枚举失败: %s", error);
    } else {
        struct srvsvc_SHARE_INFO_1_CONTAINER *container =
            &result.reply->ses.ShareInfo.Level1;
        for (uint32_t i = 0;
             container->Buffer && i < container->EntriesRead &&
             shares->count < NG_MAX_SHARES;
             i++) {
            struct srvsvc_SHARE_INFO_1 *share = &container->Buffer->share_info_1[i];
            NgShareEntry *entry;
            if ((share->type & 3u) != SHARE_TYPE_DISKTREE ||
                !share->netname.utf8 || !share->netname.utf8[0]) {
                continue;
            }
            entry = &shares->items[shares->count++];
            memset(entry, 0, sizeof(*entry));
            ng_strlcpy(entry->name, share->netname.utf8, sizeof(entry->name));
            if (share->remark.utf8) {
                ng_strlcpy(entry->remark, share->remark.utf8, sizeof(entry->remark));
            }
            entry->type = share->type;
        }
        if (shares->count > 1) {
            qsort(shares->items, shares->count, sizeof(shares->items[0]),
                  compare_share_entries);
        }
        ng_log_info("SMB 共享枚举完成: %zu 个共享", shares->count);
    }

    if (result.reply) smb2_free_data(ctx, result.reply);
    smb_context_destroy(ctx);
    return status ? -1 : 0;
}

static const NgSourceOps smb_ops = {
    .connect = smb_connect,
    .disconnect = smb_disconnect,
    .list = smb_list,
    .stat = smb_stat,
    .file_open = smb_file_open,
    .file_read = smb_file_read,
    .file_seek = smb_file_seek,
    .file_close = smb_file_close,
};

NgSource *
ng_source_smb_create(const NgSourceConfig *config)
{
    NgSource *source = calloc(1, sizeof(*source));
    SmbPriv *priv = calloc(1, sizeof(*priv));

    if (!source || !priv) {
        free(source);
        free(priv);
        return NULL;
    }
    source->config = *config;
    source->ops = &smb_ops;
    source->priv = priv;
    ng_strlcpy(source->status, "offline", sizeof(source->status));
    return source;
}
