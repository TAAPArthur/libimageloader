#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include "img_loader_private.h"

int dir_load(ImageContext* context, int fd, ImageData* data) {
    const char*path = data->name;
    int base_len = strlen(path);
    DIR* d = fdopendir(fd);
    if(!d)
        return -1;
    struct dirent * dir;
    int count = 0;
    while ((dir = readdir(d)) != NULL) {
        if(dir->d_name[0] == '.')
            continue;
        void* buf = malloc(base_len + strlen(dir->d_name) + 2);
        strcpy(buf, path);
        strcpy(buf + base_len, "/");
        strcpy(buf + base_len + 1, dir->d_name);
        //if(dir-> d_type != DT_DIR)
        addFile(context, buf, IMG_DIR_ID);
    }
    closedir(d);
    return 0;
}

void dir_close_child(ImageData* data){
    free((void*)data->name);
}
