#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#define STBI_NO_THREAD_LOCALS
#include <stb/stb_image.h>
#include <stdio.h>
#include <unistd.h>

#include "img_loader_private.h"
int stb_image_load(ImageContext* context, int fd, ImageData* data) {
    FILE* file = fdopen(dup(fd), "r");
    if(!file)
        return -1;
    int num_channels = 4;
    char *raw = stbi_load_from_file  (file, &data->image_width, &data->image_height, &num_channels, num_channels);
    fclose(file);
    if(raw) {
        data->data = raw;
        data->flags |= IMG_DATA_FLIP_RED_BLUE;
        return 0;
    }
    return -1;
}

void stb_image_close(ImageData* data) {
    stbi_image_free(data->data);
}
