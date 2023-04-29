#ifndef PIPE_LOADER_H
#define PIPE_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <fcntl.h>
#include <unistd.h>

int pipe_open(ImageLoaderContext* context, int pipeFD, ImageLoaderData* parent) {
    return parent->fd == -1;
}

ImageLoaderData* pipe_next(ImageLoaderContext* context, ImageLoaderData* parent) {
    ImageLoaderData* data = NULL;
    int pipeFD = (long)parent->parent_data;
    int fd = image_loader_create_memory_file(parent->name, 0);
    if (fd == -1) {
        return NULL;
    }

    int buf_size =  1 << 12;
    int ret;
    char buffer[255];
    do {
        ret = safe_read(pipeFD, buffer, sizeof(buffer));
        if (ret == -1 || ret && safe_write(fd, buffer, ret) == -1) {
            goto close_fd;
        }
    } while (ret);

    int end = lseek(fd, 0, SEEK_CUR);
    if(end != 0) {
        lseek(fd, 0, SEEK_SET);
        int temp_fd = dup(fd);
        data = createImageLoaderData(context, temp_fd, parent->name, IMG_DATA_KEEP_OPEN, end, getCurrentTime());
    }
close_fd:

    close(fd);
    return data;
}

void pipe_close() {}
#endif
