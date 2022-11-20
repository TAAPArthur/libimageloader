#define SCUTEST_IMPLEMENTATION
#define SCUTEST_DEFINE_MAIN
#include "../img_loader.h"
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <scutest/scutest.h>

#define TEST_IMAGE_PREFIX "tests/test_images/"
const char* TEST_IMAGE_PATHS[] = {TEST_IMAGE_PREFIX "/png/test_image.png", TEST_IMAGE_PREFIX "/png/test_image.png", NULL};
const char* TEST_IMAGE_PATH_SOME_INVALID[] = {TEST_IMAGE_PREFIX "/png/test_image.png", "tests/invalid_image.png", ".", NULL};
const char* TEST_IMAGE_PATHS_ZIP[] = {TEST_IMAGE_PREFIX "/zip/test_image.zip", TEST_IMAGE_PREFIX "/zip", TEST_IMAGE_PREFIX "test_image.zip", NULL};

const char* TEST_IMAGE_ALL_PATHS_INVALID[] = {"tests/invalid_image.png", "another_bad_image.bad", NULL};

static void simple_load_test(const char** args, unsigned mask) {
    // Load all files in args
    ImageLoaderContext* c = image_loader_create_context(args, 0, 0);
    if (mask) {
        image_loader_enable_loader_only_mask(c, mask | (1 << IMG_LOADER_DIR));
    }
    int numRealImages = 0;
    for (int i = 0; i < image_loader_get_num(c); i++) {
        ImageLoaderData* current_image = image_loader_open(c, i, NULL); // Open the first image
        assert(current_image);
        const char * path = image_loader_get_name(current_image);
        assert(path);
        char* data = image_loader_get_data(current_image);
        if (data) {
            numRealImages++;
            assert(data);
            int width = image_loader_get_width(current_image);
            int height = image_loader_get_height(current_image);
            assert(width);
            assert(height);
            int size = width * height * 4;
            char temp = data[0];
            temp = data[size - 1];
        }
        // Do stuff like draw the image
    }
    image_loader_destroy_context(c); // Free all resources
    assert(numRealImages);
}

static int starting_number_of_fds;

static int getNumberOfFilesInDir(const char* dirname) {
    DIR* d = opendir(dirname);
    if (!d)
        return -1;
    struct dirent * dir;
    int count = 0;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.')
            continue;
        count++;
    }
    closedir(d);
    return count;
}
void setup_fd_check(){
    starting_number_of_fds = getNumberOfFilesInDir("/proc/self/fd");
}

void teardown_fd_check(){
    assert(starting_number_of_fds == getNumberOfFilesInDir("/proc/self/fd"));
}

SCUTEST_SET_FIXTURE(setup_fd_check, teardown_fd_check);

#ifndef NO_PIPE_LOADER
SCUTEST(simple_workflow_pipe_loader) {
    int fds[2];
    pipe(fds);
    if(!fork()) {
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        execlp("cat", "cat", TEST_IMAGE_PREFIX "png/test_image.png", NULL);
        exit(1);
    }
    dup2(fds[0], STDIN_FILENO);
    close(fds[0]);
    close(fds[1]);
    const char* path[] = {"-", NULL};
    simple_load_test(path, image_loader_get_nonmulti_loader_masks());
    wait(NULL);
}
#endif

#ifndef NO_PPM_ASCII_LOADER
SCUTEST(simple_workflow_ppm_ascii) {
    const char* path[] = {TEST_IMAGE_PREFIX "ppm/", NULL};
    simple_load_test(path, 1 << IMG_LOADER_PPM_ASCII);
}
#endif

#ifndef NO_FARBFELD_LOADER
SCUTEST(simple_workflow_farbfeld) {
    const char* path[] = {TEST_IMAGE_PREFIX "farbfeld/", NULL};
    simple_load_test(path, 1 << IMG_LOADER_FARBFELD);
}
#endif

#ifndef NO_STB_IMAGE_LOADER
SCUTEST(simple_workflow_stb_image) {
    const char* path[] = {TEST_IMAGE_PREFIX "png/", NULL};
    simple_load_test(path, 1 << IMG_LOADER_STB_IMAGE);
}
#endif

#ifndef NO_SPNG_LOADER
SCUTEST(simple_workflow_spng) {
    const char* path[] = {TEST_IMAGE_PREFIX "png/", NULL};
    simple_load_test(path, 1 << IMG_LOADER_SPNG);
}
#endif

#ifndef NO_IMLIB2_LOADER
SCUTEST(simple_workflow_imlib2) {
    const char* path[] = {TEST_IMAGE_PREFIX "png/", NULL};
    simple_load_test(path, 1 << IMG_LOADER_IMLIB2);
}
#endif

#ifndef NO_FFMPEG_LOADER
SCUTEST(simple_workflow_ffmpeg) {
    const char* path[] = {TEST_IMAGE_PREFIX "mp4/", NULL};
    simple_load_test(path, 0);
}
#endif

#ifndef NO_MINIZ_LOADER
SCUTEST(simple_workflow_miniz) {
    const char* path[] = {TEST_IMAGE_PREFIX "zip/test_image.zip", NULL};
    simple_load_test(path, image_loader_get_nonmulti_loader_masks() | (1 << IMG_LOADER_MINIZ));
}
#endif

#ifndef NO_ARCHIVE_LOADER

SCUTEST(simple_workflow_archiver) {
    const char* path[] = {TEST_IMAGE_PREFIX "archive/", NULL};
    simple_load_test(path, image_loader_get_nonmulti_loader_masks() | (1 << IMG_LOADER_ARCHIVE));
}

#endif

#ifndef NO_ZIP_LOADER
SCUTEST(simple_workflow_zip) {
    const char* path[] = {TEST_IMAGE_PREFIX "zip/test_image.zip", NULL};
    simple_load_test(path, image_loader_get_nonmulti_loader_masks() | (1 << IMG_LOADER_MINIZ));
}
#endif

#ifndef NO_CURL_LOADER
SCUTEST(simple_workflow_curl) {
    char buffer[512] = "file://";
    int pwdLen = strlen(buffer);
    assert(getcwd(buffer + pwdLen, sizeof(buffer) - pwdLen));
    pwdLen = strlen(buffer);
    strncat(buffer, "/" TEST_IMAGE_PREFIX "png/test_image.png", sizeof(buffer) - pwdLen);
    const char* path[] = {buffer, NULL};
    simple_load_test(path, image_loader_get_nonmulti_loader_masks() | (1 << IMG_LOADER_CURL));
}
#endif

/**
 * Example of how to use loader
 */
SCUTEST(simple_workflow) {
    const char* path[] = {"tests/test_images/png/test_image.png", NULL};
    simple_load_test(path, 0);
}


SCUTEST(simple_workflow_remove_invalid) {
    ImageLoaderContext* c = image_loader_create_context(TEST_IMAGE_PATH_SOME_INVALID, 0, IMAGE_LOADER_REMOVE_INVALID);
    ImageLoaderData* current_image = NULL;
    // loop through all images; don't have to worry about going out of bounds
    for (int i = 0; i < image_loader_get_num(c); i++) {
        // close the current image and open the next;
        current_image = image_loader_open(c, i, current_image);
        // iff a valid index is passed in a non-null result will be returned because of IMAGE_LOADER_REMOVE_INVALID
        if ( i >= 0 && i < image_loader_get_num(c))
            assert(current_image);
        else
            assert(!current_image);
    }
    image_loader_destroy_context(c); // Free all resources
}

SCUTEST(simple_workflow_remove_invalid_with_helper_method) {
    const char* TEST_IMAGE_PATH_SOME_INVALID[] = {TEST_IMAGE_PATHS[0], "tests/invalid_image.png", NULL};
    ImageLoaderContext* c = image_loader_create_context(TEST_IMAGE_PATH_SOME_INVALID, 0, 0);
    int original_images = image_loader_get_num(c);
    image_loader_remove_all_invalid_images(c);
    assert(original_images != image_loader_get_num(c));
    for (int i = 0; i < image_loader_get_num(c); i++)
        assert(image_loader_open(c, i, NULL));
    image_loader_destroy_context(c); // Free all resources
}

SCUTEST(create_empty_context) {
    ImageLoaderContext* c = image_loader_create_context(NULL, 0, 0);
    // manually add from path
    assert(image_loader_add_file(c, TEST_IMAGE_PATHS[0]));

    int fd = open(TEST_IMAGE_PATHS[0], O_RDONLY | O_CLOEXEC);
    // add from fd; the fd will be owned by the lib
    assert(image_loader_add_from_fd(c, fd, "human_name"));
    assert(image_loader_open(c, 0, NULL));
    assert(image_loader_open(c, 1, NULL));
    image_loader_destroy_context(c);
}

/**
 *
 * The above tests also serve the purpose of documenting usage.
 * The following tests are aimed more aimed for verifying correctness and are more minimal
 */
static ImageLoaderContext* default_context;

void create_default_context(){
    default_context = image_loader_create_context(TEST_IMAGE_PATHS, 0, 0); // Load all files passed from stdin
}

void destroy_default_context(){
    image_loader_destroy_context(default_context);
}

SCUTEST_SET_FIXTURE(create_default_context, destroy_default_context);

SCUTEST(open_close) {}

SCUTEST(open_underflow) {
    assert(!image_loader_open(default_context, -1, NULL));
}

SCUTEST(open_overflow) {
    assert(!image_loader_open(default_context, 255, NULL));
}

SCUTEST(verify_image_data) {
    ImageLoaderData* image = image_loader_open(default_context, 0, NULL);
    assert(strcmp(TEST_IMAGE_PATHS[0], image_loader_get_name(image))==0);
    // The first test image is a 1x1 red dot with no transparency
    assert(1 == image_loader_get_height(image));
    assert(1 == image_loader_get_width(image));
    assert(0xFFFF0000 == *(int*)image_loader_get_data(image));
}

SCUTEST(close_reopen) {
    ImageLoaderData* current_image = image_loader_open(default_context, 0, NULL);
    assert(image_loader_open(default_context, 0, current_image)); // no-op
    assert(image_loader_open(default_context, 1, current_image));
    assert(image_loader_open(default_context, 0, current_image));
}

SCUTEST_SET_FIXTURE(NULL, destroy_default_context);
#if ! defined NO_MINIZ_LOADER || ! defined NO_ZIP_LOADER

SCUTEST(open_zip_normal) {
    default_context = image_loader_create_context(TEST_IMAGE_PATHS_ZIP, 1, 0);
    assert(image_loader_open(default_context, 0, NULL));
}

SCUTEST(open_zip) {
    default_context = image_loader_create_context(TEST_IMAGE_PATHS_ZIP, 0, 0);
    ImageLoaderData* image = NULL;
    /* The key part of this test is verifying we can close an empty multi-loader
     * without issue
     */
    for (int i = 0; i < image_loader_get_num(default_context); i++)
        image = image_loader_open(default_context, i, image);
}
#endif

#if ! defined NO_CURL

SCUTEST(open_url_bad) {
    default_context = image_loader_create_context(NULL, 0, IMAGE_LOADER_REMOVE_INVALID);
    assert(image_loader_add_file(default_context, "http://localhost/__not_a_real_url__"));
    assert(!image_loader_open(default_context, 0, NULL));
}
#endif

SCUTEST(open_non_null_terminated) {
    const char* path[] = {TEST_IMAGE_PATHS[0]};
    default_context = image_loader_create_context(path, 1, 0);
    assert(image_loader_get_num(default_context) == 1);
}

SCUTEST(open_dir_non_recursive) {
    default_context = image_loader_create_context(NULL, 0, IMAGE_LOADER_REMOVE_INVALID|IMAGE_LOADER_DISABLE_RECURSIVE_DIR_LOADER);
    assert(image_loader_add_file(default_context, "."));
    image_loader_open(default_context, 0, NULL);
    assert(!image_loader_get_num(default_context));
}

SCUTEST(open_pre_expand) {
    default_context = image_loader_create_context(NULL, 0, IMAGE_LOADER_PRE_EXPAND);
    assert(image_loader_add_file(default_context, "."));
    assert(image_loader_get_num(default_context) > 1);
}

SCUTEST(open_invalid) {
    default_context = image_loader_create_context(TEST_IMAGE_ALL_PATHS_INVALID, 0, IMAGE_LOADER_REMOVE_INVALID);
    while (image_loader_get_num(default_context))
        image_loader_open(default_context, 0, NULL);
}

SCUTEST(add_from_fd_open_close) {
    default_context = image_loader_create_context(0, 0, 0);
    int fd = open(TEST_IMAGE_PATHS[0], O_RDONLY | O_CLOEXEC);
    // add from fd; the fd will be owned by the lib
    assert(image_loader_add_from_fd(default_context, fd, "human_name"));
    for (int i = 0; i < 2; i++) {
        ImageLoaderData* image = image_loader_open(default_context, 0, NULL);
        assert(image);
        image_loader_close(default_context, image);
    }
}

SCUTEST(sort_images) {
    default_context = image_loader_create_context(0, 0, 0);
    assert(image_loader_add_file(default_context, TEST_IMAGE_PATHS[0]));
    int fd = open(TEST_IMAGE_PATHS[0], O_RDONLY | O_CLOEXEC);
    // add from fd; the fd will be owned by the lib
    assert(image_loader_add_from_fd(default_context, fd, "human_name"));
    ImageLoaderData* image = NULL;
    for (int i = 1; i < IMG_SORT_NUM; i++) {
        image_loader_sort(default_context, i);
        image = image_loader_open(default_context, 0, image);
        const char* firstImage = image_loader_get_name(image);
        image = image_loader_open(default_context, image_loader_get_num(default_context) -1, NULL);
        const char* lastImage = image_loader_get_name(image);
        // the negation sorts in the opposite direction
        image_loader_sort(default_context, -i);
        image = image_loader_open(default_context, image_loader_get_num(default_context) -1, NULL);
        assert(strcmp(firstImage, image_loader_get_name(image)) == 0);
        image = image_loader_open(default_context, 0, image);
        assert(strcmp(lastImage, image_loader_get_name(image)) == 0);
    }
    // Shuffling; just verify this doesn't crash
    image_loader_sort(default_context, 0);
}
