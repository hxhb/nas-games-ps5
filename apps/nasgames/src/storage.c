#include "storage.h"

#include <string.h>
#include <sys/statvfs.h>

#include "common.h"
#include "util.h"

int
ng_storage_probe(const char *path, NgStorageInfo *out)
{
    char candidate[NG_PATH_LEN];
    struct statvfs info;

    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    if (!path || !path[0]) return -1;
    ng_strlcpy(candidate, path, sizeof(candidate));

    for (int attempt = 0; attempt < 8 && candidate[0]; attempt++) {
        unsigned long block_size;
        uint64_t available;
        uint64_t total;

        if (statvfs(candidate, &info) == 0) {
            block_size = info.f_frsize ? info.f_frsize : info.f_bsize;
            if (block_size && info.f_bavail <= UINT64_MAX / block_size &&
                info.f_blocks <= UINT64_MAX / block_size) {
                available = (uint64_t)info.f_bavail * block_size;
                total = (uint64_t)info.f_blocks * block_size;
                if (total) {
                    out->known = 1;
                    out->free_bytes = available;
                    out->total_bytes = total;
                    return 0;
                }
            }
        }

        char *slash = strrchr(candidate, '/');
        if (!slash) break;
        if (slash == candidate) {
            candidate[0] = '/';
            candidate[1] = '\0';
        } else {
            *slash = '\0';
        }
    }
    return -1;
}
