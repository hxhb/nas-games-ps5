#ifndef NASGAMES_LOG_H
#define NASGAMES_LOG_H

#include "util.h"

int ng_log_init(const char *path);
void ng_log_close(void);
void ng_log_info(const char *fmt, ...);
void ng_log_error(const char *fmt, ...);
int ng_log_recent_json(NgStrBuf *out);

#endif
