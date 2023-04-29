#ifndef MINIZ_LOADER_H
#define MINIZ_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <miniz.h>
#include <unistd.h>

static size_t miniz_file_write_func(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n){
    return safe_write(*(int*)pOpaque, pBuf, n);
}
typedef struct {
    FILE* file;
    mz_zip_archive zip_archive;
    int index;
} miniz_state_t;

int miniz_open(ImageLoaderContext* context, int fd, ImageLoaderData* parent) {
    miniz_state_t * state = calloc(1, sizeof(miniz_state_t));
    state->file = safe_dup_and_fd_open(fd);
    if (!state->file) {
        return -1;
    }
    if (!mz_zip_reader_init_cfile(&state->zip_archive, state->file, 0, 0)){
        fclose(state->file);
        return -1;
    }
    parent->parent_data = state;
    image_loader_load_stats(parent);
    return 0;
}

ImageLoaderData* miniz_next(ImageLoaderContext* context, ImageLoaderData* parent) {
    miniz_state_t * state = parent->parent_data;
    unsigned index;
    while ((index = state->index++) < mz_zip_reader_get_num_files(&state->zip_archive)) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&state->zip_archive, index, &file_stat) ||
                mz_zip_reader_is_file_a_directory(&state->zip_archive, index)) {
            continue;
        }
        char* name = strdup(file_stat.m_filename);
        int fd = image_loader_create_memory_file(name, 0);
        if (fd == -1) {
            free(name);
            continue;
        }
        mz_zip_reader_extract_to_callback(&state->zip_archive, index, miniz_file_write_func, &fd, 0);
        lseek(fd, 0, SEEK_SET);

        return createImageLoaderData(context, fd, name, IMG_DATA_KEEP_OPEN | IMG_DATA_FREE_NAME,
            file_stat.m_uncomp_size, parent->mod_time);
    }
    return NULL;
}

void miniz_close(ImageLoaderData* parent) {
    miniz_state_t * state = parent->parent_data;
    mz_zip_reader_end(&state->zip_archive);
    fclose(state->file);
    free(state);
}
#endif
