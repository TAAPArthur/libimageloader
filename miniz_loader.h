#ifndef MINIZ_LOADER_H
#define MINIZ_LOADER_H

#include "img_loader_private.h"
#include <fcntl.h>
#include <miniz.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static size_t miniz_file_write_func(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n){
    return write(*(int*)pOpaque, pBuf, n);
}

int miniz_load(ImageLoaderContext* context, int fd, ImageLoaderData* parent) {
    FILE* file = fdopen(dup(fd), "r");
    mz_zip_archive zip_archive = {0};
    if (!mz_zip_reader_init_cfile(&zip_archive, file, 0, 0)){
        fclose(file);
        return -1;
    }
    image_loader_load_stats(parent);
    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
        }
        if (mz_zip_reader_is_file_a_directory(&zip_archive, i))
            continue;
        const char* name = strdup(file_stat.m_filename);
        int fd = image_loader_create_memory_file(name, 0);
        if (fd == -1) {
            return fd;
        }
        mz_zip_reader_extract_to_callback(&zip_archive, i, miniz_file_write_func, &fd, 0);
        lseek(fd, 0, SEEK_SET);

        ImageLoaderData* data = image_loader_add_from_fd_with_flags(context, fd, name, IMG_DATA_KEEP_OPEN | IMG_DATA_FREE_NAME);
        image_loader_set_stats(data, file_stat.m_uncomp_size, parent->mod_time);
    }
    mz_zip_reader_end(&zip_archive);
    fclose(file);
    return 0;
}
#endif
