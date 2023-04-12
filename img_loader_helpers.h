#ifndef IMG_LOADER_HELPERS_H
#define IMG_LOADER_HELPERS_H

#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static int inline retry_on_error() {
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
}

#ifdef DEBUG
#define debug(...) dprintf(2, __VA_ARGS__)
#else
#define debug(...)
#endif

#define _SAFE_READ_WRITE(FUNC, fd, buffer, size) \
    size_t offset = 0; \
    while (offset != size) { \
        int ret = FUNC(fd, buffer + offset, size - offset); \
        if (ret == -1) { \
            if (retry_on_error()) \
                continue; \
            perror(# FUNC " failed"); \
            return ret; \
        } else if (ret == 0) { \
            return offset; \
        } \
        offset += ret; \
    } \
    return offset; \

static int inline safe_read(int fd, void* buffer, size_t size) {
    _SAFE_READ_WRITE(read, fd, buffer, size);
}

static int inline safe_write(int fd, const void* buffer, size_t size) {
    _SAFE_READ_WRITE(write, fd, buffer, size);
}

static FILE * safe_dup_and_fd_open(int fd) {
    int temp_fd = dup(fd);
    if (temp_fd == -1)
        return NULL;
    FILE* file = fdopen(temp_fd, "r");
    if (!file) {
        close(temp_fd);
    }
    return file;
}

static time_t inline getCurrentTime() {
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return tp.tv_sec;
}

#endif

