#include "launcher.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "installer.h"
#include "log.h"
#include "ui_assets.h"
#include "util.h"

#ifdef __PROSPERO__
#define LAUNCHER_APP_ROOT "/user/app"
#define LAUNCHER_DIR LAUNCHER_APP_ROOT "/" NG_LAUNCHER_TITLE_ID
#define LAUNCHER_SCE_SYS LAUNCHER_DIR "/sce_sys"
#define LAUNCHER_APPMETA "/user/appmeta/" NG_LAUNCHER_TITLE_ID
#define LAUNCHER_STATE "/data/nasgames/launcher.state"

static uint64_t
hash_bytes(const void *data, size_t size)
{
    const uint8_t *bytes = data;
    uint64_t hash = 1469598103934665603ull;

    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

static int
file_matches(const char *path, const void *data, size_t size)
{
    struct stat st;
    uint8_t *existing;
    int fd;
    size_t done = 0;
    int matches = 0;

    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) ||
        (uint64_t)st.st_size != (uint64_t)size) {
        return 0;
    }
    existing = malloc(size ? size : 1);
    if (!existing) return 0;
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        free(existing);
        return 0;
    }
    while (done < size) {
        ssize_t n = read(fd, existing + done, size - done);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;
        done += (size_t)n;
    }
    close(fd);
    matches = done == size && !memcmp(existing, data, size);
    free(existing);
    return matches;
}

static int
write_if_changed(const char *path, const void *data, size_t size, int *changed)
{
    if (file_matches(path, data, size)) return 0;
    if (ng_write_file_atomic(path, data, size, 0666) != 0) return -1;
    *changed = 1;
    return 0;
}

static int
write_state(const char *expected)
{
    return ng_write_file_atomic(LAUNCHER_STATE, expected, strlen(expected), 0666);
}

static int
state_matches(const char *expected)
{
    char *existing = NULL;
    int length;

    length = ng_read_text_file(LAUNCHER_STATE, 1024, &existing);
    if (length < 0 || (size_t)length != strlen(expected) ||
        memcmp(existing, expected, (size_t)length) != 0) {
        free(existing);
        return 0;
    }
    free(existing);
    return 1;
}

static int
app_db_has_title(const char *title_id)
{
    char *data = NULL;
    int length;
    size_t title_length = strlen(title_id);
    int found = 0;

    length = ng_read_text_file("/system_data/priv/mms/app.db",
                               64 * 1024 * 1024, &data);
    if (length < 0) return -1;
    for (size_t i = 0; i + title_length <= (size_t)length; i++) {
        if (!memcmp(data + i, title_id, title_length)) {
            found = 1;
            break;
        }
    }
    free(data);
    return found;
}

static int
log_app_db_registration(void)
{
    int registered = app_db_has_title(NG_LAUNCHER_TITLE_ID);

    if (registered > 0) {
        ng_log_info("媒体入口已在 app.db 登记: %s", NG_LAUNCHER_TITLE_ID);
    } else if (registered == 0) {
        ng_log_error("媒体入口 app.db 登记缺失: %s", NG_LAUNCHER_TITLE_ID);
    } else {
        ng_log_error("媒体入口已注册，但无法读取 app.db 进行核对");
    }
    return registered;
}
#endif

int
ng_launcher_ensure(int port, char *error, size_t error_size)
{
#ifdef __PROSPERO__
    const NgUiAsset *icon = NULL;
    NgStrBuf json;
    int changed = 0;
    char param_path[NG_PATH_LEN];
    char icon_path[NG_PATH_LEN];
    char meta_param_path[NG_PATH_LEN];
    char meta_icon_path[NG_PATH_LEN];
    char state[192];
    int rc;
    int appmeta_ready;
    int db_registered;

    for (size_t i = 0; i < ng_ui_asset_count; i++) {
        if (!strcmp(ng_ui_assets[i].name, "/icon0.png")) {
            icon = &ng_ui_assets[i];
            break;
        }
    }
    if (!icon || !icon->data || icon->size == 0) {
        snprintf(error, error_size, "启动器图标 icon0.png 未嵌入");
        return -1;
    }
    if (ng_mkdir_p(LAUNCHER_SCE_SYS, 0777) != 0) {
        snprintf(error, error_size, "无法创建媒体入口目录: %s", strerror(errno));
        return -1;
    }
    appmeta_ready = ng_mkdir_p(LAUNCHER_APPMETA, 0777) == 0;
    if (!appmeta_ready) {
        ng_log_error("无法创建媒体入口 appmeta 目录: %s", strerror(errno));
    }
    snprintf(param_path, sizeof(param_path), "%s/param.json", LAUNCHER_SCE_SYS);
    snprintf(icon_path, sizeof(icon_path), "%s/icon0.png", LAUNCHER_SCE_SYS);
    snprintf(meta_param_path, sizeof(meta_param_path), "%s/param.json",
             LAUNCHER_APPMETA);
    snprintf(meta_icon_path, sizeof(meta_icon_path), "%s/icon0.png", LAUNCHER_APPMETA);
    unlink(LAUNCHER_SCE_SYS "/param.sfo");
    unlink(LAUNCHER_APPMETA "/param.sfo");

    ng_strbuf_init(&json);
    if (ng_strbuf_printf(&json, "{\n  \"applicationCategoryType\": %d,\n"
                           "  \"titleId\": \"%s\",\n"
                           "  \"deeplinkUri\": \"http://127.0.0.1:%d/?launcher=1\",\n"
                           "  \"localizedParameters\": {\n"
                           "    \"defaultLanguage\": \"zh-Hans\",\n"
                           "    \"en-US\": { \"titleName\": \"Game Center\" },\n"
                           "    \"zh-Hans\": { \"titleName\": \"Game Center\" },\n"
                           "    \"zh-Hant\": { \"titleName\": \"Game Center\" }\n"
                           "  }\n"
                           "}\n",
                         NG_LAUNCHER_CATEGORY, NG_LAUNCHER_TITLE_ID, port) != 0) {
        ng_strbuf_free(&json);
        snprintf(error, error_size, "无法构造媒体入口元数据");
        return -1;
    }
    snprintf(state, sizeof(state), "port=%d\njson=%016llx\nicon=%016llx\n",
             port,
             (unsigned long long)hash_bytes(json.data, json.len),
             (unsigned long long)hash_bytes(icon->data, icon->size));

    if (write_if_changed(param_path, json.data, json.len, &changed) != 0 ||
        write_if_changed(icon_path, icon->data, icon->size, &changed) != 0 ||
        (appmeta_ready &&
         (write_if_changed(meta_param_path, json.data, json.len, &changed) != 0 ||
          write_if_changed(meta_icon_path, icon->data, icon->size, &changed)))) {
        snprintf(error, error_size, "写入媒体入口资源失败: %s", strerror(errno));
        ng_strbuf_free(&json);
        return -1;
    }
    ng_strbuf_free(&json);

    if (!changed && state_matches(state)) {
        ng_log_info("媒体入口已是最新: %s", NG_LAUNCHER_TITLE_ID);
        db_registered = log_app_db_registration();
        if (db_registered != 0) return 0;
        ng_log_info("媒体入口文件存在，但 app.db 登记缺失，正在重新登记");
    }
    rc = ng_installer_register_title(NG_LAUNCHER_TITLE_ID, LAUNCHER_APP_ROOT,
                                     error, error_size);
    if (rc != 0) return -1;
    if (write_state(state) != 0) {
        ng_log_error("媒体入口已注册，但状态文件写入失败: %s", strerror(errno));
    }
    log_app_db_registration();
    ng_log_info("媒体入口已安装或更新: %s -> http://127.0.0.1:%d/",
                NG_LAUNCHER_TITLE_ID, port);
    return 0;
#else
    (void)port;
    (void)error;
    (void)error_size;
    return 0;
#endif
}
