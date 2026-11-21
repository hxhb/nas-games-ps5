#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "log.h"

static int
valid_id(const char *id)
{
    if (!id || !*id) return 0;
    for (const unsigned char *p = (const unsigned char *)id; *p; p++) {
        if (!isalnum(*p) && *p != '-' && *p != '_') return 0;
    }
    return strlen(id) < NG_ID_LEN;
}

void
ng_config_defaults(NgConfig *cfg, const char *data_dir)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->version = 1;
    cfg->port = 8098;
    cfg->auto_scan = 1;
    cfg->scan_interval_seconds = 300;
    cfg->max_scan_depth = 4;
    cfg->open_browser_on_start = 0;
    cfg->keep_awake = 1;
    ng_strlcpy(cfg->data_dir, data_dir ? data_dir : "/data/nasgames", sizeof(cfg->data_dir));
    ng_strlcpy(cfg->cache_dir, cfg->data_dir, sizeof(cfg->cache_dir));
    ng_strlcat(cfg->cache_dir, "/cache", sizeof(cfg->cache_dir));
    ng_strlcpy(cfg->art_dir, cfg->data_dir, sizeof(cfg->art_dir));
    ng_strlcat(cfg->art_dir, "/art", sizeof(cfg->art_dir));
}

static void
source_defaults(NgSourceConfig *source)
{
    memset(source, 0, sizeof(*source));
    source->enabled = 1;
    source->root_count = 1;
    ng_strlcpy(source->roots[0], "/", sizeof(source->roots[0]));
}

static void
normalize_smb_share(NgSourceConfig *source)
{
    char value[NG_TITLE_LEN];
    char share[NG_TITLE_LEN];
    char base[NG_PATH_LEN];
    char normalized[NG_PATH_LEN];
    char *slash;
    char *start;
    size_t length;

    if (source->type != NG_SOURCE_SMB) return;
    ng_strlcpy(value, source->share, sizeof(value));
    for (char *p = value; *p; p++) {
        if (*p == '\\') *p = '/';
    }
    start = value;
    while (*start == '/') start++;
    length = strlen(start);
    while (length && start[length - 1] == '/') start[--length] = '\0';
    if (!*start) {
        source->share[0] = '\0';
        return;
    }
    ng_strlcpy(share, start, sizeof(share));
    base[0] = '\0';
    slash = strchr(share, '/');
    if (slash) {
        ng_strlcpy(base, slash + 1, sizeof(base));
        *slash = '\0';
        if (ng_path_normalize(base, normalized, sizeof(normalized)) == 0) {
            ng_strlcpy(base, normalized, sizeof(base));
        } else {
            base[0] = '\0';
        }
    }
    ng_strlcpy(source->share, share, sizeof(source->share));
    if (!base[0] || !strcmp(base, "/")) return;

    for (int i = 0; i < source->root_count; i++) {
        if (!strcmp(source->roots[i], "/")) {
            ng_strlcpy(source->roots[i], base, sizeof(source->roots[i]));
            return;
        }
    }
    for (int i = 0; i < source->root_count; i++) {
        if (!strcmp(source->roots[i], base)) return;
    }
    if (source->root_count < NG_MAX_ROOTS) {
        ng_strlcpy(source->roots[source->root_count++], base, sizeof(source->roots[0]));
    }
}

NgSourceConfig *
ng_config_find_source(NgConfig *cfg, const char *id)
{
    if (!cfg || !id) return NULL;
    for (int i = 0; i < cfg->source_count; i++) {
        if (!strcmp(cfg->sources[i].id, id)) return &cfg->sources[i];
    }
    return NULL;
}

const NgSourceConfig *
ng_config_find_source_const(const NgConfig *cfg, const char *id)
{
    if (!cfg || !id) return NULL;
    for (int i = 0; i < cfg->source_count; i++) {
        if (!strcmp(cfg->sources[i].id, id)) return &cfg->sources[i];
    }
    return NULL;
}

static int
parse_roots(const NgJsonDoc *doc, int obj, NgSourceConfig *source)
{
    int roots = ng_json_obj_get(doc, obj, "roots");
    int count = roots >= 0 && doc->tokens[roots].type == NG_JSON_ARRAY ? doc->tokens[roots].size : 0;

    source->root_count = 0;
    for (int i = 0; i < count && source->root_count < NG_MAX_ROOTS; i++) {
        int item = ng_json_array_at(doc, roots, i);
        char raw[NG_PATH_LEN];
        char norm[NG_PATH_LEN];
        if (ng_json_string(doc, item, raw, sizeof(raw)) != 0) continue;
        if (ng_path_normalize(raw, norm, sizeof(norm)) != 0) continue;
        ng_strlcpy(source->roots[source->root_count++], norm,
                   sizeof(source->roots[0]));
    }
    if (source->root_count == 0) {
        char norm[NG_PATH_LEN];
        const char *fallback = "/";
        int root = ng_json_obj_get(doc, obj, "root");
        if (root >= 0 && ng_json_string(doc, root, norm, sizeof(norm)) == 0 &&
            ng_path_normalize(norm, norm, sizeof(norm)) == 0) {
            fallback = norm;
        }
        ng_strlcpy(source->roots[0], fallback, sizeof(source->roots[0]));
        source->root_count = 1;
    }
    return 0;
}

static int
parse_source(const NgJsonDoc *doc, int obj, const NgConfig *base, NgSourceConfig *source,
             char *error, size_t error_size)
{
    char type_name[32];
    char id[NG_ID_LEN];

    source_defaults(source);
    if (ng_json_obj_string(doc, obj, "id", id, sizeof(id), "") != 0 || !id[0]) {
        snprintf(id, sizeof(id), "source%d", obj);
    }
    ng_strlcpy(source->id, id, sizeof(source->id));
    ng_json_obj_string(doc, obj, "name", source->name, sizeof(source->name), source->id);
    if (ng_json_obj_string(doc, obj, "type", type_name, sizeof(type_name), "") != 0) {
        ng_json_obj_string(doc, obj, "protocol", type_name, sizeof(type_name), "smb");
    }
    source->type = ng_source_type_parse(type_name);
    source->enabled = ng_json_obj_bool(doc, obj, "enabled", 1);
    ng_json_obj_string(doc, obj, "host", source->host, sizeof(source->host), "");
    ng_json_obj_string(doc, obj, "share", source->share, sizeof(source->share), "");
    ng_json_obj_string(doc, obj, "username", source->username, sizeof(source->username), "");
    {
        char alias[NG_NAME_LEN];
        ng_strlcpy(alias, source->username, sizeof(alias));
        ng_json_obj_string(doc, obj, "user", source->username, sizeof(source->username), alias);
    }
    ng_json_obj_string(doc, obj, "password", source->password, sizeof(source->password), "");
    ng_json_obj_string(doc, obj, "domain", source->domain, sizeof(source->domain), "");
    ng_json_obj_string(doc, obj, "localRoot", source->local_root,
                       sizeof(source->local_root), "/");
    source->port = ng_json_obj_int(doc, obj, "port",
                                   source->type == NG_SOURCE_SMB ? 445 :
                                   source->type == NG_SOURCE_FTP ? 21 : 0);

    if (base && !source->password[0]) {
        const NgSourceConfig *old = ng_config_find_source_const(base, source->id);
        if (old && old->type == source->type && !strcasecmp(old->host, source->host) &&
            old->port == source->port && !strcmp(old->username, source->username) &&
            !strcasecmp(old->domain, source->domain)) {
            ng_strlcpy(source->password, old->password, sizeof(source->password));
        }
    }
    if (source->type == NG_SOURCE_NONE) {
        snprintf(error, error_size, "连接 %s 的协议无效", source->id);
        return -1;
    }
    if (!valid_id(source->id)) {
        snprintf(error, error_size, "连接 ID 无效: %s", source->id);
        return -1;
    }
    if (parse_roots(doc, obj, source) != 0) return -1;
    if (source->type == NG_SOURCE_LOCAL && !source->local_root[0]) {
        ng_strlcpy(source->local_root, "/", sizeof(source->local_root));
    }
    normalize_smb_share(source);
    if ((source->type == NG_SOURCE_SMB || source->type == NG_SOURCE_FTP) &&
        !source->host[0]) {
        snprintf(error, error_size, "连接 %s 缺少服务器地址", source->id);
        return -1;
    }
    return 0;
}

int
ng_config_from_json(const char *json, const NgConfig *base, NgConfig *out,
                    char *error, size_t error_size)
{
    NgJsonDoc doc;
    int root;
    int sources;

    if (error && error_size) error[0] = '\0';
    if (ng_json_parse(&doc, json, strlen(json)) != 0) {
        snprintf(error, error_size, "JSON 格式错误");
        return -1;
    }
    root = 0;
    if (doc.tokens[0].type != NG_JSON_OBJECT) {
        ng_json_free(&doc);
        snprintf(error, error_size, "设置必须是 JSON 对象");
        return -1;
    }
    *out = *base;
    int port = ng_json_obj_get(&doc, root, "port");
    if (port >= 0) out->port = ng_json_int(&doc, port, -1);
    out->auto_scan = ng_json_obj_bool(&doc, root, "auto_scan", out->auto_scan);
    out->scan_interval_seconds = ng_json_obj_int(&doc, root, "scan_interval_seconds",
                                                 out->scan_interval_seconds);
    out->max_scan_depth = ng_json_obj_int(&doc, root, "max_scan_depth", out->max_scan_depth);
    out->open_browser_on_start = ng_json_obj_bool(&doc, root, "open_browser_on_start",
                                                  out->open_browser_on_start);
    out->keep_awake = ng_json_obj_bool(&doc, root, "keep_awake", out->keep_awake);
    if (out->port < 1024 || out->port > 65535) {
        snprintf(error, error_size, "端口必须在 1024-65535 之间");
        ng_json_free(&doc);
        return -1;
    }
    if (out->scan_interval_seconds < 30) out->scan_interval_seconds = 30;
    if (out->scan_interval_seconds > 86400) out->scan_interval_seconds = 86400;
    if (out->max_scan_depth < 0) out->max_scan_depth = 0;
    if (out->max_scan_depth > 12) out->max_scan_depth = 12;

    sources = ng_json_obj_get(&doc, root, "sources");
    if (sources >= 0) {
        int count;
        if (doc.tokens[sources].type != NG_JSON_ARRAY) {
            snprintf(error, error_size, "sources 必须是数组");
            ng_json_free(&doc);
            return -1;
        }
        count = doc.tokens[sources].size;
        if (count > NG_MAX_SOURCES) {
            snprintf(error, error_size, "最多支持 %d 个连接", NG_MAX_SOURCES);
            ng_json_free(&doc);
            return -1;
        }
        out->source_count = 0;
        for (int i = 0; i < count; i++) {
            int item = ng_json_array_at(&doc, sources, i);
            if (item < 0 || doc.tokens[item].type != NG_JSON_OBJECT) {
                snprintf(error, error_size, "连接必须是 JSON 对象");
                ng_json_free(&doc);
                return -1;
            }
            if (parse_source(&doc, item, base, &out->sources[out->source_count],
                             error, error_size) != 0) {
                ng_json_free(&doc);
                return -1;
            }
            if (out->sources[out->source_count].type == NG_SOURCE_SMB &&
                !out->sources[out->source_count].share[0]) {
                snprintf(error, error_size, "连接 %s 缺少共享名",
                         out->sources[out->source_count].id);
                ng_json_free(&doc);
                return -1;
            }
            for (int j = 0; j < out->source_count; j++) {
                if (!strcmp(out->sources[j].id, out->sources[out->source_count].id)) {
                    snprintf(error, error_size, "连接 ID 重复: %s", out->sources[j].id);
                    ng_json_free(&doc);
                    return -1;
                }
            }
            out->source_count++;
        }
    }
    ng_json_free(&doc);
    return 0;
}

int
ng_config_source_from_json(const char *json, const NgConfig *base,
                           NgSourceConfig *out, char *error, size_t error_size)
{
    NgJsonDoc doc;
    int rc;

    if (error && error_size) error[0] = '\0';
    if (!json || !out || ng_json_parse(&doc, json, strlen(json)) != 0) {
        snprintf(error, error_size, "连接参数不是有效的 JSON");
        return -1;
    }
    if (doc.tokens[0].type != NG_JSON_OBJECT) {
        ng_json_free(&doc);
        snprintf(error, error_size, "连接参数必须是 JSON 对象");
        return -1;
    }
    rc = parse_source(&doc, 0, base, out, error, error_size);
    ng_json_free(&doc);
    return rc;
}

int
ng_config_load(NgConfig *cfg, const char *path)
{
    char *text = NULL;
    char error[256];
    NgConfig *loaded;

    if (ng_read_text_file(path, NG_MAX_HTTP_BODY, &text) < 0) return -1;
    loaded = malloc(sizeof(*loaded));
    if (!loaded) {
        free(text);
        return -1;
    }
    if (ng_config_from_json(text, cfg, loaded, error, sizeof(error)) != 0) {
        ng_log_error("config: %s", error);
        free(text);
        free(loaded);
        errno = EINVAL;
        return -1;
    }
    *cfg = *loaded;
    free(loaded);
    free(text);
    return 0;
}

static int
append_source_json(const NgSourceConfig *source, int include_secrets, NgStrBuf *out)
{
    if (ng_strbuf_append(out, "{") != 0 ||
        ng_strbuf_append(out, "\"id\":") != 0 ||
        ng_strbuf_json_string(out, source->id) != 0 ||
        ng_strbuf_append(out, ",\"name\":") != 0 ||
        ng_strbuf_json_string(out, source->name) != 0 ||
        ng_strbuf_append(out, ",\"type\":") != 0 ||
        ng_strbuf_json_string(out, ng_source_type_name(source->type)) != 0 ||
        ng_strbuf_printf(out, ",\"enabled\":%s", source->enabled ? "true" : "false") != 0 ||
        ng_strbuf_append(out, ",\"host\":") != 0 ||
        ng_strbuf_json_string(out, source->host) != 0 ||
        ng_strbuf_printf(out, ",\"port\":%d,\"share\":", source->port) != 0 ||
        ng_strbuf_json_string(out, source->share) != 0 ||
        ng_strbuf_append(out, ",\"username\":") != 0 ||
        ng_strbuf_json_string(out, source->username) != 0 ||
        ng_strbuf_append(out, ",\"domain\":") != 0 ||
        ng_strbuf_json_string(out, source->domain) != 0 ||
        ng_strbuf_append(out, ",\"localRoot\":") != 0 ||
        ng_strbuf_json_string(out, source->local_root) != 0 ||
        ng_strbuf_append(out, ",\"has_password\":") != 0 ||
        ng_strbuf_append(out, source->password[0] ? "true" : "false") != 0) {
        return -1;
    }
    if (include_secrets) {
        if (ng_strbuf_append(out, ",\"password\":") != 0 ||
            ng_strbuf_json_string(out, source->password) != 0) {
            return -1;
        }
    }
    if (ng_strbuf_append(out, ",\"roots\":[") != 0) return -1;
    for (int i = 0; i < source->root_count; i++) {
        if (i && ng_strbuf_append(out, ",") != 0) return -1;
        if (ng_strbuf_json_string(out, source->roots[i]) != 0) return -1;
    }
    return ng_strbuf_append(out, "]}");
}

int
ng_config_to_json(const NgConfig *cfg, int include_secrets, NgStrBuf *out)
{
    if (ng_strbuf_printf(out,
                         "{\"version\":%d,\"port\":%d,\"auto_scan\":%s,"
                         "\"scan_interval_seconds\":%d,\"max_scan_depth\":%d,"
                         "\"open_browser_on_start\":%s,\"keep_awake\":%s,\"data_dir\":",
                         cfg->version, cfg->port, cfg->auto_scan ? "true" : "false",
                         cfg->scan_interval_seconds, cfg->max_scan_depth,
                         cfg->open_browser_on_start ? "true" : "false",
                         cfg->keep_awake ? "true" : "false") != 0 ||
        ng_strbuf_json_string(out, cfg->data_dir) != 0 ||
        ng_strbuf_append(out, ",\"sources\":[") != 0) {
        return -1;
    }
    for (int i = 0; i < cfg->source_count; i++) {
        if (i && ng_strbuf_append(out, ",") != 0) return -1;
        if (append_source_json(&cfg->sources[i], include_secrets, out) != 0) return -1;
    }
    return ng_strbuf_append(out, "]}");
}

int
ng_config_save(const NgConfig *cfg, const char *path)
{
    NgStrBuf json;
    int rc;

    ng_strbuf_init(&json);
    if (ng_config_to_json(cfg, 1, &json) != 0) {
        ng_strbuf_free(&json);
        return -1;
    }
    if (ng_strbuf_append(&json, "\n") != 0) {
        ng_strbuf_free(&json);
        return -1;
    }
    rc = ng_write_file_atomic(path, json.data, json.len, 0600);
    ng_strbuf_free(&json);
    return rc;
}
