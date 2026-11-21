#include "installer.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#ifdef __PROSPERO__
#include <dlfcn.h>
#include <pthread_np.h>
#include <ps5/kernel.h>
#include <ps5/nid.h>
#include <sys/mount.h>
#include <sys/uio.h>
#endif

#include "app.h"
#include "http_server.h"
#include "keep_awake.h"
#include "log.h"
#include "source.h"
#include "source_prefetch.h"
#include "storage.h"
#include "util.h"

static pthread_mutex_t installer_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t installer_ready = PTHREAD_COND_INITIALIZER;
static int installer_stopping;

#define INSTALL_CANCELED (-2)

typedef struct {
    NgGame game;
    NgSource *source;
    NgInstallTarget target;
    int keep_awake;
    int cancel_requested;
    int system_handoff;
    int committing;
} InstallTask;

static InstallTask *install_queue[NG_INSTALL_QUEUE_MAX];
static InstallTask *active_task;
static int install_queue_count;
static int installer_worker_running;

static int
install_cancel_requested(void)
{
    int canceled;

    pthread_mutex_lock(&installer_lock);
    canceled = active_task && active_task->cancel_requested;
    pthread_mutex_unlock(&installer_lock);
    return canceled;
}

static void
install_set_cancel_pending(int pending)
{
    NgApp *app = ng_app();

    pthread_mutex_lock(&app->job_lock);
    if (app->job.state == NG_JOB_RUNNING) app->job.cancel_pending = pending;
    pthread_mutex_unlock(&app->job_lock);
}

static int
install_begin_commit(void)
{
    int canceled;

    pthread_mutex_lock(&installer_lock);
    canceled = active_task->cancel_requested;
    if (!canceled) active_task->committing = 1;
    pthread_mutex_unlock(&installer_lock);
    return canceled ? INSTALL_CANCELED : 0;
}

int
ng_installer_stream_snapshot(const char *id, NgGame *game, NgSource **source)
{
    int found = 0;

    pthread_mutex_lock(&installer_lock);
    if (active_task && active_task->system_handoff && !strcmp(active_task->game.id, id)) {
        *game = active_task->game;
        *source = active_task->source;
        ng_source_acquire(*source);
        found = 1;
    }
    pthread_mutex_unlock(&installer_lock);
    return found;
}

#ifdef __PROSPERO__
static int
install_begin_system_handoff(void)
{
    int canceled = 0;

    pthread_mutex_lock(&installer_lock);
    if (active_task) {
        canceled = active_task->cancel_requested;
        if (!canceled) active_task->system_handoff = 1;
    }
    pthread_mutex_unlock(&installer_lock);
    return canceled ? -1 : 0;
}
#endif

static int
ensure_dir(const char *path)
{
    return ng_mkdir_p(path, 0777);
}

static void
remove_tree_local(const char *path)
{
    struct stat st;

    if (lstat(path, &st) != 0) return;
    if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
        DIR *dir = opendir(path);
        struct dirent *entry;
        if (dir) {
            while ((entry = readdir(dir)) != NULL) {
                char child[NG_PATH_LEN];
                if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
                if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) <
                    (int)sizeof(child)) {
                    remove_tree_local(child);
                }
            }
            closedir(dir);
        }
        rmdir(path);
    } else {
        unlink(path);
    }
}

static int
has_title_metadata(const char *root)
{
    char path[NG_PATH_LEN];

    if (ng_path_join_local(path, sizeof(path), root, "sce_sys/param.json") == 0 &&
        access(path, R_OK) == 0) {
        return 1;
    }
    if (ng_path_join_local(path, sizeof(path), root, "sce_sys/param.sfo") == 0 &&
        access(path, R_OK) == 0) {
        return 1;
    }
    return 0;
}

static int
valid_tid(const char *tid)
{
    if (!tid || strlen(tid) != 9) return 0;
    for (int i = 0; i < 4; i++) {
        if (tid[i] < 'A' || tid[i] > 'Z') return 0;
    }
    for (int i = 4; i < 9; i++) {
        if (tid[i] < '0' || tid[i] > '9') return 0;
    }
    return strncmp(tid, "NPXS", 4) != 0;
}

#ifdef __PROSPERO__
static int
copy_file_fd(const char *src, const char *dst, mode_t mode)
{
    int in = -1;
    int out = -1;
    char buf[131072];
    int rc = -1;

    in = open(src, O_RDONLY);
    if (in < 0) return -1;
    if (ng_ensure_parent_dir(dst, 0777) != 0) goto done;
    out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (out < 0) goto done;
    while (1) {
        ssize_t n = read(in, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            goto done;
        }
        if (n == 0) break;
        char *p = buf;
        ssize_t left = n;
        while (left > 0) {
            ssize_t w = write(out, p, (size_t)left);
            if (w <= 0) {
                if (w < 0 && errno == EINTR) continue;
                if (w == 0) errno = EIO;
                goto done;
            }
            p += w;
            left -= w;
        }
    }
    rc = 0;
done:
    if (out >= 0 && close(out) != 0) rc = -1;
    if (in >= 0) close(in);
    if (rc == 0) chmod(dst, mode);
    return rc;
}

static int
copy_dir_local(const char *src, const char *dst)
{
    DIR *dir;
    struct dirent *entry;
    int rc = 0;

    if (ensure_dir(dst) != 0) return -1;
    dir = opendir(src);
    if (!dir) return -1;
    while ((entry = readdir(dir)) != NULL) {
        char from[NG_PATH_LEN];
        char to[NG_PATH_LEN];
        struct stat st;
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        snprintf(from, sizeof(from), "%s/%s", src, entry->d_name);
        snprintf(to, sizeof(to), "%s/%s", dst, entry->d_name);
        if (stat(from, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            if (copy_dir_local(from, to) != 0) rc = -1;
        } else if (S_ISREG(st.st_mode)) {
            if (copy_file_fd(from, to, 0666) != 0) rc = -1;
        }
    }
    closedir(dir);
    return rc;
}

static int
copy_param_json(const char *src, const char *dst)
{
    char *data = NULL;
    int len;
    int rc;

    len = ng_read_text_file(src, NG_MAX_META_BYTES, &data);
    if (len < 0) return -1;
    char *found = strstr(data, "upgradable");
    if (found) {
        const size_t old_len = strlen("upgradable");
        const size_t new_len = strlen("standard");
        memmove(found + new_len, found + old_len, strlen(found + old_len) + 1);
        memcpy(found, "standard", new_len);
        len -= (int)(old_len - new_len);
    }
    rc = ng_write_file_atomic(dst, data, (size_t)len, 0666);
    free(data);
    return rc;
}

static int
is_appmeta_file(const char *name)
{
    return !strcasecmp(name, "param.json") || !strcasecmp(name, "param.sfo") ||
           ng_has_suffix_ci(name, ".png") || ng_has_suffix_ci(name, ".dds") ||
           ng_has_suffix_ci(name, ".at9");
}
#endif

#ifdef __PROSPERO__
typedef int (*InstallTitleDirFn)(const char *, const char *, void *);
typedef int (*InstallByPackageFn)(const void *, void *, void *);
typedef int (*InstallAllFn)(void *);
typedef int (*GetInstallStatusFn)(const char *, void *);
typedef int (*GetPatchInstallStatusFn)(const char *, void *);
typedef int (*CancelInstallFn)(const char *);
typedef int (*AppInstInitializeFn)(void);

int sceUserServiceInitialize(void *);

static pthread_mutex_t appinst_lock = PTHREAD_MUTEX_INITIALIZER;
static void *appinst_handle;
static void *appinst_ipmi_handle;
static uint32_t appinst_kernel_handle;
static int appinst_kernel_loaded;
static int appinst_loaded;
static int appinst_load_result;
static int appinst_initialized;
static AppInstInitializeFn appinst_initialize;
static InstallTitleDirFn appinst_install_title_dir;
static InstallByPackageFn appinst_install_by_package;
static InstallAllFn appinst_install_all;
static GetInstallStatusFn appinst_get_install_status;
static GetPatchInstallStatusFn appinst_get_patch_install_status;
static CancelInstallFn appinst_cancel_install;
static CancelInstallFn appinst_cancel_patch;
static CancelInstallFn appinst_cancel_addcont;
static char appinst_initialize_error[256];
static char appinst_title_dir_error[256];
static char appinst_package_error[256];
static char appinst_status_error[256];

typedef struct {
    const char *uri;
    const char *ex_uri;
    const char *playgo_scenario_id;
    const char *content_id;
    const char *content_name;
    const char *icon_url;
    uint64_t install_mode;
    uint64_t reserved[7];
} pkg_metadata_t;

typedef struct {
    char content_id[48];
    int type;
    int platform;
} pkg_info_t;

typedef struct {
    char languages[30][8];
    char scenario_ids[64][3];
    char content_ids[64][48];
    long unknown[810];
} playgo_info_t;

typedef struct {
    int32_t code;
    int32_t version;
    char description[512];
    char type[9];
} install_error_t;

typedef struct {
    char status[16];
    char src_type[8];
    float remain_time;
    uint64_t downloaded;
    uint64_t initial_chunk;
    uint64_t total;
    uint32_t promote_progress;
    unsigned char reserved[100];
    install_error_t error;
    char patch_version[12];
    int32_t local_copy_percent;
    uint8_t copy_only;
} install_status_t;

_Static_assert(sizeof(pkg_metadata_t) == 0x70, "unexpected PKG metadata ABI");
_Static_assert(sizeof(pkg_info_t) == 0x38, "unexpected PKG info ABI");
_Static_assert(sizeof(playgo_info_t) == 0x2700, "unexpected PlayGo info ABI");
_Static_assert(sizeof(install_status_t) == 0x2c8, "unexpected PKG status ABI");

static intptr_t
resolve_appinst_symbol(uint32_t kernel_handle, const char *name,
                       char *error, size_t error_size)
{
    intptr_t symbol = 0;
    char nid[12] = "";
    const char *detail = NULL;

    if (!nid_encode(name, nid)) {
        ng_log_error("AppInstUtil 无法编码符号 NID: %s", name);
        nid[0] = '\0';
    }
    if (appinst_handle) {
        dlerror();
        symbol = (intptr_t)dlsym(appinst_handle, name);
        if (symbol <= 0 && nid[0]) symbol = (intptr_t)dlsym(appinst_handle, nid);
        if (symbol > 0) return symbol;
        detail = dlerror();
    }
    if (kernel_handle) {
        symbol = kernel_dynlib_dlsym(-1, kernel_handle, name);
        if (symbol > 0) return symbol;
        if (nid[0]) {
            symbol = kernel_dynlib_resolve(-1, kernel_handle, nid);
            if (symbol > 0) {
                ng_log_info("AppInstUtil 符号 %s 通过 NID %s 解析", name, nid);
                return symbol;
            }
        }
    }
    if (error && error_size) {
        if (detail && *detail) {
            snprintf(error, error_size, "%s（NID %s）", detail,
                     nid[0] ? nid : "编码失败");
        } else if (nid[0]) {
            snprintf(error, error_size, "内核与动态库均无法解析 NID %s", nid);
        } else {
            snprintf(error, error_size, "动态符号不存在");
        }
    }
    return 0;
}

static int
ensure_appinst_util(char *error, size_t error_size)
{
    int result;

    pthread_mutex_lock(&appinst_lock);
    if (!appinst_loaded) {
        uint32_t handle = 0;
        int kernel_loaded = 0;
        uint32_t sysmodule_handle = 0;

        if (kernel_dynlib_handle(-1, "libSceSysmodule.sprx", &sysmodule_handle) >= 0) {
            int (*load_module)(unsigned int) =
                (int (*)(unsigned int))kernel_dynlib_dlsym(
                    -1, sysmodule_handle, "sceSysmoduleLoadModuleInternal");
            if (load_module) {
                load_module(0x8000001du);
                load_module(0x80000011u);
                load_module(0x80000010u);
                load_module(0x80000014u);
            }
        }
        appinst_ipmi_handle = dlopen("/system/common/lib/libSceIpmi.sprx",
                                     RTLD_NOW | RTLD_GLOBAL);
        if (!appinst_ipmi_handle) {
            appinst_ipmi_handle = dlopen("libSceIpmi.sprx",
                                         RTLD_NOW | RTLD_GLOBAL);
        }
        if (kernel_dynlib_handle(-1, "libSceAppInstUtil.sprx", &handle) >= 0) {
            appinst_kernel_handle = handle;
            appinst_kernel_loaded = 1;
            kernel_loaded = 1;
        }
        if (!appinst_initialize || !appinst_install_title_dir ||
            !appinst_install_by_package || !appinst_get_install_status) {
            appinst_handle = dlopen("/system/common/lib/libSceAppInstUtil.sprx",
                                    RTLD_NOW | RTLD_GLOBAL);
            if (!appinst_handle) {
                appinst_handle = dlopen("libSceAppInstUtil.sprx",
                                        RTLD_NOW | RTLD_GLOBAL);
            }
        }
        ng_log_info("AppInstUtil 动态库: ipmi=%p appinst=%p kernel=0x%x",
                    appinst_ipmi_handle, appinst_handle, appinst_kernel_handle);
        appinst_initialize = (AppInstInitializeFn)resolve_appinst_symbol(
            appinst_kernel_handle, "sceAppInstUtilInitialize",
            appinst_initialize_error, sizeof(appinst_initialize_error));
        if (appinst_initialize) {
            int init_rc = appinst_initialize();
            appinst_initialized = init_rc == 0;
            ng_log_info("sceAppInstUtilInitialize rc=0x%08x", (unsigned)init_rc);
        }
        appinst_install_title_dir = (InstallTitleDirFn)resolve_appinst_symbol(
            appinst_kernel_handle, "sceAppInstUtilAppInstallTitleDir",
            appinst_title_dir_error, sizeof(appinst_title_dir_error));
        appinst_install_by_package = (InstallByPackageFn)resolve_appinst_symbol(
            appinst_kernel_handle, "sceAppInstUtilInstallByPackage",
            appinst_package_error, sizeof(appinst_package_error));
        appinst_install_all = (InstallAllFn)resolve_appinst_symbol(
            appinst_kernel_handle, "sceAppInstUtilAppInstallAll",
            NULL, 0);
        appinst_get_install_status = (GetInstallStatusFn)resolve_appinst_symbol(
            appinst_kernel_handle, "sceAppInstUtilGetInstallStatus",
            appinst_status_error, sizeof(appinst_status_error));
        appinst_get_patch_install_status = (GetPatchInstallStatusFn)
            resolve_appinst_symbol(appinst_kernel_handle,
                                   "sceAppInstUtilGetPatchInstallStatus",
                                   NULL, 0);
        appinst_cancel_install = (CancelInstallFn)resolve_appinst_symbol(
            appinst_kernel_handle, "sceAppInstUtilCancelInstall", NULL, 0);
        appinst_cancel_patch = (CancelInstallFn)resolve_appinst_symbol(
            appinst_kernel_handle, "sceAppInstUtilCancelInstallPatch", NULL, 0);
        appinst_cancel_addcont = (CancelInstallFn)resolve_appinst_symbol(
            appinst_kernel_handle, "sceAppInstUtilCancelInstallAddcont", NULL, 0);
        if (!appinst_initialize && kernel_loaded) {
            snprintf(appinst_initialize_error,
                     sizeof(appinst_initialize_error), "内核符号解析失败");
        }
        if (!appinst_install_title_dir && kernel_loaded &&
            !appinst_title_dir_error[0]) {
            snprintf(appinst_title_dir_error,
                     sizeof(appinst_title_dir_error), "内核符号解析失败");
        }
        if (!appinst_install_by_package && kernel_loaded &&
            !appinst_package_error[0]) {
            snprintf(appinst_package_error, sizeof(appinst_package_error),
                     "内核符号解析失败");
        }
        if (!appinst_get_install_status && kernel_loaded &&
            !appinst_status_error[0]) {
            snprintf(appinst_status_error, sizeof(appinst_status_error),
                     "内核符号解析失败");
        }
        if (!appinst_initialize) {
            appinst_load_result = -1;
            snprintf(error, error_size,
                     "AppInstUtil 缺少接口 sceAppInstUtilInitialize: %s",
                     appinst_initialize_error[0] ? appinst_initialize_error
                                                 : "模块不可用");
            ng_log_error("AppInstUtil 符号解析失败: %s", error);
        } else {
            appinst_load_result = 0;
            if (!appinst_install_title_dir) {
                ng_log_info("AppInstUtil 不支持文件夹注册: %s",
                            appinst_title_dir_error);
            }
            if (!appinst_install_by_package) {
                ng_log_info("AppInstUtil 不支持 PKG 安装: %s",
                            appinst_package_error);
            }
            if (!appinst_install_all) {
                ng_log_info("AppInstUtil 不支持全量标题登记");
            }
            if (!appinst_get_install_status) {
                ng_log_info("AppInstUtil 不支持安装进度查询: %s",
                            appinst_status_error);
            }
        }
        appinst_loaded = 1;
    }
    result = appinst_load_result;
    pthread_mutex_unlock(&appinst_lock);
    return result;
}

static int
appinst_initialize_safe(void)
{
    int rc = 0;

    pthread_mutex_lock(&appinst_lock);
    if (!appinst_initialized) {
        rc = appinst_initialize ? appinst_initialize() : -1;
        appinst_initialized = rc == 0;
    }
    pthread_mutex_unlock(&appinst_lock);
    return rc;
}

static int
appinst_cancel_pkg(const NgGame *game, const char *content_id,
                   char *error, size_t error_size)
{
    CancelInstallFn cancel = NULL;
    int rc;

    if (!ng_valid_content_id(content_id)) {
        snprintf(error, error_size, "缺少 CONTENT_ID，无法取消系统安装");
        return -1;
    }
    if (ensure_appinst_util(error, error_size) != 0) return -1;
    if (appinst_initialize_safe() != 0) {
        snprintf(error, error_size, "sceAppInstUtilInitialize 失败");
        return -1;
    }
    if (game->pkg_kind == NG_PKG_UPDATE) {
        cancel = appinst_cancel_patch;
    } else if (game->pkg_kind == NG_PKG_DLC) {
        cancel = appinst_cancel_addcont;
    } else {
        cancel = appinst_cancel_install;
    }
    if (!cancel) {
        snprintf(error, error_size, "当前系统不支持取消该类型 PKG 安装");
        return -1;
    }
    rc = cancel(content_id);
    if (rc != 0) {
        snprintf(error, error_size, "取消系统安装失败: 0x%08x", (unsigned)rc);
        return -1;
    }
    ng_log_info("已请求系统取消安装: %s (%s)", game->title_name, content_id);
    return 0;
}

static int
install_status_success(const install_status_t *status, uint64_t total)
{
    return !strcmp(status->status, "completed") || !strcmp(status->status, "installed") ||
           (!strcmp(status->status, "playable") && total && status->downloaded >= total);
}

static int
install_status_error(const char *status)
{
    return !strcmp(status, "error") || !strcmp(status, "failed");
}

static int
wait_for_pkg_install(const NgGame *game, const char *content_id,
                     GetInstallStatusFn get_status, char *error, size_t error_size)
{
    install_status_t install_status;
    install_status_t *status = &install_status;
    uint64_t started_at = ng_now_monotonic_ms();
    int unreadable = 0;
    int cancel_failed = 0;
    for (;;) {
        int rc;

        sleep(1);
        if (install_cancel_requested()) {
            if (appinst_cancel_pkg(game, content_id, error, error_size) == 0) return INSTALL_CANCELED;
            ng_log_error("%s", error);
            pthread_mutex_lock(&installer_lock);
            active_task->cancel_requested = 0;
            install_set_cancel_pending(0);
            pthread_mutex_unlock(&installer_lock);
            cancel_failed = 1;
            NgJob job = ng_app_job_snapshot();
            ng_app_job_update("取消失败", error, job.progress, job.bytes_done, job.bytes_total);
        }
        memset(status, 0, sizeof(*status));
        rc = get_status(content_id, status);
        if (rc != 0) {
            if (++unreadable < 10) continue;
            snprintf(error, error_size, "读取系统安装状态失败: 0x%08x",
                     (unsigned)rc);
            return -1;
        }
        unreadable = 0;
        status->status[sizeof(status->status) - 1] = '\0';
        status->error.description[sizeof(status->error.description) - 1] = '\0';
        status->error.type[sizeof(status->error.type) - 1] = '\0';

        if (status->error.code != 0) {
            snprintf(error, error_size, "系统安装失败: 0x%08x%s%s",
                     (unsigned)status->error.code,
                     status->error.description[0] ? " - " : "",
                     status->error.description);
            return -1;
        }
        uint64_t total = status->total ? status->total : game->size;
        uint64_t downloaded = status->downloaded;
        if (install_status_success(status, total)) {
            ng_log_info("PKG %s 系统状态 %s，已下载 %llu / %llu 字节", game->title_name,
                        status->status, (unsigned long long)downloaded, (unsigned long long)total);
            ng_app_job_update("系统安装", status->status, 99, downloaded, total);
            return 0;
        }
        if (install_status_error(status->status)) {
            snprintf(error, error_size, "系统安装失败: %s", status->status);
            return -1;
        }
        if (!strcmp(status->status, "canceled") || !strcmp(status->status, "cancelled")) {
            return INSTALL_CANCELED;
        }
        if (ng_now_monotonic_ms() - started_at > 24ull * 60 * 60 * 1000) {
            snprintf(error, error_size, "系统安装超时，最后状态: %s",
                     status->status);
            return -1;
        }

        int progress = total ? (downloaded >= total ? 99 : (int)(100.0 * downloaded / total)) : -1;
        char message[NG_ERROR_LEN];
        snprintf(message, sizeof(message), "系统状态: %s", status->status);
        ng_app_job_update(cancel_failed ? "取消失败" : "系统安装",
                          cancel_failed ? error : message, progress, downloaded, total);
    }
}

static int
ps5_nmount_pairs(const char *const pairs[][2], int pair_count, int flags)
{
    struct iovec iov[12];
    int n = 0;

    if (pair_count * 2 > (int)(sizeof(iov) / sizeof(iov[0]))) return -1;
    for (int i = 0; i < pair_count; i++) {
        iov[n].iov_base = (void *)pairs[i][0];
        iov[n].iov_len = strlen(pairs[i][0]) + 1;
        n++;
        if (pairs[i][1]) {
            iov[n].iov_base = (void *)pairs[i][1];
            iov[n].iov_len = strlen(pairs[i][1]) + 1;
        } else {
            iov[n].iov_base = NULL;
            iov[n].iov_len = 0;
        }
        n++;
    }
    return nmount(iov, (unsigned)n, flags);
}

static int
remount_system_ex(void)
{
    static const char *const pairs[][2] = {
        {"from", "/dev/ssd0.system_ex"}, {"fspath", "/system_ex"},
        {"fstype", "exfatfs"}, {"large", "yes"}, {"timezone", "static"},
        {"async", NULL}, {"ignoreacl", NULL},
    };
    return ps5_nmount_pairs(pairs, (int)(sizeof(pairs) / sizeof(pairs[0])), MNT_UPDATE);
}

static int
mount_folder(const char *tid, const char *src, char *error, size_t error_size)
{
    char dst[NG_PATH_LEN];
    char eboot[NG_PATH_LEN];
    static const char *const pairs[][2] = {
        {"fstype", "nullfs"}, {"from", NULL}, {"fspath", NULL},
    };
    const char *mount_pairs[3][2];

    snprintf(eboot, sizeof(eboot), "%s/eboot.bin", src);
    if (access(eboot, R_OK) != 0) {
        snprintf(error, error_size, "导入目录缺少 eboot.bin");
        return -1;
    }
    snprintf(dst, sizeof(dst), "/system_ex/app/%s", tid);
    if (ensure_dir(dst) != 0 && errno == EROFS) {
        if (remount_system_ex() != 0) {
            snprintf(error, error_size, "无法将 /system_ex 重新挂载为可写: %s", strerror(errno));
            return -1;
        }
        if (ensure_dir(dst) != 0) {
            snprintf(error, error_size, "无法创建 %s: %s", dst, strerror(errno));
            return -1;
        }
    }
    if (unmount(dst, 0) != 0 && errno != EINVAL && errno != ENOENT) {
        snprintf(error, error_size, "无法卸载旧挂载点 %s: %s", dst, strerror(errno));
        return -1;
    }
    for (int i = 0; i < 3; i++) {
        mount_pairs[i][0] = pairs[i][0];
        mount_pairs[i][1] = pairs[i][1];
    }
    mount_pairs[1][1] = src;
    mount_pairs[2][1] = dst;
    if (ps5_nmount_pairs(mount_pairs, 3, 0) != 0) {
        snprintf(error, error_size, "nullfs 挂载失败 %s -> %s: %s", src, dst, strerror(errno));
        return -1;
    }
    return 0;
}

static int
stage_title(const char *tid, const char *src)
{
    char sce_sys[NG_PATH_LEN];
    char app_dir[NG_PATH_LEN];
    char app_sce_sys[NG_PATH_LEN];
    char appmeta[NG_PATH_LEN];
    char param_src[NG_PATH_LEN];
    char param_dst[NG_PATH_LEN];
    DIR *dir;
    struct dirent *entry;
    int rc = 0;

    if (ng_path_join_local(sce_sys, sizeof(sce_sys), src, "sce_sys") != 0) return -1;
    snprintf(app_dir, sizeof(app_dir), "/user/app/%s", tid);
    snprintf(appmeta, sizeof(appmeta), "/user/appmeta/%s", tid);
    if (ng_path_join_local(param_src, sizeof(param_src), sce_sys, "param.json") != 0 ||
        ng_path_join_local(app_sce_sys, sizeof(app_sce_sys), app_dir, "sce_sys") != 0 ||
        ng_path_join_local(param_dst, sizeof(param_dst), app_sce_sys, "param.json") != 0) {
        return -1;
    }
    if (ensure_dir(appmeta) != 0) return -1;
    if (copy_dir_local(sce_sys, app_sce_sys) != 0) return -1;
    if ((access(param_src, F_OK) == 0 || errno != ENOENT) &&
        copy_param_json(param_src, param_dst) != 0) return -1;

    dir = opendir(sce_sys);
    if (!dir) return -1;
    while ((entry = readdir(dir)) != NULL) {
        char from[NG_PATH_LEN];
        char to[NG_PATH_LEN];
        struct stat st;
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        snprintf(from, sizeof(from), "%s/%s", sce_sys, entry->d_name);
        if (stat(from, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (!is_appmeta_file(entry->d_name)) continue;
        snprintf(to, sizeof(to), "%s/%s", appmeta, entry->d_name);
        if (!strcasecmp(entry->d_name, "param.json")) {
            if (copy_param_json(from, to) != 0) rc = -1;
        } else if (copy_file_fd(from, to, 0666) != 0) {
            rc = -1;
        }
    }
    closedir(dir);

    return rc;
}

static int
ps5_register_folder(const char *tid, const char *src, char *error, size_t error_size)
{
    sceUserServiceInitialize(NULL);
    if (ensure_appinst_util(error, error_size) != 0) return -1;
    if (!appinst_install_title_dir && !appinst_install_all) {
        snprintf(error, error_size, "当前系统不支持文件夹注册: %s",
                 appinst_title_dir_error);
        return -1;
    }
    if (stage_title(tid, src) != 0) {
        snprintf(error, error_size, "准备标题元数据失败: %s", strerror(errno));
        return -1;
    }
    if (mount_folder(tid, src, error, error_size) != 0) return -1;
    return ng_installer_register_title(tid, "/user/app/", error, error_size);
}

int
ng_installer_register_title(const char *title_id, const char *title_root,
                            char *error, size_t error_size)
{
#ifdef __PROSPERO__
    int rc;

    if (!title_id || !title_root) {
        snprintf(error, error_size, "标题参数无效");
        return -1;
    }
    if (ensure_appinst_util(error, error_size) != 0) return -1;
    rc = appinst_initialize_safe();
    if (rc != 0) {
        snprintf(error, error_size, "sceAppInstUtilInitialize 失败: 0x%08x",
                 (unsigned)rc);
        return -1;
    }
    if (appinst_install_title_dir) {
        rc = appinst_install_title_dir(title_id, title_root, NULL);
    } else if (appinst_install_all) {
        rc = appinst_install_all(NULL);
    } else {
        snprintf(error, error_size, "当前系统不支持注册标题目录: %s",
                 appinst_title_dir_error);
        return -1;
    }
    if (rc != 0 && (unsigned)rc != 0x80990002u) {
        snprintf(error, error_size, "注册标题失败: 0x%08x", (unsigned)rc);
        return -1;
    }
    return 0;
#else
    (void)title_id;
    (void)title_root;
    (void)error;
    (void)error_size;
    return 0;
#endif
}

static int
ps5_install_pkg(const NgGame *game, char *error, size_t error_size)
{
    char uri[NG_URL_LEN];
    char content_id[64];
    NgStorageInfo storage;
    pkg_metadata_t metadata = {0};
    pkg_info_t *info = calloc(1, sizeof(*info));
    playgo_info_t *playgo = calloc(1, sizeof(*playgo));
    GetInstallStatusFn get_status;
    int rc;

    if (!info || !playgo) {
        free(info);
        free(playgo);
        snprintf(error, error_size, "内存不足");
        return -1;
    }
    if (game->size > 0 && ng_storage_probe("/user", &storage) == 0 &&
        storage.known && game->size > storage.free_bytes) {
        snprintf(error, error_size,
                 "系统空间不足：需要 %llu MB，可用 %llu MB",
                 (unsigned long long)(game->size / (1024u * 1024u)),
                 (unsigned long long)(storage.free_bytes / (1024u * 1024u)));
        free(info);
        free(playgo);
        return -1;
    }
    if (install_cancel_requested()) {
        snprintf(error, error_size, "安装已取消");
        free(info);
        free(playgo);
        return INSTALL_CANCELED;
    }
    snprintf(uri, sizeof(uri), "http://127.0.0.1:%d/stream/%s",
             ng_app()->actual_port, game->id);
    metadata.uri = uri;
    metadata.ex_uri = "";
    metadata.playgo_scenario_id = "";
    metadata.content_id = game->content_id;
    metadata.content_name = game->title_name;
    metadata.icon_url = "";
    sceUserServiceInitialize(NULL);
    if (ensure_appinst_util(error, error_size) != 0) {
        free(info);
        free(playgo);
        return -1;
    }
    if (!appinst_install_by_package) {
        snprintf(error, error_size, "当前系统不支持 PKG 安装: %s",
                 appinst_package_error);
        free(info);
        free(playgo);
        return -1;
    }
    get_status = game->pkg_kind == NG_PKG_UPDATE
        ? appinst_get_patch_install_status : appinst_get_install_status;
    if (!get_status) {
        snprintf(error, error_size, "当前系统缺少此类 PKG 的安装状态接口");
        free(info);
        free(playgo);
        return -1;
    }
    rc = appinst_initialize_safe();
    if (rc != 0) {
        snprintf(error, error_size, "sceAppInstUtilInitialize 失败: 0x%08x", (unsigned)rc);
        free(info);
        free(playgo);
        return -1;
    }
    ng_app_job_update("系统安装", "正在下载并安装 PKG", -1, 0, game->size);
    if (install_begin_system_handoff() != 0) {
        snprintf(error, error_size, "安装已取消");
        free(info);
        free(playgo);
        return INSTALL_CANCELED;
    }
    rc = appinst_install_by_package(&metadata, info, playgo);
    if (rc != 0) {
        if (install_cancel_requested()) {
            free(info);
            free(playgo);
            return INSTALL_CANCELED;
        }
        snprintf(error, error_size, "PKG 安装失败: 0x%08x", (unsigned)rc);
        free(info);
        free(playgo);
        return -1;
    }
    info->content_id[sizeof(info->content_id) - 1] = '\0';
    snprintf(content_id, sizeof(content_id), "%s",
             ng_valid_content_id(info->content_id) ? info->content_id : game->content_id);
    free(info);
    free(playgo);
    if (!ng_valid_content_id(content_id)) {
        snprintf(error, error_size, "系统已受理 PKG，但未提供可查询的 CONTENT_ID");
        return -1;
    }
    ng_app_job_update("系统安装", "等待系统开始安装", -1, 0, game->size);
    rc = wait_for_pkg_install(game, content_id, get_status, error, error_size);
    return rc;
}
#else
static int
ps5_register_folder(const char *tid, const char *src, char *error, size_t error_size)
{
    (void)tid;
    (void)src;
    (void)error;
    (void)error_size;
    return 0;
}

static int
ps5_install_pkg(const NgGame *game, char *error, size_t error_size)
{
    (void)error;
    (void)error_size;
    ng_log_info("host 模式：模拟安装 PKG %s", game->title_name);
    sleep(1);
    return install_begin_commit();
}
#endif

static int
copy_remote_file(NgSource *source, const char *remote, const char *local,
                 uint64_t *bytes_done, uint64_t bytes_total, uint64_t expected_size)
{
    NgSourceFile *file;
    NgPrefetcher *prefetch = NULL;
    uint8_t *buf = NULL;
    size_t buf_size = 0;
    int out;
    int rc = 0;
    uint64_t last_update = 0;
    int fd_open = 0;
    int prefetch_active;

    file = ng_source_open_file(source, remote);
    if (!file) return -1;
    if (ng_ensure_parent_dir(local, 0777) != 0) {
        ng_source_file_close(file);
        return -1;
    }
    out = open(local, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out < 0) {
        ng_source_file_close(file);
        return -1;
    }
    fd_open = 1;
    prefetch = ng_prefetch_create(file, file->size);
    prefetch_active = ng_prefetch_is_active(prefetch);
    if (!prefetch_active) {
        buf_size = ng_prefetch_chunk_size(prefetch);
        buf = malloc(buf_size);
        if (!buf) {
            rc = -1;
            goto done;
        }
    }
    while (1) {
        const void *data = NULL;
        ssize_t n;

        if (install_cancel_requested()) {
            rc = INSTALL_CANCELED;
            break;
        }
        if (prefetch_active) {
            n = ng_prefetch_peek(prefetch, &data);
        } else {
            n = ng_source_file_read(file, buf, buf_size);
            data = buf;
        }
        if (n < 0) {
            rc = install_cancel_requested() ? INSTALL_CANCELED : -1;
            break;
        }
        if (n == 0) break;
        if (install_cancel_requested()) {
            rc = INSTALL_CANCELED;
            break;
        }
        const uint8_t *p = data;
        ssize_t left = n;
        while (left > 0) {
            ssize_t w = write(out, p, (size_t)left);
            if (w <= 0) {
                if (w < 0 && errno == EINTR) continue;
                if (w == 0) errno = EIO;
                rc = -1;
                break;
            }
            p += w;
            left -= w;
        }
        if (rc != 0) break;
        if (prefetch_active) ng_prefetch_advance(prefetch, (size_t)n);
        *bytes_done += (uint64_t)n;
        {
            uint64_t now = ng_now_monotonic_ms();
            if (now - last_update >= 250) {
                int progress = bytes_total ? (int)((*bytes_done * 100) / bytes_total) : -1;
                if (progress > 99) progress = 99;
                ng_app_job_update("安装文件", ng_path_basename(remote), progress,
                                  *bytes_done, bytes_total);
                last_update = now;
            }
        }
    }
    if (rc == 0 && fsync(out) != 0) rc = -1;
done:
    if (prefetch) ng_prefetch_destroy(prefetch);
    free(buf);
    if (fd_open) {
        if (close(out) != 0 && rc == 0) rc = -1;
        fd_open = 0;
    }
    ng_source_file_close(file);
    if (rc == 0) {
        int progress = bytes_total ? (int)((*bytes_done * 100) / bytes_total) : -1;
        if (progress > 99) progress = 99;
        ng_app_job_update("安装文件", ng_path_basename(remote), progress,
                          *bytes_done, bytes_total);
    }
    if (rc == 0 && expected_size > 0) {
        struct stat st;
        if (stat(local, &st) != 0 || (uint64_t)st.st_size != expected_size) rc = -1;
    }
    if (rc == 0) chmod(local, 0666);
    return rc;
}

static int
copy_remote_tree(NgSource *source, const char *remote, const char *local,
                 int depth, uint64_t *bytes_done, uint64_t bytes_total)
{
    NgSourceList list;
    int rc = 0;

    if (depth > 64) return -1;
    if (install_cancel_requested()) return INSTALL_CANCELED;
    if (ensure_dir(local) != 0) return -1;
    ng_source_list_init(&list);
    if (ng_source_list_dir(source, remote, &list) != 0) {
        ng_source_list_free(&list);
        return install_cancel_requested() ? INSTALL_CANCELED : -1;
    }
    for (size_t i = 0; i < list.count; i++) {
        char rpath[NG_PATH_LEN];
        char lpath[NG_PATH_LEN];
        int copied;
        if (install_cancel_requested()) {
            rc = INSTALL_CANCELED;
            break;
        }
        if (ng_source_child_path(rpath, sizeof(rpath), remote, list.items[i].name) != 0 ||
            ng_source_child_path(lpath, sizeof(lpath), local, list.items[i].name) != 0) {
            rc = -1;
            break;
        }
        if (list.items[i].is_dir) {
            copied = copy_remote_tree(source, rpath, lpath, depth + 1,
                                      bytes_done, bytes_total);
        } else {
            copied = copy_remote_file(source, rpath, lpath, bytes_done, bytes_total,
                                      list.items[i].size);
        }
        if (copied != 0) rc = copied;
        if (rc != 0) break;
    }
    ng_source_list_free(&list);
    return rc;
}

static int
install_folder(const NgGame *game, NgSource *source, NgInstallTarget target,
               char *error, size_t error_size)
{
    NgConfig cfg;
    char root[NG_PATH_LEN];
    char dest[NG_PATH_LEN];
    char staging[NG_PATH_LEN];
    char check[NG_PATH_LEN];
    uint64_t total = 0;
    uint64_t done = 0;
    int result = -1;
    int staging_created = 0;
    NgStorageInfo storage;

    if (!valid_tid(game->title_id)) {
        snprintf(error, error_size, "文件夹游戏标题 ID 无效");
        return -1;
    }
    if (target == NG_INSTALL_LIBRARY && game->packing != NG_PACK_PLAIN) {
        snprintf(error, error_size, "该文件夹使用 %s 压缩，只能安装到本机应用目录",
                 ng_packing_name(game->packing));
        return -1;
    }
    if (target != NG_INSTALL_LIBRARY && target != NG_INSTALL_HOMEBREW) {
        snprintf(error, error_size, "不支持的文件夹安装目标");
        return -1;
    }
    ng_app_config_snapshot(&cfg);
    if (install_cancel_requested()) {
        snprintf(error, error_size, "安装已取消");
        result = INSTALL_CANCELED;
        goto out;
    }
    if (target == NG_INSTALL_LIBRARY) {
        if (ng_path_join_local(root, sizeof(root), cfg.data_dir, "library") != 0) {
            snprintf(error, error_size, "本地库路径过长");
            goto out;
        }
    } else {
        ng_strlcpy(root, "/data/homebrew", sizeof(root));
    }
    if (snprintf(dest, sizeof(dest), "%s/%s", root, game->title_id) >= (int)sizeof(dest)) {
        snprintf(error, error_size, "本地导入路径过长");
        goto out;
    }
    if (ensure_dir(root) != 0) {
        snprintf(error, error_size, "无法创建目标目录: %s", strerror(errno));
        goto out;
    }
    if (target == NG_INSTALL_LIBRARY) {
        if (access(dest, F_OK) == 0) {
            if (ng_path_join_local(check, sizeof(check), dest, "eboot.bin") != 0 ||
                access(check, R_OK) != 0) {
                snprintf(error, error_size, "目标目录不完整，缺少 eboot.bin");
                goto out;
            }
            if (!has_title_metadata(dest)) {
                snprintf(error, error_size,
                         "目标目录不完整，缺少 sce_sys/param.json 或 param.sfo");
                goto out;
            }
            result = install_begin_commit();
            if (result != 0) goto out;
            ng_app_job_update("注册标题", "目标目录已存在，正在重新挂载并注册", 99, 0, 0);
            result = ps5_register_folder(game->title_id, dest, error, error_size);
            if (result == 0) ng_app_mark_game_library(game->id, game->title_id);
            goto out;
        }
    } else if (access(dest, F_OK) == 0) {
        snprintf(error, error_size,
                 "/data/homebrew/%s 已存在；如需重新安装，请先在文件管理器中删除该目录",
                 game->title_id);
        goto out;
    }
    if (snprintf(staging, sizeof(staging), "%s/.%s.nasgames.XXXXXX",
                 root, game->title_id) >= (int)sizeof(staging)) {
        snprintf(error, error_size, "临时目录路径过长");
        goto out;
    }
    ng_app_job_update("估算大小", "正在统计远端文件", -1, 0, 0);
    if (install_cancel_requested()) {
        snprintf(error, error_size, "安装已取消");
        result = INSTALL_CANCELED;
        goto out;
    }
    if (ng_source_measure_tree(source, game->path, &total, NULL, install_cancel_requested) != 0) {
        if (install_cancel_requested()) {
            result = INSTALL_CANCELED;
            goto out;
        }
        snprintf(error, error_size, "无法读取远端目录");
        goto out;
    }
    ng_app_set_game_size(game->id, source, total);
    if (install_cancel_requested()) {
        snprintf(error, error_size, "安装已取消");
        result = INSTALL_CANCELED;
        goto out;
    }
    if (ng_storage_probe(root, &storage) == 0) {
        if (total > storage.free_bytes) {
            snprintf(error, error_size, "本地空间不足：需要 %.1f GB，可用 %.1f GB",
                     total / 1073741824.0, storage.free_bytes / 1073741824.0);
            goto out;
        }
    }
    if (!mkdtemp(staging)) {
        snprintf(error, error_size, "无法创建临时目录: %s", strerror(errno));
        goto out;
    }
    ng_app_job_update("安装游戏",
                      target == NG_INSTALL_HOMEBREW
                          ? "正在安装到 /data/homebrew"
                          : "正在安装到 NAS Games 本地库",
                      0, 0, total);
    staging_created = 1;
    {
        int copied = copy_remote_tree(source, game->path, staging, 0, &done, total);
        if (copied == INSTALL_CANCELED) {
            snprintf(error, error_size, "安装已取消");
            result = INSTALL_CANCELED;
            goto out;
        }
        if (copied != 0) {
            char detail[NG_ERROR_LEN];
            ng_source_error_snapshot(source, strerror(errno ? errno : EIO), detail, sizeof(detail));
            ng_strlcpy(error, "写入安装文件失败: ", error_size);
            ng_strlcat(error, detail, error_size);
            goto out;
        }
    }
    if (done != total) {
        snprintf(error, error_size, "源目录大小在复制期间发生变化，请重新安装");
        goto out;
    }
    if (ng_path_join_local(check, sizeof(check), staging, "eboot.bin") != 0) {
        snprintf(error, error_size, "本地导入路径过长");
        goto out;
    }
    if (access(check, R_OK) != 0) {
        snprintf(error, error_size, "导入目录缺少 eboot.bin");
        goto out;
    }
    if (!has_title_metadata(staging)) {
        snprintf(error, error_size, "导入目录缺少 sce_sys/param.json 或 param.sfo");
        goto out;
    }
    result = install_begin_commit();
    if (result != 0) goto out;
    struct stat existing;
    if (lstat(dest, &existing) == 0) {
        snprintf(error, error_size, "安装目标已存在");
        result = -1;
        goto out;
    }
    if (rename(staging, dest) != 0) {
        snprintf(error, error_size, "无法提交导入目录: %s", strerror(errno));
        result = -1;
        goto out;
    }
    staging_created = 0;
    if (target == NG_INSTALL_LIBRARY) {
        ng_app_mark_game_library(game->id, game->title_id);
        ng_app_job_update("注册标题", "正在挂载并注册到主界面", 99, done, total);
        result = ps5_register_folder(game->title_id, dest, error, error_size);
    } else {
        result = 0;
    }

out:
    if (staging_created) remove_tree_local(staging);
    return result;
}

static void
run_install_task(InstallTask *task)
{
    char error[NG_ERROR_LEN] = "";
    char done_message[NG_ERROR_LEN] = "";
    const char *target = task->target == NG_INSTALL_SYSTEM ? "system" :
        task->target == NG_INSTALL_HOMEBREW ? "homebrew" : "library";
    int rc;
    NgGame current_game;
    NgSource *current_source = NULL;

    ng_app_job_set(NG_JOB_RUNNING, &task->game, target, "准备",
                   task->target == NG_INSTALL_HOMEBREW
                       ? "正在准备安装到 /data/homebrew"
                       : "正在准备安装",
                   0, 0,
                   task->game.type == NG_GAME_PKG ? task->game.size : 0);
    if (task->keep_awake) ng_keep_awake_start();
    if (install_cancel_requested()) {
        rc = INSTALL_CANCELED;
    } else if (ng_app_copy_game_source(task->game.id, &current_game, &current_source) != 0 ||
               current_source != task->source ||
               strcmp(current_game.content_id, task->game.content_id) ||
               strcmp(current_game.version, task->game.version) ||
               current_game.pkg_kind != task->game.pkg_kind ||
               current_game.packing != task->game.packing ||
               (task->game.type == NG_GAME_PKG && current_game.size != task->game.size)) {
        snprintf(error, sizeof(error), "游戏来源已变更，请重新扫描后提交安装");
        rc = -1;
    } else if (task->game.type == NG_GAME_PKG &&
               (task->game.pkg_kind == NG_PKG_UPDATE || task->game.pkg_kind == NG_PKG_DLC) &&
               !current_game.base_installed) {
        snprintf(error, sizeof(error), "请先安装同 Title ID 的基础游戏");
        rc = -1;
    } else if (task->game.type == NG_GAME_PKG) {
        ng_http_stream_cache_clear();
        rc = ps5_install_pkg(&task->game, error, sizeof(error));
        ng_http_stream_cache_clear();
    } else {
        rc = install_folder(&task->game, task->source, task->target, error, sizeof(error));
    }
    ng_source_release(current_source);
    pthread_mutex_lock(&installer_lock);
    task->committing = 1;
    pthread_mutex_unlock(&installer_lock);
    if (task->keep_awake) ng_keep_awake_stop();
    if (rc == 0) {
        if (task->game.type == NG_GAME_PKG) {
            snprintf(done_message, sizeof(done_message), "PKG 安装完成，正在刷新安装状态");
            ng_app_mark_game_installed(task->game.id, task->game.title_id);
        } else if (task->target == NG_INSTALL_LIBRARY) {
            snprintf(done_message, sizeof(done_message), "文件夹游戏已注册到主界面");
            ng_app_mark_game_installed(task->game.id, task->game.title_id);
        } else {
            snprintf(done_message, sizeof(done_message),
                     "已安装到 PS5 本机 /data/homebrew/%s",
                     task->game.title_id);
            ng_app_mark_game_homebrew(task->game.id, task->game.title_id);
        }
        ng_app_job_set(NG_JOB_DONE, &task->game, target, "完成",
                       done_message, 100, 0, 0);
        ng_log_info("安装完成: %s (%s)", task->game.title_name, task->game.title_id);
        ng_app_request_scan();
    } else if (rc == INSTALL_CANCELED) {
        ng_app_job_set(NG_JOB_CANCELED, &task->game, target, "已取消",
                       task->game.type == NG_GAME_PKG ? "系统安装已取消" : "安装已取消，临时文件已清理",
                       -1, 0, 0);
        ng_log_info("安装已取消: %s (%s)", task->game.title_name, task->game.title_id);
        ng_app_request_scan();
    } else {
        ng_app_job_set(NG_JOB_ERROR, &task->game, target, "失败",
                       error, -1, 0, 0);
        ng_log_error("安装失败: %s: %s", task->game.title_name, error);
        if (task->system_handoff) ng_app_request_scan();
    }
}

static void *
installer_main(void *unused)
{
    (void)unused;
    for (;;) {
        InstallTask *task;

        pthread_mutex_lock(&installer_lock);
        if (install_queue_count == 0 || installer_stopping) {
            installer_worker_running = 0;
            pthread_cond_broadcast(&installer_ready);
            pthread_mutex_unlock(&installer_lock);
            break;
        }
        task = install_queue[0];
        install_queue_count--;
        if (install_queue_count > 0) {
            memmove(install_queue, install_queue + 1,
                    (size_t)install_queue_count * sizeof(install_queue[0]));
        }
        active_task = task;
        task->cancel_requested = 0;
        task->system_handoff = 0;
        pthread_mutex_unlock(&installer_lock);
        run_install_task(task);
        pthread_mutex_lock(&installer_lock);
        active_task = NULL;
        pthread_mutex_unlock(&installer_lock);
        ng_source_release(task->source);
        free(task);
    }
    return NULL;
}

static int
install_task_valid(const NgGame *game, NgInstallTarget target,
                   char *error, size_t error_size)
{
    if (!game) return -1;
    if (game->type == NG_GAME_PKG && target != NG_INSTALL_SYSTEM) {
        snprintf(error, error_size, "PKG 只能通过系统安装器安装");
        return -1;
    }
    if (game->type == NG_GAME_FOLDER) {
        if (target == NG_INSTALL_SYSTEM) {
            snprintf(error, error_size, "文件夹游戏不能使用系统 PKG 安装目标");
            return -1;
        }
        if (target != NG_INSTALL_LIBRARY && target != NG_INSTALL_HOMEBREW) {
            snprintf(error, error_size, "不支持的文件夹安装目标");
            return -1;
        }
        if (target == NG_INSTALL_LIBRARY && game->packing != NG_PACK_PLAIN) {
            snprintf(error, error_size, "压缩文件夹只能安装到 /data/homebrew");
            return -1;
        }
    }
    return 0;
}

int
ng_installer_start(const NgGame *game, NgSource *source, NgInstallTarget target,
                   char *error, size_t error_size)
{
    InstallTask *task;
    pthread_t thread;
    pthread_attr_t attr;
    int start_worker = 0;

    if (install_task_valid(game, target, error, error_size) != 0) return -1;
    if (!source || !source->config.enabled) {
        snprintf(error, error_size, "游戏来源已停用或不存在，请重新扫描");
        return -1;
    }
    task = calloc(1, sizeof(*task));
    if (!task) {
        snprintf(error, error_size, "内存不足");
        return -1;
    }
    task->game = *game;
    task->source = source;
    task->target = target;
    {
        NgConfig config;
        ng_app_config_snapshot(&config);
        task->keep_awake = config.keep_awake;
    }

    pthread_mutex_lock(&installer_lock);
    if (installer_stopping) {
        pthread_mutex_unlock(&installer_lock);
        free(task);
        snprintf(error, error_size, "NAS Games 正在停止");
        return -1;
    }
    int duplicate = active_task && !strcmp(active_task->game.id, game->id);
    for (int i = 0; !duplicate && i < install_queue_count; i++) {
        duplicate = !strcmp(install_queue[i]->game.id, game->id);
    }
    if (duplicate) {
        pthread_mutex_unlock(&installer_lock);
        free(task);
        snprintf(error, error_size, "该资源已有安装任务");
        return -1;
    }
    if (install_queue_count >= NG_INSTALL_QUEUE_MAX) {
        pthread_mutex_unlock(&installer_lock);
        free(task);
        snprintf(error, error_size, "安装队列已满，请等待当前任务完成");
        return -1;
    }
    install_queue[install_queue_count++] = task;
    if (!installer_worker_running) {
        installer_worker_running = 1;
        start_worker = 1;
    }
    if (start_worker) {
        int thread_error = 0;

        if (pthread_attr_init(&attr) != 0) {
            thread_error = 1;
        } else {
            if (pthread_attr_setstacksize(&attr, 2u * 1024u * 1024u) != 0 ||
                pthread_create(&thread, &attr, installer_main, NULL) != 0) {
                thread_error = 1;
            }
            pthread_attr_destroy(&attr);
        }
        if (thread_error) {
            install_queue_count = 0;
            installer_worker_running = 0;
            pthread_mutex_unlock(&installer_lock);
            free(task);
            snprintf(error, error_size, "无法创建安装线程");
            return -1;
        }
        pthread_detach(thread);
    }
    ng_source_acquire(source);
    pthread_mutex_unlock(&installer_lock);
    return 0;
}

int
ng_installer_queue_snapshot(NgInstallQueueItem *items, int max_items)
{
    int count;

    if (!items || max_items <= 0) return 0;
    pthread_mutex_lock(&installer_lock);
    count = install_queue_count < max_items ? install_queue_count : max_items;
    for (int i = 0; i < count; i++) {
        InstallTask *task = install_queue[i];
        ng_strlcpy(items[i].game_id, task->game.id, sizeof(items[i].game_id));
        ng_strlcpy(items[i].title, task->game.title_name, sizeof(items[i].title));
        ng_strlcpy(items[i].target,
                   task->target == NG_INSTALL_SYSTEM ? "system" :
                   task->target == NG_INSTALL_HOMEBREW ? "homebrew" : "library",
                   sizeof(items[i].target));
        items[i].size = task->game.size;
    }
    pthread_mutex_unlock(&installer_lock);
    return count;
}

int
ng_installer_cancel(const char *game_id, char *message, size_t message_size)
{
    InstallTask *queued = NULL;
    int index = -1;

    pthread_mutex_lock(&installer_lock);
    if (active_task &&
        (!game_id || !game_id[0] || !strcmp(active_task->game.id, game_id))) {
        if (active_task->committing) {
            pthread_mutex_unlock(&installer_lock);
            snprintf(message, message_size, "安装正在提交，当前操作即将结束");
            return 2;
        }
        int system_handoff = active_task->system_handoff;
        active_task->cancel_requested = 1;
        install_set_cancel_pending(1);
        pthread_mutex_unlock(&installer_lock);
        snprintf(message, message_size, system_handoff
                 ? "正在取消系统安装任务"
                 : "正在取消当前安装任务");
        return 0;
    }
    if (game_id && game_id[0]) {
        for (int i = 0; i < install_queue_count; i++) {
            if (!strcmp(install_queue[i]->game.id, game_id)) {
                queued = install_queue[i];
                index = i;
                break;
            }
        }
    }
    if (queued) {
        install_queue_count--;
        if (index < install_queue_count) {
            memmove(install_queue + index, install_queue + index + 1,
                    (size_t)(install_queue_count - index) * sizeof(install_queue[0]));
        }
    }
    pthread_mutex_unlock(&installer_lock);
    if (queued) {
        ng_source_release(queued->source);
        free(queued);
        snprintf(message, message_size, "等待队列中的安装任务已取消");
        return 1;
    }
    snprintf(message, message_size, "没有可取消的安装任务");
    return -1;
}

int
ng_installer_is_busy(void)
{
    int busy;
    pthread_mutex_lock(&installer_lock);
    busy = installer_worker_running || install_queue_count > 0;
    pthread_mutex_unlock(&installer_lock);
    return busy;
}

int
ng_installer_prepare_stop(void)
{
    int busy;

    pthread_mutex_lock(&installer_lock);
    busy = active_task || install_queue_count > 0;
    if (!busy) {
        installer_stopping = 1;
        while (installer_worker_running) pthread_cond_wait(&installer_ready, &installer_lock);
    }
    pthread_mutex_unlock(&installer_lock);
    return busy ? -1 : 0;
}

int
ng_installer_is_stopping(void)
{
    int stopping;

    pthread_mutex_lock(&installer_lock);
    stopping = installer_stopping;
    pthread_mutex_unlock(&installer_lock);
    return stopping;
}

void
ng_installer_remount_imported(void)
{
#ifdef __PROSPERO__
    NgConfig cfg;
    char library[NG_PATH_LEN];
    DIR *dir;
    struct dirent *entry;

    ng_app_config_snapshot(&cfg);
    snprintf(library, sizeof(library), "%s/library", cfg.data_dir);
    dir = opendir(library);
    if (!dir) return;
    while ((entry = readdir(dir)) != NULL) {
        char src[NG_PATH_LEN];
        char check[NG_PATH_LEN];
        char error[NG_ERROR_LEN];
        if (entry->d_name[0] == '.' || !valid_tid(entry->d_name)) continue;
        snprintf(src, sizeof(src), "%s/%s", library, entry->d_name);
        snprintf(check, sizeof(check), "%s/eboot.bin", src);
        if (access(check, R_OK) != 0) continue;
        if (mount_folder(entry->d_name, src, error, sizeof(error)) != 0) {
            ng_log_error("重挂载 %s 失败: %s", entry->d_name, error);
        } else {
            ng_log_info("已重挂载导入游戏 %s", entry->d_name);
        }
    }
    closedir(dir);
#endif
}
