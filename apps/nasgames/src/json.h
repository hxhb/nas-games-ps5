#ifndef NASGAMES_JSON_H
#define NASGAMES_JSON_H

#include <stddef.h>

#define NG_JSON_MAX_TOKENS 4096

typedef enum {
    NG_JSON_UNDEFINED = 0,
    NG_JSON_OBJECT,
    NG_JSON_ARRAY,
    NG_JSON_STRING,
    NG_JSON_PRIMITIVE,
} NgJsonType;

typedef struct {
    NgJsonType type;
    int start;
    int end;
    int parent;
    int size;
    int is_key;
} NgJsonTok;

typedef struct {
    char *data;
    size_t len;
    NgJsonTok tokens[NG_JSON_MAX_TOKENS];
    int count;
} NgJsonDoc;

int ng_json_parse(NgJsonDoc *doc, const char *text, size_t len);
void ng_json_free(NgJsonDoc *doc);

int ng_json_obj_get(const NgJsonDoc *doc, int obj, const char *key);
int ng_json_array_at(const NgJsonDoc *doc, int array, int index);
int ng_json_string(const NgJsonDoc *doc, int token, char *out, size_t out_size);
int ng_json_int(const NgJsonDoc *doc, int token, int fallback);
int ng_json_bool(const NgJsonDoc *doc, int token, int fallback);
int ng_json_obj_string(const NgJsonDoc *doc, int obj, const char *key,
                       char *out, size_t out_size, const char *fallback);
int ng_json_obj_int(const NgJsonDoc *doc, int obj, const char *key, int fallback);
int ng_json_obj_bool(const NgJsonDoc *doc, int obj, const char *key, int fallback);

#endif
