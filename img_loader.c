#define _GNU_SOURCE
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "img_loader_private.h"

#ifndef NO_DIR_LOADER
#include "dir_loader.h"
#endif
#ifndef NO_IMLIB_LOADER
#include "imglib_loader.h"
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

#ifndef NO_LIBSPNG_LOADER
#include "libspng_loader.h"
#endif

#define RUN_FUNC(DATA, LOADER, FUNC) do { \
    if(getLoaderEnabled(DATA->LOADER)->FUNC)getLoaderEnabled(DATA->LOADER)->FUNC(data); \
    } while(0)

#define MULTI_LOADER   (1 << 0)
#define NO_SEEK        (1 << 1)
#define NO_FD          (1 << 2)
#define DISABLED       (1 << 3)

typedef struct {
    int id;
    int (*img_open)(ImageContext*, int fd, ImageData*);
    void (*img_close)(ImageData*);
    void (*img_close_child)(ImageData*);
    uint8_t flags;
} ImageLoader;

ImageLoader img_loaders[] = {
#ifndef NO_DIR_LOADER
    {IMG_DIR_ID, dir_load, .img_close_child=dir_close_child, .flags = MULTI_LOADER | NO_SEEK},
#endif
#ifndef NO_LIBSPNG_LOADER
    {IMG_LIBSPNG_ID, libspng_load, libspng_close},
#endif
#ifndef NO_IMLIB_LOADER
    {IMG_IMLIB_ID, imlib_load, imlib_close},
#endif
#ifndef NO_ZIP_LOADER
    {IMG_ZIP_ID, load_zip, .img_close_child=zip_close_child, .flags = MULTI_LOADER},
#endif
#ifndef NO_CURL_LOADER
    {IMG_CURL_ID, curl_load, .img_close_child=curl_close, .flags = MULTI_LOADER | NO_FD | NO_SEEK},
#endif
};

ImageLoader pipe_loader = {IMG_PIPE_ID, pipe_load };

ImageLoader* getLoaderEnabled(ImgLoaderId id){
    for(int i = 0; i < sizeof(img_loaders)/sizeof(img_loaders[0]); i++)
        if(img_loaders[i].id == id)
            return &img_loaders[i];
    if(id == IMG_PIPE_ID)
        return &pipe_loader;
    return NULL;
}

int getFD(ImageData* data) {
    int fd = data->fd;
    if(fd == -1)
        fd = open(data->name, O_RDONLY | O_CLOEXEC);
    return fd;
}

void setStats(ImageData*data, long size, long mod_time) {
    data->size = size;
    data->mod_time = mod_time;
    data->stats_loaded = 1;
}

void loadStats(ImageData*data) {
    if(data->stats_loaded)
        return;
    struct stat statbuf;
    int fd = getFD(data);
    if(!fstat(fd, &statbuf)) {
        setStats(data, statbuf.st_size, statbuf.st_mtim.tv_sec);
    }
    data->stats_loaded = 1;
}

int compareName(const ImageData* a, const ImageData* b) {
    return strcmp(a->name, b->name);
}

int compareMod(const ImageData* a, const ImageData* b) {
    return a->mod_time - b->mod_time;
}

int compareSize(const ImageData* a, const ImageData* b) {
    return a->size - b->size;
}

int compareId(const ImageData* a, const ImageData* b) {
    return a->id - b->id;
}

void sort(ImageContext* context, IMG_SORT type) {
    static int (*sort_func[])(const ImageData* a, const ImageData* b) = {
        [IMG_SORT_LOADED] = compareId,
        [IMG_SORT_NAME] = compareName,
        [IMG_SORT_MOD] = compareMod,
        [IMG_SORT_SIZE] = compareSize,
    };
    qsort(context->data, context->num, sizeof(context->data[0]), (__compar_fn_t )sort_func[type]);
}

void removeInvalid(ImageContext* context) {
    int i = 0;
    for(i = 0;i < context->num && context->data[i]; i++);
    for(int j = 1; i + j < context->num; i++)
        context->data[i] = context->data[i + j];
    context->num = i;
}

void closeImage(ImageData* data, bool force) {
    if(--data->ref_count == 0 &&  !(data->flags & IMG_DATA_KEEP_OPEN)|| force) {
        RUN_FUNC(data, loader_index, img_close);
        data->image_data = NULL;
        data->data = NULL;
    }
}

void freeImageData(ImageContext*context, ImageData* data) {
    if(data->parent_loader_index) {
        RUN_FUNC(data, parent_loader_index, img_close_child);
    }
    if(data->data)
        closeImage(data, 1);

    free(data);
    for(int i = context->num - 1; i >= 0; i--)
        if (context->data[i] == data)
            context->data[i] = NULL;
}

void destroyContext(ImageContext*context) {
    LOG("Destroy context\n");
    for(int i = context->num - 1; i >= 0; i--){
        freeImageData(context, context->data[i]);
    }
    free(context->data);
    free(context);
}

void setLoaderEnabled(ImgLoaderId id, int value){
    if(value)
        getLoaderEnabled(id)->flags &= ~DISABLED;
    else
        getLoaderEnabled(id)->flags |= DISABLED;
}

int loadImageWithLoader(ImageContext* context, int fd, ImageData*data, ImageLoader*img_loader) {
    int ret = img_loader->img_open(context, fd, data);
    LOG("Loader %d returned %d\n", img_loader->id, ret);
    if (ret == 0)
        data->loader_index = img_loader->id;
    return ret;
}

ImageData* _loadImage(ImageContext* context, ImageData*data, bool multi_lib_only) {
    int fd = getFD(data);
    for(int i = 0; i < sizeof(img_loaders)/sizeof(img_loaders[0]); i++) {
        if(multi_lib_only && (img_loaders[i].flags & MULTI_LOADER) || img_loaders[i].flags & DISABLED)
            continue;
        if(fd == -1 && !(img_loaders[i].flags & NO_FD))
            continue;
        if(loadImageWithLoader(context, fd, data, &img_loaders[i]) == 0)
            return data;
        if(!(img_loaders[i].flags & NO_SEEK))
            lseek(fd, 0, SEEK_SET);
    }
    if(fd != -1)
        close(fd);
    return NULL;
}

ImageData* loadImage(ImageContext* context, ImageData*data) {
    if(data->data || _loadImage(context, data, 0)) {
        data->ref_count++;
        return data;
    }
    return NULL;
}

struct ImageData* openImage(struct ImageContext* context, int index, struct ImageData* currentImage) {
    ImageData* data = NULL;
    if( index >= 0  && index < context->num) {
        if(currentImage == context->data[index])
            return currentImage;
        int num = context->num;
        data = loadImage(context, context->data[index]);
        int remove = 0;
        int diff = context->num - num;
        if((!data || !data->data) && context->flags & REMOVE_INVALID) {
            freeImageData(context, context->data[index]);
            context->data[index] = NULL;
            removeInvalid(context);
            return openImage(context, index, currentImage);
        }
    }
    if(currentImage) {
        closeImage(currentImage, 0);
    }
    return data;
}

ImageData* addFile(ImageContext* context, const char* file_name, ImgLoaderId  parent) {
    LOG("Attempting to add file %s\n", file_name);
    if(context->num == context->size || !context->data) {
        if(context->data)
            context->size *= 2;
        context->data = realloc(context->data, context->size * sizeof(context->data[0]));
    }
    ImageData* data = calloc(1, sizeof(ImageData));
    data->fd = -1;
    context->data[context->num] = data ;
    data->id = context->counter++;
    data->name = file_name;
    data->parent_loader_index = parent;
    if(context->flags & LOAD_STATS)
        loadStats(context->data[context->num]);
    if(context->flags & PRE_EXPAND) {
        if(_loadImage(context, context->data[context->num], 1));
    }
    context->num++;
    LOG("Added file %s %d\n", file_name, context->num);
    return data;
}

int addFromPipe(ImageContext* context, int fd, const char* name) {
    ImageData* data = addFile(context, name, 0);
    return loadImageWithLoader(context, fd, data, &pipe_loader);
}

ImageContext* createContext(const char** file_names, int num, int flags) {
    ImageContext* context = calloc(1, sizeof(ImageContext));
    context->flags = flags;
    context->size = num ? num : 16;
    for(int i = 0; (!num || i < num) && file_names && file_names[i]; i++) {
        if(file_names[i][0] == '-' && !file_names[i][1])
            addFromPipe(context, STDIN_FILENO, "stdin");
        else
            addFile(context, file_names[i], IMG_INVALID_ID);
    }
    return context;
}

const char* getImageName(const ImageData* data){return data->name;}
unsigned int getImageNum(const ImageContext* context){return context->num;}
unsigned int getImageWidth(const ImageData* data){return data->image_width;};
unsigned int getImageHeight(const ImageData* data){return data->image_height;};;
void* getRawImage(const ImageData* data) { return data->data;}

int createMemoryFile(const char* name, int size) {
    int fd = memfd_create(name, MFD_CLOEXEC);
    if(size)
        ftruncate(fd, size);
    return fd;
}

