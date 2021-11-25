#ifndef IMG_LOADER_PRIVATE
#define IMG_LOADER_PRIVATE

#include "img_loader.h"

#ifdef VERBOSE
#include <stdio.h>
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define IMG_DATA_KEEP_OPEN      (1 << 0)
#define IMG_DATA_FREE_NAME      (1 << 1)
#define IMG_DATA_FLIP_RED_BLUE  (1 << 2)

typedef struct ImageLoader ImageLoader;

typedef struct ImageLoaderData {
    int id;
    const ImageLoader* loader;
    int fd;
    const char* name;
    unsigned int image_width;
    unsigned int image_height;
    void* image_data;
    void* data;
    char stats_loaded;
    long size;
    long mod_time;
    int ref_count;
    int flags;
} ImageLoaderData;

typedef struct ImageLoaderContext {
    ImageLoaderData** data;
    int num;
    int size;
    unsigned int counter;
    unsigned int flags;
} ImageLoaderContext;

void image_loader_set_stats(ImageLoaderData*data, long size, long mod_time);
ImageLoaderData* image_loader_load_image(ImageLoaderContext* context, ImageLoaderData*data);

void image_loader_load_stats(ImageLoaderData*data);

int image_loader_create_memory_file(const char* name, int size);
#endif
