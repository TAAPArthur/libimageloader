#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zip.h>
#include "img_loader_private.h"

int zip_load(ImageContext* context, int fd, ImageData* parent) {
    int errorp;

    zip_t* zip = zip_fdopen(fd, 0, &errorp);
    if(!zip)
        return -1;

    int n = zip_get_num_entries(zip, 0);
    if(!n)
        return 0;
    loadStats(parent);
    struct zip_stat stat;
    zip_stat_init(&stat);

    for(int i=0; i < n; i++) {
        zip_file_t* file = zip_fopen_index(zip, i, 0);
        zip_stat_index(zip, i, 0, &stat);
        const char* name = stat.valid & ZIP_STAT_NAME ? stat.name : parent->name;
        name = strdup(name);
        int size = stat.size;
        int fd = createMemoryFile(name, size);
        char* buf = malloc(size);
        zip_fread(file, buf, size);
        write(fd, buf, size);
        free(buf);
        lseek(fd, 0, SEEK_SET);
        ImageData* data = addFile(context, name);
        data->parent = parent;
        setStats(data, size, stat.valid & ZIP_STAT_MTIME ? stat.mtime : parent->mod_time);
        data->fd = fd;
        data->flags |= IMG_DATA_KEEP_OPEN;
        zip_fclose(file);
    }
    zip_discard(zip);
    return 0;
}

void zip_close_child(ImageData* data){
    free((void*)data->name);
}
