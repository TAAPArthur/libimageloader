#define SCUTEST_IMPLEMENTATION
#define SCUTEST_NO_BUFFER
#define SCUTEST_DEFINE_MAIN
#include <assert.h>
#include <fcntl.h>
#include <scutest/scutest.h>
#include "../img_loader.h"

const char* TEST_IMAGE_PATHS[] = {"tests/test_image.png", "tests/test_image.png", ".", NULL};
const char* TEST_IMAGE_PATH_SOME_INVALID[] = {"tests/test_image.png", "tests/invalid_image.png", ".", NULL};
const char* TEST_IMAGE_PATHS_ZIP[] = {"tests/test_image.zip", "tests/empty.zip", "tests/test_image.zip", NULL};

const char* TEST_IMAGE_ALL_PATHS_INVALID[] = {"tests/invalid_image.png", "another_bad_image.bad", NULL};

/**
 * Example of how to use loader
 */
SCUTEST(simple_workflow) {
    // Load all files passed TEST_IMAGE_PATHS; TEST_IMAGE_PATHS is in the same
    // format as argv from main()
    ImageLoaderContext* c = image_loader_create_context(TEST_IMAGE_PATHS, 0, 0);
    ImageLoaderData* current_image = image_loader_open(c, 0, NULL); // Open the first image
    // Do stuff like draw the image
    current_image = image_loader_open(c, 1, current_image); // Open the 2nd image and close the first
    // // repeat until satisfied then
    image_loader_destroy_context(c); // Free all resources
}

SCUTEST(simple_workflow_remove_invalid) {
    ImageLoaderContext* c = image_loader_create_context(TEST_IMAGE_PATH_SOME_INVALID, 0, IMAGE_LOADER_REMOVE_INVALID);
    ImageLoaderData* current_image = NULL;
    // loop through all images; don't have to worry about going out of bounds
    for(int i = -1; i < image_loader_get_num(c) + 1; i++) {
        // close the current image and open the next;
        current_image = image_loader_open(c, i, current_image);
        // iff a valid index is passed in a non-null result will be returned because of IMAGE_LOADER_REMOVE_INVALID
        if( i >= 0 && i < image_loader_get_num(c))
            assert(current_image);
        else
            assert(!current_image);
    }
    image_loader_destroy_context(c); // Free all resources
}

SCUTEST(create_emtpy_context) {
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
    for(int i = 0; i < image_loader_get_num(default_context); i++)
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
    while(image_loader_get_num(default_context))
        image_loader_open(default_context, 0, NULL);
}
