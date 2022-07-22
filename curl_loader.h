#include <string.h>
#include <unistd.h>

#include "img_loader_private.h"
#include <curl/curl.h>

static size_t write_data(void *ptr, size_t size, size_t nmemb, int*fd) {
    return write(*fd, ptr, size * nmemb);
}

int curl_load(ImageLoaderContext* context, int _, ImageLoaderData* parent) {
    if(strstr(parent->name, "http") != parent->name)
        return -1;
    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    CURL* curl_handle = curl_easy_init();
    if(!curl_handle)
        return -1;

    /* set URL to get here */
    curl_easy_setopt(curl_handle, CURLOPT_URL, parent->name);
    /* disable progress meter, set to 0L to enable it */
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

    int fd = image_loader_create_memory_file(parent->name, 0);
    /* write the page body to this file handle */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &fd);
    /* get it! */
    if(CURLE_OK != curl_easy_perform(curl_handle)) {
        return -1;
    }

    ImageLoaderData* data = image_loader_add_file(context, parent->name);
    data->flags |= IMG_DATA_KEEP_OPEN;
    data->fd = fd;
    lseek(fd, 0, SEEK_SET);

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);


    curl_global_cleanup();
    return 0;
}
