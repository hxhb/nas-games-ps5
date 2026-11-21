#include "http_server.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "app.h"
#include "installer.h"
#include "json.h"
#include "log.h"
#include "source.h"
#include "source_prefetch.h"
#include "ui_assets.h"
#include "util.h"

#ifdef __PROSPERO__
#include <pthread_np.h>
#endif

#define HTTP_MAX_HEADER 32768
#define HTTP_MAX_PATH 4096
#define HTTP_MAX_CONNECTIONS 32
#define STREAM_FILE_SLOTS 4

typedef struct {
    char game_id[24];
    NgSourceFile *file;
    uint64_t last_used;
    int in_use;
    int stale;
} StreamFileSlot;

static pthread_mutex_t stream_files_lock = PTHREAD_MUTEX_INITIALIZER;
static StreamFileSlot stream_files[STREAM_FILE_SLOTS];

static NgSourceFile *
stream_file_acquire(const NgGame *game, NgSource *source, int *cached)
{
    NgSourceFile *file = NULL;
    NgSourceFile *retired = NULL;
    int slot = -1;

    *cached = 0;
    pthread_mutex_lock(&stream_files_lock);
    for (int i = 0; i < STREAM_FILE_SLOTS; i++) {
        if (stream_files[i].file && !stream_files[i].in_use &&
            !stream_files[i].stale &&
            stream_files[i].file->source == source &&
            !strcmp(stream_files[i].game_id, game->id) &&
            (!game->size || stream_files[i].file->size == game->size)) {
            stream_files[i].in_use = 1;
            file = stream_files[i].file;
            *cached = 1;
            pthread_mutex_unlock(&stream_files_lock);
            return file;
        }
    }
    for (int i = 0; i < STREAM_FILE_SLOTS; i++) {
        if (stream_files[i].in_use) continue;
        if (slot < 0 || stream_files[i].last_used < stream_files[slot].last_used) slot = i;
    }
    if (slot >= 0) {
        retired = stream_files[slot].file;
        memset(&stream_files[slot], 0, sizeof(stream_files[slot]));
        snprintf(stream_files[slot].game_id, sizeof(stream_files[slot].game_id),
                 "%s", game->id);
        stream_files[slot].in_use = 1;
    }
    pthread_mutex_unlock(&stream_files_lock);

    ng_source_file_close(retired);
    file = ng_source_open_file(source, game->path);
    if (slot >= 0) {
        pthread_mutex_lock(&stream_files_lock);
        stream_files[slot].file = file;
        if (!file) memset(&stream_files[slot], 0, sizeof(stream_files[slot]));
        pthread_mutex_unlock(&stream_files_lock);
    }
    if (file && slot >= 0) *cached = 1;
    return file;
}

static void
stream_file_release(NgSourceFile *file, int cached, int invalid)
{
    NgSourceFile *close_file = NULL;

    if (!file) return;
    if (!cached) {
        ng_source_file_close(file);
        return;
    }
    pthread_mutex_lock(&stream_files_lock);
    for (int i = 0; i < STREAM_FILE_SLOTS; i++) {
        if (stream_files[i].file == file) {
            if (invalid || stream_files[i].stale) {
                close_file = stream_files[i].file;
                memset(&stream_files[i], 0, sizeof(stream_files[i]));
            } else {
                stream_files[i].last_used = ng_now_monotonic_ms();
                stream_files[i].in_use = 0;
            }
            break;
        }
    }
    pthread_mutex_unlock(&stream_files_lock);
    if (close_file) ng_source_file_close(close_file);
}

void
ng_http_stream_cache_clear(void)
{
    NgSourceFile *close_files[STREAM_FILE_SLOTS] = {0};

    pthread_mutex_lock(&stream_files_lock);
    for (int i = 0; i < STREAM_FILE_SLOTS; i++) {
        if (stream_files[i].in_use) {
            stream_files[i].stale = 1;
        } else {
            close_files[i] = stream_files[i].file;
            memset(&stream_files[i], 0, sizeof(stream_files[i]));
        }
    }
    pthread_mutex_unlock(&stream_files_lock);
    for (int i = 0; i < STREAM_FILE_SLOTS; i++) {
        if (close_files[i]) ng_source_file_close(close_files[i]);
    }
}

typedef struct {
    int fd;
    char method[12];
    char raw_path[HTTP_MAX_PATH];
    char path[HTTP_MAX_PATH];
    char query[HTTP_MAX_PATH];
    char range[256];
    char host[384];
    char origin[400];
    int json_content;
    int cross_site;
    char *body;
} HttpRequest;

typedef struct {
    int fd;
    int head_only;
} HttpResponse;

static int listen_fd = -1;
static pthread_t http_thread;
static atomic_int http_stop;
static pthread_mutex_t connections_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t connections_ready = PTHREAD_COND_INITIALIZER;
static int connection_fds[HTTP_MAX_CONNECTIONS];
static int connection_count;

static int
send_all(int fd, const void *data, size_t size)
{
    const char *p = data;
    while (size) {
        ssize_t n = send(fd, p, size, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        p += n;
        size -= (size_t)n;
    }
    return 0;
}

static int
send_header(HttpResponse *resp, int code, const char *reason,
            const char *content_type, int length_known, size_t length,
            const char *extra)
{
    char header[4096];
    int n;

    if (length_known) {
        n = snprintf(header, sizeof(header),
                     "HTTP/1.1 %d %s\r\n"
                     "Connection: close\r\n"
                     "Server: nasgames/%s\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %zu\r\n"
                     "%s"
                     "\r\n",
                     code, reason ? reason : "OK", NG_VERSION,
                     content_type ? content_type : "application/octet-stream",
                     length, extra ? extra : "");
    } else {
        n = snprintf(header, sizeof(header),
                     "HTTP/1.1 %d %s\r\n"
                     "Connection: close\r\n"
                     "Server: nasgames/%s\r\n"
                     "Content-Type: %s\r\n"
                     "%s"
                     "\r\n",
                     code, reason ? reason : "OK", NG_VERSION,
                     content_type ? content_type : "application/octet-stream",
                     extra ? extra : "");
    }
    if (n < 0 || (size_t)n >= sizeof(header)) return -1;
    return send_all(resp->fd, header, (size_t)n);
}

static int
respond_buffer(HttpResponse *resp, int code, const char *reason,
               const char *content_type, const char *extra,
               const void *data, size_t size)
{
    if (send_header(resp, code, reason, content_type, 1, size, extra) != 0) return -1;
    if (!resp->head_only && size) return send_all(resp->fd, data, size);
    return 0;
}

static int
respond_text(HttpResponse *resp, int code, const char *reason, const char *text)
{
    return respond_buffer(resp, code, reason, "text/plain; charset=utf-8",
                          "Cache-Control: no-store\r\n", text, strlen(text));
}

static int
respond_json_sb(HttpResponse *resp, int code, const char *reason, NgStrBuf *sb)
{
    if (sb->failed) return respond_text(resp, 500, "Internal Server Error", "无法构造响应");
    return respond_buffer(resp, code, reason, "application/json; charset=utf-8",
                          "Cache-Control: no-store\r\n", sb->data, sb->len);
}

static int
json_error_response(HttpResponse *resp, int code, const char *reason, const char *message)
{
    NgStrBuf sb;
    int rc;
    ng_strbuf_init(&sb);
    ng_strbuf_append(&sb, "{\"ok\":false,\"error\":");
    ng_strbuf_json_string(&sb, message);
    ng_strbuf_append(&sb, "}");
    rc = respond_json_sb(resp, code, reason, &sb);
    ng_strbuf_free(&sb);
    return rc;
}

static char *
query_get(const char *query, const char *key)
{
    size_t key_len = strlen(key);
    const char *p = query;

    while (p && *p) {
        const char *next = strchr(p, '&');
        size_t len = next ? (size_t)(next - p) : strlen(p);
        if (len > key_len && p[key_len] == '=' && !strncmp(p, key, key_len)) {
            char encoded[HTTP_MAX_PATH];
            size_t value_len = len - key_len - 1;
            if (value_len >= sizeof(encoded)) return NULL;
            memcpy(encoded, p + key_len + 1, value_len);
            encoded[value_len] = '\0';
            return ng_url_decode(encoded);
        }
        if (!next) break;
        p = next + 1;
    }
    return NULL;
}

static int
parse_header_value(const char *value, size_t value_len, char *out, size_t out_size)
{
    while (value_len && (*value == ' ' || *value == '\t')) {
        value++;
        value_len--;
    }
    while (value_len && (value[value_len - 1] == ' ' || value[value_len - 1] == '\t')) {
        value_len--;
    }
    if (value_len >= out_size) return -1;
    memcpy(out, value, value_len);
    out[value_len] = '\0';
    return 0;
}

static int
read_http_request(int fd, HttpRequest *req)
{
    char buffer[HTTP_MAX_HEADER + 1];
    size_t used = 0;
    ssize_t n;
    char *header_end = NULL;
    size_t header_len = 0;
    size_t content_length = 0;
    int has_content_length = 0;
    char *line_end;

    memset(req, 0, sizeof(*req));
    req->fd = fd;
    while (used < HTTP_MAX_HEADER) {
        n = recv(fd, buffer + used, HTTP_MAX_HEADER - used, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break;
        used += (size_t)n;
        buffer[used] = '\0';
        header_end = strstr(buffer, "\r\n\r\n");
        if (header_end) break;
    }
    if (!header_end) return -1;
    header_len = (size_t)(header_end - buffer) + 4;
    line_end = strstr(buffer, "\r\n");
    if (!line_end) return -1;

    char request_line[HTTP_MAX_PATH * 2];
    size_t request_line_len = (size_t)(line_end - buffer);
    if (request_line_len >= sizeof(request_line)) return -1;
    memcpy(request_line, buffer, request_line_len);
    request_line[request_line_len] = '\0';

    char *method = request_line;
    char *target = strchr(method, ' ');
    if (!target) return -1;
    *target++ = '\0';
    char *version = strchr(target, ' ');
    if (!version) return -1;
    *version++ = '\0';
    if (strcmp(version, "HTTP/1.1") && strcmp(version, "HTTP/1.0")) return -1;
    if (target[0] != '/' || strlen(method) >= sizeof(req->method) ||
        strlen(target) >= sizeof(req->raw_path)) return -1;
    ng_strlcpy(req->method, method, sizeof(req->method));
    ng_strlcpy(req->raw_path, target, sizeof(req->raw_path));

    char *question = strchr(req->raw_path, '?');
    if (question) {
        *question = '\0';
        ng_strlcpy(req->query, question + 1, sizeof(req->query));
    }
    char *decoded = ng_url_decode(req->raw_path);
    if (!decoded) return -1;
    ng_strlcpy(req->path, decoded, sizeof(req->path));
    free(decoded);
    if (strstr(req->path, "..")) return -1;

    const char *header = line_end + 2;
    while (header < header_end) {
        const char *end = strstr(header, "\r\n");
        size_t len;
        if (!end || end > header_end) end = header_end;
        len = (size_t)(end - header);
        if (len >= 15 && !strncasecmp(header, "Content-Length:", 15)) {
            char value[32];
            if (has_content_length || parse_header_value(header + 15, len - 15, value, sizeof(value)) != 0 ||
                !isdigit((unsigned char)value[0]) || value[strspn(value, "0123456789")]) return -1;
            unsigned long long number = strtoull(value, NULL, 10);
            if (number > NG_MAX_HTTP_BODY) return -1;
            content_length = (size_t)number;
            has_content_length = 1;
        } else if (len >= 6 && !strncasecmp(header, "Range:", 6)) {
            if (req->range[0] || parse_header_value(header + 6, len - 6, req->range, sizeof(req->range)) != 0) return -1;
        } else if (len >= 5 && !strncasecmp(header, "Host:", 5)) {
            if (req->host[0] || parse_header_value(header + 5, len - 5, req->host, sizeof(req->host)) != 0) return -1;
        } else if (len >= 7 && !strncasecmp(header, "Origin:", 7)) {
            if (req->origin[0] || parse_header_value(header + 7, len - 7, req->origin, sizeof(req->origin)) != 0) return -1;
        } else if (len >= 13 && !strncasecmp(header, "Content-Type:", 13)) {
            char value[160];
            if (parse_header_value(header + 13, len - 13, value, sizeof(value)) != 0) return -1;
            char *parameters = strchr(value, ';');
            if (parameters) *parameters = '\0';
            ng_trim(value);
            req->json_content = !strcasecmp(value, "application/json");
        } else if (len >= 18 && !strncasecmp(header, "Transfer-Encoding:", 18)) {
            return -1;
        }
        if (end >= header_end) break;
        header = end + 2;
    }
    if (req->origin[0]) {
        char expected[sizeof(req->origin)];
        snprintf(expected, sizeof(expected), "http://%s", req->host);
        req->cross_site = strcasecmp(req->origin, expected) != 0;
    }
    if (content_length) {
        size_t already = used > header_len ? used - header_len : 0;
        size_t got;
        req->body = malloc(content_length + 1);
        if (!req->body) return -1;
        if (already > content_length) already = content_length;
        if (already) memcpy(req->body, buffer + header_len, already);
        got = already;
        while (got < content_length) {
            n = recv(fd, req->body + got, content_length - got, 0);
            if (n < 0) {
                if (errno == EINTR) continue;
                free(req->body);
                req->body = NULL;
                return -1;
            }
            if (n == 0) break;
            got += (size_t)n;
        }
        req->body[got] = '\0';
        if (got != content_length || memchr(req->body, '\0', got)) {
            free(req->body);
            req->body = NULL;
            return -1;
        }
    }
    return 0;
}

static int
compare_source_entries(const void *a, const void *b)
{
    const NgSourceEntry *ea = a;
    const NgSourceEntry *eb = b;
    if (ea->is_dir != eb->is_dir) return ea->is_dir ? -1 : 1;
    return strcasecmp(ea->name, eb->name);
}

static int
body_defines_source(const char *body)
{
    NgJsonDoc doc;
    int found = 0;

    if (!body || ng_json_parse(&doc, body, strlen(body)) != 0) return 0;
    if (doc.tokens[0].type == NG_JSON_OBJECT) {
        found = ng_json_obj_get(&doc, 0, "type") >= 0 ||
                ng_json_obj_get(&doc, 0, "host") >= 0 ||
                ng_json_obj_get(&doc, 0, "localRoot") >= 0;
    }
    ng_json_free(&doc);
    return found;
}

static int
parse_json_object(const char *body, NgJsonDoc *doc)
{
    if (!body || ng_json_parse(doc, body, strlen(body)) != 0) return -1;
    if (doc->tokens[0].type == NG_JSON_OBJECT) return 0;
    ng_json_free(doc);
    return -1;
}

static int
parse_draft_source(const char *body, NgSourceConfig *source,
                   char *error, size_t error_size)
{
    NgConfig base;

    if (!body || !body[0]) {
        snprintf(error, error_size, "缺少连接参数");
        return -1;
    }
    ng_app_config_snapshot(&base);
    return ng_config_source_from_json(body, &base, source, error, error_size);
}

static int
handle_browse(const HttpRequest *req, HttpResponse *resp)
{
    char *source_id = NULL;
    char *path = NULL;
    NgSourceConfig draft;
    NgSource *source = NULL;
    NgSourceList list;
    NgStrBuf sb;
    char norm[NG_PATH_LEN];
    char error[NG_ERROR_LEN];
    int rc;

    if (!strcmp(req->method, "POST")) {
        NgJsonDoc doc;
        char draft_path[NG_PATH_LEN];

        if (parse_json_object(req->body, &doc) != 0) {
            return json_error_response(resp, 400, "Bad Request", "请求格式无效");
        }
        ng_json_obj_string(&doc, 0, "path", draft_path, sizeof(draft_path), "/");
        ng_json_free(&doc);
        if (parse_draft_source(req->body, &draft, error, sizeof(error)) != 0) {
            return json_error_response(resp, 400, "Bad Request", error);
        }
        if (draft.type == NG_SOURCE_SMB && !draft.share[0]) {
            return json_error_response(resp, 400, "Bad Request",
                                       "请先填写共享名，或使用“枚举共享”选择共享");
        }
        source = ng_source_create(&draft);
        if (!source) {
            return json_error_response(resp, 500, "Internal Server Error",
                                       "无法创建临时连接");
        }
        path = strdup(draft_path);
    } else {
        source_id = query_get(req->query, "source");
        path = query_get(req->query, "path");
        if (!source_id) return json_error_response(resp, 400, "Bad Request", "缺少 source 参数");
        source = ng_app_get_source(source_id);
        free(source_id);
        if (!source) {
            free(path);
            return json_error_response(resp, 404, "Not Found", "连接不存在");
        }
    }
    if (!path) path = strdup("/");
    if (!path || ng_path_normalize(path, norm, sizeof(norm)) != 0) {
        free(path);
        ng_source_release(source);
        return json_error_response(resp, 400, "Bad Request", "目录路径无效");
    }
    free(path);
    ng_source_list_init(&list);
    if (ng_source_list_dir(source, norm, &list) != 0) {
        ng_source_error_snapshot(source, "无法读取远端目录", error, sizeof(error));
        ng_source_list_free(&list);
        ng_source_release(source);
        return json_error_response(resp, 502, "Bad Gateway", error);
    }
    qsort(list.items, list.count, sizeof(list.items[0]), compare_source_entries);
    ng_strbuf_init(&sb);
    ng_strbuf_append(&sb, "{\"ok\":true,\"path\":");
    ng_strbuf_json_string(&sb, norm);
    ng_strbuf_append(&sb, ",\"entries\":[");
    for (size_t i = 0; i < list.count; i++) {
        if (i) ng_strbuf_append(&sb, ",");
        ng_strbuf_append(&sb, "{\"name\":");
        ng_strbuf_json_string(&sb, list.items[i].name);
        ng_strbuf_printf(&sb, ",\"isDir\":%s,\"size\":%llu,\"mtime\":%llu}",
                         list.items[i].is_dir ? "true" : "false",
                         (unsigned long long)list.items[i].size,
                         (unsigned long long)list.items[i].mtime);
    }
    ng_strbuf_append(&sb, "]}");
    ng_source_list_free(&list);
    rc = respond_json_sb(resp, 200, "OK", &sb);
    ng_strbuf_free(&sb);
    ng_source_release(source);
    return rc;
}

static int
handle_settings_put(const HttpRequest *req, HttpResponse *resp)
{
    char error[256];

    if (!req->body) return json_error_response(resp, 400, "Bad Request", "缺少请求体");
    int rc = ng_app_apply_config(req->body, error, sizeof(error));
    if (rc > 0) return json_error_response(resp, 400, "Bad Request", error);
    if (rc < 0) return json_error_response(resp, 500, "Internal Server Error", error);
    return respond_text(resp, 200, "OK", "{\"ok\":true}");
}

static int
handle_test(const HttpRequest *req, HttpResponse *resp)
{
    NgJsonDoc doc;
    char id[NG_ID_LEN];
    NgSourceConfig draft;
    NgSource *source = NULL;
    NgSourceList list;
    const char *root = "/";
    char error[NG_ERROR_LEN];

    if (parse_json_object(req->body, &doc) != 0) {
        return json_error_response(resp, 400, "Bad Request", "请求格式无效");
    }
    ng_json_obj_string(&doc, 0, "id", id, sizeof(id), "");
    ng_json_free(&doc);
    if (body_defines_source(req->body)) {
        if (parse_draft_source(req->body, &draft, error, sizeof(error)) != 0) {
            return json_error_response(resp, 400, "Bad Request", error);
        }
        if (draft.type == NG_SOURCE_SMB && !draft.share[0]) {
            return json_error_response(resp, 400, "Bad Request", "SMB 连接需要填写共享名");
        }
        source = ng_source_create(&draft);
        if (!source) {
            return json_error_response(resp, 500, "Internal Server Error",
                                       "无法创建临时连接");
        }
        root = draft.roots[0];
    } else {
        source = ng_app_get_source(id);
        if (!source) return json_error_response(resp, 404, "Not Found", "连接不存在");
        root = source->config.roots[0];
    }
    ng_source_list_init(&list);
    if (ng_source_list_dir(source, root[0] ? root : "/", &list) != 0) {
        ng_source_error_snapshot(source, "连接测试失败", error, sizeof(error));
        ng_source_list_free(&list);
        ng_source_release(source);
        return json_error_response(resp, 502, "Bad Gateway", error);
    }
    NgStrBuf sb;
    ng_strbuf_init(&sb);
    ng_strbuf_printf(&sb, "{\"ok\":true,\"message\":\"连接成功，读取到 %zu 项\",\"entries\":%zu}",
                     list.count, list.count);
    ng_source_list_free(&list);
    ng_source_release(source);
    int result = respond_json_sb(resp, 200, "OK", &sb);
    ng_strbuf_free(&sb);
    return result;
}

static int
handle_shares(const HttpRequest *req, HttpResponse *resp)
{
    NgSourceConfig draft;
    NgShareList shares;
    NgStrBuf sb;
    char error[NG_ERROR_LEN];

    if (!req->body) return json_error_response(resp, 400, "Bad Request", "缺少连接参数");
    if (parse_draft_source(req->body, &draft, error, sizeof(error)) != 0) {
        return json_error_response(resp, 400, "Bad Request", error);
    }
    if (draft.type != NG_SOURCE_SMB) {
        return json_error_response(resp, 400, "Bad Request", "共享枚举仅支持 SMB");
    }
    if (ng_source_list_shares(&draft, &shares, error, sizeof(error)) != 0) {
        return json_error_response(resp, 502, "Bad Gateway", error);
    }
    ng_strbuf_init(&sb);
    ng_strbuf_append(&sb, "{\"ok\":true,\"host\":");
    ng_strbuf_json_string(&sb, draft.host);
    ng_strbuf_append(&sb, ",\"shares\":[");
    for (size_t i = 0; i < shares.count; i++) {
        if (i) ng_strbuf_append(&sb, ",");
        ng_strbuf_append(&sb, "{\"name\":");
        ng_strbuf_json_string(&sb, shares.items[i].name);
        ng_strbuf_append(&sb, ",\"remark\":");
        ng_strbuf_json_string(&sb, shares.items[i].remark);
        ng_strbuf_append(&sb, "}");
    }
    ng_strbuf_append(&sb, "]}");
    int rc = respond_json_sb(resp, 200, "OK", &sb);
    ng_strbuf_free(&sb);
    return rc;
}

static int
handle_install(const HttpRequest *req, HttpResponse *resp)
{
    NgJsonDoc doc;
    char id[24];
    char target_name[32];
    char error[NG_ERROR_LEN];
    char response[128];
    NgGame game;
    NgSource *source;
    NgInstallTarget target;
    int status = 400;
    const char *reason = "Bad Request";
    const char *message;

    if (parse_json_object(req->body, &doc) != 0) {
        return json_error_response(resp, 400, "Bad Request", "请求格式无效");
    }
    ng_json_obj_string(&doc, 0, "gameId", id, sizeof(id), "");
    if (!id[0]) ng_json_obj_string(&doc, 0, "id", id, sizeof(id), "");
    ng_json_obj_string(&doc, 0, "target", target_name, sizeof(target_name), "");
    ng_json_free(&doc);
    if (!id[0]) return json_error_response(resp, 400, "Bad Request", "缺少 gameId");
    if (ng_app_copy_game_source(id, &game, &source) != 0) {
        return json_error_response(resp, 404, "Not Found", "游戏不存在，请重新扫描");
    }
    if (game.type == NG_GAME_PKG) {
        target = NG_INSTALL_SYSTEM;
        if (target_name[0] && strcasecmp(target_name, "system") &&
            strcasecmp(target_name, "pkg")) {
            message = "PKG 只能使用 system 安装目标";
            goto invalid;
        }
        if ((game.pkg_kind == NG_PKG_UPDATE || game.pkg_kind == NG_PKG_DLC) &&
            !game.base_installed) {
            status = 409;
            reason = "Conflict";
            message = "请先安装同 Title ID 的基础游戏";
            goto invalid;
        }
    } else if (!strcasecmp(target_name, "homebrew")) {
        target = NG_INSTALL_HOMEBREW;
    } else {
        target = NG_INSTALL_LIBRARY;
        if (target_name[0] && strcasecmp(target_name, "library") &&
            strcasecmp(target_name, "app")) {
            message = "文件夹目标必须是 library 或 homebrew";
            goto invalid;
        }
        if (game.packing != NG_PACK_PLAIN) {
            status = 409;
            reason = "Conflict";
            message = "压缩文件夹不能注册到主界面，请改用 homebrew 目标";
            goto invalid;
        }
    }
    int rc = ng_installer_start(&game, source, target, error, sizeof(error));
    ng_source_release(source);
    if (rc != 0) {
        return json_error_response(resp, 409, "Conflict", error);
    }
    snprintf(response, sizeof(response), "{\"ok\":true,\"target\":\"%s\"}",
             target == NG_INSTALL_SYSTEM ? "system" :
             target == NG_INSTALL_HOMEBREW ? "homebrew" : "library");
    return respond_text(resp, 202, "Accepted", response);

invalid:
    ng_source_release(source);
    return json_error_response(resp, status, reason, message);
}

static int
handle_cancel_job(const HttpRequest *req, HttpResponse *resp)
{
    NgJsonDoc doc;
    NgStrBuf sb;
    char id[24];
    char message[NG_ERROR_LEN];
    int rc;

    if (parse_json_object(req->body, &doc) != 0) {
        return json_error_response(resp, 400, "Bad Request", "请求格式无效");
    }
    ng_json_obj_string(&doc, 0, "gameId", id, sizeof(id), "");
    ng_json_free(&doc);
    if (!id[0]) return json_error_response(resp, 400, "Bad Request", "缺少 gameId");
    rc = ng_installer_cancel(id, message, sizeof(message));
    if (rc < 0) {
        return json_error_response(resp, 404, "Not Found", message);
    }
    if (rc == 2) {
        return json_error_response(resp, 409, "Conflict", message);
    }
    ng_strbuf_init(&sb);
    ng_strbuf_append(&sb, "{\"ok\":true,\"message\":");
    ng_strbuf_json_string(&sb, message);
    ng_strbuf_append(&sb, "}");
    rc = respond_json_sb(resp, rc == 0 ? 202 : 200,
                         rc == 0 ? "Accepted" : "OK", &sb);
    ng_strbuf_free(&sb);
    return rc;
}

static int
handle_history_clear(HttpResponse *resp)
{
    ng_app_job_history_clear();
    return respond_text(resp, 200, "OK", "{\"ok\":true}");
}

static int
handle_state(HttpResponse *resp)
{
    NgStrBuf sb;
    int rc;
    ng_strbuf_init(&sb);
    if (ng_app_state_json(&sb) != 0) {
        ng_strbuf_free(&sb);
        return json_error_response(resp, 500, "Internal Server Error", "构造状态失败");
    }
    rc = respond_json_sb(resp, 200, "OK", &sb);
    ng_strbuf_free(&sb);
    return rc;
}

static int
handle_game_size(const HttpRequest *req, HttpResponse *resp)
{
    char *id = query_get(req->query, "id");
    NgGame game;
    NgSource *source;
    NgStrBuf sb;
    char error[NG_ERROR_LEN];
    uint64_t bytes = 0;
    uint64_t files = 0;
    int rc;

    if (!id || !id[0]) {
        free(id);
        return json_error_response(resp, 400, "Bad Request", "缺少 id 参数");
    }
    if (ng_app_copy_game_source(id, &game, &source) != 0) {
        free(id);
        return json_error_response(resp, 404, "Not Found", "游戏不存在");
    }
    if (game.size > 0) bytes = game.size;
    else if (game.type != NG_GAME_FOLDER) bytes = 0;
    else {
        if (!source || !source->config.enabled) {
            free(id);
            ng_source_release(source);
            return json_error_response(resp, 404, "Not Found", "来源连接不存在");
        }
        rc = ng_source_measure_tree(source, game.path, &bytes, &files, NULL);
        ng_source_error_snapshot(source, "无法统计远端目录大小", error, sizeof(error));
        if (rc != 0) {
            free(id);
            ng_source_release(source);
            return json_error_response(resp, 502, "Bad Gateway", error);
        }
        ng_app_set_game_size(game.id, source, bytes);
    }
    ng_source_release(source);

    ng_strbuf_init(&sb);
    ng_strbuf_append(&sb, "{\"ok\":true,\"id\":");
    ng_strbuf_json_string(&sb, game.id);
    ng_strbuf_printf(&sb,
                     ",\"size\":%llu,\"files\":%llu,\"known\":true}",
                     (unsigned long long)bytes,
                     (unsigned long long)files);
    rc = respond_json_sb(resp, 200, "OK", &sb);
    ng_strbuf_free(&sb);
    free(id);
    return rc;
}

static int
handle_settings_get(HttpResponse *resp)
{
    NgStrBuf sb;
    int rc;
    ng_strbuf_init(&sb);
    if (ng_app_settings_json(&sb) != 0) {
        ng_strbuf_free(&sb);
        return json_error_response(resp, 500, "Internal Server Error", "读取设置失败");
    }
    rc = respond_json_sb(resp, 200, "OK", &sb);
    ng_strbuf_free(&sb);
    return rc;
}

static int
handle_log(HttpResponse *resp)
{
    NgStrBuf sb;
    int rc;
    ng_strbuf_init(&sb);
    if (ng_app_log_json(&sb) != 0) {
        ng_strbuf_free(&sb);
        return json_error_response(resp, 500, "Internal Server Error", "读取日志失败");
    }
    rc = respond_json_sb(resp, 200, "OK", &sb);
    ng_strbuf_free(&sb);
    return rc;
}

static int
parse_range_number(const char *text, char **tail, uint64_t *value)
{
    if (!isdigit((unsigned char)*text)) return -1;
    errno = 0;
    unsigned long long number = strtoull(text, tail, 10);
    if (errno == ERANGE) return -1;
    *value = number;
    return 0;
}

static int
parse_byte_range(const char *range, uint64_t size, uint64_t *start, uint64_t *end,
                 int *partial)
{
    const char *p;
    uint64_t s;
    uint64_t e;

    *partial = 0;
    *start = 0;
    *end = size ? size - 1 : 0;
    if (strncmp(range, "bytes=", 6)) return 0;
    p = range + 6;
    if (*p == '-') {
        uint64_t suffix;
        char *tail;
        if (parse_range_number(p + 1, &tail, &suffix) != 0 || *tail || !size || !suffix) return -1;
        if (suffix > size) suffix = size;
        *start = size - suffix;
        *end = size - 1;
    } else {
        char *tail = NULL;
        if (parse_range_number(p, &tail, &s) != 0) return -1;
        if (*tail == '-') {
            if (tail[1]) {
                char *end_tail = NULL;
                if (parse_range_number(tail + 1, &end_tail, &e) != 0 || *end_tail) return -1;
            } else {
                e = size ? size - 1 : UINT64_MAX;
            }
        } else {
            return -1;
        }
        if (!size || s >= size || e < s) return -1;
        if (e >= size) e = size - 1;
        *start = s;
        *end = e;
    }
    *partial = 1;
    return 0;
}

static int
send_local_file(HttpResponse *resp, const char *path, const char *content_type,
                const char *cache_control)
{
    int fd;
    struct stat st;
    char extra[512];

    fd = open(path, O_RDONLY);
    if (fd < 0 || fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        if (fd >= 0) close(fd);
        return json_error_response(resp, 404, "Not Found", "文件不存在");
    }
    snprintf(extra, sizeof(extra), "%sAccept-Ranges: none\r\n",
             cache_control ? cache_control : "");
    if (send_header(resp, 200, "OK", content_type, 1, (size_t)st.st_size, extra) != 0) {
        close(fd);
        return -1;
    }
    if (!resp->head_only) {
        char buf[131072];
        while (1) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (n == 0) break;
            if (send_all(resp->fd, buf, (size_t)n) != 0) break;
        }
    }
    close(fd);
    return 0;
}

static int
handle_art(const HttpRequest *req, HttpResponse *resp)
{
    const char *rest = req->path + 5;
    const char *slash = strchr(rest, '/');
    char id[24];
    NgGame game;
    const char *path = NULL;
    const char *content_type = "image/png";

    if (!slash || (size_t)(slash - rest) >= sizeof(id)) {
        return json_error_response(resp, 404, "Not Found", "资源不存在");
    }
    memcpy(id, rest, (size_t)(slash - rest));
    id[slash - rest] = '\0';
    if (ng_app_copy_game(id, &game) != 0) {
        return json_error_response(resp, 404, "Not Found", "游戏不存在");
    }
    if (!strcmp(slash, "/icon")) path = game.icon_file;
    else if (!strcmp(slash, "/pic0")) path = game.pic0_file;
    else if (!strcmp(slash, "/pic1")) path = game.pic1_file;
    if (!path || !path[0]) return json_error_response(resp, 404, "Not Found", "没有缓存图片");
    if (ng_has_suffix_ci(path, ".jpg") || ng_has_suffix_ci(path, ".jpeg")) content_type = "image/jpeg";
    return send_local_file(resp, path, content_type,
                           "Cache-Control: public, max-age=3600\r\n");
}

static int
handle_stream(const HttpRequest *req, HttpResponse *resp)
{
    const char *id = req->path + 8;
    NgGame game;
    NgSource *source;
    NgSourceFile *file;
    uint64_t start = 0;
    uint64_t end = 0;
    uint64_t size;
    uint64_t remaining;
    uint64_t sent = 0;
    uint64_t started_ms = ng_now_monotonic_ms();
    NgPrefetcher *prefetch = NULL;
    uint8_t *buf = NULL;
    size_t buf_size = 0;
    int partial = 0;
    int cached = 0;
    int invalid = 0;
    char extra[512];

    if (strcmp(req->method, "GET") && !resp->head_only) {
        return json_error_response(resp, 405, "Method Not Allowed", "不支持的方法");
    }
    if (strlen(id) != 16) return json_error_response(resp, 404, "Not Found", "游戏不存在");
    if (!ng_installer_stream_snapshot(id, &game, &source) &&
        ng_app_copy_game_source(id, &game, &source) != 0) {
        return json_error_response(resp, 404, "Not Found", "游戏不存在");
    }
    if (game.type != NG_GAME_PKG) {
        ng_source_release(source);
        return json_error_response(resp, 415, "Unsupported Media Type", "只有 PKG 可以流式安装");
    }
    if (!source || !source->config.enabled) {
        ng_source_release(source);
        return json_error_response(resp, 404, "Not Found", "来源连接不存在或已停用");
    }
    file = stream_file_acquire(&game, source, &cached);
    for (int attempt = 0; file && attempt < 2; attempt++) {
        size = file->size ? file->size : game.size;
        if (parse_byte_range(req->range, size, &start, &end, &partial) != 0) {
            stream_file_release(file, cached, 0);
            ng_source_release(source);
            snprintf(extra, sizeof(extra), "Content-Range: bytes */%llu\r\n",
                     (unsigned long long)size);
            return respond_buffer(resp, 416, "Range Not Satisfiable", "text/plain; charset=utf-8",
                                  extra, "请求范围无效", strlen("请求范围无效"));
        }
        if (resp->head_only || ng_source_file_seek(file, start) == 0) break;
        stream_file_release(file, cached, 1);
        file = cached && attempt == 0 ? stream_file_acquire(&game, source, &cached) : NULL;
    }
    ng_source_release(source);
    if (!file) return json_error_response(resp, 502, "Bad Gateway", "无法打开或定位远端文件");
    remaining = partial ? end - start + 1 : size;
    ng_log_info("PKG 流式请求: %s range=%llu-%llu size=%llu partial=%d",
                game.title_name, (unsigned long long)start,
                (unsigned long long)(partial ? end :
                                     (size ? size - 1 : 0)),
                (unsigned long long)remaining, partial);
    if (partial) {
        snprintf(extra, sizeof(extra),
                 "Accept-Ranges: bytes\r\nContent-Range: bytes %llu-%llu/%llu\r\n"
                 "Cache-Control: no-store\r\n",
                 (unsigned long long)start, (unsigned long long)end,
                 (unsigned long long)size);
    } else {
        snprintf(extra, sizeof(extra), "Accept-Ranges: bytes\r\nCache-Control: no-store\r\n");
    }
    if (!resp->head_only && cached) prefetch = ng_prefetch_create(file, remaining);
    if (!resp->head_only && !ng_prefetch_is_active(prefetch)) {
        buf_size = ng_prefetch_chunk_size(prefetch);
        if (size && remaining < buf_size) buf_size = (size_t)remaining;
        buf = malloc(buf_size);
        if (!buf) {
            ng_prefetch_destroy(prefetch);
            stream_file_release(file, cached, 0);
            return json_error_response(resp, 503, "Service Unavailable", "传输缓冲不足");
        }
    }
    if (send_header(resp, partial ? 206 : 200, partial ? "Partial Content" : "OK",
                    "application/octet-stream", size > 0, (size_t)remaining, extra) != 0) {
        free(buf);
        ng_prefetch_destroy(prefetch);
        stream_file_release(file, cached, 0);
        return -1;
    }
    if (resp->head_only) {
        stream_file_release(file, cached, 0);
        return 0;
    }
    while (!size || remaining) {
        const void *data = buf;
        size_t want = buf_size;
        if (size && remaining < want) want = (size_t)remaining;
        ssize_t n = buf ? ng_source_file_read(file, buf, want) : ng_prefetch_peek(prefetch, &data);
        if (n < 0) {
            invalid = 1;
            break;
        }
        if (n == 0) {
            if (size && remaining) invalid = 1;
            break;
        }
        if (send_all(resp->fd, data, (size_t)n) != 0) {
            invalid = 1;
            break;
        }
        if (!buf) ng_prefetch_advance(prefetch, (size_t)n);
        sent += (uint64_t)n;
        if (size) remaining -= (uint64_t)n;
    }
    {
        uint64_t elapsed = ng_now_monotonic_ms() - started_ms;
        ng_log_info("PKG 流式结束: %s 发送 %.1f MiB，耗时 %.2f s，%.1f MiB/s",
                    game.title_name, sent / 1048576.0, elapsed / 1000.0,
                    elapsed ? (sent * 1000.0 / 1048576.0 / (double)elapsed) : 0.0);
    }
    free(buf);
    ng_prefetch_destroy(prefetch);
    stream_file_release(file, cached, invalid);
    return 0;
}

static int
handle_api(const HttpRequest *req, HttpResponse *resp)
{
    if (strcmp(req->method, "GET") && strcmp(req->method, "HEAD") && req->cross_site) {
        return json_error_response(resp, 403, "Forbidden", "此操作需要同源请求");
    }
    if (!strcmp(req->path, "/api/state")) return handle_state(resp);
    if (!strcmp(req->path, "/api/game-size")) return handle_game_size(req, resp);
    if (!strcmp(req->path, "/api/settings")) {
        if (!strcmp(req->method, "GET")) return handle_settings_get(resp);
        if (!strcmp(req->method, "PUT") || !strcmp(req->method, "POST")) {
            return handle_settings_put(req, resp);
        }
        return json_error_response(resp, 405, "Method Not Allowed", "不支持的方法");
    }
    if (!strcmp(req->path, "/api/browse")) return handle_browse(req, resp);
    if (!strcmp(req->path, "/api/test")) return handle_test(req, resp);
    if (!strcmp(req->path, "/api/shares") && !strcmp(req->method, "POST")) {
        return handle_shares(req, resp);
    }
    if (!strcmp(req->path, "/api/install") && !strcmp(req->method, "POST")) {
        return handle_install(req, resp);
    }
    if (!strcmp(req->path, "/api/cancel") && !strcmp(req->method, "POST")) {
        return handle_cancel_job(req, resp);
    }
    if (!strcmp(req->path, "/api/history/clear") &&
        !strcmp(req->method, "POST")) {
        return handle_history_clear(resp);
    }
    if (!strcmp(req->path, "/api/scan") && !strcmp(req->method, "POST")) {
        char response[80];
        snprintf(response, sizeof(response), "{\"ok\":true,\"sequence\":%llu}",
                 (unsigned long long)ng_app_request_scan());
        return respond_text(resp, 202, "Accepted", response);
    }
    if (!strcmp(req->path, "/api/shutdown") && !strcmp(req->method, "POST")) {
        if (!req->json_content) {
            return json_error_response(resp, 415, "Unsupported Media Type", "此操作需要 JSON 请求");
        }
        if (ng_installer_prepare_stop() != 0) {
            return json_error_response(resp, 409, "Conflict", "有安装任务运行或排队，请稍后更新");
        }
        return respond_text(resp, 202, "Accepted", "{\"ok\":true}");
    }
    if (!strcmp(req->path, "/api/log")) return handle_log(resp);
    if (!strcmp(req->path, "/api/health")) {
        return respond_text(resp, 200, "OK", "{\"ok\":true,\"version\":\"" NG_VERSION "\"}");
    }
    return json_error_response(resp, 404, "Not Found", "接口不存在");
}

static int
handle_static(const HttpRequest *req, HttpResponse *resp)
{
    const char *path = req->path;
    const char *wanted = NULL;
    if (!strcmp(path, "/") || !strcmp(path, "/index.html")) wanted = "/index.html";
    else if (!strcmp(path, "/app.js")) wanted = "/app.js";
    else if (!strcmp(path, "/app.css")) wanted = "/app.css";
    else if (!strcmp(path, "/icon.svg")) wanted = "/icon.svg";
    else if (!strcmp(path, "/icon0.png")) wanted = "/icon0.png";
    else if (!strcmp(path, "/favicon.ico")) {
        wanted = "/icon.svg";
    }
    if (!wanted) return respond_text(resp, 404, "Not Found", "Not found");
    for (size_t i = 0; i < ng_ui_asset_count; i++) {
        if (!strcmp(ng_ui_assets[i].name, wanted)) {
            return respond_buffer(resp, 200, "OK", ng_ui_assets[i].content_type,
                                  "Cache-Control: no-cache\r\n",
                                  ng_ui_assets[i].data, ng_ui_assets[i].size);
        }
    }
    return respond_text(resp, 404, "Not Found", "Not found");
}

static void *
connection_main(void *arg)
{
    int slot = (int)(intptr_t)arg;
    int fd = connection_fds[slot] - 1;
    HttpRequest req;
    HttpResponse resp;
    struct timeval tv = {20, 0};
    int socket_buffer = 512 * 1024;
    int nodelay = 1;

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &socket_buffer, sizeof(socket_buffer));
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &socket_buffer, sizeof(socket_buffer));
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
#ifdef SO_NOSIGPIPE
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
    memset(&resp, 0, sizeof(resp));
    resp.fd = fd;
    if (read_http_request(fd, &req) != 0) {
        resp.head_only = 0;
        respond_text(&resp, 400, "Bad Request", "Bad request");
        goto done;
    }
    resp.head_only = !strcmp(req.method, "HEAD");
    if (!strncmp(req.path, "/api/", 5)) {
        handle_api(&req, &resp);
    } else if (!strncmp(req.path, "/art/", 5)) {
        handle_art(&req, &resp);
    } else if (!strncmp(req.path, "/stream/", 8)) {
        handle_stream(&req, &resp);
    } else if (!strcmp(req.method, "GET") || resp.head_only) {
        handle_static(&req, &resp);
    } else {
        respond_text(&resp, 405, "Method Not Allowed", "Method not allowed");
    }
    free(req.body);
done:
    pthread_mutex_lock(&connections_lock);
    shutdown(fd, SHUT_RDWR);
    close(fd);
    connection_fds[slot] = 0;
    connection_count--;
    pthread_cond_signal(&connections_ready);
    pthread_mutex_unlock(&connections_lock);
    return NULL;
}

static void *
http_main(void *arg)
{
    (void)arg;
    pthread_attr_t attr;
    int attr_ready = pthread_attr_init(&attr) == 0;

    if (attr_ready) pthread_attr_setstacksize(&attr, 1024u * 1024u);
    while (!http_stop) {
        struct pollfd pfd = {listen_fd, POLLIN, 0};
        int rc = poll(&pfd, 1, 500);
        if (rc <= 0) continue;
        int fd = accept(listen_fd, NULL, NULL);
        if (fd < 0) continue;
        pthread_t thread;
        int slot;
        pthread_mutex_lock(&connections_lock);
        for (slot = 0; slot < HTTP_MAX_CONNECTIONS && connection_fds[slot]; slot++) {}
        if (slot < HTTP_MAX_CONNECTIONS) {
            connection_fds[slot] = fd + 1;
            connection_count++;
        }
        pthread_mutex_unlock(&connections_lock);
        if (slot == HTTP_MAX_CONNECTIONS) {
            close(fd);
        } else if (pthread_create(&thread, attr_ready ? &attr : NULL,
                                  connection_main, (void *)(intptr_t)slot) == 0) {
            pthread_detach(thread);
        } else {
            close(fd);
            pthread_mutex_lock(&connections_lock);
            connection_fds[slot] = 0;
            connection_count--;
            pthread_mutex_unlock(&connections_lock);
        }
    }
    if (attr_ready) pthread_attr_destroy(&attr);
    return NULL;
}

int
ng_http_start(int port)
{
    struct sockaddr_in addr;
    int one = 1;
    int socket_buffer = 512 * 1024;

    signal(SIGPIPE, SIG_IGN);
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        ng_log_error("socket: %s", strerror(errno));
        return -1;
    }
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    setsockopt(listen_fd, SOL_SOCKET, SO_RCVBUF, &socket_buffer,
               sizeof(socket_buffer));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        ng_log_error("bind %d: %s", port, strerror(errno));
        close(listen_fd);
        listen_fd = -1;
        return -1;
    }
    if (listen(listen_fd, 64) != 0) {
        ng_log_error("listen: %s", strerror(errno));
        close(listen_fd);
        listen_fd = -1;
        return -1;
    }
    http_stop = 0;
    if (pthread_create(&http_thread, NULL, http_main, NULL) != 0) {
        close(listen_fd);
        listen_fd = -1;
        return -1;
    }
    ng_log_info("HTTP 服务监听 0.0.0.0:%d", port);
    return 0;
}

void
ng_http_stop(void)
{
    if (listen_fd < 0) return;
    http_stop = 1;
    pthread_join(http_thread, NULL);
    close(listen_fd);
    listen_fd = -1;
    pthread_mutex_lock(&connections_lock);
    for (int i = 0; i < HTTP_MAX_CONNECTIONS; i++) {
        if (connection_fds[i]) shutdown(connection_fds[i] - 1, SHUT_RDWR);
    }
    while (connection_count) pthread_cond_wait(&connections_ready, &connections_lock);
    pthread_mutex_unlock(&connections_lock);
    ng_http_stream_cache_clear();
}
