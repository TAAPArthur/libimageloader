#ifndef ARCHIVE_LOADER_H
#define ARCHIVE_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <archive.h>
#include <archive_entry.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    struct archive *a;
    struct archive_entry *entry;
} archive_state_t;

int archive_open(ImageLoaderContext* context, int fd, ImageLoaderData* parent) {
    archive_state_t * state = malloc(sizeof(archive_state_t));
    state->a = archive_read_new();
    archive_read_support_filter_all(state->a);
    archive_read_support_format_all(state->a);
    if(archive_read_open_fd(state->a, fd, 4096) == ARCHIVE_OK) {
        parent->parent_data = state;
        image_loader_load_stats(parent);
        return 0;
    }
    return -1;
}

ImageLoaderData* archive_next(ImageLoaderContext* context, ImageLoaderData* parent) {
    archive_state_t * state = parent->parent_data;

    while (archive_read_next_header(state->a, &state->entry) == ARCHIVE_OK) {
        const char* name = strdup(archive_entry_pathname(state->entry));
        if(archive_entry_filetype(state->entry) == AE_IFDIR) {
            archive_read_data_skip(state->a);
            continue;
        }
        int memory_fd = image_loader_create_memory_file(name, 0);
        int r;
        while((r = archive_read_data_into_fd(state->a, memory_fd)) != ARCHIVE_EOF) {
            if(r != ARCHIVE_OK)
                break;
        };
        lseek(memory_fd, 0, SEEK_SET);
        return createImageLoaderData(context, memory_fd, name, IMG_DATA_KEEP_OPEN | IMG_DATA_FREE_NAME,
           archive_entry_size(state->entry), parent->mod_time);
    }
    return NULL;
}

void archive_close(ImageLoaderData* parent) {
    archive_read_free(((archive_state_t*)parent->parent_data)->a);
}
#endif
