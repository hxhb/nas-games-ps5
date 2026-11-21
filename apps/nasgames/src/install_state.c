#include "install_state.h"

#include <errno.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __PROSPERO__
#include <dlfcn.h>
#endif

#include "log.h"
#include "json.h"
#include "pkg_meta.h"
#include "util.h"

static int
regular_file_at(const char *root, const char *relative)
{
    char path[NG_PATH_LEN];
    struct stat st;
    if (!root || !relative ||
        snprintf(path, sizeof(path), "%s/%s", root, relative) >= (int)sizeof(path)) {
        return 0;
    }
    return stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0;
}

static int
title_directory_complete(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode) &&
           regular_file_at(path, "eboot.bin") &&
           (regular_file_at(path, "sce_sys/param.json") ||
            regular_file_at(path, "sce_sys/param.sfo"));
}

static void *
read_local_file(const char *path, size_t max_bytes, size_t *out_size)
{
    FILE *file;
    long length;
    void *data;

    if (out_size) *out_size = 0;
    file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) <= 0 ||
        (size_t)length > max_bytes || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    data = malloc((size_t)length + 1);
    if (!data) {
        fclose(file);
        return NULL;
    }
    if (fread(data, 1, (size_t)length, file) != (size_t)length) {
        free(data);
        fclose(file);
        return NULL;
    }
    ((char *)data)[length] = '\0';
    fclose(file);
    if (out_size) *out_size = (size_t)length;
    return data;
}

static int
local_file_size(const char *path, uint64_t *size)
{
    struct stat st;

    if (!size || stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0) {
        return -1;
    }
    *size = (uint64_t)st.st_size;
    return 0;
}

static int
pkg_header_valid(const char *path)
{
    unsigned char header[4];
    FILE *file;
    size_t count;

    file = fopen(path, "rb");
    if (!file) return 0;
    count = fread(header, 1, sizeof(header), file);
    fclose(file);
    return count == sizeof(header) && header[0] == 0x7f &&
           (!memcmp(header + 1, "CNT", 3) || !memcmp(header + 1, "FIH", 3));
}

static int
json_file_u64(const char *path, const char *key, uint64_t *value)
{
    NgJsonDoc doc;
    char *data;
    char number[32];
    size_t data_size = 0;
    int token;
    int length;
    int result = -1;

    if (!value) return -1;
    data = read_local_file(path, 512u * 1024u, &data_size);
    if (!data) return -1;
    if (ng_json_parse(&doc, data, data_size) == 0) {
        token = ng_json_obj_get(&doc, 0, key);
        if (token >= 0 && doc.tokens[token].type == NG_JSON_PRIMITIVE) {
            length = doc.tokens[token].end - doc.tokens[token].start;
            if (length > 0 && length < (int)sizeof(number)) {
                memcpy(number, doc.data + doc.tokens[token].start, (size_t)length);
                number[length] = '\0';
                if (number[0] != '-') {
                    char *end = NULL;
                    errno = 0;
                    unsigned long long parsed = strtoull(number, &end, 10);
                    if (!errno && end != number && *end == '\0') {
                        *value = (uint64_t)parsed;
                        result = 0;
                    }
                }
            }
        }
        ng_json_free(&doc);
    }
    free(data);
    return result;
}

static int
streamed_package_complete(const char *path, const char *filename,
                          const char *json_name, const char *pbm_name)
{
    char pkg[NG_PATH_LEN];
    char manifest[NG_PATH_LEN];
    uint64_t actual;
    uint64_t expected;

    if (ng_path_join_local(pkg, sizeof(pkg), path, filename) != 0 ||
        !pkg_header_valid(pkg) || local_file_size(pkg, &actual) != 0) return 0;
    if (ng_path_join_local(manifest, sizeof(manifest), path, json_name) == 0 &&
        json_file_u64(manifest, "originalFileSize", &expected) == 0 && expected) {
        return actual == expected;
    }
    return regular_file_at(path, pbm_name) || (!strcmp(filename, "app.pkg") && regular_file_at(path, json_name));
}

static int
dlc_payload_complete(const char *path)
{
    char pkg[NG_PATH_LEN];
    struct stat st;

    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) return 0;
    if (ng_path_join_local(pkg, sizeof(pkg), path, "ac.pkg") == 0 && stat(pkg, &st) == 0) {
        return streamed_package_complete(path, "ac.pkg", "ac.json", "ac.pbm");
    }
    return regular_file_at(path, "sce_sys/param.sfo") ||
           regular_file_at(path, "sce_sys/param.json");
}

static int
content_id_matches(const char *candidate, const char *wanted)
{
    const char *dash;
    const char *candidate_label;
    const char *wanted_label;

    if (!candidate || !candidate[0] || !wanted || !wanted[0]) return 0;
    if (!strcasecmp(candidate, wanted)) return 1;

    candidate_label = strstr(candidate, "_00-");
    if (candidate_label) {
        candidate_label += 4;
    } else {
        dash = strrchr(candidate, '-');
        candidate_label = dash && strlen(dash + 1) >= 4 ? dash + 1 : candidate;
    }
    wanted_label = strstr(wanted, "_00-");
    if (wanted_label) {
        wanted_label += 4;
    } else {
        dash = strrchr(wanted, '-');
        wanted_label = dash && strlen(dash + 1) >= 4 ? dash + 1 : wanted;
    }
    return !strcasecmp(candidate_label, wanted_label) ||
           !strcasecmp(candidate, wanted_label) ||
           !strcasecmp(candidate_label, wanted);
}

static int
addcont_metadata_matches(const char *path, const char *content_id)
{
    static const char *files[] = {
        "param.sfo",
        "sce_sys/param.sfo",
        "param.json",
        "sce_sys/param.json",
    };

    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        char candidate[NG_PATH_LEN];
        void *data;
        size_t data_size = 0;
        NgPkgMeta meta;
        int parsed;

        if (snprintf(candidate, sizeof(candidate), "%s/%s", path, files[i]) >=
            (int)sizeof(candidate)) {
            continue;
        }
        data = read_local_file(candidate, 512u * 1024u, &data_size);
        if (!data) continue;
        memset(&meta, 0, sizeof(meta));
        parsed = i < 2
            ? ng_app_meta_from_sfo(data, data_size, &meta)
            : ng_app_meta_from_json(data, &meta);
        free(data);
        if (parsed == 0 && content_id_matches(meta.content_id, content_id)) return 1;
    }
    return 0;
}

static int appinst_title_state(const char *title_id);
static int appinst_dlc_state(const char *content_id);

static int
title_app_installed(const char *title_id)
{
    static const char *roots[] = {
        "/user/app/%s",
        "/system_ex/app/%s",
        "/mnt/ext0/user/app/%s",
        "/mnt/ext1/user/app/%s",
    };

    int partial = 0;
    for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]); i++) {
        char path[NG_PATH_LEN];
        if (snprintf(path, sizeof(path), roots[i], title_id) >= (int)sizeof(path)) continue;
        if (title_directory_complete(path) ||
            streamed_package_complete(path, "app.pkg", "app.json", "app.pbm")) return 1;
        if (regular_file_at(path, "app.pkg") || regular_file_at(path, "app.json") ||
            regular_file_at(path, "app.pbm")) partial = 1;
    }
    return partial ? -1 : appinst_title_state(title_id) > 0;
}

#ifdef __PROSPERO__
typedef int (*AppInstInitializeFn)(void);
typedef int (*AppExistsFn)(const char *);
typedef int (*GetAddcontStatusFn)(const char *, int *);

static pthread_once_t appinst_once = PTHREAD_ONCE_INIT;
static void *appinst_handle;
static AppExistsFn appinst_app_exists;
static GetAddcontStatusFn appinst_addcont_status;

static void
load_appinst_util(void)
{
    AppInstInitializeFn initialize;

    appinst_handle = dlopen("libSceAppInstUtil.sprx", RTLD_NOW | RTLD_LOCAL);
    if (!appinst_handle) {
        ng_log_info("AppInstUtil 不可用，安装判定将使用文件系统: %s", dlerror());
        return;
    }
    initialize = (AppInstInitializeFn)dlsym(appinst_handle,
                                            "sceAppInstUtilInitialize");
    appinst_app_exists = (AppExistsFn)dlsym(appinst_handle,
                                            "sceAppInstUtilAppExists");
    appinst_addcont_status = (GetAddcontStatusFn)dlsym(
        appinst_handle, "sceAppInstUtilGetAddcontInstalledStatus");
    if (initialize) {
        int rc = initialize();
        if (rc != 0) ng_log_info("AppInstUtil 初始化返回 0x%08x", (unsigned)rc);
    }
    if (!appinst_app_exists || !appinst_addcont_status) {
        ng_log_info("AppInstUtil 缺少安装状态查询接口，使用文件系统兜底");
    }
}

static int
appinst_title_state(const char *title_id)
{
    int rc;

    pthread_once(&appinst_once, load_appinst_util);
    if (!appinst_app_exists) return -1;
    rc = appinst_app_exists(title_id);
    if (rc < 0) return -1;
    return rc > 0;
}

static int
appinst_dlc_state(const char *content_id)
{
    int rc;
    int status = 0;

    pthread_once(&appinst_once, load_appinst_util);
    if (!appinst_addcont_status) return -1;
    rc = appinst_addcont_status(content_id, &status);
    if (rc != 0) return -1;
    return status == 1;
}
#else
static int
appinst_title_state(const char *title_id)
{
    (void)title_id;
    return -1;
}

static int
appinst_dlc_state(const char *content_id)
{
    (void)content_id;
    return -1;
}
#endif

static void
title_app_version(const char *title_id, char *version, size_t version_size)
{
    static const char *paths[] = {
        "/system_data/priv/appmeta/%s/param.sfo",
        "/system_data/priv/appmeta/%s/param.json",
        "/system_data/priv/appmeta/%s/patch/param.sfo",
        "/system_data/priv/appmeta/%s/patch/param.json",
        "/system_data/priv/appmeta/external/%s/param.sfo",
        "/system_data/priv/appmeta/external/%s/param.json",
        "/user/appmeta/%s/param.sfo",
        "/user/appmeta/%s/param.json",
        "/user/appmeta/%s/patch/param.sfo",
        "/user/appmeta/%s/patch/param.json",
        "/user/patch/%s/sce_sys/param.sfo",
        "/user/patch/%s/sce_sys/param.json",
        "/user/patch/%s/param.sfo",
        "/user/patch/%s/param.json",
        "/user/patch0/%s/sce_sys/param.sfo",
        "/user/patch0/%s/sce_sys/param.json",
        "/user/app/%s/sce_sys/param.sfo",
        "/user/app/%s/sce_sys/param.json",
        "/system_ex/app/%s/sce_sys/param.sfo",
        "/system_ex/app/%s/sce_sys/param.json",
        "/mnt/ext0/user/app/%s/sce_sys/param.sfo",
        "/mnt/ext0/user/app/%s/sce_sys/param.json",
        "/mnt/ext1/user/app/%s/sce_sys/param.sfo",
        "/mnt/ext1/user/app/%s/sce_sys/param.json",
    };
    char best[64] = "";

    if (!version || version_size == 0) return;
    version[0] = '\0';
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        char path[NG_PATH_LEN];
        void *data;
        size_t data_size = 0;
        NgPkgMeta meta;
        if (snprintf(path, sizeof(path), paths[i], title_id) >= (int)sizeof(path)) continue;
        data = read_local_file(path, 512u * 1024u, &data_size);
        if (!data) continue;
        memset(&meta, 0, sizeof(meta));
        int parsed = ng_has_suffix_ci(path, ".json")
            ? ng_app_meta_from_json(data, &meta)
            : ng_app_meta_from_sfo(data, data_size, &meta);
        if (parsed == 0 && meta.version[0] &&
            (!best[0] || ng_version_compare(meta.version, best) > 0)) {
            ng_strlcpy(best, meta.version, sizeof(best));
        }
        free(data);
    }
    if (best[0]) ng_strlcpy(version, best, version_size);
}

static int
addcont_name_matches(const char *name, const char *content_id)
{
    return content_id_matches(name, content_id);
}

typedef enum {
    NG_DLC_ABSENT = 0,
    NG_DLC_COMPLETE,
    NG_DLC_PARTIAL,
} NgDlcState;

static NgDlcState
dlc_roots_state(const char *title_id, const char *content_id,
                const char *const *roots, size_t root_count)
{
    NgDlcState result = NG_DLC_ABSENT;

    for (size_t i = 0; i < root_count; i++) {
        char path[NG_PATH_LEN];
        DIR *dir;
        struct dirent *entry;
        if (snprintf(path, sizeof(path), roots[i], title_id) >= (int)sizeof(path)) continue;
        dir = opendir(path);
        if (!dir) continue;
        while ((entry = readdir(dir)) != NULL) {
            char child[NG_PATH_LEN];
            struct stat st;
            int matches;

            if (entry->d_name[0] == '.') continue;
            if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >=
                (int)sizeof(child)) {
                continue;
            }
            matches = addcont_name_matches(entry->d_name, content_id);
            if (!matches && stat(child, &st) == 0 && S_ISDIR(st.st_mode)) {
                matches = addcont_metadata_matches(child, content_id);
            }
            if (!matches) continue;
            if (dlc_payload_complete(child)) {
                closedir(dir);
                return NG_DLC_COMPLETE;
            }
            result = NG_DLC_PARTIAL;
        }
        closedir(dir);
    }
    return result;
}

static NgDlcState
dlc_installation_state(const char *title_id, const char *content_id)
{
    static const char *payload_roots[] = {
        "/user/addcont/%s",
        "/system_ex/addcont/%s",
        "/mnt/ext0/user/addcont/%s",
        "/mnt/ext1/user/addcont/%s",
        "/user/app/%s/addcont",
        "/system_ex/app/%s/addcont",
        "/mnt/ext0/user/app/%s/addcont",
        "/mnt/ext1/user/app/%s/addcont",
    };
    NgDlcState payload_state;

    payload_state = dlc_roots_state(title_id, content_id, payload_roots,
                                    sizeof(payload_roots) / sizeof(payload_roots[0]));
    if (payload_state == NG_DLC_ABSENT && appinst_dlc_state(content_id) == 1) return NG_DLC_COMPLETE;
    return payload_state;
}

int
ng_install_state_probe(const NgConfig *config, const char *title_id,
                       NgPkgKind kind, const char *package_version,
                       const char *content_id, NgInstallState *state)
{
    char library[NG_PATH_LEN];
    char homebrew[NG_PATH_LEN];
    int title_state;

    if (!state) return -1;
    memset(state, 0, sizeof(*state));
    if (!config || !ng_valid_title_id(title_id)) return 0;

    title_state = title_app_installed(title_id);
    state->title_installed = title_state > 0;
    if (state->title_installed) {
        title_app_version(title_id, state->title_version, sizeof(state->title_version));
    }
    if (kind == NG_PKG_DLC) {
        state->package_known = ng_valid_content_id(content_id);
        if (state->package_known) {
            NgDlcState dlc_state = dlc_installation_state(title_id, content_id);
            state->package_installed = dlc_state == NG_DLC_COMPLETE;
            state->package_partial = dlc_state == NG_DLC_PARTIAL;
        }
        ng_log_info("DLC %s %s: %s", title_id,
                    content_id && content_id[0] ? content_id : "(缺少 CONTENT_ID)",
                    !state->package_known ? "无法判定" :
                    state->package_installed ? "载荷完整" :
                    state->package_partial ? "载荷不完整" : "未安装");
    } else if (kind == NG_PKG_UPDATE) {
        if (!state->title_installed) {
            state->package_known = 1;
            state->package_installed = 0;
        } else if (package_version && package_version[0] && state->title_version[0]) {
            state->package_known = 1;
            state->package_installed =
                ng_version_compare(state->title_version, package_version) >= 0;
        } else {
            state->package_known = 0;
            state->package_installed = 0;
        }
    } else {
        state->package_known = 1;
        state->package_installed = state->title_installed;
        state->package_partial = title_state < 0;
    }

    if (ng_path_join_local(library, sizeof(library), config->data_dir,
                           "library") == 0) {
        char path[NG_PATH_LEN];
        if (snprintf(path, sizeof(path), "%s/%s", library, title_id) < (int)sizeof(path)) {
            state->library = title_directory_complete(path);
        }
    }
    if (snprintf(homebrew, sizeof(homebrew), "/data/homebrew/%s", title_id) <
        (int)sizeof(homebrew)) {
        state->homebrew = title_directory_complete(homebrew);
    }
    return 0;
}

int
ng_install_state_find_asset(const char *title_id, const char *name,
                            char *out, size_t out_size)
{
    static const char *roots[] = {
        "/user/appmeta/%s",
        "/system_ex/appmeta/%s",
        "/user/app/%s/sce_sys",
        "/system_ex/app/%s/sce_sys",
        "/mnt/ext0/user/app/%s/sce_sys",
        "/mnt/ext1/user/app/%s/sce_sys",
    };

    if (!out || out_size == 0) return -1;
    out[0] = '\0';
    if (!ng_valid_title_id(title_id) || !name || !name[0]) return -1;
    for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]); i++) {
        char root[NG_PATH_LEN];
        char path[NG_PATH_LEN];
        struct stat st;
        if (snprintf(root, sizeof(root), roots[i], title_id) >= (int)sizeof(root)) continue;
        if (snprintf(path, sizeof(path), "%s/%s", root, name) >= (int)sizeof(path)) continue;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0) {
            ng_strlcpy(out, path, out_size);
            return 0;
        }
    }
    return -1;
}
