#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "img_loader_private.h"

int dir_load(ImageLoaderContext* context, int fd, ImageLoaderData* data) {
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
        char* buf = malloc(base_len + strlen(dir->d_name) + 2);
        sprintf(buf, "%s%s%s", path, path[base_len-1] == '/' ? "" : "/", dir->d_name);
        if(dir->d_type != DT_DIR || !(context->flags & IMAGE_LOADER_DISABLE_RECURSIVE_DIR_LOADER))
            image_loader_add_file(context, buf)->flags |= IMG_DATA_FREE_NAME;
    }
    closedir(d);
    return 0;
}
