#include "title_launcher.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "util.h"

#ifdef __PROSPERO__
#include <pthread.h>

typedef struct {
    uint32_t structsize;
    uint32_t user_id;
    uint32_t app_opt;
    uint64_t crash_report;
    uint32_t check_flag;
} NgLaunchContext;

int sceUserServiceGetForegroundUser(uint32_t *user_id);
int sceSystemServiceLaunchApp(const char *title_id, char **argv,
                              NgLaunchContext *context);
int sceSystemServiceGetAppIdOfRunningBigApp(void);
int sceSystemServiceKillApp(int app_id, int how, int reason, int core_dump);

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
