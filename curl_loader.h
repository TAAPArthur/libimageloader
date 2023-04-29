#ifndef CURL_LOADER_H
#define CURL_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <curl/curl.h>
#include <string.h>
#include <unistd.h>

static size_t write_data(void *ptr, size_t size, size_t nmemb, int*fd) {
    return safe_write(*fd, ptr, size * nmemb);
}

void curl_close(ImageLoaderData* parent) {
    /* cleanup curl stuff */
    curl_easy_cleanup(parent->parent_data);

    curl_global_cleanup();
}

int curl_open(ImageLoaderContext* context, int _, ImageLoaderData* parent) {
    if (!strstr(parent->name, "://"))
        return -1;
    int ret = -1;
    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    parent->parent_data = curl_easy_init();
    if (!parent->parent_data) {
        curl_global_cleanup();
        return -1;
    }
    parent->scratch = 0;

    /* set URL to get here */
    curl_easy_setopt(parent->parent_data, CURLOPT_URL, parent->name);
    /* disable progress meter, set to 0L to enable it */
    curl_easy_setopt(parent->parent_data, CURLOPT_NOPROGRESS, 1L);
    /* send all data to this function  */
    curl_easy_setopt(parent->parent_data, CURLOPT_WRITEFUNCTION, write_data);
    return 0;
}

ImageLoaderData* curl_next(ImageLoaderContext* context, ImageLoaderData* parent) {
    CURL* curl_handle = parent->parent_data;
    if (parent->scratch++)
        return NULL;

    int fd = image_loader_create_memory_file(parent->name, 0);
    if (fd == -1)
        return NULL;
    /* write the page body to this file handle */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &fd);
    /* get it! */
    if (CURLE_OK != curl_easy_perform(curl_handle)) {
        close(fd);
        return NULL;
    }
    int pos = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);

    return createImageLoaderData(context, fd, parent->name, IMG_DATA_KEEP_OPEN, pos, getCurrentTime());
}
#endif
