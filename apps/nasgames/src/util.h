#ifndef NASGAMES_UTIL_H
#define NASGAMES_UTIL_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
    int failed;
} NgStrBuf;

void ng_strbuf_init(NgStrBuf *sb);
void ng_strbuf_free(NgStrBuf *sb);
int ng_strbuf_reserve(NgStrBuf *sb, size_t extra);
int ng_strbuf_append(NgStrBuf *sb, const char *text);
int ng_strbuf_appendn(NgStrBuf *sb, const char *text, size_t len);
int ng_strbuf_printf(NgStrBuf *sb, const char *fmt, ...);
int ng_strbuf_json_string(NgStrBuf *sb, const char *text);

void ng_strlcpy(char *dst, const char *src, size_t size);
void ng_strlcat(char *dst, const char *src, size_t size);
void ng_trim(char *text);

int ng_mkdir_p(const char *path, mode_t mode);
int ng_read_text_file(const char *path, size_t max_bytes, char **out);
int ng_write_file_atomic(const char *path, const void *data, size_t size, mode_t mode);
int ng_copy_file_local(const char *src, const char *dst, mode_t mode);
int ng_ensure_parent_dir(const char *path, mode_t mode);

uint64_t ng_hash64(const char *text);
void ng_make_game_id(char *out, size_t size, const char *source_id, const char *path);
int ng_path_normalize(const char *input, char *out, size_t out_size);
int ng_path_join_local(char *out, size_t out_size, const char *root,
                       const char *remote_path);
const char *ng_path_basename(const char *path);
const char *ng_path_extension(const char *path);
int ng_path_has_prefix(const char *path, const char *prefix);
int ng_extract_title_id(const char *text, char *out, size_t out_size);
int ng_valid_title_id(const char *title_id);
int ng_valid_content_id(const char *content_id);
int ng_has_suffix_ci(const char *text, const char *suffix);
int ng_version_compare(const char *left, const char *right);

char *ng_url_decode(const char *text);
int ng_url_encode(NgStrBuf *out, const char *text);
int ng_parse_int(const char *text, int fallback);
uint64_t ng_now_epoch(void);
uint64_t ng_now_monotonic_ms(void);

const char *ng_source_type_name(int type);
int ng_source_type_parse(const char *name);
const char *ng_game_type_name(int type);
const char *ng_packing_name(int packing);
const char *ng_pkg_kind_name(int kind);

#endif
