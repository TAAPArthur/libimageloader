#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "img_loader_private.h"
#include <fcntl.h>
#include <unistd.h>

int pipe_load(ImageLoaderContext* context, int pipeFD, ImageLoaderData* parent) {
    int fd = image_loader_create_memory_file(parent->name, 0);

    int buf_size =  1 << 12;
    int ret;
    while((ret = splice(pipeFD, NULL, fd, NULL, buf_size, 0)))
        if(ret < 0) {
            close(fd);
            return ret;
        }

    int end = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    ImageLoaderData* data;
    while(1) {
        int temp_fd = dup(fd);
        data = image_loader_add_file(context, parent->name);
        data->fd = temp_fd;
        data->flags |= IMG_DATA_KEEP_OPEN;
        if(!image_loader_load_image(context, data)) {
            break;
        }
        close(temp_fd);
        if(lseek(temp_fd, 0, SEEK_CUR) == end)
            break;
    }
    close(fd);
    return 0;
}
