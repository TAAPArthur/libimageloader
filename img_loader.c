#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "img_loader_private.h"

#ifndef NO_DIR_LOADER
#include "dir_loader.h"
#endif

#ifndef NO_PPM_ASCII_LOADER
#include "ppm_ascii_loader.h"
#endif

#ifndef NO_FARBFELD_LOADER
#include "farbfeld_loader.h"
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

#ifndef NO_ARCHIVE_LOADER
#include "archive_loader.h"
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

#ifndef NO_FFMPEG_LOADER
#include "ffmpeg_loader.h"
#endif

#define RUN_FUNC(DATA, LOADER, FUNC) do { \
    if (getLoaderEnabled(DATA->LOADER)->FUNC)getLoaderEnabled(DATA->LOADER)->FUNC(data); \
    } while (0)

#define MULTI_LOADER   (1 << 0)
#define NO_SEEK        (1 << 1)
#define NO_FD          (1 << 2)

typedef struct ImageLoader {
    const char* name;
    int (*img_open)(ImageLoaderContext*, int fd, ImageLoaderData*);
    void (*img_close)(ImageLoaderData*);
    char flags;
} ImageLoader;
#define CREATE_LOADER_WITH_FLAGS(NAME, FLAGS) { # NAME, NAME ## _load, .img_close= NAME ## _close, .flags=FLAGS}
#define CREATE_LOADER(NAME) CREATE_LOADER_WITH_FLAGS(NAME, 0)
#define CREATE_PARENT_LOADER(NAME, FLAGS){  # NAME , NAME ## _load, .flags = FLAGS}

static const ImageLoader img_loaders[] = {
#ifndef NO_DIR_LOADER
    [IMG_LOADER_DIR] = CREATE_PARENT_LOADER(dir, MULTI_LOADER | NO_SEEK),
#endif
#ifndef NO_SPNG_LOADER
    [IMG_LOADER_SPNG] = CREATE_LOADER(spng),
#endif
#ifndef NO_STB_IMAGE_LOADER
    [IMG_LOADER_STB_IMAGE] = CREATE_LOADER(stb_image),
#endif
#ifndef NO_FARBFELD_LOADER
    [IMG_LOADER_FARBFELD] = CREATE_LOADER(farbfeld),
#endif
#ifndef NO_PPM_ASCII_LOADER
    [IMG_LOADER_PPM_ASCII] = CREATE_LOADER(ppm_ascii),
#endif
#ifndef NO_MINIZ_LOADER
    [IMG_LOADER_MINIZ] = CREATE_PARENT_LOADER(miniz, MULTI_LOADER),
#endif
#ifndef NO_ARCHIVE_LOADER
    [IMG_LOADER_ARCHIVE] = CREATE_PARENT_LOADER(archive, MULTI_LOADER),
#endif
#ifndef NO_ZIP_LOADER
    [IMG_LOADER_ZIP] = CREATE_PARENT_LOADER(zip, MULTI_LOADER),
#endif
#ifndef NO_IMLIB2_LOADER
    [IMG_LOADER_IMLIB2] = CREATE_LOADER(imlib2),
#endif
#ifndef NO_CURL_LOADER
    [IMG_LOADER_CURL] = CREATE_PARENT_LOADER(curl, MULTI_LOADER | NO_FD | NO_SEEK),
#endif
#ifndef NO_FFMPEG_LOADER
    [IMG_LOADER_FFMPEG] = CREATE_LOADER(ffmpeg),
#endif
};

static ImageLoader pipe_loader = CREATE_PARENT_LOADER(pipe, MULTI_LOADER | NO_SEEK);

static int image_data_get_fd(ImageLoaderData* data) {
    int fd = data->fd;
    if (fd == -1) {
        fd = open(data->name, O_RDONLY | O_CLOEXEC);
        data->fd = fd;
    }
    return fd;
}

static void image_loader_flip_red_blue(ImageLoaderData* data) {
    char* raw = data->data;
    for (int i = 0; i < data->image_width * data->image_height *4; i+=4) {
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
    if (data->stats_loaded)
        return;
    struct stat statbuf;
    int fd = image_data_get_fd(data);
    if (!fstat(fd, &statbuf)) {
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
static int compareLoadedId(const void* a, const void* b) {
    return (*(ImageLoaderData**)a)->loaded_id - (*(ImageLoaderData**)b)->loaded_id;
}


static int compareRandom(const void* a, const void* b) {
    return rand() - RAND_MAX/2;
}

void image_loader_sort(ImageLoaderContext* context, int type) {
    static int (*sort_func[])(const void*, const void* b) = {
        [IMG_SORT_RANDOM] = compareRandom,
        [IMG_SORT_ADDED] = compareId,
        [IMG_SORT_LOADED] = compareLoadedId,
        [IMG_SORT_NAME] = compareName,
        [IMG_SORT_MOD] = compareMod,
        [IMG_SORT_SIZE] = compareSize,
    };

    if (!(context->flags & IMAGE_LOADER_LOAD_STATS) && abs(type) > IMG_SORT_NAME)
        for (int i = 0;i < context->num; i++) {
            image_loader_load_stats(context->data[i]);
        }
    qsort(context->data, context->num, sizeof(context->data[0]), sort_func[abs(type)]);

    if (type < 0) {
        for (int i = 0;i < context->num/2; i++) {
            void* temp = context->data[i];
            context->data[i] = context->data[context->num -1 - i];
            context->data[context->num -1 - i] = temp;
        }
    }
}

static void image_loader_remove_image_at_index(ImageLoaderContext* context, int n) {
    for (int i = n + 1; i < context->num; i++)
        context->data[i - 1] = context->data[i];
    context->num--;
}


void image_loader_close_force(ImageLoaderContext*context, ImageLoaderData* data, int force) {
    if (!data || !data->data) {
        if(data && data->fd != -1) {
            close(data->fd);
            data->fd = -1;
        }
        return;
    }
    if (--data->ref_count == 0 && (!(data->flags & IMG_DATA_KEEP_OPEN) || context->flags & IMAGE_LOADER_FORCE_CLOSE)|| force) {
        data->loader->img_close(data);
        data->image_data = NULL;
        data->data = NULL;
        close(data->fd);
        data->fd = -1;
    }
}

void image_loader_close(ImageLoaderContext*context, ImageLoaderData* data) {
    image_loader_close_force(context, data, 0);
}

static void image_loader_free_data(ImageLoaderContext*context, ImageLoaderData* data) {
    image_loader_close_force(context, data, 1);
    if (data->flags & IMG_DATA_FREE_NAME)
        free((void*)data->name);
    free(data);
    for (int i = context->num - 1; i >= 0; i--)
        if (context->data[i] == data)
            context->data[i] = NULL;
}

void image_loader_destroy_context(ImageLoaderContext*context) {
    IMG_LIB_LOG("Destroy context\n");
    for (int i = context->num - 1; i >= 0; i--){
        image_loader_free_data(context, context->data[i]);
    }
    free(context->data);
    free(context);
}

static int image_loader_load_with_loader(ImageLoaderContext* context, int fd, ImageLoaderData*data, const ImageLoader* img_loader) {
    int ret = img_loader->img_open(context, fd, data);
    if (ret == 0) {
        data->loader = img_loader;
        if (data->flags & IMG_DATA_FLIP_RED_BLUE)
            image_loader_flip_red_blue(data);
        data->loaded_id = context->counter++;
        assert(!data->data == (img_loader->flags & MULTI_LOADER));
    }
    return ret;
}

static ImageLoaderData* _image_loader_load_image(ImageLoaderContext* context, ImageLoaderData*data, int multi_lib_only) {
    IMG_LIB_LOG("Loading file %s\n", data->name);
    int fd = image_data_get_fd(data);
    if (data->loader)
        return image_loader_load_with_loader(context, fd, data, data->loader) == 0 ? data : NULL;

    for (int i = 0; i < sizeof(img_loaders)/sizeof(img_loaders[0]); i++) {
        // Skip if disabled for the context or not compiled in
        if (context->disabled_loaders & (1 << i) || !img_loaders[i].name)
            continue;
        if (multi_lib_only && !(img_loaders[i].flags & MULTI_LOADER))
            continue;
        if (fd == -1 && !(img_loaders[i].flags & NO_FD))
            continue;
        if (image_loader_load_with_loader(context, fd, data, &img_loaders[i]) == 0) {
            IMG_LIB_LOG("Found loader %s for %s\n", img_loaders[i].name, data->name);
            if (fd != -1)
                close(fd);
            return data;
        }
        if (!(img_loaders[i].flags & NO_SEEK))
            lseek(fd, 0, SEEK_SET);
    }
    if (fd != -1)
        close(fd);
    IMG_LIB_LOG("Could not load %s\n", data->name);
    return NULL;
}

ImageLoaderData* image_loader_load_image(ImageLoaderContext* context, ImageLoaderData*data) {
    if (data->data || _image_loader_load_image(context, data, 0)) {
        data->ref_count++;
        return data;
    }
    return NULL;
}

ImageLoaderData* image_loader_open(ImageLoaderContext* context, int index, ImageLoaderData* currentImage) {
    ImageLoaderData* data = NULL;
    if ( index >= 0  && index < context->num) {
        if (currentImage == context->data[index])
            return currentImage;
        int num = context->num;
        data = image_loader_load_image(context, context->data[index]);
        int remove = 0;
        int diff = context->num - num;
        if ((!data || !data->data) && context->flags & IMAGE_LOADER_REMOVE_INVALID ) {
            image_loader_free_data(context, context->data[index]);
            context->data[index] = NULL;
            image_loader_remove_image_at_index(context, index);
            return image_loader_open(context, index, currentImage);
        }
    }
    if (currentImage) {
        image_loader_close(context, currentImage);
    }
    return data;
}

ImageLoaderData* image_loader_add_from_fd_with_flags(ImageLoaderContext* context, int fd, const char* file_name, unsigned int flags) {
    IMG_LIB_LOG("Adding file %s\n", file_name);
    if (context->num == context->size || !context->data) {
        if (context->data)
            context->size *= 2;
        context->data = realloc(context->data, context->size * sizeof(context->data[0]));
    }
    ImageLoaderData* data = calloc(1, sizeof(ImageLoaderData));
    data->fd = fd;
    context->data[context->num++] = data ;
    data->id = context->counter++;
    data->name = file_name;
    data->flags = flags;
    if (context->flags & IMAGE_LOADER_LOAD_STATS)
        image_loader_load_stats(data);
    if (context->flags & IMAGE_LOADER_PRE_EXPAND) {
        if (_image_loader_load_image(context, data, 1));
    }
    return data;
}

ImageLoaderData* image_loader_add_file(ImageLoaderContext* context, const char* file_name) {
    return image_loader_add_from_fd_with_flags(context, -1, file_name, 0);
}

ImageLoaderData* image_loader_add_from_fd(ImageLoaderContext* context, int fd, const char* name) {
    return image_loader_add_from_fd_with_flags(context, fd, name, IMG_DATA_KEEP_OPEN);
}

ImageLoaderData* image_loader_add_from_pipe(ImageLoaderContext* context, int fd, const char* name) {
    ImageLoaderData* data = image_loader_add_from_fd(context, fd, name);
    data->loader = &pipe_loader;
    return data;
}

void image_loader_remove_all_invalid_images(ImageLoaderContext* context) {
    for (int i = image_loader_get_num(context) - 1; i >= 0; i--){
        ImageLoaderData* data = image_loader_open(context, i, NULL);
        if ((!data || !data->data) && !(context->flags & IMAGE_LOADER_REMOVE_INVALID)) {
            image_loader_remove_image_at_index(context, i);
            image_loader_close(context, data);
        }

    }
}

ImageLoaderContext* image_loader_create_context(const char** file_names, int num, int flags) {
    ImageLoaderContext* context = calloc(1, sizeof(ImageLoaderContext));
    context->flags = flags;
    context->size = num ? num : 16;
    for (int i = 0; (!num || i < num) && file_names && file_names[i]; i++) {
        if (file_names[i][0] == '-' && !file_names[i][1])
            image_loader_add_from_pipe(context, dup(STDIN_FILENO), "stdin");
        else
            image_loader_add_file(context, file_names[i]);
    }
    return context;
}

void image_loader_enable_loader_only(ImageLoaderContext* context, IMAGE_LOADER_INDEX loader) {
    image_loader_enable_loader_only_mask(context, 1 << loader);
}

void image_loader_enable_loader_only_mask(ImageLoaderContext* context, unsigned loaders) {
    context->disabled_loaders = loaders;
    context->disabled_loaders = ~context->disabled_loaders;
    IMG_LIB_LOG("Disabled mask 0x%hx from 0x%hx\n", context->disabled_loaders, loaders);

}

unsigned int image_loader_get_multi_loader_masks() {
    unsigned int mask = 0;
    for (int i = 0; i < sizeof(img_loaders)/sizeof(img_loaders[0]); i++) {
        if (img_loaders[i].flags & MULTI_LOADER) {
            mask |= 1 << i;
        }
    }
    return mask;
}

const char* image_loader_get_name(const ImageLoaderData* data){return data->name;}
unsigned int image_loader_get_height(const ImageLoaderData* data){return data->image_height;};;
unsigned int image_loader_get_num(const ImageLoaderContext* context){return context->num;}
unsigned int image_loader_get_width(const ImageLoaderData* data){return data->image_width;};
void* image_loader_get_data(const ImageLoaderData* data) { return data->data;}

int image_loader_create_memory_file(const char* name, int size) {
    int fd;
#ifndef HAVE_LINUX
    char template[] = "/tmp/.tmp_img_loaderXXXXXX";
    fd = mkstemp(template);
    if (fd != -1) {
        unlink(template);
    }
#else
    fd = memfd_create(name, MFD_CLOEXEC);
    if (size)
        ftruncate(fd, size);
#endif
    return fd;
}
