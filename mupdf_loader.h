#ifndef MUPDF_LOADER_H
#define MUPDF_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <mupdf/fitz.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    fz_context *ctx;
    fz_document *doc;
    FILE* file;
    int page_count;
    int index;
    fz_matrix transform;
} mupdf_state_t;

void mupdf_close(ImageLoaderData* parent) {
    mupdf_state_t * state = parent->parent_data;
    parent->parent_data = NULL;
    if (state->doc)
        fz_drop_document(state->ctx, state->doc);
    if (state->file)
        fclose(state->file);
    fz_drop_context(state->ctx);
    free(state);
}

int mupdf_open(ImageLoaderData* parent) {
    mupdf_state_t * state = calloc(1, sizeof(mupdf_state_t));
    state->ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    state->doc = NULL;
    state->file = NULL;

    if (!state->ctx) {
        return EXIT_FAILURE;
    }
    parent->parent_data = state;

    /* Register the default file types to handle. */
    fz_try(state->ctx)
        fz_register_document_handlers(state->ctx);
    fz_catch(state->ctx)
    {
        debug("cannot register document handlers: %s\n", fz_caught_message(state->ctx));
        mupdf_close(parent);
        return -1;
    }

    state->file = safe_dup_and_fd_open(parent->fd);
    if (!state->file) {
        mupdf_close(parent);
        return -1;
    }
    /* Open the document. */
    fz_try(state->ctx) {
        // TODO auto close file
        state->doc = fz_open_document_with_stream(state->ctx, parent->name, fz_open_file_ptr_no_close(state->ctx, state->file));
    } fz_catch(state->ctx) {
        debug("cannot open document: %s\n", fz_caught_message(state->ctx));
        mupdf_close(parent);
        return -1;
    }

    /* Count the number of pages. */
    state->page_count = fz_count_pages(state->ctx, state->doc);

    float zoom = 150;
    state->transform = fz_scale(zoom / 100, zoom / 100);
    return 0;
}

ImageLoaderData* mupdf_next(ImageLoaderData* parent) {
    int parentNameLen = strlen(parent->name);
    int bufferLen = parentNameLen + 16;
    mupdf_state_t * state = parent->parent_data;
    int index;
    while ((index = state->index++) < state->page_count) {
        /* Render page to an RGB pixmap. */
        fz_try(state->ctx) {
            fz_pixmap *pix = fz_new_pixmap_from_page_number(state->ctx, state->doc, index, state->transform, fz_device_rgb(state->ctx), 0);
            char* name = malloc(bufferLen);
            snprintf(name, bufferLen, "%s Page %03d", parent->name, index);
            ImageLoaderData* data = createImageLoaderData(-1, name, IMG_DATA_KEEP_OPEN | IMG_DATA_FREE_NAME | IMG_DATA_FLIP_RED_BLUE, pix->n * pix->w * pix->h, parent->mod_time);
            image_loader_load_raw_image(data, pix->samples, pix->w, pix->h, pix->stride, pix->n);
            fz_drop_pixmap(state->ctx, pix);
            return data;

        } fz_catch(state->ctx) {
            debug("cannot render page: %s\n", fz_caught_message(state->ctx));
        }
    }
    return NULL;
}
#endif
