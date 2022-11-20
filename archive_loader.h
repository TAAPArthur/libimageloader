#ifndef ARCHIVE_LOADER_H
#define ARCHIVE_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <archive.h>
#include <archive_entry.h>
#include <string.h>

int archive_load(ImageLoaderContext* context, int fd, ImageLoaderData* parent) {
    struct archive *a;
    struct archive_entry *entry;
    int ret;

    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    if((ret = archive_read_open_fd(a, fd, 4096)) == 0) {
        image_loader_load_stats(parent);
        while(archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            const char* name = strdup(archive_entry_pathname(entry));
            if(archive_entry_filetype(entry) == AE_IFDIR) {
                archive_read_data_skip(a);
                continue;
            }
            int memory_fd = image_loader_create_memory_file(name, 0);
            int r;
            while((r = archive_read_data_into_fd(a, memory_fd)) != ARCHIVE_EOF) {
                if(r != ARCHIVE_OK)
                    break;
            };
            lseek(memory_fd, 0, SEEK_SET);
            ImageLoaderData* data = image_loader_add_from_fd_with_flags(context, memory_fd, name, IMG_DATA_KEEP_OPEN | IMG_DATA_FREE_NAME);
            image_loader_set_stats(data, archive_entry_size(entry), parent->mod_time);
        }
    }
    archive_read_free(a);
    return ret == ARCHIVE_OK ? 0 : -1;
}
#endif
