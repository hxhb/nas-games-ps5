#include "log.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __PROSPERO__
#include <ps5/klog.h>
#endif

#define NG_LOG_RING 32768
#define NG_LOG_MAX_FILE (1024 * 1024)

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *log_file;
static char log_path[1024];
static size_t log_size;
static char ring[NG_LOG_RING];
static size_t ring_len;

static void
ring_append(const char *line)
{
    size_t len = strlen(line);

    if (len >= NG_LOG_RING) {
        line += len - NG_LOG_RING + 1;
        len = NG_LOG_RING - 1;
    }
    if (ring_len + len > NG_LOG_RING - 1) {
        size_t drop = ring_len + len - (NG_LOG_RING - 1);
        while (drop < ring_len && ring[drop - 1] != '\n') drop++;
        memmove(ring, ring + drop, ring_len - drop);
        ring_len -= drop;
    }
    memcpy(ring + ring_len, line, len);
    ring_len += len;
    ring[ring_len] = '\0';
}

int
ng_log_init(const char *path)
{
    FILE *fp;
    long size = 0;

    pthread_mutex_lock(&log_lock);
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    snprintf(log_path, sizeof(log_path), "%s", path ? path : "");
    fp = path && *path ? fopen(path, "a+") : NULL;
    if (fp) {
        if (fseek(fp, 0, SEEK_END) == 0) size = ftell(fp);
        if (size > NG_LOG_MAX_FILE) {
            fclose(fp);
            fp = fopen(path, "w");
            size = 0;
        }
    }
    log_file = fp;
    log_size = size > 0 ? (size_t)size : 0;
    ring_len = 0;
    ring[0] = '\0';
    pthread_mutex_unlock(&log_lock);
    return fp ? 0 : -1;
}

void
ng_log_close(void)
{
    pthread_mutex_lock(&log_lock);
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_lock);
}

static void
log_write(const char *level, const char *fmt, va_list ap)
{
    char message[2048];
    char line[2304];
    time_t now = time(NULL);
    struct tm tmv;
    char stamp[32];

    vsnprintf(message, sizeof(message), fmt, ap);
    localtime_r(&now, &tmv);
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tmv);
    snprintf(line, sizeof(line), "[%s] %-5s %s\n", stamp, level, message);

    pthread_mutex_lock(&log_lock);
#ifdef __PROSPERO__
    klog_printf("nasgames.elf %s", line);
#endif
    if (log_file) {
        size_t length = strlen(line);
        if (log_size + length > NG_LOG_MAX_FILE) {
            fclose(log_file);
            log_file = fopen(log_path, "w");
            log_size = 0;
        }
        if (log_file && fputs(line, log_file) >= 0) log_size += length;
        if (log_file) fflush(log_file);
    }
    ring_append(line);
    pthread_mutex_unlock(&log_lock);
}

void
ng_log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_write("INFO", fmt, ap);
    va_end(ap);
}

void
ng_log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_write("ERROR", fmt, ap);
    va_end(ap);
}

int
ng_log_recent_json(NgStrBuf *out)
{
    int rc;

    pthread_mutex_lock(&log_lock);
    rc = ng_strbuf_json_string(out, ring);
    pthread_mutex_unlock(&log_lock);
    return rc;
}
