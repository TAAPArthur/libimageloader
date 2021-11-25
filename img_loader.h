#ifndef IMG_LOADER
#define IMG_LOADER

typedef struct ImageLoaderData ImageLoaderData;
typedef struct ImageLoaderContext ImageLoaderContext;


/**
 * When an invalid image is detected, instead of returning NULL, remove that
 * image and load the next one until a valid image is found.
 */
#define IMAGE_LOADER_REMOVE_INVALID (1 << 0)
/* AUTO load stats when adding an image to a context instead of waiting for the image to be loaded */
#define IMAGE_LOADER_LOAD_STATS     (1 << 1)
/* Recursively loading images when file is added.
 * For example, when a directory is added, by default, the children of the
 * directly won't be added until the directory is accessed. Same is true for
 * zip files. With this flag, they will be added immediately */
#define IMAGE_LOADER_PRE_EXPAND     (1 << 2)

/**
 * When an images refcount reaches zero (ie it has been closed as much as it
 * has been opened), it will be freeded even if the content can't be re-opened
 * (ie it came from a pipe). This provides a way to ensure that only a single
 * image is loaded in memory at once.
 */
#define IMAGE_LOADER_FORCE_CLOSE     (1 << 3)

/**
 * Create a context with all files from file_names
 * @param file_names - list of string; Note this won't ever be freeded
 * @param num - length of file_names or 0 if file_names is null terminated
 * @param flags - a bitmap of flags to control behavior
 */
ImageLoaderContext* image_loader_create_context(const char** file_names, int num, int flags);
/**
 * Destroy a context an all data it contains
 */
void image_loader_destroy_context(ImageLoaderContext*context);
/**
 * Add file pointed to by file_name to this context
 */
ImageLoaderData* image_loader_add_file(ImageLoaderContext* context, const char* file_name);


/**
 * Opens the nth index image and optionally closes currentImage
 * @param context
 * @param index
 * @param current_image - a currently opened image that will be closed or NULL. If the image at position index is the same as current_image, this method is a no-op
 */
ImageLoaderData* image_loader_open(ImageLoaderContext* context, int index, ImageLoaderData* current_image);


typedef enum {
    /* Sort images by order they were initially loaded */
    IMG_SORT_LOADED,
    /* Sort images by name */
    IMG_SORT_NAME,
    /* Sort images by modification time */
    IMG_SORT_MOD,
    /* Sort images by size time */
    IMG_SORT_SIZE,
} IMAGE_LOADER_SORT_KEY;

/**
 * Sort images.
 */
void image_loader_sort(ImageLoaderContext* context, IMAGE_LOADER_SORT_KEY type);

const char* image_loader_get_name(const ImageLoaderData*);
unsigned int image_loader_get_height(const ImageLoaderData*);
unsigned int image_loader_get_num(const ImageLoaderContext* context);
unsigned int image_loader_get_width(const ImageLoaderData*);
/**
 * Returns the 4-channel uncompressed image data
 */
void* image_loader_get_data(const ImageLoaderData*);

#endif
