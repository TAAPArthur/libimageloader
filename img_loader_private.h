#ifndef IMG_LOADER_PRIVATE
#define IMG_LOADER_PRIVATE

#include "img_loader.h"
#include <stdbool.h>

#ifdef VERBOSE
#include <stdio.h>
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define IMG_DATA_KEEP_OPEN      (1 << 0)
#define IMG_DATA_FREE_NAME      (1 << 1)
#define IMG_DATA_FLIP_RED_BLUE  (1 << 2)

void setStats(ImageData*data, long size, long mod_time) ;
typedef struct ImageLoader ImageLoader;

typedef struct ImageData {
    int id;
    const ImageLoader* loader;
    int fd;
    const char* name;
    unsigned int image_width;
    unsigned int image_height;
    void* image_data;
    void* data;
    bool stats_loaded;
    long size;
    long mod_time;
    int ref_count;
    int flags;
} ImageData;

typedef struct ImageContext {
    ImageData** data;
    int num;
    int size;
    unsigned int counter;
    unsigned int flags;

} ImageContext;

ImageData* loadImage(ImageContext* context, ImageData*data);

void loadStats(ImageData*data);

void flipRedBlue(ImageData* data);
#endif
