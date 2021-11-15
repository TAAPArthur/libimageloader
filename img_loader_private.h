#ifndef IMG_LOADER_PRIVATE
#define IMG_LOADER_PRIVATE

#include "img_loader.h"
#include <stdbool.h>

#ifndef NDEBUG
#include <stdio.h>
#define DEBUG(...) printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

#define IMG_DATA_KEEP_OPEN 1

void setStats(ImageData*data, long size, long mod_time) ;

typedef struct ImageData {
    int id;
    int loader_index;
    int parent_loader_index;
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
#endif