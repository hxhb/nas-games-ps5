#ifndef NASGAMES_COMMON_H
#define NASGAMES_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#define NG_APP_NAME "NAS Games"
#define NG_VERSION "1.0.2"

#define NG_MAX_SOURCES 16
#define NG_MAX_ROOTS 12
#define NG_MAX_GAMES 2048
#define NG_MAX_ART_BYTES (32u * 1024u * 1024u)
#define NG_MAX_META_BYTES (2u * 1024u * 1024u)
#define NG_MAX_HTTP_BODY (1024u * 1024u)

#define NG_ID_LEN 32
#define NG_NAME_LEN 96
#define NG_FILE_NAME_LEN (255 * 3 + 1)
#define NG_TITLE_LEN 256
#define NG_PATH_LEN 1024
#define NG_URL_LEN 2048
#define NG_ERROR_LEN 320
#define NG_JOB_HISTORY_MAX 16

typedef enum {
    NG_SOURCE_NONE = 0,
    NG_SOURCE_SMB,
    NG_SOURCE_FTP,
    NG_SOURCE_LOCAL,
} NgSourceType;

typedef enum {
    NG_GAME_FOLDER = 0,
    NG_GAME_PKG,
} NgGameType;

typedef enum {
    NG_PKG_UNKNOWN = 0,
    NG_PKG_BASE,
    NG_PKG_UPDATE,
    NG_PKG_DLC,
} NgPkgKind;

typedef enum {
    NG_INSTALL_SYSTEM = 0,
    NG_INSTALL_LIBRARY,
    NG_INSTALL_HOMEBREW,
} NgInstallTarget;

typedef enum {
    NG_PACK_PLAIN = 0,
    NG_PACK_ZSTD,
    NG_PACK_LZ4,
} NgPacking;

typedef enum {
    NG_JOB_IDLE = 0,
    NG_JOB_RUNNING,
    NG_JOB_DONE,
    NG_JOB_ERROR,
    NG_JOB_CANCELED,
} NgJobState;

typedef struct {
    char id[NG_ID_LEN];
    char name[NG_NAME_LEN];
    NgSourceType type;
    char host[NG_TITLE_LEN];
    int port;
    char share[NG_TITLE_LEN];
    char username[NG_NAME_LEN];
    char password[NG_TITLE_LEN];
    char domain[NG_NAME_LEN];
    char local_root[NG_PATH_LEN];
    char roots[NG_MAX_ROOTS][NG_PATH_LEN];
    int root_count;
    int enabled;
} NgSourceConfig;

typedef struct {
    int version;
    int port;
    int auto_scan;
    int scan_interval_seconds;
    int max_scan_depth;
    int open_browser_on_start;
    int keep_awake;
    char data_dir[NG_PATH_LEN];
    char cache_dir[NG_PATH_LEN];
    char art_dir[NG_PATH_LEN];
    NgSourceConfig sources[NG_MAX_SOURCES];
    int source_count;
} NgConfig;

typedef struct {
    char id[24];
    char source_id[NG_ID_LEN];
    char source_name[NG_NAME_LEN];
    char title_id[16];
    char title_name[NG_TITLE_LEN];
    char version[64];
    char category[32];
    char content_id[64];
    char installed_version[64];
    char path[NG_PATH_LEN];
    NgGameType type;
    NgPkgKind pkg_kind;
    NgPacking packing;
    uint64_t size;
    char icon_file[NG_PATH_LEN];
    char pic0_file[NG_PATH_LEN];
    char pic1_file[NG_PATH_LEN];
    int installable;
    int installed;
    int install_known;
    int install_partial;
    int base_installed;
    int library_copy;
    int homebrew_copy;
} NgGame;

typedef struct {
    NgGame *items;
    size_t count;
    size_t capacity;
} NgCatalog;

typedef struct {
    NgJobState state;
    char game_id[24];
    char title_id[16];
    char title_name[NG_TITLE_LEN];
    char target[16];
    char stage[NG_NAME_LEN];
    char message[NG_ERROR_LEN];
    int progress;
    int cancel_pending;
    uint64_t sequence;
    time_t started_at;
    time_t finished_at;
    uint64_t bytes_done;
    uint64_t bytes_total;
    uint64_t bytes_per_second;
} NgJob;

#endif
