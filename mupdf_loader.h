#ifndef MUPDF_LOADER_H
#define MUPDF_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#define NDEBUG
#include <mupdf/fitz.h>
#undef NDEBUG
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int mupdf_load(ImageLoaderContext* context, int fd, ImageLoaderData* parent) {
    fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    fz_document *doc = NULL;
    FILE* file = NULL;

    if (!ctx) {
        return EXIT_FAILURE;
    }
    int ret = EXIT_FAILURE;

    /* Register the default file types to handle. */
    fz_try(ctx)
        fz_register_document_handlers(ctx);
    fz_catch(ctx)
    {
        debug("cannot register document handlers: %s\n", fz_caught_message(ctx));
        goto cleanup;
    }

    file = safe_dup_and_fd_open(fd);
    if (!file) {
        goto cleanup;
    }
    /* Open the document. */
    fz_try(ctx) {
        doc = fz_open_document_with_stream(ctx, parent->name, fz_open_file_ptr_no_close(ctx, file));
    } fz_catch(ctx) {
        debug("cannot open document: %s\n", fz_caught_message(ctx));
        goto cleanup;
    }
    int page_count;

    /* Count the number of pages. */
    fz_try(ctx)
        page_count = fz_count_pages(ctx, doc);
    fz_catch(ctx) {
        debug("cannot count number of pages: %s\n", fz_caught_message(ctx));
        goto cleanup;
    }

    int parentNameLen = strlen(parent->name);
    int bufferLen = parentNameLen + 16;

    float zoom = 150;
    fz_matrix transform = fz_scale(zoom / 100, zoom / 100);
    for (int i = 0; i < page_count; i++) {
        /* Render page to an RGB pixmap. */
        fz_try(ctx) {
            fz_pixmap *pix = fz_new_pixmap_from_page_number(ctx, doc, i, transform, fz_device_rgb(ctx), 0);
            char* name = malloc(bufferLen);
            snprintf(name, bufferLen, "%s Page %03d", parent->name, i);
            ImageLoaderData* data = image_loader_add_file_with_flags(context, name, IMG_DATA_KEEP_OPEN | IMG_DATA_FREE_NAME | IMG_DATA_FLIP_RED_BLUE);
            image_loader_load_raw_image(data, pix->samples, pix->w, pix->h, pix->stride, pix->n);
            fz_drop_pixmap(ctx, pix);

        } fz_catch(ctx) {
            debug("cannot render page: %s\n", fz_caught_message(ctx));
            goto cleanup;
        }
    }
    ret = 0;
cleanup:
    if (doc)
        fz_drop_document(ctx, doc);
    if (file) {
        fclose(file);
    }
    fz_drop_context(ctx);
    return ret;
}
#endif
