#define _GNU_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "img_loader_private.h"

#ifndef NO_DIR_LOADER
#include "dir_loader.h"
#endif

#ifndef NO_STB_IMAGE_LOADER
#include "stb_image_loader.h"
#endif

#ifndef NO_SPNG_LOADER
#include "spng_loader.h"
#endif

#ifndef NO_IMLIB2_LOADER
#include "imlib2_loader.h"
#endif

#ifndef NO_MINIZ_LOADER
#include "miniz_loader.h"
#endif

#ifndef NO_ZIP_LOADER
#include "zip_loader.h"
#endif

#ifndef NO_CURL_LOADER
#include "curl_loader.h"
#endif

#ifndef NO_PIPE_LOADER
#include "pipe_loader.h"
#endif

#define RUN_FUNC(DATA, LOADER, FUNC) do { \
    if(getLoaderEnabled(DATA->LOADER)->FUNC)getLoaderEnabled(DATA->LOADER)->FUNC(data); \
    } while(0)

#define MULTI_LOADER   (1 << 0)
#define NO_SEEK        (1 << 1)
#define NO_FD          (1 << 2)

typedef struct ImageLoader {
    const char* name;
    int (*img_open)(ImageLoaderContext*, int fd, ImageLoaderData*);
    void (*img_close)(ImageLoaderData*);
    char flags;
} ImageLoader;
#define CREATE_LOADER(NAME) { # NAME, NAME ## _load, .img_close= NAME ## _close}
#define CREATE_PARENT_LOADER(NAME, FLAGS){  # NAME , NAME ## _load, .flags = FLAGS}

static const ImageLoader img_loaders[] = {
#ifndef NO_DIR_LOADER
    CREATE_PARENT_LOADER(dir, MULTI_LOADER | NO_SEEK),
#endif
#ifndef NO_SPNG_LOADER
    CREATE_LOADER(spng),
#endif
#ifndef NO_STB_IMAGE_LOADER
    CREATE_LOADER(stb_image),
#endif
#ifndef NO_MINIZ_LOADER
    CREATE_PARENT_LOADER(miniz, MULTI_LOADER),
#endif
#ifndef NO_ZIP_LOADER
    CREATE_PARENT_LOADER(zip, MULTI_LOADER),
#endif
#ifndef NO_IMLIB2_LOADER
    CREATE_LOADER(imlib2),
#endif
#ifndef NO_CURL_LOADER
    CREATE_PARENT_LOADER(curl, MULTI_LOADER | NO_FD | NO_SEEK),
#endif
};

static ImageLoader pipe_loader = CREATE_PARENT_LOADER(pipe, MULTI_LOADER | NO_SEEK);

static int image_data_get_fd(ImageLoaderData* data) {
    int fd = data->fd;
    if(fd == -1)
        fd = open(data->name, O_RDONLY | O_CLOEXEC);
    return fd;
}

static void image_loader_flip_red_blue(ImageLoaderData* data) {
    char* raw = data->data;
    for(int i = 0; i < data->image_width * data->image_height *4; i+=4) {
        char temp = raw[i];
        raw[i] = raw[i+2];
        raw[i+2] = temp;
    }
}

void image_loader_set_stats(ImageLoaderData*data, long size, long mod_time) {
    data->size = size;
    data->mod_time = mod_time;
    data->stats_loaded = 1;
}

void image_loader_load_stats(ImageLoaderData*data) {
    if(data->stats_loaded)
        return;
    struct stat statbuf;
    int fd = image_data_get_fd(data);
    if(!fstat(fd, &statbuf)) {
        image_loader_set_stats(data, statbuf.st_size, statbuf.st_mtim.tv_sec);
    }
    data->stats_loaded = 1;
}

static int compareName(const void* a, const void* b) {
    return strcmp((*(ImageLoaderData**)a)->name, (*(ImageLoaderData**)b)->name);
}

static int compareMod(const void* a, const void* b) {
    return (*(ImageLoaderData**)a)->mod_time - (*(ImageLoaderData**)b)->mod_time;
}

static int compareSize(const void* a, const void* b) {
    return (*(ImageLoaderData**)a)->size - (*(ImageLoaderData**)b)->size;
}

static int compareId(const void* a, const void* b) {
    return (*(ImageLoaderData**)a)->id - (*(ImageLoaderData**)b)->id;
}

void image_loader_sort(ImageLoaderContext* context, IMAGE_LOADER_SORT_KEY type) {
    static int (*sort_func[])(const void*, const void* b) = {
        [IMG_SORT_LOADED] = compareId,
        [IMG_SORT_NAME] = compareName,
        [IMG_SORT_MOD] = compareMod,
        [IMG_SORT_SIZE] = compareSize,
    };
    if(!(context->flags & IMAGE_LOADER_LOAD_STATS) && abs(type) > IMG_SORT_NAME)
        for(int i = 0;i < context->num; i++) {
            image_loader_load_stats(context->data[i]);
        }
    qsort(context->data, context->num, sizeof(context->data[0]), sort_func[abs(type)]);

    if(type < 0) {
        for(int i = 0;i < context->num/2; i++) {
            void* temp = context->data[i];
            context->data[i] = context->data[context->num -1 - i];
            context->data[context->num -1 - i] = temp;
        }
    }
}

static void removeInvalid(ImageLoaderContext* context) {
    int i = 0;
    for(i = 0;i < context->num && context->data[i]; i++);
    for(int j = 1; i + j < context->num; i++)
        context->data[i] = context->data[i + j];
    context->num = i;
}

void image_loader_close(ImageLoaderData* data, int force) {
    if(--data->ref_count == 0 &&  !(data->flags & IMG_DATA_KEEP_OPEN)|| force) {
        data->loader->img_close(data);
        data->image_data = NULL;
        data->data = NULL;
    }
}

static void image_loader_free_data(ImageLoaderContext*context, ImageLoaderData* data) {
    if(data->data)
        image_loader_close(data, 1);

    if(data->flags & IMG_DATA_FREE_NAME)
        free((void*)data->name);

    free(data);
    for(int i = context->num - 1; i >= 0; i--)
        if (context->data[i] == data)
            context->data[i] = NULL;
}

void image_loader_destroy_context(ImageLoaderContext*context) {
    LOG("Destroy context\n");
    for(int i = context->num - 1; i >= 0; i--){
        image_loader_free_data(context, context->data[i]);
    }
    free(context->data);
    free(context);
}

static int image_loader_load_with_loader(ImageLoaderContext* context, int fd, ImageLoaderData*data, const ImageLoader* img_loader) {
    int ret = img_loader->img_open(context, fd, data);
    LOG("Loader %s returned %d\n", img_loader->name, ret);
    if (ret == 0) {
        data->loader = img_loader;
        if(data->flags & IMG_DATA_FLIP_RED_BLUE)
            image_loader_flip_red_blue(data);
    }
    return ret;
}

static ImageLoaderData* _image_loader_load_image(ImageLoaderContext* context, ImageLoaderData*data, int multi_lib_only) {
    int fd = image_data_get_fd(data);
    if(data->loader)
        return image_loader_load_with_loader(context, fd, data, data->loader) == 0 ? data : NULL;

    for(int i = 0; i < sizeof(img_loaders)/sizeof(img_loaders[0]); i++) {
        if(multi_lib_only && (img_loaders[i].flags & MULTI_LOADER))
            continue;
        if(fd == -1 && !(img_loaders[i].flags & NO_FD))
            continue;
        if(image_loader_load_with_loader(context, fd, data, &img_loaders[i]) == 0)
            return data;
        if(!(img_loaders[i].flags & NO_SEEK))
            lseek(fd, 0, SEEK_SET);
    }
    if(fd != -1)
        close(fd);
    return NULL;
}

ImageLoaderData* image_loader_load_image(ImageLoaderContext* context, ImageLoaderData*data) {
    if(data->data || _image_loader_load_image(context, data, 0)) {
        data->ref_count++;
        return data;
    }
    return NULL;
}

ImageLoaderData* image_loader_open(ImageLoaderContext* context, int index, ImageLoaderData* currentImage) {
    ImageLoaderData* data = NULL;
    if( index >= 0  && index < context->num) {
        if(currentImage == context->data[index])
            return currentImage;
        int num = context->num;
        data = image_loader_load_image(context, context->data[index]);
        int remove = 0;
        int diff = context->num - num;
        if((!data || !data->data) && context->flags & IMAGE_LOADER_REMOVE_INVALID ) {
            image_loader_free_data(context, context->data[index]);
            context->data[index] = NULL;
            removeInvalid(context);
            return image_loader_open(context, index, currentImage);
        }
    }
    if(currentImage) {
        image_loader_close(currentImage, context->flags & IMAGE_LOADER_FORCE_CLOSE);
    }
    return data;
}

ImageLoaderData* image_loader_add_file(ImageLoaderContext* context, const char* file_name) {
    LOG("Attempting to add file %s\n", file_name);
    if(context->num == context->size || !context->data) {
        if(context->data)
            context->size *= 2;
        context->data = realloc(context->data, context->size * sizeof(context->data[0]));
    }
    ImageLoaderData* data = calloc(1, sizeof(ImageLoaderData));
    data->fd = -1;
    context->data[context->num] = data ;
    data->id = context->counter++;
    data->name = file_name;
    if(context->flags & IMAGE_LOADER_LOAD_STATS)
        image_loader_load_stats(context->data[context->num]);
    if(context->flags & IMAGE_LOADER_PRE_EXPAND) {
        if(_image_loader_load_image(context, context->data[context->num], 1));
    }
    context->num++;
    LOG("Added file %s %d\n", file_name, context->num);
    return data;
}

ImageLoaderData* image_loader_add_from_fd(ImageLoaderContext* context, int fd, const char* name) {
    ImageLoaderData* data = image_loader_add_file(context, name);
    data->fd = fd;
    return data;
}

ImageLoaderData* image_loader_add_from_pipe(ImageLoaderContext* context, int fd, const char* name) {
    ImageLoaderData* data = image_loader_add_from_fd(context, fd, name);
    data->loader = &pipe_loader;
    return data;
}

ImageLoaderContext* image_loader_create_context(const char** file_names, int num, int flags) {
    ImageLoaderContext* context = calloc(1, sizeof(ImageLoaderContext));
    context->flags = flags;
    context->size = num ? num : 16;
    for(int i = 0; (!num || i < num) && file_names && file_names[i]; i++) {
        if(file_names[i][0] == '-' && !file_names[i][1])
            image_loader_add_from_pipe(context, STDIN_FILENO, "stdin");
        else
            image_loader_add_file(context, file_names[i]);
    }
    return context;
}

const char* image_loader_get_name(const ImageLoaderData* data){return data->name;}
unsigned int image_loader_get_height(const ImageLoaderData* data){return data->image_height;};;
unsigned int image_loader_get_num(const ImageLoaderContext* context){return context->num;}
unsigned int image_loader_get_width(const ImageLoaderData* data){return data->image_width;};
void* image_loader_get_data(const ImageLoaderData* data) { return data->data;}

int image_loader_create_memory_file(const char* name, int size) {
    int fd = memfd_create(name, MFD_CLOEXEC);
    if(size)
        ftruncate(fd, size);
    return fd;
}


