#ifndef NASGAMES_TITLE_LAUNCHER_H
#define NASGAMES_TITLE_LAUNCHER_H

#include <stddef.h>

typedef enum {
    NG_TITLE_LAUNCH_ACCEPTED = 0,
    NG_TITLE_LAUNCH_BUSY,
    NG_TITLE_LAUNCH_ERROR,
} NgTitleLaunchResult;

NgTitleLaunchResult ng_title_launcher_launch(const char *title_id,
                                             char *error, size_t error_size);

#endif
