#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <curl/curl.h>
#include "img_loader_private.h"

static size_t write_data(void *ptr, size_t size, size_t nmemb, int*fd) {
    return write(*fd, ptr, size * nmemb);
}

static int curl_counter;

int curl_load(ImageContext* context, int _, ImageData* parent) {
    if(!parent->parent_loader_index || strstr(parent->name, "http") != parent->name)
        return -1;
    if(!curl_counter++)
        curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    CURL* curl_handle = curl_easy_init();

    /* set URL to get here */
    curl_easy_setopt(curl_handle, CURLOPT_URL, parent->name);
    /* disable progress meter, set to 0L to enable it */
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

    int fd = createMemoryFile(parent->name, 0);
    /* write the page body to this file handle */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &fd);
    /* get it! */
    curl_easy_perform(curl_handle);

    ImageData* data = addFile(context, parent->name, IMG_CURL_ID);
    data->flags |= IMG_DATA_KEEP_OPEN;
    data->fd = fd;
    lseek(fd, 0, SEEK_SET);

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    return 0;
}

void curl_close() {
    if(!--curl_counter) {
        curl_global_cleanup();
    }
}
