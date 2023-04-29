#ifndef IMLIB2_LOADER_H
#define IMLIB2_LOADER_H

#include "img_loader_private.h"
#include <Imlib2.h>
#include <unistd.h>

int imlib2_load(ImageLoaderData* data) {
    int fd2 = dup(data->fd);
	if ((data->image_data = imlib_load_image_fd(fd2, data->name)) == NULL) {
		return -1;
    }
    imlib_context_set_image(data->image_data);
    data->data = imlib_image_get_data_for_reading_only();
    data->image_width = imlib_image_get_width();
    data->image_height = imlib_image_get_height();
	return 0;
}

void imlib2_close(ImageLoaderData* data) {
    imlib_context_set_image(data->image_data);
    imlib_free_image();
}
#endif
