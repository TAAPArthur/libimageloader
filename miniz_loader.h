#include <miniz.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "img_loader_private.h"

static size_t miniz_file_write_func(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n){
    return write(*(int*)pOpaque, pBuf, n);
}

int miniz_load(ImageContext* context, int fd, ImageData* parent) {
    FILE* file = fdopen(dup(fd), "r");
    mz_zip_archive zip_archive = {0};
    if(!mz_zip_reader_init_cfile(&zip_archive, file, 0, 0)){
        fclose(file);
        return -1;
    }
    loadStats(parent);
    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
        }
        if(mz_zip_reader_is_file_a_directory(&zip_archive, i))
            continue;
        const char* name = strdup(file_stat.m_filename);
        int fd = createMemoryFile(name, 0);
        mz_zip_reader_extract_to_callback(&zip_archive, i, miniz_file_write_func, &fd, 0);
        ImageData* data = addFile(context, name);
        setStats(data, file_stat.m_uncomp_size, parent->mod_time);
        data->fd = fd;
        data->flags |= IMG_DATA_KEEP_OPEN | IMG_DATA_FREE_NAME;
    }
    mz_zip_reader_end(&zip_archive);
    close(fd);
    return 0;
}
