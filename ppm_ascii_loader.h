#ifndef PPM_LOADER_H
#define PPM_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

static int readNumber(int fd, char* buffer, int offset, int buffSize, int* dest) {
    int num = 0;
    int comment = 0;
    int i = offset;
    int visted_num = 0;
    int currentSize = buffSize;
readNumberRestart:
    if (buffer[i] == 0 || i >= buffSize) {
        int ret = read(fd, buffer, buffSize);
        if (ret == -1 && retry_on_error())
            goto readNumberRestart;
        if (ret == 0 || ret == -1) {
            i = buffSize;
            goto readNumberEnd;
        }
        assert(ret);
        currentSize = ret;
        i = 0;
    }
    int end = 0;
    while (i < buffSize && !end && i < currentSize) {
        if (comment) {
            if (buffer[i] == '\n') {
                comment = 0;
            }
        }
        else if (buffer[i] >= '0' && buffer[i] <= '9') {
            num = num * 10 + buffer[i] - '0';
            visted_num = 1;
        }
        else if (buffer[i] == '#') {
            comment = 1;
        } else if (visted_num) {
            end = 1;
        }
        i++;
    }
    if (comment || !end) {
        goto readNumberRestart;
    }
readNumberEnd:
    *dest = num;
    return i;
}

int ppm_ascii_load(ImageLoaderData* data) {
    char buffer[256] = {0};
    int ret = safe_read(data->fd, buffer, 3);
    const unsigned int type = buffer[1] - '0';
    if (buffer[0] != 'P' || type >=7) {
        return -1;
    }
    if (type >= 4) {
        return -2;
    }

    int offset = 0;
    buffer[0] = 0;
    offset = readNumber(data->fd, buffer, offset, sizeof(buffer), &data->image_width);
    offset = readNumber(data->fd, buffer, offset, sizeof(buffer), &data->image_height);

    int size = data->image_width * data->image_height * 4;
    data->data = malloc(size);

    int scale = 255;
    if (type != 4 && type != 1) {
        offset = readNumber(data->fd, buffer, offset, sizeof(buffer), &scale);
        scale = 256 / (scale + 1);
    }
    int byte;
    if (type <= 2) {
        for (int i = 0; i < size; i+=4) {
            offset = readNumber(data->fd, buffer, offset, sizeof(buffer), &byte);
            if (type == 1) {
                byte = !byte;
            }
            for (int n = 0; n < 3; n++) {
                ((char*)data->data)[i + n] = byte * scale;
            }
            ((char*)data->data)[i + 3] = 0;
        }
    } else if (type == 3) {
        for (int i = 0; i < size; i+=4) {
            for (int n = 2; n >= 0; n--){
                offset = readNumber(data->fd, buffer, offset, sizeof(buffer), &byte);
                ((char*)data->data)[i + n] = byte * scale;
            }
            ((char*)data->data)[i + 3] = 0;
        }
    }
	return 0;
}

void ppm_ascii_close(ImageLoaderData* data) {
    free(data->data);
}

#endif

