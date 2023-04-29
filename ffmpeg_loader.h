#ifndef FFMPEG_LOADER_H
#define FFMPEG_LOADER_H

#include "img_loader_helpers.h"
#include "img_loader_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int img_loader_spawn(char* cmd[], int fd_in, int fd_out) {
    int pid = fork();
    if (pid == 0) {
        dup2(fd_out, STDOUT_FILENO);
        dup2(fd_in, STDIN_FILENO);
        close(fd_out);
        close(fd_in);
        execvp(cmd[0], cmd);
        perror("Failed to exec");
        exit(1);
    }
    return pid;
}

static void* img_loader_read_all_data(int fd, char* buffer, int buffer_size) {
    int size = 0;
    int allocate_memory = !buffer;
    if (!buffer) {
        buffer_size = 1<<12;
        buffer = malloc(buffer_size);
    }
    int ret = 0;
    while (ret = read(fd, buffer + size, buffer_size - size)) {
        if (ret == -1) {
            if (retry_on_error())
                continue;
            else {
                perror("Read failed");
                free(buffer);
                close(fd);
                return NULL;
            }
        }
        size += ret;
        if (allocate_memory) {
            if (size == buffer_size) {
                buffer_size *= 2;
            }
            buffer = realloc(buffer, buffer_size);
        }
    }
    return allocate_memory ? realloc(buffer, size) : buffer;
}
static int img_loader_wait(int pid) {
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
}

static char* img_loader_spawn_and_wait(int fd, char* cmd[], char* buffer, int buffSize, int* exitStatus) {
    int fds[2] = {0};
    if (pipe(fds) == -1) {
        perror("Failed to create pipe");
        return NULL;
    }
    int pid = img_loader_spawn(cmd, fd, fds[1]);
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return NULL;
    }
    close(fds[1]);
    void * data = img_loader_read_all_data(fds[0], buffer, buffSize);
    close(fds[0]);
    *exitStatus = img_loader_wait(pid);
    return data;
}

static int ffmpeg_get_size(int fd, int * width, int * height) {
    static char* fprobe_cmd[] = {"ffprobe", "-loglevel", "quiet", "-select_streams", "v:0", "-show_entries", "stream=width,height", "-of", "csv=s=x:p=0", "-", NULL};
    char size_buffer[16] = {0};
    int errCode;
    char * buffer = img_loader_spawn_and_wait(fd, fprobe_cmd, size_buffer, sizeof(size_buffer), &errCode);
    if (buffer) {
        if (errCode == 127) {
            *width = 1024;
            *height = 720;
            return 0;
        } else if (errCode == 0) {
            *width = atoi(buffer);
            if (strchr(buffer, 'x') == NULL) {
                return -1;
            }
            *height = atoi(strchr(buffer, 'x') + 1);
            if (*width == 0 || *height == 0) {
                return -1;
            }
        }
    }
    return errCode;
}

int ffmpeg_load(ImageLoaderData* data) {
    if (ffmpeg_get_size(data->fd, &data->image_width, &data->image_height)) {
        return -1;
    }
    char size_buffer[16] = {0};
    snprintf(size_buffer, sizeof(size_buffer), "%dx%d", data->image_width, data->image_height);
    char* ffmpeg_cmd[] = {"ffmpeg", "-loglevel", "quiet", "-i",  "-", "-vcodec", "rawvideo", "-pix_fmt", "bgra", "-vframes" ,"1", "-f", "image2", "-s", size_buffer, "-", NULL};
    lseek(data->fd, 0, SEEK_SET);
    int exit_code;
    data->data = img_loader_spawn_and_wait(data->fd, ffmpeg_cmd, NULL, 0, &exit_code);
    return exit_code;
}

void ffmpeg_close(ImageLoaderData* data) {
    free(data->data);
}
#endif
