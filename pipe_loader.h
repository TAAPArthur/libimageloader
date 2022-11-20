#ifndef PIPE_LOADER_H
#define PIPE_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <fcntl.h>
#include <unistd.h>

int pipe_load(ImageLoaderContext* context, int pipeFD, ImageLoaderData* parent) {
    int fd = image_loader_create_memory_file(parent->name, 0);
    if (fd == -1) {
        return fd;
    }

    int buf_size =  1 << 12;
    int ret;
    int err = -1;
    char buffer[255];
    do {
        ret = safe_read(pipeFD, buffer, sizeof(buffer));
        if (ret == -1 || ret && safe_write(fd, buffer, ret) == -1) {
            goto close_fd;
        }
    } while (ret);

    int end = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    ImageLoaderData* data;
    while (1) {
        int temp_fd = dup(fd);
        data = image_loader_add_file(context, parent->name);
        data->fd = temp_fd;
        data->flags |= IMG_DATA_KEEP_OPEN;
        if (!image_loader_load_image(context, data)) {
            break;
        }
        if (lseek(fd, 0, SEEK_CUR) == end)
            break;
    }
    err = 0;
close_fd:
    close(fd);
    return 0;
}
#endif
