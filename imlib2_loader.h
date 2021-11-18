#include <Imlib2.h>
#include <unistd.h>
#include "img_loader_private.h"

int imlib2_load(ImageContext* context, int fd, ImageData* data) {
    int fd2 = dup(fd);
	if ((data->image_data = imlib_load_image_fd(fd2, data->name)) == NULL) {
		return -1;
    }
    imlib_context_set_image(data->image_data);
    data->data = imlib_image_get_data_for_reading_only();
    data->image_width = imlib_image_get_width();
    data->image_height = imlib_image_get_height();
	return 0;
}

void imlib2_close(ImageData* data) {
    imlib_context_set_image(data->image_data);
    imlib_free_image();
}
