#ifndef ZIP_LOADER_H
#define ZIP_LOADER_H

#include "img_loader_private.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zip.h>

int zip_load(ImageLoaderContext* context, int fd, ImageLoaderData* parent) {
    int errorp;

    zip_t* zip = zip_fdopen(fd, 0, &errorp);
    if (!zip)
        return -1;

    int n = zip_get_num_entries(zip, 0);
    if (!n)
        return 0;
    image_loader_load_stats(parent);
    struct zip_stat stat;
    zip_stat_init(&stat);

    for (int i=0; i < n; i++) {
        zip_file_t* file = zip_fopen_index(zip, i, 0);
        zip_stat_index(zip, i, 0, &stat);
        const char* name = stat.valid & ZIP_STAT_NAME ? stat.name : parent->name;
        name = strdup(name);
        int size = stat.size;
        int fd = image_loader_create_memory_file(name, size);
        if (fd == -1) {
            return fd;
        }
        char* buf = malloc(size);
        zip_fread(file, buf, size);
        write(fd, buf, size);
        free(buf);
        lseek(fd, 0, SEEK_SET);

        ImageLoaderData* data = image_loader_add_from_fd_with_flags(context, fd, name, IMG_DATA_KEEP_OPEN | IMG_DATA_FREE_NAME);
        image_loader_set_stats(data, size, stat.valid & ZIP_STAT_MTIME ? stat.mtime : parent->mod_time);
        zip_fclose(file);
    }
    zip_discard(zip);
    return 0;
}
#endif
