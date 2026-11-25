#include "app.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "http_server.h"
#include "log.h"
#include "installer.h"
#include "scanner.h"
#include "storage.h"

static NgApp g_app;
static uint64_t job_speed_last_bytes;
static uint64_t job_speed_last_ms;
static uint64_t job_speed_ema;
static uint64_t job_sequence;
static NgSource *get_source_locked(const char *id);

NgApp *
ng_app(void)
{
    return &g_app;
}

static void
free_catalog(NgCatalog *catalog)
{
    free(catalog->items);
    memset(catalog, 0, sizeof(*catalog));
}

static uint64_t
measured_size_get_locked(const char *game_id)
{
    for (int i = 0; i < g_app.measured_size_count; i++) {
        if (!strcmp(g_app.measured_sizes[i].game_id, game_id)) {
            return g_app.measured_sizes[i].size;
        }
    }
    return 0;
}

static void
measured_size_put_locked(const char *game_id, uint64_t size)
{
    for (int i = 0; i < g_app.measured_size_count; i++) {
        if (!strcmp(g_app.measured_sizes[i].game_id, game_id)) {
            g_app.measured_sizes[i].size = size;
            return;
        }
    }
    if (g_app.measured_size_count >= NG_MAX_GAMES) return;
    NgMeasuredSize *entry = &g_app.measured_sizes[g_app.measured_size_count++];
    ng_strlcpy(entry->game_id, game_id, sizeof(entry->game_id));
    entry->size = size;
}

static void
release_sources(NgSource *sources[], int count)
{
    for (int i = 0; i < count; i++) ng_source_release(sources[i]);
}

static void
rebuild_sources_locked(NgSource *retired[NG_MAX_SOURCES], int *retired_count)
{
    NgSource *next[NG_MAX_SOURCES];
    int next_count = 0;

    *retired_count = 0;
    for (int i = 0; i < g_app.config.source_count; i++) {
        const NgSourceConfig *config = &g_app.config.sources[i];
        NgSource *source = NULL;

        for (int j = 0; j < g_app.source_count; j++) {
            if (g_app.sources[j] &&
                !memcmp(&g_app.sources[j]->config, config, sizeof(*config))) {
                source = g_app.sources[j];
                g_app.sources[j] = NULL;
                break;
            }
        }
        if (!source) source = ng_source_create(config);
        if (!source) {
            ng_log_error("source %s: 无法创建", config->id);
            continue;
        }
        next[next_count++] = source;
    }
    for (int i = 0; i < g_app.source_count; i++) {
        if (g_app.sources[i]) retired[(*retired_count)++] = g_app.sources[i];
    }
    memcpy(g_app.sources, next, (size_t)next_count * sizeof(next[0]));
    g_app.source_count = next_count;
}

int
ng_app_init(const char *data_dir)
{
    char path[NG_PATH_LEN];
    NgSource *retired[NG_MAX_SOURCES];
    int retired_count = 0;

    memset(&g_app, 0, sizeof(g_app));
    pthread_mutex_init(&g_app.lock, NULL);
    pthread_cond_init(&g_app.scan_cond, NULL);
    pthread_mutex_init(&g_app.catalog_lock, NULL);
    pthread_mutex_init(&g_app.job_lock, NULL);
    ng_config_defaults(&g_app.config, data_dir && *data_dir ? data_dir : "/data/nasgames");
    ng_strlcpy(g_app.config_path, g_app.config.data_dir, sizeof(g_app.config_path));
    ng_strlcat(g_app.config_path, "/config.json", sizeof(g_app.config_path));

    if (ng_mkdir_p(g_app.config.data_dir, 0777) != 0 ||
        ng_mkdir_p(g_app.config.cache_dir, 0777) != 0 ||
        ng_mkdir_p(g_app.config.art_dir, 0777) != 0) {
        snprintf(path, sizeof(path), "%s", g_app.config.data_dir);
        ng_mkdir_p(path, 0777);
    }
    if (ng_path_join_local(path, sizeof(path), g_app.config.data_dir, "nasgames.log") != 0) {
        return -1;
    }
    ng_log_init(path);

    if (ng_config_load(&g_app.config, g_app.config_path) == 0) {
        ng_log_info("配置已加载: %s", g_app.config_path);
    } else if (errno == ENOENT) {
        ng_log_info("创建默认配置: %s", g_app.config_path);
        if (ng_config_save(&g_app.config, g_app.config_path) != 0) {
            ng_log_error("写入默认配置失败: %s", strerror(errno));
        }
    } else {
        ng_log_error("读取配置失败，保留 %s 并使用默认设置: %s",
                     g_app.config_path, strerror(errno));
    }
    ng_mkdir_p(g_app.config.cache_dir, 0777);
    ng_mkdir_p(g_app.config.art_dir, 0777);
    if (ng_path_join_local(path, sizeof(path), g_app.config.data_dir, "library") != 0) {
        return -1;
    }
    ng_mkdir_p(path, 0777);
    g_app.actual_port = g_app.config.port;
    g_app.started_at = ng_now_epoch();
    g_app.job.state = NG_JOB_IDLE;
    ng_strlcpy(g_app.scan_message, "等待扫描", sizeof(g_app.scan_message));
    pthread_mutex_lock(&g_app.lock);
    rebuild_sources_locked(retired, &retired_count);
    pthread_mutex_unlock(&g_app.lock);
    release_sources(retired, retired_count);
    ng_log_info("%s %s 已初始化，数据目录 %s", NG_APP_NAME, NG_VERSION, g_app.config.data_dir);
    return 0;
}

int
ng_app_start_scanner(void)
{
    ng_app_request_scan();
    return ng_scanner_start();
}

void
ng_app_stop(void)
{
    NgSource *retired[NG_MAX_SOURCES];
    int retired_count = 0;

    ng_scanner_stop();
    pthread_mutex_lock(&g_app.lock);
    for (int i = 0; i < g_app.source_count; i++) {
        if (g_app.sources[i]) retired[retired_count++] = g_app.sources[i];
        g_app.sources[i] = NULL;
    }
    g_app.source_count = 0;
    pthread_mutex_unlock(&g_app.lock);
    release_sources(retired, retired_count);
    pthread_mutex_lock(&g_app.catalog_lock);
    free_catalog(&g_app.catalog);
    g_app.measured_size_count = 0;
    pthread_mutex_unlock(&g_app.catalog_lock);
}

void
ng_app_config_snapshot(NgConfig *out)
{
    pthread_mutex_lock(&g_app.lock);
    *out = g_app.config;
    pthread_mutex_unlock(&g_app.lock);
}

void
ng_app_data_dir_snapshot(char *out, size_t out_size)
{
    if (!out || !out_size) return;
    pthread_mutex_lock(&g_app.lock);
    ng_strlcpy(out, g_app.config.data_dir, out_size);
    pthread_mutex_unlock(&g_app.lock);
}

int
ng_app_keep_awake_enabled(void)
{
    int enabled;
    pthread_mutex_lock(&g_app.lock);
    enabled = g_app.config.keep_awake;
    pthread_mutex_unlock(&g_app.lock);
    return enabled;
}

int
ng_app_server_port(void)
{
    int port;
    pthread_mutex_lock(&g_app.lock);
    port = g_app.actual_port ? g_app.actual_port : g_app.config.port;
    pthread_mutex_unlock(&g_app.lock);
    return port;
}

int
ng_app_open_browser_on_start(void)
{
    int enabled;
    pthread_mutex_lock(&g_app.lock);
    enabled = g_app.config.open_browser_on_start;
    pthread_mutex_unlock(&g_app.lock);
    return enabled;
}

int
ng_app_parse_source_json(const char *json, NgSourceConfig *out,
                         char *error, size_t error_size)
{
    int rc;

    pthread_mutex_lock(&g_app.lock);
    rc = ng_config_source_from_json(json, &g_app.config, out, error, error_size);
    pthread_mutex_unlock(&g_app.lock);
    return rc;
}

int
ng_app_set_game_size(const char *game_id, NgSource *source, uint64_t size)
{
    if (!game_id || !game_id[0] || size == 0) return -1;
    pthread_mutex_lock(&g_app.lock);
    NgSource *current = get_source_locked(source->config.id);
    if (current != source) {
        pthread_mutex_unlock(&g_app.lock);
        ng_source_release(current);
        return -1;
    }
    pthread_mutex_lock(&g_app.catalog_lock);
    for (size_t i = 0; i < g_app.catalog.count; i++) {
        if (!strcmp(g_app.catalog.items[i].id, game_id)) {
            measured_size_put_locked(game_id, size);
            if (g_app.catalog.items[i].size != size) {
                g_app.catalog.items[i].size = size;
                g_app.catalog_revision++;
            }
            break;
        }
    }
    pthread_mutex_unlock(&g_app.catalog_lock);
    pthread_mutex_unlock(&g_app.lock);
    ng_source_release(current);
    return 0;
}

int
ng_app_apply_config(const char *json, char *error, size_t error_size)
{
    int rc;
    NgConfig *copy;
    int source_count = 0;
    NgSource *retired[NG_MAX_SOURCES];
    int retired_count = 0;

    copy = malloc(sizeof(*copy));
    if (!copy) {
        snprintf(error, error_size, "内存不足");
        return -1;
    }
    pthread_mutex_lock(&g_app.lock);
    if (ng_config_from_json(json, &g_app.config, copy, error, error_size) != 0) {
        pthread_mutex_unlock(&g_app.lock);
        free(copy);
        return 1;
    }
    rc = ng_config_save(copy, g_app.config_path);
    if (rc != 0) snprintf(error, error_size, "保存配置失败: %s", strerror(errno));
    if (rc == 0) {
        int source_changed = copy->source_count != g_app.config.source_count ||
            memcmp(copy->sources, g_app.config.sources,
                   (size_t)copy->source_count * sizeof(copy->sources[0])) != 0;
        source_count = copy->source_count;
        g_app.config = *copy;
        g_app.config_generation++;
        rebuild_sources_locked(retired, &retired_count);
        g_app.scan_requested = 1;
        pthread_cond_broadcast(&g_app.scan_cond);
        if (source_changed) {
            size_t kept = 0;
            pthread_mutex_lock(&g_app.catalog_lock);
            for (size_t i = 0; i < g_app.catalog.count; i++) {
                NgGame *game = &g_app.catalog.items[i];
                int removed = 0;
                for (int j = 0; j < retired_count; j++) {
                    if (!strcmp(game->source_id, retired[j]->config.id)) {
                        removed = 1;
                        break;
                    }
                }
                if (!removed) {
                    if (game->type == NG_GAME_FOLDER) game->size = 0;
                    g_app.catalog.items[kept++] = *game;
                }
            }
            g_app.catalog.count = kept;
            g_app.catalog_revision++;
            g_app.catalog_config_generation++;
            g_app.measured_size_count = 0;
            pthread_mutex_unlock(&g_app.catalog_lock);
        }
    }
    pthread_mutex_unlock(&g_app.lock);
    free(copy);
    if (retired_count) ng_http_stream_cache_clear();
    release_sources(retired, retired_count);
    if (rc != 0) return -1;
    ng_log_info("配置已更新，共 %d 个连接", source_count);
    return 0;
}

static NgSource *
get_source_locked(const char *id)
{
    NgSource *source = NULL;

    if (!id) return NULL;
    for (int i = 0; i < g_app.source_count; i++) {
        if (!strcmp(g_app.sources[i]->config.id, id)) {
            source = g_app.sources[i];
            ng_source_acquire(source);
            break;
        }
    }
    return source;
}

NgSource *
ng_app_get_source(const char *id)
{
    NgSource *source;

    pthread_mutex_lock(&g_app.lock);
    source = get_source_locked(id);
    pthread_mutex_unlock(&g_app.lock);
    return source;
}

int
ng_app_copy_game(const char *id, NgGame *out)
{
    int rc = -1;
    pthread_mutex_lock(&g_app.catalog_lock);
    for (size_t i = 0; i < g_app.catalog.count; i++) {
        if (!strcmp(g_app.catalog.items[i].id, id)) {
            *out = g_app.catalog.items[i];
            rc = 0;
            break;
        }
    }
    pthread_mutex_unlock(&g_app.catalog_lock);
    return rc;
}

int
ng_app_copy_game_source(const char *id, NgGame *game, NgSource **source)
{
    int rc;

    *source = NULL;
    pthread_mutex_lock(&g_app.lock);
    rc = ng_app_copy_game(id, game);
    if (rc == 0) *source = get_source_locked(game->source_id);
    pthread_mutex_unlock(&g_app.lock);
    return rc;
}

int
ng_app_mark_game_installed(const char *game_id, const char *title_id)
{
    int changed = 0;
    int mark_base = 0;

    pthread_mutex_lock(&g_app.catalog_lock);
    for (size_t i = 0; i < g_app.catalog.count; i++) {
        if (game_id && !strcmp(g_app.catalog.items[i].id, game_id)) {
            if (!g_app.catalog.items[i].installed ||
                !g_app.catalog.items[i].install_known ||
                g_app.catalog.items[i].install_partial) {
                changed = 1;
            }
            g_app.catalog.items[i].installed = 1;
            g_app.catalog.items[i].install_known = 1;
            g_app.catalog.items[i].install_partial = 0;
            mark_base = g_app.catalog.items[i].pkg_kind == NG_PKG_BASE;
            break;
        }
    }
    if (mark_base && title_id && title_id[0]) {
        for (size_t i = 0; i < g_app.catalog.count; i++) {
            NgGame *game = &g_app.catalog.items[i];
            if (!strcmp(game->title_id, title_id) && !game->base_installed) {
                game->base_installed = 1;
                changed = 1;
            }
        }
    }
    if (changed) g_app.catalog_revision++;
    pthread_mutex_unlock(&g_app.catalog_lock);
    return 0;
}

static int
mark_game_copy_flag(const char *game_id, const char *title_id, int homebrew)
{
    int changed = 0;

    pthread_mutex_lock(&g_app.catalog_lock);
    for (size_t i = 0; i < g_app.catalog.count; i++) {
        NgGame *item = &g_app.catalog.items[i];
        int matched = (game_id && !strcmp(item->id, game_id)) ||
            (!game_id && title_id && title_id[0] && !strcmp(item->title_id, title_id));
        if (matched) {
            int *flag = homebrew ? &item->homebrew_copy : &item->library_copy;
            if (!*flag) changed = 1;
            *flag = 1;
            break;
        }
    }
    if (changed) g_app.catalog_revision++;
    pthread_mutex_unlock(&g_app.catalog_lock);
    return 0;
}

int
ng_app_mark_game_library(const char *game_id, const char *title_id)
{
    return mark_game_copy_flag(game_id, title_id, 0);
}

int
ng_app_mark_game_homebrew(const char *game_id, const char *title_id)
{
    return mark_game_copy_flag(game_id, title_id, 1);
}

int
ng_app_replace_catalog(NgGame *items, size_t count, unsigned generation, int incomplete)
{
    int applied = 0;
    pthread_mutex_lock(&g_app.lock);
    if (generation == g_app.config_generation) {
        pthread_mutex_lock(&g_app.catalog_lock);
        if (incomplete && g_app.catalog.count) {
            pthread_mutex_unlock(&g_app.catalog_lock);
            pthread_mutex_unlock(&g_app.lock);
            return -1;
        }
        for (size_t i = 0; i < count; i++) {
            uint64_t measured;
            if (items[i].size != 0) continue;
            measured = measured_size_get_locked(items[i].id);
            if (measured > 0) items[i].size = measured;
        }
        if (count == g_app.catalog.count &&
            (!count || !memcmp(items, g_app.catalog.items, count * sizeof(*items)))) {
            free(items);
        } else {
            free_catalog(&g_app.catalog);
            g_app.catalog.items = items;
            g_app.catalog.count = count;
            g_app.catalog.capacity = count;
            g_app.catalog_revision++;
        }
        pthread_mutex_unlock(&g_app.catalog_lock);
        applied = 1;
    }
    pthread_mutex_unlock(&g_app.lock);
    return applied;
}

uint64_t
ng_app_request_scan(void)
{
    pthread_mutex_lock(&g_app.lock);
    g_app.scan_requested = 1;
    uint64_t sequence = g_app.scan_sequence + 1;
    pthread_cond_broadcast(&g_app.scan_cond);
    pthread_mutex_unlock(&g_app.lock);
    return sequence;
}

void
ng_app_wake_scanner(void)
{
    pthread_mutex_lock(&g_app.lock);
    pthread_cond_broadcast(&g_app.scan_cond);
    pthread_mutex_unlock(&g_app.lock);
}

void
ng_app_wait_scan_signal(unsigned int timeout_ms)
{
    struct timespec deadline;

    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += (time_t)(timeout_ms / 1000u);
    deadline.tv_nsec += (long)((timeout_ms % 1000u) * 1000000u);
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec += 1;
        deadline.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&g_app.lock);
    if (!g_app.scan_requested) {
        pthread_cond_timedwait(&g_app.scan_cond, &g_app.lock, &deadline);
    }
    pthread_mutex_unlock(&g_app.lock);
}

int
ng_app_scan_begin(NgConfig *config, NgSource *sources[], unsigned *generation)
{
    int count;

    pthread_mutex_lock(&g_app.lock);
    *config = g_app.config;
    *generation = g_app.config_generation;
    count = g_app.source_count;
    for (int i = 0; i < count; i++) {
        sources[i] = g_app.sources[i];
        ng_source_acquire(sources[i]);
    }
    g_app.scan_busy = 1;
    g_app.scan_sequence++;
    g_app.scan_requested = 0;
    g_app.scan_started_at = ng_now_epoch();
    ng_strlcpy(g_app.scan_message, "正在扫描远端目录", sizeof(g_app.scan_message));
    pthread_mutex_unlock(&g_app.lock);
    return count;
}

void
ng_app_scan_mark_finished(int ok, const char *message)
{
    pthread_mutex_lock(&g_app.lock);
    g_app.scan_busy = 0;
    g_app.scan_finished_at = ng_now_epoch();
    ng_strlcpy(g_app.scan_message, message ? message : (ok ? "扫描完成" : "扫描失败"),
               sizeof(g_app.scan_message));
    pthread_mutex_unlock(&g_app.lock);
}

void
ng_app_job_set(NgJobState state, const NgGame *game, const char *target,
               const char *stage, const char *message, int progress,
               uint64_t done, uint64_t total)
{
    pthread_mutex_lock(&g_app.job_lock);
    NgJob previous = g_app.job;
    memset(&g_app.job, 0, sizeof(g_app.job));
    g_app.job.state = state;
    g_app.job.sequence = ++job_sequence;
    if (game) {
        ng_strlcpy(g_app.job.game_id, game->id, sizeof(g_app.job.game_id));
        ng_strlcpy(g_app.job.title_id, game->title_id, sizeof(g_app.job.title_id));
        ng_strlcpy(g_app.job.title_name, game->title_name, sizeof(g_app.job.title_name));
    }
    ng_strlcpy(g_app.job.target,
               target && target[0] ? target : previous.target,
               sizeof(g_app.job.target));
    ng_strlcpy(g_app.job.stage, stage ? stage : "", sizeof(g_app.job.stage));
    ng_strlcpy(g_app.job.message, message ? message : "", sizeof(g_app.job.message));
    g_app.job.progress = progress;
    g_app.job.bytes_done = done;
    g_app.job.bytes_total = total;
    g_app.job.started_at = time(NULL);
    g_app.job.bytes_per_second = 0;
    job_speed_last_bytes = done;
    job_speed_last_ms = ng_now_monotonic_ms();
    job_speed_ema = 0;
    if (state == NG_JOB_DONE || state == NG_JOB_ERROR ||
        state == NG_JOB_CANCELED) {
        int same_job = game && previous.state == NG_JOB_RUNNING &&
            !strcmp(previous.game_id, g_app.job.game_id);
        g_app.job.finished_at = time(NULL);
        if (same_job) {
            g_app.job.sequence = previous.sequence;
            g_app.job.started_at = previous.started_at;
            if (!g_app.job.bytes_total) g_app.job.bytes_total = previous.bytes_total;
            if (!g_app.job.bytes_done) {
                g_app.job.bytes_done =
                    state == NG_JOB_DONE && g_app.job.bytes_total
                        ? g_app.job.bytes_total
                        : previous.bytes_done;
            }
        }
        if (g_app.job_history_count < NG_JOB_HISTORY_MAX) {
            g_app.job_history_count++;
        }
        if (g_app.job_history_count > 1) {
            memmove(g_app.job_history + 1, g_app.job_history,
                    (size_t)(g_app.job_history_count - 1) *
                        sizeof(g_app.job_history[0]));
        }
        g_app.job_history[0] = g_app.job;
    }
    pthread_mutex_unlock(&g_app.job_lock);
}

void
ng_app_job_update(const char *stage, const char *message, int progress,
                  uint64_t done, uint64_t total)
{
    uint64_t now = ng_now_monotonic_ms();

    pthread_mutex_lock(&g_app.job_lock);
    if (stage) ng_strlcpy(g_app.job.stage, stage, sizeof(g_app.job.stage));
    if (message) ng_strlcpy(g_app.job.message, message, sizeof(g_app.job.message));
    g_app.job.progress = progress;
    g_app.job.bytes_done = done;
    g_app.job.bytes_total = total;
    if (g_app.job.state == NG_JOB_RUNNING && now > job_speed_last_ms &&
        now - job_speed_last_ms >= 250) {
        if (done >= job_speed_last_bytes) {
            uint64_t elapsed = now - job_speed_last_ms;
            uint64_t instantaneous =
                (done - job_speed_last_bytes) * 1000u / elapsed;
            job_speed_ema = job_speed_ema
                ? (job_speed_ema * 3 + instantaneous) / 4
                : instantaneous;
            g_app.job.bytes_per_second = job_speed_ema;
        } else {
            g_app.job.bytes_per_second = 0;
            job_speed_ema = 0;
        }
        job_speed_last_bytes = done;
        job_speed_last_ms = now;
    }
    pthread_mutex_unlock(&g_app.job_lock);
}

NgJob
ng_app_job_snapshot(void)
{
    NgJob job;
    pthread_mutex_lock(&g_app.job_lock);
    job = g_app.job;
    pthread_mutex_unlock(&g_app.job_lock);
    return job;
}

static int
job_history_snapshot(NgJob *job, NgJob *items)
{
    int count;

    pthread_mutex_lock(&g_app.job_lock);
    *job = g_app.job;
    count = g_app.job_history_count;
    memcpy(items, g_app.job_history, (size_t)count * sizeof(items[0]));
    pthread_mutex_unlock(&g_app.job_lock);
    return count;
}

void
ng_app_job_history_clear(void)
{
    pthread_mutex_lock(&g_app.job_lock);
    g_app.job_history_count = 0;
    if (g_app.job.state == NG_JOB_DONE || g_app.job.state == NG_JOB_ERROR ||
        g_app.job.state == NG_JOB_CANCELED) {
        memset(&g_app.job, 0, sizeof(g_app.job));
        g_app.job.state = NG_JOB_IDLE;
    }
    pthread_mutex_unlock(&g_app.job_lock);
}

static void
state_game_json(NgStrBuf *out, const NgGame *game)
{
    const char *install_status = "available";

    if (game->installed) {
        if (game->pkg_kind == NG_PKG_UPDATE && game->installed_version[0] &&
            game->version[0] &&
            ng_version_compare(game->installed_version, game->version) > 0) {
            install_status = "superseded";
        } else {
            install_status = "installed";
        }
    } else if (game->install_partial) {
        install_status = "incomplete";
    } else if (!game->install_known) {
        install_status = "unknown";
    } else if ((game->pkg_kind == NG_PKG_UPDATE || game->pkg_kind == NG_PKG_DLC) &&
               !game->base_installed) {
        install_status = "needs-base";
    }

    ng_strbuf_append(out, "{");
    ng_strbuf_append(out, "\"id\":");
    ng_strbuf_json_string(out, game->id);
    ng_strbuf_append(out, ",\"titleId\":");
    ng_strbuf_json_string(out, game->title_id);
    ng_strbuf_append(out, ",\"title\":");
    ng_strbuf_json_string(out, game->title_name);
    ng_strbuf_append(out, ",\"version\":");
    ng_strbuf_json_string(out, game->version);
    ng_strbuf_append(out, ",\"category\":");
    ng_strbuf_json_string(out, game->category);
    ng_strbuf_append(out, ",\"contentId\":");
    ng_strbuf_json_string(out, game->content_id);
    ng_strbuf_append(out, ",\"pkgKind\":");
    ng_strbuf_json_string(out, ng_pkg_kind_name(game->pkg_kind));
    ng_strbuf_append(out, ",\"path\":");
    ng_strbuf_json_string(out, game->path);
    ng_strbuf_append(out, ",\"sourceId\":");
    ng_strbuf_json_string(out, game->source_id);
    ng_strbuf_append(out, ",\"sourceName\":");
    ng_strbuf_json_string(out, game->source_name);
    ng_strbuf_append(out, ",\"type\":");
    ng_strbuf_json_string(out, ng_game_type_name(game->type));
    ng_strbuf_append(out, ",\"packing\":");
    ng_strbuf_json_string(out, ng_packing_name(game->packing));
    ng_strbuf_printf(out,
                     ",\"size\":%llu,\"installable\":%s,\"installed\":%s,"
                     "\"launchable\":%s,"
                     "\"installKnown\":%s,\"installPartial\":%s,"
                     "\"baseInstalled\":%s,\"installStatus\":\"%s\","
                     "\"libraryCopy\":%s,\"homebrewCopy\":%s",
                     (unsigned long long)game->size,
                     game->installable ? "true" : "false",
                     game->installed ? "true" : "false",
                     game->base_installed &&
                         ng_valid_title_id(game->title_id) ? "true" : "false",
                     game->install_known ? "true" : "false",
                     game->install_partial ? "true" : "false",
                     game->base_installed ? "true" : "false",
                     install_status,
                     game->library_copy ? "true" : "false",
                     game->homebrew_copy ? "true" : "false");
    ng_strbuf_append(out, ",\"installedVersion\":");
    ng_strbuf_json_string(out, game->installed_version);
    if (game->icon_file[0]) {
        ng_strbuf_printf(out, ",\"icon\":\"/art/%s/icon\"", game->id);
    } else {
        ng_strbuf_append(out, ",\"icon\":null");
    }
    if (game->pic0_file[0]) {
        ng_strbuf_printf(out, ",\"pic0\":\"/art/%s/pic0\"", game->id);
    } else {
        ng_strbuf_append(out, ",\"pic0\":null");
    }
    if (game->pic1_file[0]) {
        ng_strbuf_printf(out, ",\"pic1\":\"/art/%s/pic1\"", game->id);
    } else {
        ng_strbuf_append(out, ",\"pic1\":null");
    }
    ng_strbuf_append(out, "}");
}

static void
state_storage_json(NgStrBuf *out, const char *key, const char *path)
{
    NgStorageInfo info;

    ng_storage_probe(path, &info);
    ng_strbuf_printf(out,
                     "\"%s\":{\"known\":%s,\"free\":%llu,\"total\":%llu}",
                     key, info.known ? "true" : "false",
                     (unsigned long long)info.free_bytes,
                     (unsigned long long)info.total_bytes);
}

static const char *
job_state_name(NgJobState state)
{
    switch (state) {
    case NG_JOB_RUNNING: return "running";
    case NG_JOB_DONE: return "done";
    case NG_JOB_ERROR: return "error";
    case NG_JOB_CANCELED: return "canceled";
    default: return "idle";
    }
}

int
ng_app_state_json_since(NgStrBuf *out, int has_revision,
                        uint64_t known_revision,
                        const char *known_instance)
{
    NgJob job;
    NgJob history[NG_JOB_HISTORY_MAX];
    int busy;
    uint64_t started;
    uint64_t finished;
    uint64_t scan_sequence;
    char scan_message[NG_ERROR_LEN];
    char instance[64];
    int port;
    int auto_scan;
    int interval;
    char data_dir[NG_PATH_LEN];
    char library_path[NG_PATH_LEN];
    NgInstallQueueItem wait_items[NG_INSTALL_QUEUE_MAX];
    int wait_count;
    int history_count;
    int instance_matches;

    pthread_mutex_lock(&g_app.lock);
    busy = g_app.scan_busy;
    started = g_app.scan_started_at;
    finished = g_app.scan_finished_at;
    scan_sequence = g_app.scan_sequence;
    ng_strlcpy(scan_message, g_app.scan_message, sizeof(scan_message));
    port = g_app.actual_port;
    auto_scan = g_app.config.auto_scan;
    interval = g_app.config.scan_interval_seconds;
    ng_strlcpy(data_dir, g_app.config.data_dir, sizeof(data_dir));
    pthread_mutex_unlock(&g_app.lock);
    snprintf(instance, sizeof(instance), "%ld-%llu",
             (long)getpid(), (unsigned long long)g_app.started_at);
    instance_matches = !known_instance || !known_instance[0] ||
        !strcmp(known_instance, instance);
    history_count = job_history_snapshot(&job, history);
    wait_count = ng_installer_queue_snapshot(wait_items, NG_INSTALL_QUEUE_MAX);
    if (ng_path_join_local(library_path, sizeof(library_path), data_dir, "library") != 0) {
        library_path[0] = '\0';
    }

    ng_strbuf_printf(out,
                     "{\"ok\":true,\"app\":\"%s\",\"version\":\"%s\","
                     "\"scan\":{\"busy\":%s,\"sequence\":%llu,\"startedAt\":%llu,\"finishedAt\":%llu,"
                     "\"message\":",
                     NG_APP_NAME, NG_VERSION,
                     busy ? "true" : "false", (unsigned long long)scan_sequence,
                     (unsigned long long)started, (unsigned long long)finished);
    ng_strbuf_json_string(out, scan_message);
    ng_strbuf_printf(out,
                     "},\"server\":{\"port\":%d,\"autoScan\":%s,\"interval\":%d,\"instance\":\"%s\"},"
                     "\"job\":{\"state\":",
                     port, auto_scan ? "true" : "false", interval, instance);
    ng_strbuf_json_string(out, job_state_name(job.state));
    ng_strbuf_append(out, ",\"gameId\":");
    ng_strbuf_json_string(out, job.game_id);
    ng_strbuf_append(out, ",\"target\":");
    ng_strbuf_json_string(out, job.target);
    ng_strbuf_append(out, ",\"title\":");
    ng_strbuf_json_string(out, job.title_name);
    ng_strbuf_append(out, ",\"stage\":");
    ng_strbuf_json_string(out, job.stage);
    ng_strbuf_append(out, ",\"message\":");
    ng_strbuf_json_string(out, job.message);
    ng_strbuf_printf(out,
                     ",\"progress\":%d,\"bytesDone\":%llu,\"bytesTotal\":%llu,"
                     "\"bytesPerSecond\":%llu,\"cancelPending\":%s,\"sequence\":%llu,\"startedAt\":%llu,"
                     "\"finishedAt\":%llu,\"queued\":%d,\"queue\":[",
                     job.progress, (unsigned long long)job.bytes_done,
                     (unsigned long long)job.bytes_total,
                     (unsigned long long)job.bytes_per_second,
                     job.cancel_pending ? "true" : "false",
                     (unsigned long long)job.sequence,
                     (unsigned long long)job.started_at,
                     (unsigned long long)job.finished_at, wait_count);
    for (int i = 0; i < wait_count; i++) {
        if (i) ng_strbuf_append(out, ",");
        ng_strbuf_append(out, "{\"gameId\":");
        ng_strbuf_json_string(out, wait_items[i].game_id);
        ng_strbuf_append(out, ",\"title\":");
        ng_strbuf_json_string(out, wait_items[i].title);
        ng_strbuf_append(out, ",\"target\":");
        ng_strbuf_json_string(out, wait_items[i].target);
        ng_strbuf_printf(out, ",\"size\":%llu}",
                         (unsigned long long)wait_items[i].size);
    }
    ng_strbuf_append(out, "]},\"history\":[");
    for (int i = 0; i < history_count; i++) {
        const NgJob *item = &history[i];
        if (i) ng_strbuf_append(out, ",");
        ng_strbuf_append(out, "{\"state\":");
        ng_strbuf_json_string(out, job_state_name(item->state));
        ng_strbuf_append(out, ",\"gameId\":");
        ng_strbuf_json_string(out, item->game_id);
        ng_strbuf_append(out, ",\"title\":");
        ng_strbuf_json_string(out, item->title_name);
        ng_strbuf_append(out, ",\"target\":");
        ng_strbuf_json_string(out, item->target);
        ng_strbuf_append(out, ",\"stage\":");
        ng_strbuf_json_string(out, item->stage);
        ng_strbuf_append(out, ",\"message\":");
        ng_strbuf_json_string(out, item->message);
        ng_strbuf_printf(out,
                         ",\"progress\":%d,\"sequence\":%llu,\"bytesDone\":%llu,"
                         "\"bytesTotal\":%llu,\"startedAt\":%llu,"
                         "\"finishedAt\":%llu}",
                         item->progress,
                         (unsigned long long)item->sequence,
                         (unsigned long long)item->bytes_done,
                         (unsigned long long)item->bytes_total,
                         (unsigned long long)item->started_at,
                         (unsigned long long)item->finished_at);
    }
    ng_strbuf_append(out, "],");

    ng_strbuf_append(out, "\"storage\":{");
    state_storage_json(out, "system", "/user");
    ng_strbuf_append(out, ",");
    state_storage_json(out, "library", library_path);
    ng_strbuf_append(out, ",");
    state_storage_json(out, "homebrew", "/data/homebrew");
    ng_strbuf_append(out, "},\"sources\":[");
    pthread_mutex_lock(&g_app.lock);
    for (int i = 0; i < g_app.source_count; i++) {
        NgSource *source = g_app.sources[i];
        char status[32];
        char error[NG_ERROR_LEN];
        if (i) ng_strbuf_append(out, ",");
        pthread_mutex_lock(&source->lock);
        ng_strlcpy(status, source->status, sizeof(status));
        ng_strlcpy(error, source->error, sizeof(error));
        pthread_mutex_unlock(&source->lock);
        ng_strbuf_append(out, "{\"id\":");
        ng_strbuf_json_string(out, source->config.id);
        ng_strbuf_append(out, ",\"name\":");
        ng_strbuf_json_string(out, source->config.name);
        ng_strbuf_append(out, ",\"type\":");
        ng_strbuf_json_string(out, ng_source_type_name(source->config.type));
        ng_strbuf_printf(out, ",\"enabled\":%s,\"status\":",
                         source->config.enabled ? "true" : "false");
        ng_strbuf_json_string(out, status);
        ng_strbuf_append(out, ",\"error\":");
        ng_strbuf_json_string(out, error);
        ng_strbuf_append(out, "}");
    }
    pthread_mutex_unlock(&g_app.lock);
    pthread_mutex_lock(&g_app.catalog_lock);
    if (has_revision && instance_matches &&
        g_app.catalog_revision == known_revision) {
        ng_strbuf_printf(out,
                         "],\"catalogRevision\":%llu,\"catalogConfigGeneration\":%u,"
                         "\"gamesUnchanged\":true}",
                         (unsigned long long)g_app.catalog_revision,
                         g_app.catalog_config_generation);
        pthread_mutex_unlock(&g_app.catalog_lock);
        return out->failed ? -1 : 0;
    }
    ng_strbuf_printf(out,
                     "],\"catalogRevision\":%llu,\"catalogConfigGeneration\":%u,"
                     "\"gamesUnchanged\":false,\"games\":[",
                     (unsigned long long)g_app.catalog_revision,
                     g_app.catalog_config_generation);
    for (size_t i = 0; i < g_app.catalog.count; i++) {
        if (i) ng_strbuf_append(out, ",");
        state_game_json(out, &g_app.catalog.items[i]);
    }
    pthread_mutex_unlock(&g_app.catalog_lock);
    return ng_strbuf_append(out, "]}");
}

int
ng_app_state_json(NgStrBuf *out)
{
    return ng_app_state_json_since(out, 0, 0, NULL);
}

int
ng_app_settings_json(NgStrBuf *out)
{
    int rc;
    pthread_mutex_lock(&g_app.lock);
    rc = ng_config_to_json(&g_app.config, 0, out);
    pthread_mutex_unlock(&g_app.lock);
    return rc;
}

int
ng_app_log_json(NgStrBuf *out)
{
    if (ng_strbuf_append(out, "{\"ok\":true,\"log\":") != 0) return -1;
    if (ng_log_recent_json(out) != 0) return -1;
    return ng_strbuf_append(out, "}");
}
