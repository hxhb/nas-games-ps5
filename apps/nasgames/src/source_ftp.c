#include "source.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "util.h"

typedef struct {
    int ctrl;
} FtpPriv;

typedef struct {
    int data;
    FtpPriv control;
    char path[NG_PATH_LEN];
    uint64_t offset;
    uint64_t size;
    int finished;
    int response_pending;
} FtpFile;

static void
ftp_set_timeout(int fd, int seconds)
{
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#ifdef SO_NOSIGPIPE
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
}

static int
tcp_connect(const char *host, int port)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *ai;
    char service[16];
    int fd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(service, sizeof(service), "%d", port);
    if (getaddrinfo(host, service, &hints, &res) != 0) return -1;
    for (ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;
        ftp_set_timeout(fd, 25);
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

static int
ftp_send(FtpPriv *priv, const char *text)
{
    size_t left = strlen(text);
    const char *ptr = text;
    while (left) {
        ssize_t n = send(priv->ctrl, ptr, left, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        ptr += n;
        left -= (size_t)n;
    }
    return 0;
}

static int
ftp_readline(FtpPriv *priv, char *line, size_t size)
{
    size_t used = 0;
    while (used + 1 < size) {
        char c;
        ssize_t n = recv(priv->ctrl, &c, 1, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        if (c == '\n') {
            if (used && line[used - 1] == '\r') used--;
            line[used] = '\0';
            return 0;
        }
        line[used++] = c;
    }
    line[used] = '\0';
    return -1;
}

static int
ftp_response(FtpPriv *priv, char *text, size_t text_size)
{
    char line[2048];
    int code;
    int multiline;

    if (text && text_size) text[0] = '\0';
    if (ftp_readline(priv, line, sizeof(line)) != 0) return -1;
    if (strlen(line) < 3 || !isdigit((unsigned char)line[0]) ||
        !isdigit((unsigned char)line[1]) || !isdigit((unsigned char)line[2])) {
        return -1;
    }
    code = (line[0] - '0') * 100 + (line[1] - '0') * 10 + (line[2] - '0');
    multiline = line[3] == '-';
    if (text && text_size) ng_strlcpy(text, line, text_size);
    while (multiline) {
        if (ftp_readline(priv, line, sizeof(line)) != 0) return -1;
        if (strlen(line) >= 4 && atoi(line) == code && line[3] == ' ') {
            if (text && text_size) ng_strlcpy(text, line + 4, text_size);
            break;
        }
    }
    return code;
}

static int
ftp_command(FtpPriv *priv, char *text, size_t text_size, const char *fmt, ...)
{
    char command[NG_PATH_LEN + 256];
    va_list ap;

    va_start(ap, fmt);
    int length = vsnprintf(command, sizeof(command), fmt, ap);
    va_end(ap);
    if (text && text_size) text[0] = '\0';
    if (length < 0 || (size_t)length >= sizeof(command) || strpbrk(command, "\r\n")) {
        errno = EINVAL;
        return -1;
    }
    if (ftp_send(priv, command) != 0 || ftp_send(priv, "\r\n") != 0) return -1;
    return ftp_response(priv, text, text_size);
}

static void
ftp_close_control(FtpPriv *priv)
{
    if (priv->ctrl >= 0) {
        close(priv->ctrl);
        priv->ctrl = -1;
    }
}

static int
ftp_login_control(NgSource *source, FtpPriv *priv)
{
    char text[512];
    int code;

    ftp_close_control(priv);
    priv->ctrl = tcp_connect(source->config.host,
                             source->config.port > 0 ? source->config.port : 21);
    if (priv->ctrl < 0) {
        ng_source_set_error(source, "FTP 连接失败: %s", strerror(errno));
        return -1;
    }
    code = ftp_response(priv, text, sizeof(text));
    if (code != 220 && code != 120) {
        ng_source_set_error(source, "FTP 欢迎响应异常 (%d): %.180s", code, text);
        goto fail;
    }
    if (code == 120) {
        code = ftp_response(priv, text, sizeof(text));
        if (code != 220) goto fail;
    }
    code = ftp_command(priv, text, sizeof(text), "USER %s",
                       source->config.username[0] ? source->config.username : "anonymous");
    if (code == 331) {
        code = ftp_command(priv, text, sizeof(text), "PASS %s", source->config.password);
    }
    if (code != 230 && code != 202) {
        ng_source_set_error(source, "FTP 登录失败 (%d)", code);
        goto fail;
    }
    code = ftp_command(priv, text, sizeof(text), "TYPE I");
    if (code != 200) {
        ng_source_set_error(source, "FTP TYPE I 失败 (%d): %.180s", code, text);
        goto fail;
    }
    if (ftp_command(priv, text, sizeof(text), "OPTS UTF8 ON") < 0) goto fail;
    return 0;

fail:
    ftp_close_control(priv);
    return -1;
}

static int
ftp_login(NgSource *source)
{
    return ftp_login_control(source, source->priv);
}

static void
ftp_disconnect(NgSource *source)
{
    ftp_close_control(source->priv);
    source->connected = 0;
}

static int
ftp_passive(NgSource *source, FtpPriv *priv)
{
    char text[512];
    int code;

    code = ftp_command(priv, text, sizeof(text), "EPSV");
    if (code == 229) {
        char *start = strchr(text, '(');
        char *end = start ? strchr(start, ')') : NULL;
        if (start && end) {
            char delimiter = start[1];
            if (end - start >= 6 && start[2] == delimiter && start[3] == delimiter &&
                end[-1] == delimiter) {
                char *tail;
                long port = strtol(start + 4, &tail, 10);
                if (tail == end - 1 && port > 0 && port <= 65535) {
                    return tcp_connect(source->config.host, (int)port);
                }
            }
        }
    }
    code = ftp_command(priv, text, sizeof(text), "PASV");
    if (code == 227) {
        unsigned first_port_byte, second_port_byte;
        char *start = strchr(text, '(');
        if (start && sscanf(start + 1, "%*u,%*u,%*u,%*u,%3u,%3u",
                            &first_port_byte, &second_port_byte) == 2 &&
            first_port_byte <= 255 && second_port_byte <= 255 &&
            (first_port_byte || second_port_byte)) {
            return tcp_connect(source->config.host, (int)(first_port_byte * 256 + second_port_byte));
        }
    }
    ng_source_set_error(source, "FTP 被动模式失败 (%d): %.180s", code, text);
    if (priv == source->priv && (code < 0 || code == 421)) ftp_disconnect(source);
    return -1;
}

static int
ftp_parse_size(const char *text, uint64_t *size)
{
    char *end;

    while (isspace((unsigned char)*text)) text++;
    if (!isdigit((unsigned char)*text)) return -1;
    errno = 0;
    unsigned long long value = strtoull(text, &end, 10);
    if (errno || (*end && *end != ';' && !isspace((unsigned char)*end))) return -1;
    *size = value;
    return 0;
}

static int
ftp_add_list_entry(NgSourceList *list, const char *name, int is_dir,
                   uint64_t size, uint64_t mtime)
{
    NgSourceEntry entry;
    if (!name || !*name || !strcmp(name, ".") || !strcmp(name, "..")) return 0;
    if (strlen(name) >= sizeof(entry.name)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memset(&entry, 0, sizeof(entry));
    ng_strlcpy(entry.name, name, sizeof(entry.name));
    entry.is_dir = is_dir;
    entry.size = size;
    entry.mtime = mtime;
    return ng_source_list_add(list, &entry);
}

static int
ftp_parse_mlsd_line(char *line, NgSourceList *list)
{
    char *space = strchr(line, ' ');
    char *facts;
    char *name;
    int is_dir = 0;
    uint64_t size = 0;

    if (!space) return 0;
    *space = '\0';
    facts = line;
    name = space + 1;
    while (*name == ' ') name++;
    char *type = strstr(facts, "type=");
    if (type) {
        type += 5;
        if (!strncasecmp(type, "dir", 3)) is_dir = 1;
    }
    char *sz = strstr(facts, "size=");
    if (sz && ftp_parse_size(sz + 5, &size) != 0) return -1;
    return ftp_add_list_entry(list, name, is_dir, size, 0);
}

static int
ftp_parse_list_line(char *line, NgSourceList *list)
{
    char *fields[9];
    int count = 0;
    char *p = line;
    int is_dir;
    uint64_t size = 0;

    if (!*line || !strncasecmp(line, "total ", 6)) return 0;
    fields[count++] = p;
    while (*p && count < 9) {
        if (*p == ' ') {
            *p = '\0';
            while (*++p == ' ') {}
            if (*p) fields[count++] = p;
        } else {
            p++;
        }
    }
    if (count < 9) return 0;
    is_dir = fields[0][0] == 'd';
    if (ftp_parse_size(fields[4], &size) != 0) return -1;
    return ftp_add_list_entry(list, fields[8], is_dir, size, 0);
}

static int
ftp_read_data_list(int data, int mlsd, NgSourceList *list)
{
    char buf[8192];
    size_t used = 0;
    int rc = 0;

    while (1) {
        ssize_t n = recv(data, buf + used, sizeof(buf) - used - 1, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            rc = -1;
            break;
        }
        if (n == 0) {
            if (used) rc = mlsd ? ftp_parse_mlsd_line(buf, list) : ftp_parse_list_line(buf, list);
            break;
        }
        used += (size_t)n;
        buf[used] = '\0';
        char *start = buf;
        char *nl;
        while ((nl = strchr(start, '\n')) != NULL) {
            *nl = '\0';
            size_t len = strlen(start);
            if (len && start[len - 1] == '\r') start[len - 1] = '\0';
            if (mlsd) {
                if (ftp_parse_mlsd_line(start, list) != 0) rc = -1;
            } else {
                if (ftp_parse_list_line(start, list) != 0) rc = -1;
            }
            start = nl + 1;
        }
        used = strlen(start);
        memmove(buf, start, used + 1);
        if (rc != 0 || used >= sizeof(buf) - 1) return -1;
    }
    return rc;
}

static int
ftp_list(NgSource *source, const char *path, NgSourceList *list)
{
    FtpPriv *priv = source->priv;
    char text[512];
    int data;
    int code;
    int rc = 0;

    data = ftp_passive(source, priv);
    if (data < 0) return -1;
    code = ftp_command(priv, text, sizeof(text), "MLSD %s", path);
    if (code != 125 && code != 150) {
        close(data);
        data = -1;
        if (code == 500 || code == 501 || code == 502 || code == 504) {
            data = ftp_passive(source, priv);
            if (data < 0) return -1;
            code = ftp_command(priv, text, sizeof(text), "LIST %s", path);
        }
        if (code != 125 && code != 150) {
            if (data >= 0) close(data);
            ng_source_set_error(source, "FTP 列目录失败 (%d): %.180s", code, text);
            if (code < 0 || code == 421) ftp_disconnect(source);
            return -1;
        }
        if (ftp_read_data_list(data, 0, list) != 0) rc = -1;
    } else {
        if (ftp_read_data_list(data, 1, list) != 0) rc = -1;
    }
    shutdown(data, SHUT_RDWR);
    close(data);
    if (rc != 0) {
        ng_source_set_error(source, "FTP 目录数据读取失败");
        ftp_disconnect(source);
        return -1;
    }
    code = ftp_response(priv, text, sizeof(text));
    if (code != 226 && code != 250) {
        ng_source_set_error(source, "FTP 列目录收尾异常 (%d): %.180s", code, text);
        ftp_disconnect(source);
        rc = -1;
    }
    return rc;
}

static int
ftp_stat(NgSource *source, const char *path, NgSourceEntry *entry)
{
    FtpPriv *priv = source->priv;
    char text[512];
    int code;

    memset(entry, 0, sizeof(*entry));
    ng_strlcpy(entry->name, ng_path_basename(path), sizeof(entry->name));
    code = ftp_command(priv, text, sizeof(text), "SIZE %s", path);
    if (code == 213 && ftp_parse_size(text + 3, &entry->size) == 0) {
        entry->is_dir = 0;
        return 0;
    }
    if (code >= 0 && code != 421) code = ftp_command(priv, text, sizeof(text), "CWD %s", path);
    if (code == 250) {
        entry->is_dir = 1;
        return 0;
    }
    ng_source_set_error(source, "FTP stat 失败 (%d): %.180s", code, text);
    if (code < 0 || code == 421) ftp_disconnect(source);
    return -1;
}

static int
ftp_start_retr(NgSource *source, FtpFile *file, uint64_t offset)
{
    FtpPriv *priv = &file->control;
    char text[512];
    int code;
    int data;

    if (priv->ctrl < 0 && ftp_login_control(source, priv) != 0) return -1;
    data = ftp_passive(source, priv);
    if (data < 0) return -1;
    if (offset > 0) {
        code = ftp_command(priv, text, sizeof(text), "REST %llu",
                           (unsigned long long)offset);
        if (code != 350) {
            close(data);
            ng_source_set_error(source, "FTP REST 失败 (%d): %.180s", code, text);
            return -1;
        }
    }
    code = ftp_command(priv, text, sizeof(text), "RETR %s", file->path);
    if (code != 125 && code != 150) {
        close(data);
        ng_source_set_error(source, "FTP 下载失败 (%d): %.180s", code, text);
        return -1;
    }
    file->data = data;
    file->offset = offset;
    file->finished = 0;
    file->response_pending = 1;
    return 0;
}

static void *
ftp_file_open(NgSource *source, const char *path, uint64_t *size)
{
    FtpPriv *priv = source->priv;
    FtpFile *file = calloc(1, sizeof(*file));
    char text[512];
    int code;

    if (!file) return NULL;
    file->data = -1;
    file->control.ctrl = -1;
    ng_strlcpy(file->path, path, sizeof(file->path));
    code = ftp_command(priv, text, sizeof(text), "SIZE %s", path);
    if (code < 0 || code == 421) {
        ng_source_set_error(source, "FTP SIZE 失败 (%d)", code);
        ftp_disconnect(source);
        free(file);
        return NULL;
    }
    if (code == 213) ftp_parse_size(text + 3, &file->size);
    *size = file->size;
    return file;
}

static ssize_t
ftp_file_read(NgSource *source, void *handle, void *buf, size_t size)
{
    FtpFile *file = handle;
    char text[512];
    ssize_t n;

    if (!size || file->finished) return 0;
    if (file->data < 0 && ftp_start_retr(source, file, file->offset) != 0) return -1;
    while (1) {
        n = recv(file->data, buf, size, 0);
        if (n < 0 && errno == EINTR) continue;
        break;
    }
    if (n < 0) {
        ng_source_set_error(source, "FTP 数据读取失败: %s", strerror(errno));
        return -1;
    }
    if (n == 0) {
        int code;
        close(file->data);
        file->data = -1;
        code = ftp_response(&file->control, text, sizeof(text));
        file->response_pending = 0;
        file->finished = 1;
        if (code != 226 && code != 250) {
            ng_source_set_error(source, "FTP 下载收尾异常 (%d): %.180s", code, text);
            return -1;
        }
    } else {
        file->offset += (uint64_t)n;
    }
    return n;
}

static int
ftp_file_seek(NgSource *source, void *handle, uint64_t offset)
{
    FtpFile *file = handle;
    (void)source;

    if (file->offset != offset || file->finished) {
        if (file->data >= 0) close(file->data);
        file->data = -1;
        if (file->response_pending) ftp_close_control(&file->control);
        file->response_pending = 0;
        file->offset = offset;
        file->finished = 0;
    }
    return 0;
}

static void
ftp_file_close(NgSource *source, void *handle)
{
    FtpFile *file = handle;
    (void)source;
    if (file->data >= 0) close(file->data);
    ftp_close_control(&file->control);
    free(file);
}

static const NgSourceOps ftp_ops = {
    .connect = ftp_login,
    .disconnect = ftp_disconnect,
    .list = ftp_list,
    .stat = ftp_stat,
    .file_open = ftp_file_open,
    .file_read = ftp_file_read,
    .file_seek = ftp_file_seek,
    .file_close = ftp_file_close,
};

NgSource *
ng_source_ftp_create(const NgSourceConfig *config)
{
    NgSource *source = calloc(1, sizeof(*source));
    FtpPriv *priv = calloc(1, sizeof(*priv));

    if (!source || !priv) {
        free(source);
        free(priv);
        return NULL;
    }
    priv->ctrl = -1;
    source->config = *config;
    source->ops = &ftp_ops;
    source->priv = priv;
    ng_strlcpy(source->status, "offline", sizeof(source->status));
    return source;
}
