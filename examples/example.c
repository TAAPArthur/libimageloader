#include "../img_loader.h"
#include <stdio.h>

/**
 * This is a dummy program to dump every byte of an uncompressed image(s) which
 * are passed in as args.
 * Every row of the image will be printed space separated on the same line
 */
int main(int argc, const char*argv[]) {
    ImageLoaderContext* c = image_loader_create_context(argv + 1, 0, IMAGE_LOADER_REMOVE_INVALID);
    ImageLoaderData* current_image = NULL;
    for(int i = 0; i < image_loader_get_num(c); i++) {
        current_image = image_loader_open(c, i, current_image);
        if(current_image) {
            printf("File: %s %d x %d \n", image_loader_get_name(current_image), image_loader_get_width(current_image), image_loader_get_height(current_image));
            const int* data = image_loader_get_data(current_image);
            for(int h = 0, n = 0; h < image_loader_get_height(current_image); h++) {
                for(int w = 0; w < image_loader_get_width(current_image); w++, n++)
                    printf("%08x ", data[n]);
                printf("\n");
            }
        }
    }
    image_loader_destroy_context(c);
    return 0;
}
