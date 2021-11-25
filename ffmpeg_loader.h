#include "img_loader_helper.h"
#include "img_loader_private.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static char* ffmpeg_cmd[] = {"ffmpeg", "-i",  "-", "-vframes" ,"1", "-f", "image2", "-", NULL};
int ffmpeg_load(ImageLoaderContext* context, int fd, ImageLoaderData* parent) {
    int fd_dest = image_loader_create_memory_file(parent->name, 0);
    int fd_in=dup(fd);

    int pid = fork();
    if(pid == 0) {
        dup2(fd_dest, STDOUT_FILENO);
        dup2(fd_in, STDIN_FILENO);
        close(STDERR_FILENO);
        execvp(ffmpeg_cmd[0], ffmpeg_cmd);
        exit(1);
    }
    else if(pid < 0)
        return -1;
    close(fd_in);
    int status = 0;
    waitpid(pid, &status, 0);
    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
    if(exit_code )
        return exit_code;
    close(fd);

    ImageLoaderData* data = image_loader_add_file(context, parent->name);
    data->fd = fd_dest;
    return 0;
}
