#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __PROSPERO__
#include <pthread_np.h>
#include <ps5/klog.h>
#endif

#include "app.h"
#include "http_server.h"
#include "installer.h"
#include "launcher.h"
#include "log.h"

#ifdef __PROSPERO__
int sceSystemServiceLaunchWebBrowser(const char *uri, void *reserved);
int sceUserServiceInitialize(void *reserved);
int sceUserServiceTerminate(void);

typedef struct {
    char padding[45];
    char message[3075];
} NgNotification;

int sceKernelSendNotificationRequest(int, NgNotification *, size_t, int);
int sceKernelSetProcessName(const char *name);
#endif

static volatile sig_atomic_t g_stop;
static int instance_lock_fd = -1;
#ifdef __PROSPERO__
static int user_service_ready;
#endif

static int
ensure_user_service(void)
{
#ifdef __PROSPERO__
    int rc;

    if (user_service_ready) return 0;
    rc = sceUserServiceInitialize(NULL);
    if (rc == 0) user_service_ready = 1;
    return rc;
#else
    return 0;
#endif
}

static void
set_process_name(void)
{
#ifdef __PROSPERO__
    int rc = sceKernelSetProcessName("nasgames.elf");
    if (rc != 0) klog_printf("nasgames.elf: sceKernelSetProcessName -> %d\n", rc);
    syscall(SYS_thr_set_name, -1, "nasgames.elf");
#endif
}

static int
acquire_instance_lock(const char *data_dir)
{
    char path[NG_PATH_LEN];
    char pid[32];
    int length;

    if (!data_dir || !*data_dir) return -1;
    ng_mkdir_p(data_dir, 0777);
    if (snprintf(path, sizeof(path), "%s/nasgames.lock", data_dir) >= (int)sizeof(path)) {
        return -1;
    }
    instance_lock_fd = open(path, O_RDWR | O_CREAT, 0666);
    if (instance_lock_fd < 0) return -1;
    if (flock(instance_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        close(instance_lock_fd);
        instance_lock_fd = -1;
        return 1;
    }
    if (ftruncate(instance_lock_fd, 0) != 0) {
        close(instance_lock_fd);
        instance_lock_fd = -1;
        return -1;
    }
    length = snprintf(pid, sizeof(pid), "%ld\n", (long)getpid());
    if (length > 0) {
        ssize_t written = write(instance_lock_fd, pid, (size_t)length);
        (void)written;
    }
    return 0;
}

static void
release_instance_lock(void)
{
    if (instance_lock_fd < 0) return;
    flock(instance_lock_fd, LOCK_UN);
    close(instance_lock_fd);
    instance_lock_fd = -1;
}

static void
startup_log(const char *message)
{
#ifdef __PROSPERO__
    klog_printf("nasgames.elf: %s\n", message);
#else
    (void)message;
#endif
}

static void
handle_signal(int signal_number)
{
    (void)signal_number;
    g_stop = 1;
}

static void
notify(const char *message)
{
#ifdef __PROSPERO__
    NgNotification request;

    memset(&request, 0, sizeof(request));
    snprintf(request.message, sizeof(request.message), "%s", message);
    sceKernelSendNotificationRequest(0, &request, sizeof(request), 0);
#else
    (void)message;
#endif
}

static void
open_browser(const NgConfig *config)
{
#ifdef __PROSPERO__
    char uri[128];

    if (!config->open_browser_on_start) return;
    snprintf(uri, sizeof(uri), "http://127.0.0.1:%d", config->port);
    sleep(1);
    if (ensure_user_service() != 0) {
        ng_log_error("无法初始化用户服务，跳过自动打开浏览器");
        return;
    }
    if (sceSystemServiceLaunchWebBrowser(uri, NULL) != 0) {
        ng_log_error("无法自动打开浏览器: %s", uri);
    } else {
        ng_log_info("已在浏览器中打开 %s", uri);
    }
#else
    (void)config;
#endif
}

int
main(int argc, char **argv)
{
    NgConfig config;
    const char *data_dir = "/data/nasgames";
    struct sigaction action;
    int rc;

#ifndef __PROSPERO__
    if (argc > 1 && argv[1] && argv[1][0]) data_dir = argv[1];
#else
    (void)argc;
    (void)argv;
#endif

    set_process_name();
    startup_log("main entered");

    {
        int lock_result = acquire_instance_lock(data_dir);
        if (lock_result == 1) {
            startup_log("another instance is already running");
            notify("NAS Games 已在运行，本次启动已取消");
            return EXIT_FAILURE;
        }
        if (lock_result != 0) {
            startup_log("failed to acquire the single-instance lock");
            notify("NAS Games: 无法创建单实例锁");
            return EXIT_FAILURE;
        }
    }

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
#ifdef SIGHUP
    sigaction(SIGHUP, &action, NULL);
#endif

    startup_log("initializing application");
    if (ng_app_init(data_dir) != 0) {
        startup_log("application initialization failed");
        notify("NAS Games: 初始化失败");
        release_instance_lock();
        return EXIT_FAILURE;
    }
    startup_log("application initialized");
    startup_log("restoring imported folders");
    ng_installer_remount_imported();
    ng_installer_repair_homebrew_permissions();
    startup_log("imported folders restored");
    ng_app_config_snapshot(&config);
    startup_log("starting HTTP server");
    rc = ng_http_start(config.port);
    if (rc != 0) {
        startup_log("HTTP server start failed");
        ng_log_error("无法启动 HTTP 服务，端口 %d：%s", config.port, strerror(errno));
        notify("NAS Games: HTTP 端口启动失败");
        ng_app_stop();
        ng_log_close();
        release_instance_lock();
        return EXIT_FAILURE;
    }
    startup_log("HTTP server started");
    if (ensure_user_service() != 0) {
        ng_log_error("无法初始化用户服务，媒体入口可能无法登记");
    }
    {
        char launcher_error[NG_ERROR_LEN];
        if (ng_launcher_ensure(config.port, launcher_error,
                               sizeof(launcher_error)) != 0) {
            ng_log_error("媒体入口安装失败: %s", launcher_error);
            notify("NAS Games 媒体入口安装失败，主服务仍可使用");
        }
    }
    if (ng_app_start_scanner() != 0) {
        ng_log_error("无法启动后台扫描线程");
    }
    startup_log("scanner started");

    {
        char message[128];
        snprintf(message, sizeof(message),
                 "NAS Games 已启动，可在浏览器打开 http://127.0.0.1:%d",
                 config.port);
        notify(message);
    }
    if (config.open_browser_on_start) {
        startup_log("opening browser");
        open_browser(&config);
    }
    startup_log("main loop entered");

    while (!ng_installer_is_stopping()) {
        if (g_stop && ng_installer_prepare_stop() == 0) break;
        sleep(1);
    }

    ng_log_info("正在停止 NAS Games");
    ng_http_stop();
    ng_app_stop();
    ng_log_close();
    release_instance_lock();
#ifdef __PROSPERO__
    if (user_service_ready) sceUserServiceTerminate();
#endif
    return EXIT_SUCCESS;
}
