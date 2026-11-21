#include "scanner.h"

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef __PROSPERO__
#include <pthread_np.h>
#endif

#include "app.h"
#include "art_thumb.h"
#include "install_state.h"
#include "installer.h"
#include "json.h"
#include "log.h"
#include "pkg_meta.h"
#include "source.h"
#include "util.h"

#define ART_THUMB_WIDTH 1920
#define ART_THUMB_HEIGHT 1080
#define ART_THUMB_QUALITY 86

typedef struct {
    NgGame *items;
    size_t count;
    size_t capacity;
} GameVec;

static pthread_t scanner_thread;
static int scanner_running;
static atomic_int scanner_stop;

static int
game_vec_add(GameVec *vec, const NgGame *game)
{
    NgGame *items;
    size_t cap;

    if (vec->count >= NG_MAX_GAMES) return -1;
    if (vec->count == vec->capacity) {
        cap = vec->capacity ? vec->capacity * 2 : 64;
        if (cap > NG_MAX_GAMES) cap = NG_MAX_GAMES;
        items = realloc(vec->items, cap * sizeof(*items));
        if (!items) return -1;
        vec->items = items;
        vec->capacity = cap;
    }
    vec->items[vec->count++] = *game;
    return 0;
}

static const NgSourceEntry *
find_entry(const NgSourceList *list, const char *name)
{
    for (size_t i = 0; i < list->count; i++) {
        if (!strcasecmp(list->items[i].name, name)) return &list->items[i];
    }
    return NULL;
}

static void
child_path(char *out, size_t out_size, const char *parent, const char *name)
{
    const char *separator = !strcmp(parent, "/") ? "" : "/";
    if (snprintf(out, out_size, "%s%s%s", parent, separator, name) >= (int)out_size) {
        out[0] = '\0';
    }
}

static int
local_file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0;
}

static int
json_try_string(const NgJsonDoc *doc, int obj, const char *key,
                char *out, size_t out_size)
{
    int token = ng_json_obj_get(doc, obj, key);
    return token >= 0 ? ng_json_string(doc, token, out, out_size) : -1;
}

static void
cache_art(NgSource *source, const char *remote_path, const char *dest)
{
    char *data = NULL;
    size_t size = 0;

    if (local_file_exists(dest)) return;
    if (!remote_path[0] || ng_source_read_all(source, remote_path, NG_MAX_ART_BYTES, &data, &size) < 0) {
        return;
    }
    if (size == 0 || ng_write_file_atomic(dest, data, size, 0666) != 0) {
        ng_log_error("art: 写入 %s 失败: %s", dest, strerror(errno));
    }
    free(data);
}

static void
cache_pkg_art(NgSource *source, const char *path, uint64_t offset, uint32_t size,
              const char *dest)
{
    void *data = NULL;
    size_t data_size = 0;

    if (!dest[0] || local_file_exists(dest)) return;
    if (ng_pkg_read_range(source, path, offset, size, &data, &data_size) != 0) return;
    if (data_size == 0 || ng_write_file_atomic(dest, data, data_size, 0666) != 0) {
        ng_log_error("art: 写入 %s 失败: %s", dest, strerror(errno));
    }
    free(data);
}

static int
is_skipped_dir(const char *name)
{
    static const char *skip[] = {
        "System Volume Information", "$RECYCLE.BIN", "@eaDir", "#recycle",
        ".snapshot", ".Trash", "lost+found", NULL
    };
    if (name[0] == '.') return 1;
    for (int i = 0; skip[i]; i++) {
        if (!strcasecmp(name, skip[i])) return 1;
    }
    return 0;
}

static int
duplicate_folder(const GameVec *vec, const char *tid)
{
    if (!tid || !tid[0]) return 0;
    for (size_t i = 0; i < vec->count; i++) {
        if (vec->items[i].type == NG_GAME_FOLDER &&
            !strcmp(vec->items[i].title_id, tid)) {
            return 1;
        }
    }
    return 0;
}

static int
duplicate_pkg(const GameVec *vec, const char *tid, NgPkgKind kind,
              const char *version, const char *content_id)
{
    const char *candidate_id = content_id ? content_id : "";

    if (!tid || !tid[0]) return 0;
    for (size_t i = 0; i < vec->count; i++) {
        const NgGame *game = &vec->items[i];
        if (game->type != NG_GAME_PKG || strcmp(game->title_id, tid)) continue;
        if (game->pkg_kind != kind) continue;
        if (strcmp(game->version, version ? version : "")) continue;
        if (candidate_id[0] && game->content_id[0] &&
            strcasecmp(game->content_id, candidate_id)) {
            continue;
        }
        return 1;
    }
    return 0;
}

static int
art_path(char *out, size_t out_size, const NgConfig *cfg, const char *game_id,
         const char *suffix)
{
    char name[64];

    if (snprintf(name, sizeof(name), "%s-%s", game_id, suffix) >= (int)sizeof(name)) {
        return -1;
    }
    return ng_path_join_local(out, out_size, cfg->art_dir, name);
}

static int
art_thumb_path(char *out, size_t out_size, const NgConfig *cfg,
               const char *game_id, const char *suffix)
{
    char name[80];

    if (snprintf(name, sizeof(name), "%s-%s-thumb.jpg", game_id, suffix) >=
        (int)sizeof(name)) {
        return -1;
    }
    return ng_path_join_local(out, out_size, cfg->art_dir, name);
}

static int
local_art_thumbnail(const char *source, const char *dest)
{
    if (local_file_exists(dest)) return 0;
    if (ng_has_suffix_ci(source, ".jpg") || ng_has_suffix_ci(source, ".jpeg")) {
        char *data;
        int size = ng_read_text_file(source, NG_MAX_ART_BYTES, &data);
        if (size < 0) return -1;
        int rc = size > 0 ? ng_write_file_atomic(dest, data, (size_t)size, 0666) : -1;
        free(data);
        return rc;
    }
    return ng_art_thumbnail_make(source, dest, ART_THUMB_WIDTH,
                                 ART_THUMB_HEIGHT, ART_THUMB_QUALITY);
}

static int
cache_art_thumbnail(NgSource *source, const char *remote_path,
                    const char *png_path, const char *thumb_path)
{
    if (local_file_exists(thumb_path)) {
        unlink(png_path);
        return 1;
    }
    cache_art(source, remote_path, png_path);
    if (!local_file_exists(png_path)) return 0;
    if (local_art_thumbnail(png_path, thumb_path) != 0) return 0;
    unlink(png_path);
    return 1;
}

static int
cache_pkg_art_thumbnail(NgSource *source, const char *pkg_path,
                        uint64_t offset, uint32_t size,
                        const char *png_path, const char *thumb_path)
{
    if (local_file_exists(thumb_path)) {
        unlink(png_path);
        return 1;
    }
    cache_pkg_art(source, pkg_path, offset, size, png_path);
    if (!local_file_exists(png_path)) return 0;
    if (local_art_thumbnail(png_path, thumb_path) != 0) return 0;
    unlink(png_path);
    return 1;
}

static void
inherit_art_file(const NgConfig *cfg, NgGame *game, const char *source_path,
                 const char *suffix)
{
    char dest[NG_PATH_LEN];
    char thumb[NG_PATH_LEN];

    if (!source_path || !source_path[0] || !local_file_exists(source_path)) return;
    if (!strcmp(suffix, "pic0.png") || !strcmp(suffix, "pic1.png")) {
        const char *base = !strcmp(suffix, "pic0.png") ? "pic0" : "pic1";
        if (art_thumb_path(thumb, sizeof(thumb), cfg, game->id, base) == 0 &&
            local_art_thumbnail(source_path, thumb) == 0) {
            if (art_path(dest, sizeof(dest), cfg, game->id, suffix) == 0 &&
                strcmp(dest, source_path)) {
                unlink(dest);
            }
            if (!strcmp(base, "pic0")) {
                ng_strlcpy(game->pic0_file, thumb, sizeof(game->pic0_file));
            } else {
                ng_strlcpy(game->pic1_file, thumb, sizeof(game->pic1_file));
            }
            return;
        }
    }
    if (art_path(dest, sizeof(dest), cfg, game->id, suffix) != 0) return;
    if (local_file_exists(dest) || ng_copy_file_local(source_path, dest, 0666) == 0) {
        if (!strcmp(suffix, "icon.png")) {
            ng_strlcpy(game->icon_file, dest, sizeof(game->icon_file));
        } else if (!strcmp(suffix, "pic0.png")) {
            ng_strlcpy(game->pic0_file, dest, sizeof(game->pic0_file));
        } else if (!strcmp(suffix, "pic1.png")) {
            ng_strlcpy(game->pic1_file, dest, sizeof(game->pic1_file));
        }
    }
}

static void
inherit_catalog_art(const NgConfig *cfg, GameVec *games)
{
    for (size_t i = 0; i < games->count; i++) {
        NgGame *game = &games->items[i];
        if (!ng_valid_title_id(game->title_id)) continue;

        if (!game->icon_file[0]) {
            char path[NG_PATH_LEN];
            if (ng_install_state_find_asset(game->title_id, "icon0.png", path,
                                            sizeof(path)) == 0) {
                inherit_art_file(cfg, game, path, "icon.png");
            }
            if (!game->icon_file[0]) {
                for (size_t j = 0; j < games->count; j++) {
                    const NgGame *other = &games->items[j];
                    if (j == i || strcmp(other->title_id, game->title_id) ||
                        !other->icon_file[0]) {
                        continue;
                    }
                    inherit_art_file(cfg, game, other->icon_file, "icon.png");
                    if (game->icon_file[0]) break;
                }
            }
        }

        if (!game->pic0_file[0]) {
            char path[NG_PATH_LEN];
            if (ng_install_state_find_asset(game->title_id, "pic0.png", path,
                                            sizeof(path)) == 0) {
                inherit_art_file(cfg, game, path, "pic0.png");
            }
            if (!game->pic0_file[0]) {
                for (size_t j = 0; j < games->count; j++) {
                    const NgGame *other = &games->items[j];
                    if (j == i || strcmp(other->title_id, game->title_id) ||
                        !other->pic0_file[0]) {
                        continue;
                    }
                    inherit_art_file(cfg, game, other->pic0_file, "pic0.png");
                    if (game->pic0_file[0]) break;
                }
            }
        }

        if (!game->pic0_file[0] && !game->pic1_file[0]) {
            char path[NG_PATH_LEN];
            if (ng_install_state_find_asset(game->title_id, "pic1.png", path,
                                            sizeof(path)) == 0) {
                inherit_art_file(cfg, game, path, "pic1.png");
            }
        }
    }
}

static int
add_folder_game(NgSource *source, const NgConfig *cfg, const char *source_name,
                const char *path, const NgSourceList *listing, GameVec *games)
{
    char param_path[NG_PATH_LEN];
    char icon_path[NG_PATH_LEN];
    char pic0_path[NG_PATH_LEN];
    char pic1_path[NG_PATH_LEN];
    char pic0_thumb[NG_PATH_LEN];
    char pic1_thumb[NG_PATH_LEN];
    char *data = NULL;
    size_t data_size = 0;
    NgPkgMeta meta;
    NgInstallState state;
    NgGame game;
    const NgSourceEntry *index;

    memset(&meta, 0, sizeof(meta));
    child_path(param_path, sizeof(param_path), path, "sce_sys/param.json");
    if (ng_source_read_all(source, param_path, NG_MAX_META_BYTES, &data, &data_size) >= 0) {
        if (ng_app_meta_from_json(data, &meta) != 0) {
            ng_log_error("%s: 无效的 param.json", path);
        }
        free(data);
        data = NULL;
    }
    if (!ng_valid_title_id(meta.title_id)) {
        child_path(param_path, sizeof(param_path), path, "sce_sys/param.sfo");
        if (ng_source_read_all(source, param_path, NG_MAX_META_BYTES, &data, &data_size) >= 0) {
            if (ng_app_meta_from_sfo(data, data_size, &meta) != 0) {
                ng_log_error("%s: 无效的 param.sfo", path);
            }
            free(data);
            data = NULL;
        }
    }
    if (!ng_valid_title_id(meta.title_id)) {
        return 0;
    }
    if (!meta.title_name[0]) {
        ng_strlcpy(meta.title_name, meta.title_id, sizeof(meta.title_name));
    }
    if (duplicate_folder(games, meta.title_id)) return 0;

    memset(&game, 0, sizeof(game));
    ng_make_game_id(game.id, sizeof(game.id), source->config.id, path);
    ng_strlcpy(game.source_id, source->config.id, sizeof(game.source_id));
    ng_strlcpy(game.source_name, source_name, sizeof(game.source_name));
    ng_strlcpy(game.title_id, meta.title_id, sizeof(game.title_id));
    ng_strlcpy(game.title_name, meta.title_name, sizeof(game.title_name));
    ng_strlcpy(game.version, meta.version, sizeof(game.version));
    ng_strlcpy(game.category, meta.category, sizeof(game.category));
    ng_strlcpy(game.content_id, meta.content_id, sizeof(game.content_id));
    if (!ng_valid_content_id(game.content_id)) game.content_id[0] = '\0';
    ng_strlcpy(game.path, path, sizeof(game.path));
    game.type = NG_GAME_FOLDER;
    game.pkg_kind = NG_PKG_BASE;
    game.packing = NG_PACK_PLAIN;
    index = find_entry(listing, "ampr_zstd.index");
    if (index) game.packing = NG_PACK_ZSTD;
    index = find_entry(listing, "ampr_assets.index");
    if (index) game.packing = NG_PACK_LZ4;
    game.installable = game.packing == NG_PACK_PLAIN;
    if (ng_install_state_probe(cfg, game.title_id, NG_PKG_BASE, game.version,
                               game.content_id, &state) == 0) {
        game.base_installed = state.title_installed;
        game.installed = state.package_installed;
        game.install_known = state.package_known;
        game.install_partial = state.package_partial;
        ng_strlcpy(game.installed_version, state.title_version,
                   sizeof(game.installed_version));
        game.library_copy = state.library;
        game.homebrew_copy = state.homebrew;
    }

    child_path(icon_path, sizeof(icon_path), path, "sce_sys/icon0.png");
    if (art_path(game.icon_file, sizeof(game.icon_file), cfg, game.id, "icon.png") != 0) {
        game.icon_file[0] = '\0';
    }
    if (game.icon_file[0]) cache_art(source, icon_path, game.icon_file);
    if (!local_file_exists(game.icon_file)) game.icon_file[0] = '\0';

    child_path(pic0_path, sizeof(pic0_path), path, "sce_sys/pic0.png");
    if (art_path(game.pic0_file, sizeof(game.pic0_file), cfg, game.id, "pic0.png") != 0) {
        game.pic0_file[0] = '\0';
    }
    if (art_thumb_path(pic0_thumb, sizeof(pic0_thumb), cfg, game.id, "pic0") != 0) {
        pic0_thumb[0] = '\0';
    }
    if (game.pic0_file[0] && pic0_thumb[0] &&
        cache_art_thumbnail(source, pic0_path, game.pic0_file, pic0_thumb)) {
        ng_strlcpy(game.pic0_file, pic0_thumb, sizeof(game.pic0_file));
    } else if (!local_file_exists(game.pic0_file)) {
        game.pic0_file[0] = '\0';
    }

    if (!game.pic0_file[0]) {
        child_path(pic1_path, sizeof(pic1_path), path, "sce_sys/pic1.png");
        if (art_path(game.pic1_file, sizeof(game.pic1_file), cfg, game.id, "pic1.png") != 0) {
            game.pic1_file[0] = '\0';
        }
        if (art_thumb_path(pic1_thumb, sizeof(pic1_thumb), cfg, game.id, "pic1") != 0) {
            pic1_thumb[0] = '\0';
        }
        if (game.pic1_file[0] && pic1_thumb[0] &&
            cache_art_thumbnail(source, pic1_path, game.pic1_file, pic1_thumb)) {
            ng_strlcpy(game.pic1_file, pic1_thumb, sizeof(game.pic1_file));
        } else if (!local_file_exists(game.pic1_file)) {
            game.pic1_file[0] = '\0';
        }
    }
    if (!game.pic0_file[0] && game.pic1_file[0]) {
        ng_strlcpy(game.pic0_file, game.pic1_file, sizeof(game.pic0_file));
    }

    if (game_vec_add(games, &game) != 0) return -1;
    ng_log_info("发现文件夹游戏 %s %s (%s)", game.title_id, game.title_name,
                ng_packing_name(game.packing));
    return 1;
}

static const NgSourceEntry *
find_sidecar(const NgSourceList *listing, const char *pkg_name, char *rel, size_t rel_size)
{
    char base[NG_FILE_NAME_LEN];
    const char *dot;
    size_t len;

    ng_strlcpy(base, pkg_name, sizeof(base));
    dot = strrchr(base, '.');
    if (dot) ((char *)dot)[0] = '\0';
    len = strlen(base);
    for (size_t i = 0; i < listing->count; i++) {
        const char *name = listing->items[i].name;
        size_t name_len = strlen(name);
        if (listing->items[i].is_dir) continue;
        if (name_len < len) continue;
        if (!strncasecmp(name, base, len) && !strcasecmp(name + len, ".meta.json")) {
            snprintf(rel, rel_size, "%s", name);
            return &listing->items[i];
        }
        if (!strncasecmp(name, base, len) && !strcasecmp(name + len, ".json")) {
            snprintf(rel, rel_size, "%s", name);
            return &listing->items[i];
        }
    }
    return NULL;
}

static int
add_pkg_game(NgSource *source, const NgConfig *cfg, const char *source_name,
             const char *dir, const NgSourceEntry *pkg, const NgSourceList *listing,
             GameVec *games)
{
    char path[NG_PATH_LEN];
    char sidecar_rel[NG_NAME_LEN];
    char sidecar_path[NG_PATH_LEN];
    char title[NG_TITLE_LEN];
    char tid[16];
    char icon_name[NG_PATH_LEN];
    char pic0_name[NG_PATH_LEN];
    char pic1_name[NG_PATH_LEN];
    char pic0_thumb[NG_PATH_LEN];
    char pic1_thumb[NG_PATH_LEN];
    char meta_error[NG_ERROR_LEN];
    char *json = NULL;
    const NgSourceEntry *sidecar;
    NgPkgMeta meta;
    NgInstallState state;
    NgGame game;
    NgJsonDoc doc;
    int root = 0;

    child_path(path, sizeof(path), dir, pkg->name);
    if (!path[0]) return -1;
    memset(&game, 0, sizeof(game));
    memset(&meta, 0, sizeof(meta));
    ng_make_game_id(game.id, sizeof(game.id), source->config.id, path);
    ng_strlcpy(game.source_id, source->config.id, sizeof(game.source_id));
    ng_strlcpy(game.source_name, source_name, sizeof(game.source_name));
    ng_strlcpy(game.path, path, sizeof(game.path));
    game.type = NG_GAME_PKG;
    game.packing = NG_PACK_PLAIN;
    game.size = pkg->size;
    game.installable = 1;
    icon_name[0] = pic0_name[0] = pic1_name[0] = '\0';

    if (ng_pkg_probe(source, path, pkg->size, &meta, meta_error, sizeof(meta_error)) != 0) {
        ng_log_error("PKG 元数据: %s: %s", path, meta_error[0] ? meta_error : "解析失败");
    }
    if (ng_valid_title_id(meta.title_id)) {
        ng_strlcpy(game.title_id, meta.title_id, sizeof(game.title_id));
    }
    ng_strlcpy(game.title_name, meta.title_name, sizeof(game.title_name));
    ng_strlcpy(game.version, meta.version, sizeof(game.version));
    ng_strlcpy(game.category, meta.category, sizeof(game.category));
    ng_strlcpy(game.content_id, meta.content_id, sizeof(game.content_id));
    game.pkg_kind = meta.kind;

    sidecar = find_sidecar(listing, pkg->name, sidecar_rel, sizeof(sidecar_rel));
    if (sidecar) {
        child_path(sidecar_path, sizeof(sidecar_path), dir, sidecar_rel);
        if (sidecar_path[0] &&
            ng_source_read_all(source, sidecar_path, NG_MAX_META_BYTES, &json, NULL) >= 0 &&
            ng_json_parse(&doc, json, strlen(json)) == 0) {
            char value[NG_TITLE_LEN];
            if (!ng_valid_title_id(game.title_id) &&
                json_try_string(&doc, root, "titleId", value, sizeof(value)) == 0 &&
                ng_valid_title_id(value)) {
                ng_strlcpy(game.title_id, value, sizeof(game.title_id));
            }
            if (!game.title_name[0] &&
                json_try_string(&doc, root, "titleName", value, sizeof(value)) == 0 &&
                value[0]) {
                ng_strlcpy(game.title_name, value, sizeof(game.title_name));
            } else if (!game.title_name[0] &&
                       json_try_string(&doc, root, "name", value, sizeof(value)) == 0 &&
                       value[0]) {
                ng_strlcpy(game.title_name, value, sizeof(game.title_name));
            }
            if (!game.version[0] &&
                json_try_string(&doc, root, "version", value, sizeof(value)) == 0 &&
                value[0]) {
                ng_strlcpy(game.version, value, sizeof(game.version));
            }
            if (!game.content_id[0] &&
                json_try_string(&doc, root, "contentId", value, sizeof(value)) == 0 &&
                value[0]) {
                ng_strlcpy(game.content_id, value, sizeof(game.content_id));
            }
            if (!game.content_id[0] &&
                json_try_string(&doc, root, "content_id", value, sizeof(value)) == 0 &&
                value[0]) {
                ng_strlcpy(game.content_id, value, sizeof(game.content_id));
            }
            if (json_try_string(&doc, root, "icon", icon_name, sizeof(icon_name)) != 0) {
                json_try_string(&doc, root, "cover", icon_name, sizeof(icon_name));
            }
            if (json_try_string(&doc, root, "pic0", pic0_name, sizeof(pic0_name)) != 0) {
                json_try_string(&doc, root, "background", pic0_name, sizeof(pic0_name));
            }
            json_try_string(&doc, root, "pic1", pic1_name, sizeof(pic1_name));
            ng_json_free(&doc);
        }
        free(json);
    }
    if (!ng_valid_content_id(game.content_id)) game.content_id[0] = '\0';
    if (!ng_valid_title_id(game.title_id)) {
        ng_extract_title_id(pkg->name, tid, sizeof(tid));
        ng_strlcpy(game.title_id, tid, sizeof(game.title_id));
    }
    if (!game.title_name[0]) {
        ng_strlcpy(title, pkg->name, sizeof(title));
        char *dot = strrchr(title, '.');
        if (dot) *dot = '\0';
        ng_strlcpy(game.title_name, title, sizeof(game.title_name));
    }
    if (ng_install_state_probe(cfg, game.title_id, game.pkg_kind, game.version,
                               game.content_id, &state) == 0) {
        game.base_installed = state.title_installed;
        game.installed = state.package_installed;
        game.install_known = state.package_known;
        game.install_partial = state.package_partial;
        ng_strlcpy(game.installed_version, state.title_version,
                   sizeof(game.installed_version));
        game.library_copy = state.library;
        game.homebrew_copy = state.homebrew;
    }
    if (duplicate_pkg(games, game.title_id, game.pkg_kind, game.version,
                      game.content_id)) {
        return 0;
    }

    if (meta.has_icon) {
        if (art_path(game.icon_file, sizeof(game.icon_file), cfg, game.id, "icon.png") == 0) {
            cache_pkg_art(source, path, meta.icon_offset, meta.icon_size, game.icon_file);
        }
    } else if (icon_name[0]) {
        char remote[NG_PATH_LEN];
        child_path(remote, sizeof(remote), dir, icon_name);
        if (art_path(game.icon_file, sizeof(game.icon_file), cfg, game.id, "icon.png") != 0) {
            game.icon_file[0] = '\0';
        }
        if (game.icon_file[0]) cache_art(source, remote, game.icon_file);
    }
    if (!local_file_exists(game.icon_file)) game.icon_file[0] = '\0';
    if (art_thumb_path(pic0_thumb, sizeof(pic0_thumb), cfg, game.id, "pic0") != 0) {
        pic0_thumb[0] = '\0';
    }
    if (meta.has_pic0) {
        if (art_path(game.pic0_file, sizeof(game.pic0_file), cfg, game.id, "pic0.png") == 0 &&
            pic0_thumb[0] &&
            cache_pkg_art_thumbnail(source, path, meta.pic0_offset, meta.pic0_size,
                                    game.pic0_file, pic0_thumb)) {
            ng_strlcpy(game.pic0_file, pic0_thumb, sizeof(game.pic0_file));
        }
    } else if (pic0_name[0]) {
        char remote[NG_PATH_LEN];
        child_path(remote, sizeof(remote), dir, pic0_name);
        if (art_path(game.pic0_file, sizeof(game.pic0_file), cfg, game.id, "pic0.png") != 0) {
            game.pic0_file[0] = '\0';
        }
        if (game.pic0_file[0] && pic0_thumb[0] &&
            cache_art_thumbnail(source, remote, game.pic0_file, pic0_thumb)) {
            ng_strlcpy(game.pic0_file, pic0_thumb, sizeof(game.pic0_file));
        }
    }
    if (!local_file_exists(game.pic0_file)) game.pic0_file[0] = '\0';
    if (!game.pic0_file[0]) {
        if (art_thumb_path(pic1_thumb, sizeof(pic1_thumb), cfg, game.id, "pic1") != 0) {
            pic1_thumb[0] = '\0';
        }
        if (meta.has_pic1) {
            if (art_path(game.pic1_file, sizeof(game.pic1_file), cfg, game.id, "pic1.png") == 0 &&
                pic1_thumb[0] &&
                cache_pkg_art_thumbnail(source, path, meta.pic1_offset, meta.pic1_size,
                                        game.pic1_file, pic1_thumb)) {
                ng_strlcpy(game.pic1_file, pic1_thumb, sizeof(game.pic1_file));
            }
        } else if (pic1_name[0]) {
            char remote[NG_PATH_LEN];
            child_path(remote, sizeof(remote), dir, pic1_name);
            if (art_path(game.pic1_file, sizeof(game.pic1_file), cfg, game.id, "pic1.png") != 0) {
                game.pic1_file[0] = '\0';
            }
            if (game.pic1_file[0] && pic1_thumb[0] &&
                cache_art_thumbnail(source, remote, game.pic1_file, pic1_thumb)) {
                ng_strlcpy(game.pic1_file, pic1_thumb, sizeof(game.pic1_file));
            }
        }
        if (!local_file_exists(game.pic1_file)) game.pic1_file[0] = '\0';
    }
    if (!game.pic0_file[0] && game.pic1_file[0]) {
        ng_strlcpy(game.pic0_file, game.pic1_file, sizeof(game.pic0_file));
    }
    if (game_vec_add(games, &game) != 0) return -1;
    ng_log_info("发现 PKG %s %s [%s] (%llu MB)", game.title_id, game.title_name,
                ng_pkg_kind_name(game.pkg_kind),
                (unsigned long long)(game.size / (1024 * 1024)));
    return 1;
}

static int
scan_dir(NgSource *source, const NgConfig *cfg, const char *source_name,
         const char *path, int depth, GameVec *games)
{
    NgSourceList list;
    const NgSourceEntry *eboot;
    const NgSourceEntry *sce_sys;
    int rc = 0;

    if (scanner_stop) return 0;
    ng_source_list_init(&list);
    if (ng_source_list_dir(source, path, &list) != 0) {
        ng_log_error("source %s: 无法读取 %s", source->config.id, path);
        ng_source_list_free(&list);
        return -1;
    }
    eboot = find_entry(&list, "eboot.bin");
    sce_sys = find_entry(&list, "sce_sys");
    if (eboot && !eboot->is_dir && sce_sys && sce_sys->is_dir) {
        int added = add_folder_game(source, cfg, source_name, path, &list, games);
        if (added == 1) {
            ng_source_list_free(&list);
            return 0;
        }
        if (added < 0) {
            ng_source_list_free(&list);
            return -1;
        }
    }

    for (size_t i = 0; i < list.count; i++) {
        const NgSourceEntry *entry = &list.items[i];
        if (scanner_stop) break;
        if (!entry->is_dir && ng_has_suffix_ci(entry->name, ".pkg")) {
            if (add_pkg_game(source, cfg, source_name, path, entry, &list, games) < 0) rc = -1;
        }
    }
    if (depth < cfg->max_scan_depth) {
        for (size_t i = 0; i < list.count; i++) {
            const NgSourceEntry *entry = &list.items[i];
            char child[NG_PATH_LEN];
            if (!entry->is_dir || is_skipped_dir(entry->name)) continue;
            child_path(child, sizeof(child), path, entry->name);
            if (!child[0] || scan_dir(source, cfg, source_name, child, depth + 1, games) != 0) rc = -1;
        }
    }
    ng_source_list_free(&list);
    return rc;
}

static int
compare_games(const void *a, const void *b)
{
    const NgGame *ga = a;
    const NgGame *gb = b;
    int rc = strcasecmp(ga->title_name, gb->title_name);
    if (rc) return rc;
    rc = strcasecmp(ga->source_name, gb->source_name);
    return rc ? rc : strcmp(ga->id, gb->id);
}

static int
run_scan(void)
{
    NgConfig *cfg;
    NgSource *sources[NG_MAX_SOURCES];
    int source_count;
    unsigned generation;
    GameVec games;
    char message[NG_ERROR_LEN];
    int failures = 0;

    memset(&games, 0, sizeof(games));
    cfg = malloc(sizeof(*cfg));
    if (!cfg) return -1;
    source_count = ng_app_scan_begin(cfg, sources, &generation);
    ng_log_info("开始扫描 %d 个连接", cfg->source_count);

    for (int i = 0; i < source_count && !scanner_stop; i++) {
        NgSource *source = sources[i];
        if (!source->config.enabled) continue;
        for (int r = 0; r < source->config.root_count && !scanner_stop; r++) {
            if (scan_dir(source, cfg, source->config.name, source->config.roots[r], 0,
                         &games) != 0) {
                failures++;
            }
        }
    }
    if (scanner_stop) {
        free(games.items);
        snprintf(message, sizeof(message), "扫描已停止");
    } else {
        inherit_catalog_art(cfg, &games);
        if (games.count > 1) qsort(games.items, games.count, sizeof(games.items[0]), compare_games);
        int applied = ng_app_replace_catalog(games.items, games.count, generation, failures != 0);
        if (applied <= 0) {
            free(games.items);
            if (applied < 0) {
                snprintf(message, sizeof(message), "扫描不完整，%d 个扫描目录读取失败，保留现有游戏库", failures);
            } else {
                snprintf(message, sizeof(message), "连接设置已更新，等待重新扫描");
            }
        } else if (failures) {
            snprintf(message, sizeof(message), "扫描完成，%d 个扫描目录读取不完整，已加载 %zu 个资源",
                     failures, games.count);
        } else {
            snprintf(message, sizeof(message), "已加载 %zu 个资源", games.count);
        }
    }
    ng_app_scan_mark_finished(failures == 0, message);
    ng_log_info("%s", message);
    for (int i = 0; i < source_count; i++) ng_source_release(sources[i]);
    free(cfg);
    return 0;
}

static void *
scanner_main(void *arg)
{
    NgConfig *cfg;

    (void)arg;
    cfg = malloc(sizeof(*cfg));
    if (!cfg) return NULL;
    while (!scanner_stop) {
        int requested = 0;
        int transfer_busy = 0;
        uint64_t finished;
        time_t now;
        struct timespec ts = {1, 0};

        nanosleep(&ts, NULL);
        if (scanner_stop) break;
        ng_app_config_snapshot(cfg);
        pthread_mutex_lock(&ng_app()->lock);
        requested = ng_app()->scan_requested;
        finished = ng_app()->scan_finished_at;
        pthread_mutex_unlock(&ng_app()->lock);
        transfer_busy = ng_installer_is_busy();
        now = time(NULL);
        if (!requested) {
            if (transfer_busy) continue;
            if (!cfg->auto_scan || !cfg->source_count) continue;
            if (finished && (uint64_t)now < finished + (uint64_t)cfg->scan_interval_seconds) {
                continue;
            }
        }
        run_scan();
    }
    free(cfg);
    return NULL;
}

int
ng_scanner_start(void)
{
    pthread_attr_t attr;

    if (scanner_running) return 0;
    scanner_stop = 0;
    if (pthread_attr_init(&attr) != 0) return -1;
    pthread_attr_setstacksize(&attr, 2u * 1024u * 1024u);
    if (pthread_create(&scanner_thread, &attr, scanner_main, NULL) != 0) {
        pthread_attr_destroy(&attr);
        ng_log_error("无法启动扫描线程");
        return -1;
    }
    pthread_attr_destroy(&attr);
    scanner_running = 1;
    return 0;
}

void
ng_scanner_stop(void)
{
    if (!scanner_running) return;
    scanner_stop = 1;
    pthread_join(scanner_thread, NULL);
    scanner_running = 0;
}
