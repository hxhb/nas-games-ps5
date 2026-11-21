#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

static int
ng_strbuf_grow(NgStrBuf *sb, size_t need)
{
    size_t cap;
    char *data;

    if (need <= sb->cap) {
        return 0;
    }
    cap = sb->cap ? sb->cap : 256;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) {
            errno = ENOMEM;
            return -1;
        }
        cap *= 2;
    }
    data = realloc(sb->data, cap);
    if (!data) {
        return -1;
    }
    sb->data = data;
    sb->cap = cap;
    return 0;
}

void
ng_strbuf_init(NgStrBuf *sb)
{
    memset(sb, 0, sizeof(*sb));
}

void
ng_strbuf_free(NgStrBuf *sb)
{
    free(sb->data);
    memset(sb, 0, sizeof(*sb));
}

int
ng_strbuf_reserve(NgStrBuf *sb, size_t extra)
{
    if (sb->failed) return -1;
    if (extra > SIZE_MAX - sb->len - 1) {
        sb->failed = 1;
        errno = EOVERFLOW;
        return -1;
    }
    if (ng_strbuf_grow(sb, sb->len + extra + 1) != 0) {
        sb->failed = 1;
        return -1;
    }
    return 0;
}

int
ng_strbuf_appendn(NgStrBuf *sb, const char *text, size_t len)
{
    if (ng_strbuf_reserve(sb, len) != 0) {
        return -1;
    }
    memcpy(sb->data + sb->len, text, len);
    sb->len += len;
    sb->data[sb->len] = '\0';
    return 0;
}

int
ng_strbuf_append(NgStrBuf *sb, const char *text)
{
    return ng_strbuf_appendn(sb, text, strlen(text));
}

int
ng_strbuf_printf(NgStrBuf *sb, const char *fmt, ...)
{
    va_list ap;
    va_list cp;
    int needed;

    va_start(ap, fmt);
    va_copy(cp, ap);
    needed = vsnprintf(NULL, 0, fmt, cp);
    va_end(cp);
    if (needed < 0) {
        sb->failed = 1;
        va_end(ap);
        return -1;
    }
    if (ng_strbuf_reserve(sb, (size_t)needed) != 0) {
        va_end(ap);
        return -1;
    }
    vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, ap);
    va_end(ap);
    sb->len += (size_t)needed;
    return 0;
}

static size_t
utf8_sequence_length(const unsigned char *text)
{
    unsigned char first = text[0];
    if (first < 0x80) return 1;
    if (first < 0xc2 || first > 0xf4) return 0;
    size_t length = first <= 0xdf ? 2 : first <= 0xef ? 3 : 4;
    for (size_t index = 1; index < length; index++) {
        if ((text[index] & 0xc0) != 0x80) return 0;
    }
    if ((first == 0xe0 && text[1] < 0xa0) || (first == 0xed && text[1] >= 0xa0) ||
        (first == 0xf0 && text[1] < 0x90) || (first == 0xf4 && text[1] >= 0x90)) return 0;
    return length;
}

int
ng_strbuf_json_string(NgStrBuf *sb, const char *text)
{
    const unsigned char *p = (const unsigned char *)(text ? text : "");

    if (ng_strbuf_append(sb, "\"") != 0) {
        return -1;
    }
    while (*p) {
        if (*p >= 0x80) {
            size_t length = utf8_sequence_length(p);
            if (length) {
                if (ng_strbuf_appendn(sb, (const char *)p, length) != 0) return -1;
                p += length;
            } else {
                if (ng_strbuf_append(sb, "\\ufffd") != 0) return -1;
                p++;
            }
            continue;
        }
        switch (*p) {
        case '"':
            if (ng_strbuf_append(sb, "\\\"") != 0) return -1;
            break;
        case '\\':
            if (ng_strbuf_append(sb, "\\\\") != 0) return -1;
            break;
        case '\b':
            if (ng_strbuf_append(sb, "\\b") != 0) return -1;
            break;
        case '\f':
            if (ng_strbuf_append(sb, "\\f") != 0) return -1;
            break;
        case '\n':
            if (ng_strbuf_append(sb, "\\n") != 0) return -1;
            break;
        case '\r':
            if (ng_strbuf_append(sb, "\\r") != 0) return -1;
            break;
        case '\t':
            if (ng_strbuf_append(sb, "\\t") != 0) return -1;
            break;
        default:
            if (*p < 0x20) {
                if (ng_strbuf_printf(sb, "\\u%04x", *p) != 0) return -1;
            } else if (ng_strbuf_appendn(sb, (const char *)p, 1) != 0) {
                return -1;
            }
            break;
        }
        p++;
    }
    return ng_strbuf_append(sb, "\"");
}

void
ng_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len;

    if (!size) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    len = strlen(src);
    if (len >= size) {
        len = size - 1;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
}

void
ng_strlcat(char *dst, const char *src, size_t size)
{
    size_t used;
    size_t left;

    if (!size) {
        return;
    }
    used = strlen(dst);
    if (used >= size - 1) {
        return;
    }
    left = size - used - 1;
    ng_strlcpy(dst + used, src, left + 1);
}

void
ng_trim(char *text)
{
    char *start;
    char *end;

    if (!text) {
        return;
    }
    start = text;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
}

int
ng_mkdir_p(const char *path, mode_t mode)
{
    char tmp[NG_PATH_LEN];
    size_t len;

    if (!path || !*path) {
        errno = EINVAL;
        return -1;
    }
    ng_strlcpy(tmp, path, sizeof(tmp));
    len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') {
        tmp[--len] = '\0';
    }
    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') {
            continue;
        }
        *p = '\0';
        if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
            return -1;
        }
        *p = '/';
    }
    if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

int
ng_read_text_file(const char *path, size_t max_bytes, char **out)
{
    FILE *fp;
    char *buf;
    size_t cap;
    size_t len = 0;
    size_t n;

    *out = NULL;
    fp = fopen(path, "rb");
    if (!fp) {
        return -1;
    }
    cap = max_bytes < 65536 ? max_bytes + 1 : 65536;
    if (cap > max_bytes + 1) {
        cap = max_bytes + 1;
    }
    buf = malloc(cap);
    if (!buf) {
        fclose(fp);
        return -1;
    }
    while ((n = fread(buf + len, 1, cap - len - 1, fp)) > 0) {
        len += n;
        if (len >= max_bytes) {
            break;
        }
        if (len + 1 == cap) {
            size_t next = cap * 2;
            char *tmp;
            if (next > max_bytes + 1) next = max_bytes + 1;
            if (next <= cap) break;
            tmp = realloc(buf, next);
            if (!tmp) {
                free(buf);
                fclose(fp);
                return -1;
            }
            buf = tmp;
            cap = next;
        }
    }
    if (len == max_bytes && fgetc(fp) != EOF) {
        free(buf);
        fclose(fp);
        errno = EFBIG;
        return -1;
    }
    if (ferror(fp)) {
        free(buf);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    buf[len] = '\0';
    *out = buf;
    return (int)len;
}

int
ng_ensure_parent_dir(const char *path, mode_t mode)
{
    char tmp[NG_PATH_LEN];
    char *slash;

    ng_strlcpy(tmp, path, sizeof(tmp));
    slash = strrchr(tmp, '/');
    if (!slash || slash == tmp) {
        return 0;
    }
    *slash = '\0';
    return ng_mkdir_p(tmp, mode);
}

int
ng_write_file_atomic(const char *path, const void *data, size_t size, mode_t mode)
{
    char tmp[NG_PATH_LEN];
    int fd;
    const char *ptr = data;
    size_t left = size;
    int error;

    if (ng_ensure_parent_dir(path, 0777) != 0) {
        return -1;
    }
    if (snprintf(tmp, sizeof(tmp), "%s.tmp.XXXXXX", path) >= (int)sizeof(tmp)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    fd = mkstemp(tmp);
    if (fd < 0) {
        return -1;
    }
    while (left) {
        ssize_t n = write(fd, ptr, left);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            if (!n) errno = EIO;
            goto fail;
        }
        ptr += n;
        left -= (size_t)n;
    }
    if (fchmod(fd, mode) != 0 && errno != EOPNOTSUPP) goto fail;
    if (fsync(fd) != 0 && errno != EINVAL) goto fail;
    if (close(fd) != 0) {
        fd = -1;
        goto fail;
    }
    fd = -1;
    if (rename(tmp, path) != 0) goto fail;
    return 0;

fail:
    error = errno;
    if (fd >= 0) close(fd);
    unlink(tmp);
    errno = error;
    return -1;
}

int
ng_copy_file_local(const char *src, const char *dst, mode_t mode)
{
    FILE *in;
    FILE *out;
    char buf[65536];
    size_t n;
    int rc = 0;

    in = fopen(src, "rb");
    if (!in) return -1;
    if (ng_ensure_parent_dir(dst, 0777) != 0) {
        fclose(in);
        return -1;
    }
    out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            rc = -1;
            break;
        }
    }
    if (ferror(in)) rc = -1;
    if (fclose(out) != 0) rc = -1;
    fclose(in);
    if (rc == 0) chmod(dst, mode);
    return rc;
}

uint64_t
ng_hash64(const char *text)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    const unsigned char *p = (const unsigned char *)(text ? text : "");

    while (*p) {
        hash ^= *p++;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

void
ng_make_game_id(char *out, size_t size, const char *source_id, const char *path)
{
    char key[NG_PATH_LEN + NG_ID_LEN + 2];
    uint64_t hash;

    snprintf(key, sizeof(key), "%s:%s", source_id ? source_id : "", path ? path : "");
    hash = ng_hash64(key);
    snprintf(out, size, "%016llx", (unsigned long long)hash);
}

int
ng_path_normalize(const char *input, char *out, size_t out_size)
{
    char src[NG_PATH_LEN * 2];
    char *parts[256];
    int count = 0;
    char *save = NULL;
    char *tok;
    size_t used = 0;

    if (!input) input = "/";
    if (strlen(input) >= sizeof(src)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    ng_strlcpy(src, input, sizeof(src));
    for (char *p = src; *p; p++) {
        if (*p == '\\') *p = '/';
    }
    tok = strtok_r(src, "/", &save);
    while (tok) {
        if (!strcmp(tok, ".") || !*tok) {
            tok = strtok_r(NULL, "/", &save);
            continue;
        }
        if (!strcmp(tok, "..")) {
            if (count == 0) {
                errno = EACCES;
                return -1;
            }
            count--;
        } else {
            if (count >= (int)(sizeof(parts) / sizeof(parts[0]))) {
                errno = ENAMETOOLONG;
                return -1;
            }
            parts[count++] = tok;
        }
        tok = strtok_r(NULL, "/", &save);
    }
    if (out_size < 2) return -1;
    out[used++] = '/';
    for (int i = 0; i < count; i++) {
        size_t len = strlen(parts[i]);
        if (i > 0) {
            if (used + 1 >= out_size) return -1;
            out[used++] = '/';
        }
        if (used + len >= out_size) return -1;
        memcpy(out + used, parts[i], len);
        used += len;
    }
    out[used] = '\0';
    return 0;
}

int
ng_path_join_local(char *out, size_t out_size, const char *root, const char *remote_path)
{
    char norm[NG_PATH_LEN];
    int n;

    if (ng_path_normalize(remote_path, norm, sizeof(norm)) != 0) {
        return -1;
    }
    if (!strcmp(norm, "/")) {
        n = snprintf(out, out_size, "%s", root);
    } else {
        n = snprintf(out, out_size, "%s%s", root, norm);
    }
    return n > 0 && (size_t)n < out_size ? 0 : -1;
}

const char *
ng_path_basename(const char *path)
{
    const char *slash;

    if (!path) return "";
    slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

const char *
ng_path_extension(const char *path)
{
    const char *base = ng_path_basename(path);
    const char *dot = strrchr(base, '.');
    return dot ? dot + 1 : "";
}

int
ng_path_has_prefix(const char *path, const char *prefix)
{
    size_t len;

    if (!path || !prefix || !*prefix) return 0;
    len = strlen(prefix);
    if (strncmp(path, prefix, len) != 0) return 0;
    return path[len] == '\0' || path[len] == '/' || prefix[len - 1] == '/';
}

int
ng_extract_title_id(const char *text, char *out, size_t out_size)
{
    size_t len;

    if (out_size < 10) return -1;
    out[0] = '\0';
    if (!text) return -1;
    len = strlen(text);
    for (size_t i = 0; i + 9 <= len; i++) {
        int ok = 1;
        for (int j = 0; j < 4; j++) {
            if (!isupper((unsigned char)text[i + j])) ok = 0;
        }
        for (int j = 4; j < 9; j++) {
            if (!isdigit((unsigned char)text[i + j])) ok = 0;
        }
        if (ok && strncmp(text + i, "NPXS", 4) != 0) {
            memcpy(out, text + i, 9);
            out[9] = '\0';
            return 0;
        }
    }
    return -1;
}

int
ng_valid_title_id(const char *title_id)
{
    if (!title_id || strlen(title_id) != 9) return 0;
    for (int i = 0; i < 4; i++) {
        if (!isupper((unsigned char)title_id[i])) return 0;
    }
    for (int i = 4; i < 9; i++) {
        if (!isdigit((unsigned char)title_id[i])) return 0;
    }
    return strncmp(title_id, "NPXS", 4) != 0;
}

int
ng_valid_content_id(const char *content_id)
{
    if (!content_id || !content_id[0] || strlen(content_id) >= 64) return 0;
    return content_id[strspn(content_id,
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_")] == '\0';
}

int
ng_has_suffix_ci(const char *text, const char *suffix)
{
    size_t tlen;
    size_t slen;

    if (!text || !suffix) return 0;
    tlen = strlen(text);
    slen = strlen(suffix);
    if (slen > tlen) return 0;
    for (size_t i = 0; i < slen; i++) {
        if (tolower((unsigned char)text[tlen - slen + i]) !=
            tolower((unsigned char)suffix[i])) {
            return 0;
        }
    }
    return 1;
}

int
ng_version_compare(const char *left, const char *right)
{
    const char *a = left ? left : "";
    const char *b = right ? right : "";

    while (*a || *b) {
        unsigned long va = 0;
        unsigned long vb = 0;
        while (*a && !isdigit((unsigned char)*a)) a++;
        while (*b && !isdigit((unsigned char)*b)) b++;
        while (isdigit((unsigned char)*a)) {
            if (va <= (ULONG_MAX - 9) / 10) va = va * 10 + (unsigned long)(*a - '0');
            a++;
        }
        while (isdigit((unsigned char)*b)) {
            if (vb <= (ULONG_MAX - 9) / 10) vb = vb * 10 + (unsigned long)(*b - '0');
            b++;
        }
        if (va < vb) return -1;
        if (va > vb) return 1;
        if (!*a && !*b) break;
        if (!*a) {
            while (*b && !isdigit((unsigned char)*b)) b++;
            if (!*b) break;
        }
        if (!*b) {
            while (*a && !isdigit((unsigned char)*a)) a++;
            if (!*a) break;
        }
    }
    return 0;
}

static int
hex_value(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

char *
ng_url_decode(const char *text)
{
    size_t len;
    char *out;
    size_t j = 0;

    if (!text) return strdup("");
    len = strlen(text);
    out = malloc(len + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '%' && i + 2 < len) {
            int hi = hex_value((unsigned char)text[i + 1]);
            int lo = hex_value((unsigned char)text[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out[j++] = (char)((hi << 4) | lo);
                i += 2;
                continue;
            }
        }
        out[j++] = text[i] == '+' ? ' ' : text[i];
    }
    out[j] = '\0';
    return out;
}

int
ng_url_encode(NgStrBuf *out, const char *text)
{
    static const char hex[] = "0123456789ABCDEF";
    const unsigned char *p = (const unsigned char *)(text ? text : "");

    while (*p) {
        if (isalnum(*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~' || *p == '/') {
            if (ng_strbuf_appendn(out, (const char *)p, 1) != 0) return -1;
        } else {
            char esc[3] = {'%', hex[*p >> 4], hex[*p & 15]};
            if (ng_strbuf_appendn(out, esc, 3) != 0) return -1;
        }
        p++;
    }
    return 0;
}

int
ng_parse_int(const char *text, int fallback)
{
    char *end = NULL;
    long value;

    if (!text || !*text) return fallback;
    errno = 0;
    value = strtol(text, &end, 10);
    if (errno || !end || *end) return fallback;
    if (value < INT_MIN || value > INT_MAX) return fallback;
    return (int)value;
}

uint64_t
ng_now_epoch(void)
{
    return (uint64_t)time(NULL);
}

uint64_t
ng_now_monotonic_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

const char *
ng_source_type_name(int type)
{
    switch (type) {
    case NG_SOURCE_SMB: return "smb";
    case NG_SOURCE_FTP: return "ftp";
    case NG_SOURCE_LOCAL: return "local";
    default: return "none";
    }
}

int
ng_source_type_parse(const char *name)
{
    if (!name) return NG_SOURCE_NONE;
    if (!strcasecmp(name, "smb") || !strcasecmp(name, "cifs")) return NG_SOURCE_SMB;
    if (!strcasecmp(name, "ftp")) return NG_SOURCE_FTP;
    if (!strcasecmp(name, "local") || !strcasecmp(name, "file")) return NG_SOURCE_LOCAL;
    return NG_SOURCE_NONE;
}

const char *
ng_game_type_name(int type)
{
    return type == NG_GAME_PKG ? "pkg" : "folder";
}

const char *
ng_packing_name(int packing)
{
    switch (packing) {
    case NG_PACK_ZSTD: return "zstd";
    case NG_PACK_LZ4: return "lz4";
    default: return "plain";
    }
}

const char *
ng_pkg_kind_name(int kind)
{
    switch (kind) {
    case NG_PKG_BASE: return "base";
    case NG_PKG_UPDATE: return "update";
    case NG_PKG_DLC: return "dlc";
    default: return "unknown";
    }
}
