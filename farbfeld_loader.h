#ifndef FARBFELD_LOADER_H
#define FARBFELD_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int farbfeld_load(ImageLoaderData* data) {
    char buffer[9] = {0};
    int ret = safe_read(data->fd, buffer, 8);
    if (ret == -1 || memcmp(buffer, "farbfeld", 8)) {
        return -1;
    }
    ret = 0;
    ret |= safe_read(data->fd, buffer, 4);
    data->image_width = buffer[3] + (buffer[2] << 8) + (buffer[1] << 16) + (buffer[0] << 24);
    ret |= safe_read(data->fd, buffer, 4);
    data->image_height = buffer[3] + (buffer[2] << 8) + (buffer[1] << 16) + (buffer[0] << 24);
    if (ret == -1 || data->image_width == 0 || data->image_height == 0) {
        return -1;
    }
    int size = data->image_width * data->image_height * 4;
    data->data = malloc(size);
    int writeCount = 0;
    while (writeCount < size) {
        int ret = read(data->fd, data->data + writeCount, size - writeCount > 4096 ? 4096 : size - writeCount);
        if (ret == -1) {
            if (retry_on_error())
                continue;
            break;
        }
        for (int i = 0; i < ret; i += 2) {
            data->data[writeCount++] = data->data[i];
        }
    }
    return 0;
}

void farbfeld_close(ImageLoaderData* data) {
    free(data->data);
}
#endif

