#ifndef STB_IMAGE_LOADER
#define STB_IMAGE_LOADER

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#define STBI_NO_THREAD_LOCALS

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <stb/stb_image.h>
#include <unistd.h>

int stb_image_load(ImageLoaderData* data) {
    FILE* file = safe_dup_and_fd_open(data->fd);
    if (!file) {
        return -1;
    }
    int num_channels = 4;
    char *raw = stbi_load_from_file(file, &data->image_width, &data->image_height, &num_channels, num_channels);
    fclose(file);
    if (raw) {
        data->data = raw;
        data->flags |= IMG_DATA_FLIP_RED_BLUE;
        return 0;
    }
    return -1;
}

void stb_image_close(ImageLoaderData* data) {
    stbi_image_free(data->data);
}
#endif
