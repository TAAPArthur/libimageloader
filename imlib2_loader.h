#ifndef IMLIB2_LOADER_H
#define IMLIB2_LOADER_H

#include "img_loader_private.h"
#include <Imlib2.h>
#include <unistd.h>

int imlib2_open(ImageLoaderData* parent) {
    int fd2 = dup(data->fd);
	if ((data->parent_data = imlib_load_image_fd(fd2, parent->name)) == NULL) {
		return -1;
    }
    return 0;
}

ImageLoaderData* imlib2_next(ImageLoaderData* parent) {
    if (parent->flags & IMG_MARK)
        return NULL;
    imlib_context_set_image(parent->parent_data);
    int size = imlib_image_get_width() * imlib_image_get_height() * 4;
    ImageLoaderData* data = createImageLoaderData(-1, parent->name, IMG_DATA_KEEP_OPEN, size, parent->mod_time);
    image_loader_load_raw_image(data, imlib_image_get_data_for_reading_only(), imlib_image_get_width(), imlib_image_get_height(), imlib_image_get_width(), 4);
    imlib_free_image_and_decache();
	return data;
}

void imlib2_close(ImageLoaderData* data) {}
#endif
