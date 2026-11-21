#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef __PROSPERO__
#include <sys/sysctl.h>
#include <sys/user.h>
#endif

#define LOCK_PATH "/data/nasgames/nasgames.lock"

static const char *test_paths[] = {
    "/data/nasgames/test-plain",
    "/data/nasgames/probe.txt",
    "/data/nasgames/library/FAKE02932",
    "/data/homebrew/FAKE02932",
};

static int
remove_tree(const char *path, char *error, size_t error_size)
{
    struct stat info;

    if (lstat(path, &info) != 0) {
        if (errno == ENOENT) return 0;
        snprintf(error, error_size, "无法读取 %s: %s", path, strerror(errno));
        return -1;
    }
    if (S_ISDIR(info.st_mode)) {
        DIR *dir = opendir(path);
        struct dirent *entry;

        if (!dir) {
            snprintf(error, error_size, "无法打开 %s: %s", path, strerror(errno));
            return -1;
        }
        while ((entry = readdir(dir)) != NULL) {
            char child[1024];

            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
            if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >=
                (int)sizeof(child)) {
                snprintf(error, error_size, "路径过长: %s/%s", path, entry->d_name);
                closedir(dir);
                return -1;
            }
            if (remove_tree(child, error, error_size) != 0) {
                closedir(dir);
                return -1;
            }
        }
        closedir(dir);
        if (rmdir(path) != 0) {
            snprintf(error, error_size, "无法删除 %s: %s", path, strerror(errno));
            return -1;
        }
        return 0;
    }
    if (unlink(path) != 0) {
        snprintf(error, error_size, "无法删除 %s: %s", path, strerror(errno));
        return -1;
    }
    return 0;
}

static int
read_process_name(long pid, char *name, size_t name_size)
{
    char path[64];
    FILE *file;

    snprintf(path, sizeof(path), "/proc/%ld/comm", pid);
    file = fopen(path, "r");
    if (!file) return -1;
    if (!fgets(name, (int)name_size, file)) {
        fclose(file);
        return -1;
    }
    fclose(file);
    name[strcspn(name, "\r\n")] = '\0';
    return name[0] ? 0 : -1;
}

static int
read_cmdline(long pid, char *buffer, size_t buffer_size)
{
    char path[64];
    FILE *file;
    size_t length;

    snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid);
    file = fopen(path, "rb");
    if (!file) return -1;
    length = fread(buffer, 1, buffer_size - 1, file);
    fclose(file);
    buffer[length] = '\0';
    for (size_t i = 0; i < length; i++) {
        if (buffer[i] == '\0') buffer[i] = ' ';
    }
    return length ? 0 : -1;
}

static int
is_nasgames_process(long pid)
{
    char name[128];
    char command[1024];

#ifdef __PROSPERO__
    int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)pid};
    struct kinfo_proc process;
    size_t size = sizeof(process);
    memset(&process, 0, sizeof(process));
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &process, &size, NULL, 0) == 0 &&
        size > offsetof(struct kinfo_proc, ki_comm) && process.ki_pid == pid) {
        return !strcmp(process.ki_comm, "nasgames.elf") || !strcmp(process.ki_comm, "NASGames");
    }
#endif

    if (read_process_name(pid, name, sizeof(name)) == 0) {
        return !strcmp(name, "nasgames.elf") || !strcmp(name, "NASGames");
    }
    if (read_cmdline(pid, command, sizeof(command)) == 0) {
        command[strcspn(command, " ")] = '\0';
        const char *base = strrchr(command, '/');
        return !strcmp(base ? base + 1 : command, "nasgames.elf");
    }
    return 0;
}

static int
stop_previous_instance(int *lock_fd)
{
    FILE *file;
    long pid = 0;
    struct timespec delay = {0, 200 * 1000 * 1000};
    int descriptor = open(LOCK_PATH, O_RDWR);

    *lock_fd = -1;
    if (descriptor < 0) {
        if (errno == ENOENT) return 0;
        fprintf(stderr, "Cannot read %s: %s\n", LOCK_PATH, strerror(errno));
        return -1;
    }
    *lock_fd = descriptor;
    if (flock(descriptor, LOCK_EX | LOCK_NB) == 0) return 0;
    if (errno != EWOULDBLOCK) return -1;
    int snapshot = dup(descriptor);
    file = snapshot >= 0 ? fdopen(snapshot, "r") : NULL;
    if (!file) {
        if (snapshot >= 0) close(snapshot);
        return -1;
    }
    if (fscanf(file, "%ld", &pid) != 1 || pid <= 1 || pid > INT_MAX) {
        fclose(file);
        fprintf(stderr, "Invalid Game Center lock file\n");
        return -1;
    }
    fclose(file);

    if (!is_nasgames_process(pid)) {
        fprintf(stderr, "Refusing to stop unverified process %ld from %s\n", pid, LOCK_PATH);
        return -1;
    }
    if (kill((pid_t)pid, SIGTERM) != 0) {
        fprintf(stderr, "Cannot stop Game Center process %ld: %s\n",
                pid, strerror(errno));
        return -1;
    }
    for (int attempt = 0; attempt < 50; attempt++) {
        if (flock(descriptor, LOCK_EX | LOCK_NB) == 0) return 0;
        nanosleep(&delay, NULL);
    }
    fprintf(stderr, "Game Center process %ld did not stop in time\n", pid);
    return -1;
}

int
main(void)
{
    char error[512];
    int lock_fd;

    if (stop_previous_instance(&lock_fd) != 0) return EXIT_FAILURE;
    for (size_t i = 0; i < sizeof(test_paths) / sizeof(test_paths[0]); i++) {
        if (remove_tree(test_paths[i], error, sizeof(error)) != 0) {
            fprintf(stderr, "%s\n", error);
            return EXIT_FAILURE;
        }
        printf("Cleaned %s\n", test_paths[i]);
    }
    sync();
    if (lock_fd >= 0) close(lock_fd);
    return EXIT_SUCCESS;
}
