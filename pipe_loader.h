#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include "img_loader_private.h"



int pipe_load(ImageContext* context, int pipeFD, ImageData* parent) {
    int fd = createMemoryFile(parent->name, 0);

    int buf_size =  1 << 12;
    int ret;
    while((ret = splice(pipeFD, NULL, fd, NULL, buf_size, 0)))
        if(ret < 0) {
            close(fd);
            return ret;
        }

    int end = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    ImageData* data;
    while(1) {
        int temp_fd = dup(fd);
        data = addFile(context, parent->name, IMG_PIPE_ID);
        data->fd = temp_fd;
        data->flags |= IMG_DATA_KEEP_OPEN;
        if(!loadImage(context, data)) {
            break;
        }
        close(temp_fd);
        if(lseek(temp_fd, 0, SEEK_CUR) == end)
            break;
    }
    close(fd);
    return 0;
}
