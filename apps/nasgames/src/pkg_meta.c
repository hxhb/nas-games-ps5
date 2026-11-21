#include "pkg_meta.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "json.h"
#include "util.h"

#define PKG_HEADER_BYTES 0x200u
#define PKG_CNT_HEADER_BYTES 0x80u
#define PKG_MAX_ENTRIES 2048u
#define PKG_MAX_TABLE_OFFSET 0x200000u
#define PKG_MAX_STRING_TABLE (1024u * 1024u)
#define PKG_MAX_PARAM_BYTES (256u * 1024u)
#define PKG_MAX_SFO_BYTES (512u * 1024u)
#define PKG_ENTRY_PARAM_JSON 0x2000u
#define PKG_ENTRY_PARAM_SFO 0x1000u
#define PKG_ENTRY_NAMES 0x0200u
#define PKG_ENTRY_ICON0_PNG 0x1200u
#define PKG_ENTRY_PIC0_PNG 0x1220u
#define PKG_ENTRY_PIC1_PNG 0x1006u
#define PKG_ENTRY_APP_PLAYGO 0x1008u
#define PKG_ENTRY_TARGET_DELTA 0x0407u
#define PKG_ENTRY_ORIGIN_DELTA 0x0408u

static uint16_t
read_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}

static uint32_t
read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t
read_le64(const uint8_t *p)
{
    uint64_t value = 0;
    for (int i = 7; i >= 0; i--) value = (value << 8) | p[i];
    return value;
}

static uint32_t
read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void
copy_value(char *dst, size_t dst_size, const void *src, size_t src_size)
{
    size_t length = src_size;

    if (!dst || dst_size == 0) return;
    if (!src) length = 0;
    while (length > 0 && ((const char *)src)[length - 1] == '\0') length--;
    if (length >= dst_size) length = dst_size - 1;
    if (length > 0) memcpy(dst, src, length);
    dst[length] = '\0';
}

static void
set_error(char *error, size_t error_size, const char *message)
{
    if (error && error_size) snprintf(error, error_size, "%s", message ? message : "");
}

static int
source_read_exact(NgSourceFile *file, uint64_t offset, void *buffer, size_t size)
{
    size_t done = 0;

    if (!file || (!buffer && size)) return -1;
    if (file->size && (offset > file->size || size > file->size - offset)) return -1;
    if (ng_source_file_seek(file, offset) != 0) return -1;
    while (done < size) {
        ssize_t count = ng_source_file_read(file, (char *)buffer + done, size - done);
        if (count <= 0) return -1;
        done += (size_t)count;
    }
    return 0;
}

static int
entry_payload_valid(uint64_t cnt_offset, uint64_t file_size, uint32_t data_offset,
                    uint32_t data_size, uint64_t *absolute)
{
    uint64_t start;

    if (cnt_offset > file_size || data_offset > file_size - cnt_offset) return 0;
    start = cnt_offset + data_offset;
    if (start > file_size || data_size > file_size - start) return 0;
    if (absolute) *absolute = start;
    return 1;
}

static const char *
entry_name(const char *table, size_t table_size, uint32_t offset)
{
    const char *name;

    if (!table || offset >= table_size) return NULL;
    name = table + offset;
    if (!memchr(name, '\0', table_size - offset)) return NULL;
    return name;
}

static int
sfo_language_rank(int index)
{
    switch (index) {
    case 11: return 100;
    case 10: return 90;
    case 1: return 80;
    case 18: return 70;
    case 21: return 60;
    default: return index >= 0 && index < 30 ? 10 : -1;
    }
}

int
ng_app_meta_from_json(const char *json, NgPkgMeta *meta)
{
    static const char *preferred[] = {
        "zh-CN", "zh-Hans", "zh-Hant", "zh", "en-US", "en", NULL
    };
    NgJsonDoc doc;
    int root;
    int localized;

    if (!json || !meta || ng_json_parse(&doc, json, strlen(json)) != 0) return -1;
    root = 0;
    if (doc.tokens[root].type != NG_JSON_OBJECT) {
        ng_json_free(&doc);
        return -1;
    }
    if (!meta->title_id[0]) {
        ng_json_obj_string(&doc, root, "titleId", meta->title_id,
                           sizeof(meta->title_id), "");
        if (!meta->title_id[0]) {
            ng_json_obj_string(&doc, root, "title_id", meta->title_id,
                               sizeof(meta->title_id), "");
        }
    }
    if (!meta->version[0]) {
        ng_json_obj_string(&doc, root, "contentVersion", meta->version,
                           sizeof(meta->version), "");
        if (!meta->version[0]) {
            ng_json_obj_string(&doc, root, "appVersion", meta->version,
                               sizeof(meta->version), "");
        }
        if (!meta->version[0]) {
            ng_json_obj_string(&doc, root, "version", meta->version,
                               sizeof(meta->version), "");
        }
    }
    if (!meta->category[0]) {
        ng_json_obj_string(&doc, root, "category", meta->category,
                           sizeof(meta->category), "");
        if (!meta->category[0]) {
            ng_json_obj_string(&doc, root, "applicationCategoryType", meta->category,
                               sizeof(meta->category), "");
        }
    }
    if (!meta->content_id[0]) {
        ng_json_obj_string(&doc, root, "contentId", meta->content_id,
                           sizeof(meta->content_id), "");
        if (!meta->content_id[0]) {
            ng_json_obj_string(&doc, root, "content_id", meta->content_id,
                               sizeof(meta->content_id), "");
        }
        if (!meta->content_id[0]) {
            ng_json_obj_string(&doc, root, "entitlementLabel", meta->content_id,
                               sizeof(meta->content_id), "");
        }
        if (!meta->content_id[0]) {
            ng_json_obj_string(&doc, root, "entitlement_label", meta->content_id,
                               sizeof(meta->content_id), "");
        }
    }

    localized = ng_json_obj_get(&doc, root, "localizedParameters");
    if (!meta->title_name[0] && localized >= 0 &&
        doc.tokens[localized].type == NG_JSON_OBJECT) {
        char default_language[64] = "";
        ng_json_obj_string(&doc, localized, "defaultLanguage", default_language,
                           sizeof(default_language), "");
        for (int i = 0; preferred[i] && !meta->title_name[0]; i++) {
            int language = ng_json_obj_get(&doc, localized, preferred[i]);
            if (language >= 0 && doc.tokens[language].type == NG_JSON_OBJECT) {
                ng_json_obj_string(&doc, language, "titleName", meta->title_name,
                                   sizeof(meta->title_name), "");
            }
        }
        if (!meta->title_name[0] && default_language[0]) {
            int language = ng_json_obj_get(&doc, localized, default_language);
            if (language >= 0 && doc.tokens[language].type == NG_JSON_OBJECT) {
                ng_json_obj_string(&doc, language, "titleName", meta->title_name,
                                   sizeof(meta->title_name), "");
            }
        }
        if (!meta->title_name[0]) {
            for (int i = localized + 1; i + 1 < doc.count; i++) {
                int language = i + 1;
                if (doc.tokens[i].parent != localized || !doc.tokens[i].is_key) continue;
                if (doc.tokens[language].parent != localized ||
                    doc.tokens[language].type != NG_JSON_OBJECT) continue;
                ng_json_obj_string(&doc, language, "titleName", meta->title_name,
                                   sizeof(meta->title_name), "");
                if (meta->title_name[0]) break;
            }
        }
    }
    if (!meta->title_name[0]) {
        ng_json_obj_string(&doc, root, "titleName", meta->title_name,
                           sizeof(meta->title_name), "");
        if (!meta->title_name[0]) {
            ng_json_obj_string(&doc, root, "title", meta->title_name,
                               sizeof(meta->title_name), "");
        }
    }
    ng_json_free(&doc);
    return 0;
}

int
ng_app_meta_from_sfo(const void *data, size_t size, NgPkgMeta *meta)
{
    const uint8_t *sfo = data;
    uint32_t key_table;
    uint32_t data_table;
    uint32_t entries;
    char title[NG_TITLE_LEN] = "";
    char version[64] = "";
    char app_version[64] = "";
    int title_rank = -1;

    if (!sfo || !meta || size < 20 || memcmp(sfo, "\x00PSF", 4) != 0) return -1;
    key_table = read_le32(sfo + 0x08);
    data_table = read_le32(sfo + 0x0C);
    entries = read_le32(sfo + 0x10);
    if (key_table >= size || data_table >= size || entries > 1024) return -1;
    if (entries > (size - 20) / 16) return -1;

    for (uint32_t i = 0; i < entries; i++) {
        const uint8_t *entry = sfo + 20 + (size_t)i * 16;
        uint16_t key_offset = read_le16(entry);
        uint32_t data_length = read_le32(entry + 4);
        uint32_t data_offset = read_le32(entry + 12);
        const char *key;
        const char *value;
        size_t key_space;

        if (key_offset >= size - key_table) continue;
        key = (const char *)sfo + key_table + key_offset;
        key_space = size - key_table - key_offset;
        if (!memchr(key, '\0', key_space)) continue;
        if (data_offset > size || data_length > size ||
            data_offset > size - data_table || data_length > size - data_table - data_offset) {
            continue;
        }
        value = (const char *)sfo + data_table + data_offset;
        if (!strcmp(key, "TITLE_ID") && !meta->title_id[0]) {
            copy_value(meta->title_id, sizeof(meta->title_id), value, data_length);
        } else if (!strcmp(key, "CATEGORY") && !meta->category[0]) {
            copy_value(meta->category, sizeof(meta->category), value, data_length);
        } else if (!strcmp(key, "CONTENT_ID") && !meta->content_id[0]) {
            copy_value(meta->content_id, sizeof(meta->content_id), value, data_length);
        } else if (!strcmp(key, "ENTITLEMENT_LABEL") && !meta->content_id[0]) {
            copy_value(meta->content_id, sizeof(meta->content_id), value, data_length);
        } else if (!strcmp(key, "APP_VER") && !app_version[0]) {
            copy_value(app_version, sizeof(app_version), value, data_length);
        } else if (!strcmp(key, "VERSION") && !version[0]) {
            copy_value(version, sizeof(version), value, data_length);
        } else if (!strcmp(key, "TITLE")) {
            if (title_rank < 0) {
                copy_value(title, sizeof(title), value, data_length);
                title_rank = 0;
            }
        } else if (!strncmp(key, "TITLE_", 6) && isdigit((unsigned char)key[6])) {
            char *end = NULL;
            long index = strtol(key + 6, &end, 10);
            int rank;
            if (!end || *end || index < 0 || index >= 30) continue;
            rank = sfo_language_rank((int)index);
            if (rank <= title_rank) continue;
            copy_value(title, sizeof(title), value, data_length);
            title_rank = rank;
        }
    }

    if (!meta->title_name[0] && title[0]) {
        ng_strlcpy(meta->title_name, title, sizeof(meta->title_name));
    }
    if (!meta->version[0]) {
        ng_strlcpy(meta->version, app_version[0] ? app_version : version,
                   sizeof(meta->version));
    }
    return 0;
}

static int
locate_cnt(NgSourceFile *file, uint64_t file_size, uint64_t *cnt_offset)
{
    uint8_t header[PKG_HEADER_BYTES];
    uint64_t candidate;
    uint8_t magic[4];

    if (source_read_exact(file, 0, header, sizeof(header)) != 0) return -1;
    if (!memcmp(header, "\x7f" "CNT", 4)) {
        *cnt_offset = 0;
        return 0;
    }
    if (memcmp(header, "\x7f" "FIH", 4) != 0) return -1;

    candidate = read_le64(header + 0x58);
    if (candidate > 0 && candidate < file_size &&
        source_read_exact(file, candidate, magic, sizeof(magic)) == 0 &&
        !memcmp(magic, "\x7f" "CNT", 4)) {
        *cnt_offset = candidate;
        return 0;
    }
    for (size_t offset = 0x10; offset + 8 <= sizeof(header); offset += 8) {
        candidate = read_le64(header + offset);
        if (candidate < 0x10000 || candidate >= file_size || (candidate % 0x1000) != 0) {
            continue;
        }
        if (source_read_exact(file, candidate, magic, sizeof(magic)) == 0 &&
            !memcmp(magic, "\x7f" "CNT", 4)) {
            *cnt_offset = candidate;
            return 0;
        }
    }
    return -1;
}

static NgPkgKind
classify_package(const NgPkgMeta *meta, uint32_t flags, int has_delta, int has_app_playgo)
{
    if (has_delta || has_app_playgo || !strncasecmp(meta->category, "gp", 2)) {
        return NG_PKG_UPDATE;
    }
    if (!strncasecmp(meta->category, "ac", 2) ||
        !strncasecmp(meta->category, "al", 2) ||
        !strcasecmp(meta->category, "addcont")) {
        return NG_PKG_DLC;
    }
    if (!strncasecmp(meta->category, "gd", 2) ||
        !strncasecmp(meta->category, "bd", 2) ||
        !strncasecmp(meta->category, "gc", 2) ||
        !strncasecmp(meta->category, "wt", 2)) {
        return NG_PKG_BASE;
    }
    if ((flags & 0xffu) == 0x1eu || (flags & 0xff000000u) == 0x41000000u) {
        return NG_PKG_UPDATE;
    }
    return NG_PKG_BASE;
}

static void
merge_meta(NgPkgMeta *target, const NgPkgMeta *source)
{
    if (!target->title_id[0] && source->title_id[0]) {
        ng_strlcpy(target->title_id, source->title_id, sizeof(target->title_id));
    }
    if (!target->title_name[0] && source->title_name[0]) {
        ng_strlcpy(target->title_name, source->title_name, sizeof(target->title_name));
    }
    if (!target->version[0] && source->version[0]) {
        ng_strlcpy(target->version, source->version, sizeof(target->version));
    }
    if (!target->category[0] && source->category[0]) {
        ng_strlcpy(target->category, source->category, sizeof(target->category));
    }
    if (!target->content_id[0] && source->content_id[0]) {
        ng_strlcpy(target->content_id, source->content_id, sizeof(target->content_id));
    }
}

static void
title_id_from_content_id(const char *content_id, char *title_id, size_t title_id_size)
{
    const char *dash = strchr(content_id, '-');
    const char *underscore;
    size_t length;

    if (!dash) return;
    underscore = strchr(dash + 1, '_');
    if (!underscore) return;
    length = (size_t)(underscore - dash - 1);
    if (length != 9 || length >= title_id_size) return;
    memcpy(title_id, dash + 1, length);
    title_id[length] = '\0';
    if (!ng_valid_title_id(title_id)) title_id[0] = '\0';
}

int
ng_pkg_probe(NgSource *source, const char *path, uint64_t requested_size,
             NgPkgMeta *meta, char *error, size_t error_size)
{
    NgSourceFile *file;
    uint64_t file_size;
    uint64_t cnt_offset = 0;
    uint8_t cnt_header[PKG_CNT_HEADER_BYTES];
    uint32_t entry_count;
    uint32_t table_offset;
    uint32_t flags;
    uint8_t *table = NULL;
    char *strings = NULL;
    size_t table_size;
    size_t string_size = 0;
    int has_delta = 0;
    int has_app_playgo = 0;
    int rc = -1;

    if (!source || !path || !meta) return -1;
    memset(meta, 0, sizeof(*meta));
    if (error && error_size) error[0] = '\0';
    file = ng_source_open_file(source, path);
    if (!file) {
        set_error(error, error_size, "无法打开 PKG 文件");
        return -1;
    }
    file_size = file->size ? file->size : requested_size;
    if (!file_size) {
        set_error(error, error_size, "无法确定 PKG 文件大小");
        goto out;
    }
    if (locate_cnt(file, file_size, &cnt_offset) != 0) {
        set_error(error, error_size, "未找到 PKG 内的 CNT 元数据容器");
        goto out;
    }
    if (source_read_exact(file, cnt_offset, cnt_header, sizeof(cnt_header)) != 0 ||
        memcmp(cnt_header, "\x7f" "CNT", 4) != 0) {
        set_error(error, error_size, "PKG CNT 头无效或读取失败");
        goto out;
    }
    flags = read_be32(cnt_header + 0x04);
    entry_count = read_be32(cnt_header + 0x10);
    table_offset = read_be32(cnt_header + 0x18);
    if (entry_count == 0 || entry_count > PKG_MAX_ENTRIES ||
        table_offset >= PKG_MAX_TABLE_OFFSET) {
        set_error(error, error_size, "PKG 条目表参数无效");
        goto out;
    }
    table_size = (size_t)entry_count * 32u;
    if (!entry_payload_valid(cnt_offset, file_size, table_offset,
                             (uint32_t)table_size, NULL)) {
        set_error(error, error_size, "PKG 条目表超出文件范围");
        goto out;
    }
    table = malloc(table_size);
    if (!table) {
        set_error(error, error_size, "内存不足，无法读取 PKG 条目表");
        goto out;
    }
    if (source_read_exact(file, cnt_offset + table_offset, table, table_size) != 0) {
        set_error(error, error_size, "读取 PKG 条目表失败");
        goto out;
    }
    for (uint32_t i = 0; i < entry_count; i++) {
        const uint8_t *entry = table + (size_t)i * 32;
        uint32_t type = read_be32(entry);
        if (type != PKG_ENTRY_NAMES) continue;
        uint32_t string_offset = read_be32(entry + 16);
        uint32_t string_length = read_be32(entry + 20);
        if (string_length == 0 || string_length > PKG_MAX_STRING_TABLE) break;
        if (!entry_payload_valid(cnt_offset, file_size, string_offset,
                                 string_length, NULL)) {
            break;
        }
        strings = malloc((size_t)string_length + 1);
        if (!strings) break;
        if (source_read_exact(file, cnt_offset + string_offset,
                              strings, string_length) != 0) {
            free(strings);
            strings = NULL;
            break;
        }
        strings[string_length] = '\0';
        string_size = string_length;
        break;
    }

    for (uint32_t i = 0; i < entry_count; i++) {
        const uint8_t *entry = table + (size_t)i * 32;
        uint32_t type = read_be32(entry);
        uint32_t name_offset = read_be32(entry + 4);
        uint32_t data_offset = read_be32(entry + 16);
        uint32_t data_size = read_be32(entry + 20);
        uint64_t absolute = 0;
        const char *name = entry_name(strings, string_size, name_offset);

        if (!entry_payload_valid(cnt_offset, file_size, data_offset, data_size, &absolute)) {
            continue;
        }
        if (type == PKG_ENTRY_TARGET_DELTA || type == PKG_ENTRY_ORIGIN_DELTA ||
            (name && (!strcmp(name, "target-deltainfo.dat") ||
                      !strcmp(name, "origin-deltainfo.dat")))) {
            has_delta = 1;
        }
        if (type == PKG_ENTRY_APP_PLAYGO ||
            (name && !strcmp(name, "app/playgo-chunk.dat"))) {
            has_app_playgo = 1;
        }
        if (type == PKG_ENTRY_ICON0_PNG || (name && !strcmp(name, "icon0.png"))) {
            if (data_size > 0 && data_size <= NG_MAX_ART_BYTES) {
                meta->icon_offset = absolute;
                meta->icon_size = data_size;
                meta->has_icon = 1;
            }
        }
        if (type == PKG_ENTRY_PIC0_PNG || (name && !strcmp(name, "pic0.png"))) {
            if (data_size > 0 && data_size <= NG_MAX_ART_BYTES) {
                meta->pic0_offset = absolute;
                meta->pic0_size = data_size;
                meta->has_pic0 = 1;
            }
        }
        if (type == PKG_ENTRY_PIC1_PNG || (name && !strcmp(name, "pic1.png"))) {
            if (data_size > 0 && data_size <= NG_MAX_ART_BYTES) {
                meta->pic1_offset = absolute;
                meta->pic1_size = data_size;
                meta->has_pic1 = 1;
            }
        }
        if ((type == PKG_ENTRY_PARAM_JSON || (name && !strcmp(name, "param.json"))) &&
            data_size > 0 && data_size <= PKG_MAX_PARAM_BYTES) {
            char *text = malloc((size_t)data_size + 1);
            NgPkgMeta parsed;
            if (text && source_read_exact(file, absolute, text, data_size) == 0) {
                text[data_size] = '\0';
                memset(&parsed, 0, sizeof(parsed));
                if (ng_app_meta_from_json(text, &parsed) == 0) merge_meta(meta, &parsed);
            }
            free(text);
        }
        if ((type == PKG_ENTRY_PARAM_SFO || (name && !strcmp(name, "param.sfo"))) &&
            data_size > 0 && data_size <= PKG_MAX_SFO_BYTES) {
            uint8_t *data = malloc(data_size);
            NgPkgMeta parsed;
            if (data && source_read_exact(file, absolute, data, data_size) == 0) {
                memset(&parsed, 0, sizeof(parsed));
                if (ng_app_meta_from_sfo(data, data_size, &parsed) == 0) merge_meta(meta, &parsed);
            }
            free(data);
        }
    }

    if (!ng_valid_content_id(meta->content_id)) {
        copy_value(meta->content_id, sizeof(meta->content_id), cnt_header + 0x40, 48);
        if (!ng_valid_content_id(meta->content_id)) meta->content_id[0] = '\0';
    }
    if (!meta->title_id[0]) {
        title_id_from_content_id(meta->content_id, meta->title_id,
                                 sizeof(meta->title_id));
    }
    if (!ng_valid_title_id(meta->title_id)) meta->title_id[0] = '\0';
    meta->kind = classify_package(meta, flags, has_delta, has_app_playgo);
    rc = 0;

out:
    free(strings);
    free(table);
    ng_source_file_close(file);
    return rc;
}

int
ng_pkg_read_range(NgSource *source, const char *path, uint64_t offset,
                  uint32_t size, void **out, size_t *out_size)
{
    NgSourceFile *file;
    void *buffer;

    if (!source || !path || !out || size == 0 || size > NG_MAX_ART_BYTES) return -1;
    *out = NULL;
    if (out_size) *out_size = 0;
    file = ng_source_open_file(source, path);
    if (!file) return -1;
    if (file->size && (offset > file->size || size > file->size - offset)) {
        ng_source_file_close(file);
        errno = EINVAL;
        return -1;
    }
    buffer = malloc(size);
    if (!buffer) {
        ng_source_file_close(file);
        errno = ENOMEM;
        return -1;
    }
    if (source_read_exact(file, offset, buffer, size) != 0) {
        free(buffer);
        ng_source_file_close(file);
        errno = EIO;
        return -1;
    }
    ng_source_file_close(file);
    *out = buffer;
    if (out_size) *out_size = size;
    return 0;
}
