#include "title_launcher.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "log.h"
#include "util.h"

#ifdef __PROSPERO__
#include <pthread.h>
#include <sys/sysctl.h>
#include <sys/user.h>

typedef struct {
    uint32_t structsize;
    uint32_t user_id;
    uint32_t app_opt;
    uint64_t crash_report;
    uint32_t check_flag;
} NgLaunchContext;

typedef struct {
    uint32_t app_id;
    uint64_t unknown1;
    char title_id[14];
    char unknown2[0x3c];
} NgAppInfo;

int sceUserServiceGetForegroundUser(uint32_t *user_id);
int sceSystemServiceLaunchApp(const char *title_id, char **argv,
                              NgLaunchContext *context);
int sceSystemServiceGetAppIdOfRunningBigApp(void);
int sceSystemServiceKillApp(int app_id, int how, int reason, int core_dump);
int sceKernelGetAppInfo(pid_t pid, NgAppInfo *info);
int sceLncUtilKillApp(uint32_t app_id);

typedef struct {
    char padding[45];
    char message[3075];
} NgLaunchNotification;

int sceKernelSendNotificationRequest(int, NgLaunchNotification *, size_t, int);

static pthread_mutex_t launch_lock = PTHREAD_MUTEX_INITIALIZER;
static int switch_pending;

#define NG_LNC_ALREADY_RUNNING_KILL_NEEDED 0x80940010u
#define NG_LNC_ALREADY_RUNNING 0x8094000Cu

static void
launch_notify(const char *message)
{
    NgLaunchNotification request;

    memset(&request, 0, sizeof(request));
    snprintf(request.message, sizeof(request.message), "%s", message);
    sceKernelSendNotificationRequest(0, &request, sizeof(request), 0);
}

static int
close_web_processes(char *error, size_t error_size)
{
#ifdef __PROSPERO__
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PROC, 0};
    pid_t pids[32];
    uint32_t app_ids[32];
    int count = 0;
    int app_count = 0;
    int closed = 0;
    size_t size = 0;
    void *buffer = NULL;

    for (int attempt = 0; attempt < 3; attempt++) {
        if (sysctl(mib, 4, NULL, &size, NULL, 0) != 0 || size == 0 ||
            size > 16u * 1024u * 1024u) {
            snprintf(error, error_size, "无法读取进程列表: %s", strerror(errno));
            return -1;
        }
        buffer = malloc(size);
        if (!buffer) break;
        size_t used = size;
        if (sysctl(mib, 4, buffer, &used, NULL, 0) == 0) {
            size = used;
            break;
        }
        int saved_errno = errno;
        free(buffer);
        buffer = NULL;
        if (saved_errno != ENOMEM) {
            snprintf(error, error_size, "无法读取进程列表: %s",
                     strerror(saved_errno));
            return -1;
        }
    }
    if (!buffer) {
        snprintf(error, error_size, "读取进程列表失败");
        return -1;
    }
    for (char *cursor = buffer; cursor < (char *)buffer + size;) {
        struct kinfo_proc *process = (struct kinfo_proc *)cursor;
        int struct_size = process->ki_structsize;

        if (struct_size <= 0 ||
            cursor + struct_size > (char *)buffer + size) {
            break;
        }
        if (strcmp(process->ki_comm, "SceNKWebProcess") == 0 &&
            count < (int)(sizeof(pids) / sizeof(pids[0]))) {
            NgAppInfo info;
            memset(&info, 0, sizeof(info));
            if (sceKernelGetAppInfo(process->ki_pid, &info) == 0) {
                info.title_id[sizeof(info.title_id) - 1] = '\0';
                ng_log_info("网页进程 pid=%ld app_id=%u title=%s",
                            (long)process->ki_pid, info.app_id,
                            info.title_id);
                int known = 0;
                for (int i = 0; i < app_count; i++) {
                    if (app_ids[i] == info.app_id) {
                        known = 1;
                        break;
                    }
                }
                if (!known && info.app_id > 1 &&
                    app_count < (int)(sizeof(app_ids) / sizeof(app_ids[0]))) {
                    app_ids[app_count++] = info.app_id;
                }
            }
            pids[count++] = process->ki_pid;
        }
        cursor += struct_size;
    }
    free(buffer);

    for (int i = 0; i < app_count; i++) {
        if (sceLncUtilKillApp(app_ids[i]) == 0) {
            ng_log_info("已关闭网页应用 app_id=%u", app_ids[i]);
            closed++;
        } else {
            ng_log_error("无法关闭网页应用 app_id=%u，尝试结束渲染进程",
                         app_ids[i]);
        }
    }
    for (int i = 0; closed == 0 && i < count; i++) {
        if (pids[i] <= 1) continue;
        ng_log_info("关闭 Game Center 网页进程 pid=%ld", (long)pids[i]);
        if (kill(pids[i], SIGTERM) == 0) {
            closed++;
        } else if (errno != ESRCH) {
            ng_log_error("无法关闭网页进程 pid=%ld: %s",
                         (long)pids[i], strerror(errno));
        }
    }
    if (closed > 0) {
        usleep(180000);
        for (int i = 0; i < count; i++) {
            if (pids[i] > 1 && kill(pids[i], 0) == 0) {
                ng_log_info("强制结束残留网页进程 pid=%ld", (long)pids[i]);
                kill(pids[i], SIGKILL);
            }
        }
    }
    if (error && error_size) {
        snprintf(error, error_size, "已关闭 %d 个网页进程", closed);
    }
    return closed;
#else
    (void)error;
    (void)error_size;
    return 1;
#endif
}

static int
launch_title(const char *title_id)
{
    NgLaunchContext context;
    char *argv[] = {NULL};
    int rc;

    memset(&context, 0, sizeof(context));
    context.structsize = sizeof(context);
    rc = sceUserServiceGetForegroundUser(&context.user_id);
    if (rc != 0) return rc;
    return sceSystemServiceLaunchApp(title_id, argv, &context);
}

static int
launch_after_foreground_exit(const char *title_id, int rc)
{
    for (int attempt = 0;
         attempt < 2 && (uint32_t)rc == NG_LNC_ALREADY_RUNNING_KILL_NEEDED;
         attempt++) {
        int app_id = sceSystemServiceGetAppIdOfRunningBigApp();
        if (app_id <= 0) {
            usleep(200000);
            rc = launch_title(title_id);
            continue;
        }
        int kill_rc = sceSystemServiceKillApp(app_id, -1, 0, 0);
        ng_log_info("启动 %s 前关闭前台应用 id=%d rc=0x%08X",
                    title_id, app_id, (unsigned)kill_rc);
        for (int i = 0; i < 50; i++) {
            int current = sceSystemServiceGetAppIdOfRunningBigApp();
            if (current <= 0 || current != app_id) break;
            usleep(100000);
        }
        rc = launch_title(title_id);
    }
    return rc;
}

static void *
switch_and_launch(void *argument)
{
    char *title_id = argument;
    int rc;

    usleep(300000);
    rc = launch_title(title_id);
    rc = launch_after_foreground_exit(title_id, rc);
    if (rc >= 0) {
        ng_log_info("启动完成：%s，应用 ID %d", title_id, rc);
    } else if ((uint32_t)rc == NG_LNC_ALREADY_RUNNING) {
        ng_log_info("启动完成：%s 已在前台运行", title_id);
    } else {
        char notice[96];
        ng_log_error("启动失败：%s rc=0x%08X",
                     title_id, (unsigned)rc);
        snprintf(notice, sizeof(notice),
                 "Game Center 无法启动 %s（0x%08X）", title_id, (unsigned)rc);
        launch_notify(notice);
    }
    if (rc >= 0 || (uint32_t)rc == NG_LNC_ALREADY_RUNNING) {
        char close_error[NG_ERROR_LEN];
        usleep(250000);
        if (close_web_processes(close_error, sizeof(close_error)) > 0) {
            ng_log_info("启动后已关闭 Game Center 网页窗口: %s", close_error);
        }
    }

    free(title_id);
    pthread_mutex_lock(&launch_lock);
    switch_pending = 0;
    pthread_mutex_unlock(&launch_lock);
    return NULL;
}
#endif

NgTitleLaunchResult
ng_title_launcher_launch(const char *title_id, char *error, size_t error_size)
{
    if (!ng_valid_title_id(title_id)) {
        snprintf(error, error_size, "Title ID 无效，无法启动");
        return NG_TITLE_LAUNCH_ERROR;
    }

#ifdef __PROSPERO__
    char *worker_title;
    pthread_t thread;

    if (pthread_mutex_trylock(&launch_lock) != 0) {
        snprintf(error, error_size, "已有启动请求正在处理");
        return NG_TITLE_LAUNCH_BUSY;
    }
    if (switch_pending) {
        pthread_mutex_unlock(&launch_lock);
        snprintf(error, error_size, "已有启动请求正在处理");
        return NG_TITLE_LAUNCH_BUSY;
    }

    worker_title = strdup(title_id);
    if (!worker_title) {
        pthread_mutex_unlock(&launch_lock);
        snprintf(error, error_size, "内存不足，无法创建启动任务");
        return NG_TITLE_LAUNCH_ERROR;
    }

    switch_pending = 1;
    if (pthread_create(&thread, NULL, switch_and_launch, worker_title) != 0) {
        free(worker_title);
        switch_pending = 0;
        pthread_mutex_unlock(&launch_lock);
        snprintf(error, error_size, "无法创建启动任务");
        return NG_TITLE_LAUNCH_ERROR;
    }
    pthread_detach(thread);
    pthread_mutex_unlock(&launch_lock);
    ng_log_info("已接受启动请求: %s", title_id);
    if (error && error_size) error[0] = '\0';
    return NG_TITLE_LAUNCH_ACCEPTED;
#else
    (void)error;
    (void)error_size;
    snprintf(error, error_size, "仅 PS5 实机支持直接启动游戏");
    return NG_TITLE_LAUNCH_ERROR;
#endif
}

int
ng_title_launcher_close_web_ui(char *error, size_t error_size)
{
#ifdef __PROSPERO__
    int count = close_web_processes(error, error_size);
    if (count < 0) return -1;
    return count > 0 ? 0 : 1;
#else
    (void)error;
    (void)error_size;
    return 1;
#endif
}
