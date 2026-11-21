#ifndef NASGAMES_LAUNCHER_H
#define NASGAMES_LAUNCHER_H

#include <stddef.h>

#define NG_LAUNCHER_TITLE_ID "NASG00001"
#define NG_LAUNCHER_CATEGORY 65536

int ng_launcher_ensure(int port, char *error, size_t error_size);

#endif
