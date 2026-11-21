#ifndef NASGAMES_INSTALLER_H
#define NASGAMES_INSTALLER_H

#include <stddef.h>

#include "common.h"
#include "source.h"

#define NG_INSTALL_QUEUE_MAX 8

typedef struct {
    char game_id[24];
    char title[NG_TITLE_LEN];
    char target[16];
    uint64_t size;
} NgInstallQueueItem;

int ng_installer_start(const NgGame *game, NgSource *source, NgInstallTarget target,
                       char *error, size_t error_size);
int ng_installer_stream_snapshot(const char *id, NgGame *game, NgSource **source);
int ng_installer_is_busy(void);
int ng_installer_prepare_stop(void);
int ng_installer_is_stopping(void);
int ng_installer_queue_snapshot(NgInstallQueueItem *items, int max_items);
int ng_installer_cancel(const char *game_id, char *message, size_t message_size);
void ng_installer_remount_imported(void);
int ng_installer_register_title(const char *title_id, const char *title_root,
                                char *error, size_t error_size);

#endif
