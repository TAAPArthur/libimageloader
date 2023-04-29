#ifndef DIR_LOADER_H
#define DIR_LOADER_H

#include "img_loader_private.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

int dir_open(ImageLoaderData* parent) {
    DIR* d = fdopendir(parent->fd);
    if (!d)
        return -1;
    parent->parent_data = d;
    return 0;
}

ImageLoaderData* dir_next(ImageLoaderData* parent) {
    struct dirent * dir;
    DIR* d = parent->parent_data;
    const char*path = parent->name;
    int base_len = strlen(path);
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.')
            continue;
        char* name = malloc(base_len + strlen(dir->d_name) + 2);
        strcpy(name, path);
        if (path[base_len-1] != '/')
            strcat(name, "/");
        strcat(name, dir->d_name);
        return createSimpleImageLoaderData(name, IMG_DATA_FREE_NAME);
    }
    return NULL;
}

void dir_close(ImageLoaderData* data) {
    closedir(data->parent_data);
}
#endif
