#ifndef IMG_LOADER
#define IMG_LOADER

typedef struct ImageData ImageData;
typedef struct ImageContext ImageContext;

#define REMOVE_INVALID (1 << 0)
#define LOAD_STATS     (1 << 1)
#define PRE_EXPAND     (1 << 2)

typedef enum {
    IMG_SORT_LOADED,
    IMG_SORT_NAME,
    IMG_SORT_MOD,
    IMG_SORT_SIZE,
} IMG_SORT;

typedef enum {
    IMG_INVALID_ID,
    IMG_DIR_ID,
    IMG_IMLIB_ID,
    IMG_ZIP_ID,
    IMG_CURL_ID,
    IMG_PIPE_ID,
} ImgLoaderId;


ImageData* addFile(ImageContext* context, const char* file_name, ImgLoaderId parent_loader);
ImageContext* createContext(const char** file_names, int num, int flags);
void destroyContext(ImageContext*context);


ImageData* openImage(ImageContext* context, int index, ImageData* currentImage);

const char* getImageName(const ImageData*);
unsigned int getImageWidth(const ImageData*);
unsigned int getImageHeight(const ImageData*);
void* getRawImage(const ImageData*);
unsigned int getImageNum(const ImageContext* context);

int createMemoryFile(const char* name, int size);
void setLoaderEnabled(ImgLoaderId id, int value);
#endif
