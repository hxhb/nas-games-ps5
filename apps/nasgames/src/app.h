#ifndef NASGAMES_APP_H
#define NASGAMES_APP_H

#include <pthread.h>
#include <stddef.h>

#include "common.h"
#include "config.h"
#include "source.h"
#include "util.h"

typedef struct {
    char game_id[24];
    uint64_t size;
} NgMeasuredSize;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t scan_cond;
    pthread_mutex_t catalog_lock;
    pthread_mutex_t job_lock;
    char config_path[NG_PATH_LEN];
    NgConfig config;
    NgCatalog catalog;
    uint64_t catalog_revision;
    unsigned catalog_config_generation;
    NgMeasuredSize measured_sizes[NG_MAX_GAMES];
    int measured_size_count;
    NgSource *sources[NG_MAX_SOURCES];
    int source_count;
    unsigned config_generation;
    int scan_requested;
    int scan_busy;
    uint64_t scan_sequence;
    uint64_t scan_started_at;
    uint64_t scan_finished_at;
    char scan_message[NG_ERROR_LEN];
    NgJob job;
    NgJob job_history[NG_JOB_HISTORY_MAX];
    int job_history_count;
    int actual_port;
    uint64_t started_at;
    pthread_t scan_thread;
    int scan_thread_started;
} NgApp;

NgApp *ng_app(void);
int ng_app_init(const char *data_dir);
int ng_app_start_scanner(void);
void ng_app_stop(void);

void ng_app_config_snapshot(NgConfig *out);
void ng_app_data_dir_snapshot(char *out, size_t out_size);
int ng_app_keep_awake_enabled(void);
int ng_app_server_port(void);
int ng_app_open_browser_on_start(void);
int ng_app_parse_source_json(const char *json, NgSourceConfig *out,
                             char *error, size_t error_size);
int ng_app_apply_config(const char *json, char *error, size_t error_size);
NgSource *ng_app_get_source(const char *id);
int ng_app_copy_game(const char *id, NgGame *out);
int ng_app_copy_game_source(const char *id, NgGame *game, NgSource **source);
int ng_app_mark_game_installed(const char *game_id, const char *title_id);
int ng_app_mark_game_library(const char *game_id, const char *title_id);
int ng_app_mark_game_homebrew(const char *game_id, const char *title_id);
int ng_app_replace_catalog(NgGame *items, size_t count, unsigned generation, int incomplete);
uint64_t ng_app_request_scan(void);
void ng_app_wake_scanner(void);
void ng_app_wait_scan_signal(unsigned int timeout_ms);
int ng_app_scan_begin(NgConfig *config, NgSource *sources[], unsigned *generation);
void ng_app_scan_mark_finished(int ok, const char *message);
int ng_app_set_game_size(const char *game_id, NgSource *source, uint64_t size);

void ng_app_job_set(NgJobState state, const NgGame *game, const char *target,
                    const char *stage, const char *message, int progress,
                    uint64_t done, uint64_t total);
void ng_app_job_update(const char *stage, const char *message, int progress,
                       uint64_t done, uint64_t total);
NgJob ng_app_job_snapshot(void);
void ng_app_job_history_clear(void);

int ng_app_state_json(NgStrBuf *out);
int ng_app_state_json_since(NgStrBuf *out, int has_revision,
                            uint64_t known_revision,
                            const char *known_instance);
int ng_app_settings_json(NgStrBuf *out);
int ng_app_log_json(NgStrBuf *out);

#endif
