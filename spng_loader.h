#include <spng.h>
#include <stdlib.h>
#include "img_loader_private.h"

int spng_load(ImageContext* context, int fd, ImageData* data) {

    spng_ctx *ctx;
    struct spng_ihdr ihdr;
    char*image = NULL;
    size_t size;

    ctx = spng_ctx_new(0);
    if(ctx == NULL) return -1;

    if(spng_set_png_file(ctx, fdopen(fd, "r"))) goto err;

    if(spng_get_ihdr(ctx, &ihdr)) goto err;

    if(spng_decoded_image_size(ctx, SPNG_FMT_RGBA8 , &size)) goto err;

    image = malloc(size);
    if(image == NULL) goto err;

    if(spng_decode_image(ctx, image, size, SPNG_FMT_RGBA8, 0)) goto err;


    data->data = image;
    data->image_width = ihdr.width;
    data->image_height = ihdr.height;
    data->flags |= IMG_DATA_FLIP_RED_BLUE;

    spng_ctx_free(ctx);
    return 0;

err:
    if(image != NULL) free(image);
    spng_ctx_free(ctx);

    return -1;
}

void spng_close(ImageData* data) {
    free(data->data);
}
