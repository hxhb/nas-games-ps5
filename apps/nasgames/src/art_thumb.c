#include "art_thumb.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_MAX_DIMENSIONS 8192
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int
ng_art_thumbnail_make(const char *src, const char *dest,
                      int max_width, int max_height, int quality)
{
    unsigned char *input;
    unsigned char *output = NULL;
    char temp[1024];
    int width;
    int height;
    int channels;
    int target_width;
    int target_height;
    int ok;
    int fd;

    if (!src || !dest || !dest[0] || max_width <= 0 || max_height <= 0) return -1;
    input = stbi_load(src, &width, &height, &channels, 3);
    if (!input) return -1;

    target_width = width;
    target_height = height;
    if (width > max_width || height > max_height) {
        double scale_x = (double)max_width / (double)width;
        double scale_y = (double)max_height / (double)height;
        double scale = scale_x < scale_y ? scale_x : scale_y;
        target_width = (int)(width * scale + 0.5);
        target_height = (int)(height * scale + 0.5);
        if (target_width < 1) target_width = 1;
        if (target_height < 1) target_height = 1;
    }

    output = malloc((size_t)target_width * (size_t)target_height * 3u);
    if (!output) {
        stbi_image_free(input);
        return -1;
    }
    if (!stbir_resize_uint8_linear(input, width, height, 0,
                                   output, target_width, target_height, 0,
                                   STBIR_RGB)) {
        free(output);
        stbi_image_free(input);
        return -1;
    }

    if (snprintf(temp, sizeof(temp), "%s.tmp.XXXXXX", dest) >= (int)sizeof(temp)) {
        free(output);
        stbi_image_free(input);
        return -1;
    }
    fd = mkstemp(temp);
    if (fd < 0) {
        free(output);
        stbi_image_free(input);
        return -1;
    }
    close(fd);
    ok = stbi_write_jpg(temp, target_width, target_height, 3, output, quality);
    free(output);
    stbi_image_free(input);
    if (!ok) {
        remove(temp);
        return -1;
    }
    if (rename(temp, dest) != 0) {
        remove(temp);
        return -1;
    }
    return 0;
}
