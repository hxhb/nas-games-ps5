#ifndef NASGAMES_HTTP_SERVER_H
#define NASGAMES_HTTP_SERVER_H

int ng_http_start(int port);
void ng_http_stop(void);
void ng_http_stream_cache_clear(void);

#endif
