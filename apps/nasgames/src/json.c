#include "json.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hex4(const char *text, unsigned *value);

static void
skip_ws(const char *text, size_t len, size_t *pos)
{
    while (*pos < len && text[*pos] && strchr(" \t\r\n", text[*pos])) {
        (*pos)++;
    }
}

static int
parse_string(NgJsonDoc *doc, size_t *pos, int parent, int is_key)
{
    size_t i = *pos + 1;
    int token;

    if (doc->count >= NG_JSON_MAX_TOKENS) return -1;
    token = doc->count++;
    doc->tokens[token].type = NG_JSON_STRING;
    doc->tokens[token].start = (int)i;
    doc->tokens[token].parent = parent;
    doc->tokens[token].size = 0;
    doc->tokens[token].is_key = is_key;
    while (i < doc->len) {
        unsigned char c = (unsigned char)doc->data[i];
        if (c == '"') {
            doc->tokens[token].end = (int)i;
            *pos = i + 1;
            return token;
        }
        if (c == '\\') {
            if (++i >= doc->len || !doc->data[i]) return -1;
            if (doc->data[i] == 'u') {
                unsigned code;
                if (doc->len - i < 5 || hex4(doc->data + i + 1, &code) != 0) return -1;
                i += 5;
            } else {
                if (!strchr("\"\\/bfnrt", doc->data[i])) return -1;
                i++;
            }
            continue;
        }
        if (c < 0x20) return -1;
        i++;
    }
    return -1;
}

static int
parse_primitive(NgJsonDoc *doc, size_t *pos)
{
    const char *text = doc->data;
    size_t end = *pos;
    const char *literal = NULL;

    switch (text[end]) {
    case 't': literal = "true"; break;
    case 'f': literal = "false"; break;
    case 'n': literal = "null"; break;
    }
    if (literal) {
        size_t length = strlen(literal);
        if (doc->len - end < length || memcmp(text + end, literal, length)) return -1;
        *pos = end + length;
        return 0;
    }
    if (text[end] == '-') end++;
    if (!isdigit((unsigned char)text[end])) return -1;
    if (text[end] == '0') end++;
    else while (isdigit((unsigned char)text[end])) end++;
    if (text[end] == '.') {
        if (!isdigit((unsigned char)text[++end])) return -1;
        while (isdigit((unsigned char)text[end])) end++;
    }
    if (text[end] == 'e' || text[end] == 'E') {
        end++;
        if (text[end] == '-' || text[end] == '+') end++;
        if (!isdigit((unsigned char)text[end])) return -1;
        while (isdigit((unsigned char)text[end])) end++;
    }
    *pos = end;
    return 0;
}

static int
parse_value(NgJsonDoc *doc, size_t *pos, int parent, int depth)
{
    skip_ws(doc->data, doc->len, pos);
    if (*pos >= doc->len || depth >= 128 || doc->count >= NG_JSON_MAX_TOKENS) return -1;
    char first = doc->data[*pos];
    if (first == '"') return parse_string(doc, pos, parent, 0) >= 0 ? 0 : -1;

    int index = doc->count++;
    NgJsonTok *token = &doc->tokens[index];
    token->parent = parent;
    token->start = (int)*pos;
    if (first != '{' && first != '[') {
        token->type = NG_JSON_PRIMITIVE;
        if (parse_primitive(doc, pos) != 0) return -1;
        token->end = (int)*pos;
        return 0;
    }

    token->type = first == '{' ? NG_JSON_OBJECT : NG_JSON_ARRAY;
    char closing = first == '{' ? '}' : ']';
    (*pos)++;
    skip_ws(doc->data, doc->len, pos);
    if (doc->data[*pos] != closing) {
        for (;;) {
            if (first == '{') {
                if (doc->data[*pos] != '"' || parse_string(doc, pos, index, 1) < 0) return -1;
                skip_ws(doc->data, doc->len, pos);
                if (doc->data[*pos] != ':') return -1;
                (*pos)++;
            }
            if (parse_value(doc, pos, index, depth + 1) != 0) return -1;
            token->size++;
            skip_ws(doc->data, doc->len, pos);
            if (doc->data[*pos] == closing) break;
            if (doc->data[*pos] != ',') return -1;
            (*pos)++;
            skip_ws(doc->data, doc->len, pos);
        }
    }
    token->end = (int)(*pos)++;
    return 0;
}

int
ng_json_parse(NgJsonDoc *doc, const char *text, size_t len)
{
    size_t pos = 0;

    memset(doc, 0, sizeof(*doc));
    if (!text || len > INT_MAX) return -1;
    doc->data = malloc(len + 1);
    if (!doc->data) return -1;
    memcpy(doc->data, text, len);
    doc->data[len] = '\0';
    doc->len = len;
    if (parse_value(doc, &pos, -1, 0) == 0) {
        skip_ws(doc->data, doc->len, &pos);
        if (pos == doc->len) return 0;
    }
    ng_json_free(doc);
    return -1;
}

void
ng_json_free(NgJsonDoc *doc)
{
    free(doc->data);
    memset(doc, 0, sizeof(*doc));
}

static int
append_utf8(char *out, size_t out_size, size_t *used, unsigned value)
{
    if (value <= 0x7f) {
        if (*used + 1 >= out_size) return -1;
        out[(*used)++] = (char)value;
    } else if (value <= 0x7ff) {
        if (*used + 2 >= out_size) return -1;
        out[(*used)++] = (char)(0xc0 | (value >> 6));
        out[(*used)++] = (char)(0x80 | (value & 0x3f));
    } else if (value <= 0xffff) {
        if (*used + 3 >= out_size) return -1;
        out[(*used)++] = (char)(0xe0 | (value >> 12));
        out[(*used)++] = (char)(0x80 | ((value >> 6) & 0x3f));
        out[(*used)++] = (char)(0x80 | (value & 0x3f));
    } else {
        if (*used + 4 >= out_size) return -1;
        out[(*used)++] = (char)(0xf0 | (value >> 18));
        out[(*used)++] = (char)(0x80 | ((value >> 12) & 0x3f));
        out[(*used)++] = (char)(0x80 | ((value >> 6) & 0x3f));
        out[(*used)++] = (char)(0x80 | (value & 0x3f));
    }
    return 0;
}

static int
hex4(const char *p, unsigned *value)
{
    unsigned v = 0;
    for (int i = 0; i < 4; i++) {
        int c = (unsigned char)p[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
        else return -1;
    }
    *value = v;
    return 0;
}

static int
decode_string(const NgJsonDoc *doc, int token, char *out, size_t out_size)
{
    const char *p;
    const char *end;
    size_t used = 0;

    if (token < 0 || token >= doc->count || doc->tokens[token].type != NG_JSON_STRING) {
        return -1;
    }
    p = doc->data + doc->tokens[token].start;
    end = doc->data + doc->tokens[token].end;
    while (p < end) {
        unsigned char c = (unsigned char)*p++;
        if (c != '\\') {
            if (used + 1 >= out_size) return -1;
            out[used++] = (char)c;
            continue;
        }
        if (p >= end) return -1;
        c = (unsigned char)*p++;
        switch (c) {
        case '"': case '\\': case '/': break;
        case 'b': c = '\b'; break;
        case 'f': c = '\f'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case 'u': {
            unsigned cp;
            if (end - p < 4 || hex4(p, &cp) != 0) return -1;
            p += 4;
            if (cp >= 0xd800 && cp <= 0xdbff && end - p >= 6 &&
                p[0] == '\\' && p[1] == 'u') {
                unsigned low;
                if (hex4(p + 2, &low) == 0 && low >= 0xdc00 && low <= 0xdfff) {
                    cp = 0x10000 + ((cp - 0xd800) << 10) + (low - 0xdc00);
                    p += 6;
                }
            }
            if (cp == 0 || (cp >= 0xd800 && cp <= 0xdfff)) return -1;
            if (append_utf8(out, out_size, &used, cp) != 0) return -1;
            continue;
        }
        default:
            return -1;
        }
        if (used + 1 >= out_size) return -1;
        out[used++] = (char)c;
    }
    out[used] = '\0';
    return 0;
}

static int
token_equals(const NgJsonDoc *doc, int token, const char *text)
{
    char buf[256];
    if (!doc || token < 0 || token >= doc->count) return 0;
    if (doc->tokens[token].type == NG_JSON_STRING) {
        if (decode_string(doc, token, buf, sizeof(buf)) != 0) return 0;
        return strcmp(buf, text) == 0;
    }
    if (doc->tokens[token].type == NG_JSON_PRIMITIVE) {
        size_t len = (size_t)(doc->tokens[token].end - doc->tokens[token].start);
        return strlen(text) == len &&
               strncmp(doc->data + doc->tokens[token].start, text, len) == 0;
    }
    return 0;
}

int
ng_json_obj_get(const NgJsonDoc *doc, int obj, const char *key)
{
    int i;

    if (!doc || obj < 0 || obj >= doc->count || doc->tokens[obj].type != NG_JSON_OBJECT) {
        return -1;
    }
    for (i = obj + 1; i < doc->count; i++) {
        if (doc->tokens[i].start >= doc->tokens[obj].end) break;
        if (doc->tokens[i].parent != obj) continue;
        if (doc->tokens[i].type == NG_JSON_STRING && doc->tokens[i].is_key) {
            if (token_equals(doc, i, key) && i + 1 < doc->count &&
                doc->tokens[i + 1].parent == obj) {
                return i + 1;
            }
        }
    }
    return -1;
}

int
ng_json_array_at(const NgJsonDoc *doc, int array, int index)
{
    int seen = 0;
    if (!doc || array < 0 || array >= doc->count ||
        doc->tokens[array].type != NG_JSON_ARRAY || index < 0) {
        return -1;
    }
    for (int i = array + 1; i < doc->count; i++) {
        if (doc->tokens[i].start >= doc->tokens[array].end) break;
        if (doc->tokens[i].parent != array) continue;
        if (seen++ == index) return i;
    }
    return -1;
}

int
ng_json_string(const NgJsonDoc *doc, int token, char *out, size_t out_size)
{
    if (!out || out_size == 0) return -1;
    if (token < 0) {
        out[0] = '\0';
        return -1;
    }
    return decode_string(doc, token, out, out_size);
}

int
ng_json_int(const NgJsonDoc *doc, int token, int fallback)
{
    char buf[64];
    char *end = NULL;
    long value;

    if (!doc || token < 0 || token >= doc->count ||
        doc->tokens[token].type != NG_JSON_PRIMITIVE) {
        return fallback;
    }
    if ((size_t)(doc->tokens[token].end - doc->tokens[token].start) >= sizeof(buf)) {
        return fallback;
    }
    memcpy(buf, doc->data + doc->tokens[token].start,
           (size_t)(doc->tokens[token].end - doc->tokens[token].start));
    buf[doc->tokens[token].end - doc->tokens[token].start] = '\0';
    errno = 0;
    value = strtol(buf, &end, 10);
    if (errno || !end || *end || value < INT32_MIN || value > INT32_MAX) {
        return fallback;
    }
    return (int)value;
}

int
ng_json_bool(const NgJsonDoc *doc, int token, int fallback)
{
    if (!doc || token < 0 || token >= doc->count) return fallback;
    if (token_equals(doc, token, "true")) return 1;
    if (token_equals(doc, token, "false")) return 0;
    return ng_json_int(doc, token, fallback);
}

int
ng_json_obj_string(const NgJsonDoc *doc, int obj, const char *key,
                   char *out, size_t out_size, const char *fallback)
{
    int token = ng_json_obj_get(doc, obj, key);
    if (token >= 0 && ng_json_string(doc, token, out, out_size) == 0) {
        return 0;
    }
    if (out && out_size) {
        out[0] = '\0';
        if (fallback) {
            size_t len = strlen(fallback);
            if (len >= out_size) len = out_size - 1;
            memcpy(out, fallback, len);
            out[len] = '\0';
        }
    }
    return -1;
}

int
ng_json_obj_int(const NgJsonDoc *doc, int obj, const char *key, int fallback)
{
    return ng_json_int(doc, ng_json_obj_get(doc, obj, key), fallback);
}

int
ng_json_obj_bool(const NgJsonDoc *doc, int obj, const char *key, int fallback)
{
    return ng_json_bool(doc, ng_json_obj_get(doc, obj, key), fallback);
}
