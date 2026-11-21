#include "keep_awake.h"

#include <pthread.h>
#include <unistd.h>

#include "log.h"

#ifdef __PROSPERO__
extern int sceSystemServicePowerTick(void) __attribute__((weak));
#endif

static pthread_t keep_awake_thread;
static volatile int keep_awake_stop;
static int keep_awake_running;
static int keep_awake_supported = -1;

static int
power_tick(void)
{
#ifdef __PROSPERO__
    int (*volatile tick)(void) = sceSystemServicePowerTick;
    return tick ? tick() : -1;
#else
    return -1;
#endif
}

static void *
keep_awake_main(void *unused)
{
    (void)unused;
    if (keep_awake_supported < 0) {
        keep_awake_supported = power_tick() == 0;
        if (!keep_awake_supported) {
            ng_log_info("当前系统不支持安装期间保持唤醒，已跳过");
        }
    }
    while (!keep_awake_stop) {
        if (keep_awake_supported) power_tick();
        for (int i = 0; i < 20 && !keep_awake_stop; i++) sleep(1);
    }
    return NULL;
}

int
ng_keep_awake_start(void)
{
    if (keep_awake_running) return 0;
    keep_awake_stop = 0;
    if (pthread_create(&keep_awake_thread, NULL, keep_awake_main, NULL) != 0) {
        ng_log_error("无法启动安装保活线程");
        return -1;
    }
    keep_awake_running = 1;
    return 0;
}

void
ng_keep_awake_stop(void)
{
    if (!keep_awake_running) return;
    keep_awake_stop = 1;
    pthread_join(keep_awake_thread, NULL);
    keep_awake_running = 0;
}
