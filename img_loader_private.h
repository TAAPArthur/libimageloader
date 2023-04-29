#ifndef IMG_LOADER_PRIVATE
#define IMG_LOADER_PRIVATE

#include "img_loader.h"

#ifdef VERBOSE
#include <stdio.h>
#define IMG_LIB_LOG(...) printf(__VA_ARGS__)
#else
#define IMG_LIB_LOG(...)
#endif

#define IMG_DATA_KEEP_OPEN      (1 << 0)
#define IMG_DATA_FREE_NAME      (1 << 1)
#define IMG_DATA_FLIP_RED_BLUE  (1 << 2)

typedef struct ImageLoader ImageLoader;

typedef struct ImageLoaderData {
    unsigned int id;
    const ImageLoader* loader;
    const char* name;
    int fd;
    unsigned int image_width;
    unsigned int image_height;
    void* image_data;
    union {
        void* parent_data;
        char* data;
    };
    char stats_loaded;
    long size;
    long mod_time;
    int ref_count;
    int flags;
    unsigned char scratch;
} ImageLoaderData;

typedef struct ImageLoaderContext {
    ImageLoaderData** data;
    int num;
    int size;
    unsigned int counter;
    unsigned int flags;
    unsigned int disabled_loaders;
} ImageLoaderContext;

void image_loader_set_stats(ImageLoaderData*data, long size, long mod_time);

void image_loader_load_stats(ImageLoaderData*data);

int image_loader_create_memory_file(const char* name, int size);

void image_loader_load_raw_image(ImageLoaderData* data, const char* src, int width, int height, int stride, int channels);

ImageLoaderData* createImageLoaderData(const ImageLoaderContext* context, int fd, const char* file_name, unsigned int flags, unsigned long size, unsigned long mod_time);
ImageLoaderData* createSimpleImageLoaderData(const ImageLoaderContext* context, const char* file_name, unsigned int flags) {
    return createImageLoaderData(context, -1, file_name, flags, 0, 0);
};

#endif
