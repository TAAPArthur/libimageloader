# An abstraction around image loading

This project is meant to provide a common interface for loading various image formats provided by various backbends. I noticed a lot of projects duplicated the same code to interface with real image loaders (libjpeg, libpng, etc) and thought that was a waste as it is generally uninteresting to the actual goal of that project. Other populate abstraction layers like Imlib2 are just huge and bloated. This aims to be a really thin wrapper.

The main goal is for this to be used as a backend for image viewers. It is not mean to scale, convert, resize or render images.

# Make & install
```
./configure
make
make install
```
# Example usage
See [tests/test.c] for a more detailed example. Here is some sample usage:
```
void* c = image_loader_create_context(argv, 0, 0); // Load all files passed from stdin
void* current_image = image_loader_open_image(c, 0, NULL); // Open the first image
image_loader_get_data(current_image) // Retrive RGBA32 uncompressed image data
image_loader_destroy_context(c); // Free all resources
```

# Features
* Load many formats including png, jpeg, and gif
* Load from directories
* Load remote images (requires libcurl)
* Load zip archives which includes comic book and epub formats (images only) (requires libzip or miniz)
* Load from pipe (only a single image due to limitation from libs)
* All dependencies can be compiled out

# Backends
* [farbfeld](https://tools.suckless.org/farbfeld/) (farbfeld)
* [ffmpeg](https://ffmpeg.org/) (video formats)
* [Imlib2](https://docs.enlightenment.org/api/imlib2/html/) (many image formats)
* [libcurl](https://curl.se/) (remote files)
* [libzip](https://libzip.org/)  (compressed formats)
* [miniz](https://github.com/richgel999/miniz) (compressed formats)
* [spng](https://libspng.org/) (png)
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) (many image formats)

More backends can be added given they follow the requirement of being able to be integrated in under 50 lines, don't have dependencies, and don't allocate memory unless they are used and release all memory when no longer needed. Imlib2 and libcurl break these last requirements. Being thread safe is a plus which libcurl also breaks.


# Why
Loading images is a task that many programs need to do, but itself isn't interesting. Even though there are dedicated libraries to load given formats, they aren't all the easiest to interface with and there are many to choose from with the less popular solutions generally being simplifier and faster. Instead of making application developers make the choice of which library to use (and therefore which image formats to support), this project aims to be a thin wrapper around many of them to allow the user to use the application they want with the backend they want.

For example suppose you were using library X which supports image formats A, B, C and you are developing something like an image viewer. Suppose a user comes to you and requests that you add support for format D. Instead of you having to judge whether the extra support is worth it, if you use this library they could make the request here and your application would then be able to support that new format. Or suppose that a library X has a security vulnerability or is just slow and the end user wants to fix that. If this library was used, the user could replace the backend with a more secure/faster one seamlessly.

It also covers common cases like directory loading, loading from stdin and loading from archives, so you don't have to re-invent the wheel for common features.

# Why Not
This library is all about giving users choice. If your users don't care as in they are fine with whatever dependencies you force on them, then this library isn't for you.


# Converting
Keep in mind that if you convert from another backend like Imlib2, you'll lose features. This library doesn't support rendering to an X display, scaling nor converting an image. You give this library a path/fd and it gives you the image width/height and decoded image data.You have to do the rendering/scaling yourself. Note [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) has an easy api to do resizing.


# FAQ
## Does this library support cache
No. We ref count images, so if you don't explicitly close an image, it will be "cached" and load quickly if revisited. There isn't a LRU or size limited cache. Although the image size is available so you could do something more complex if desired

## I don't like the fact that library X is a dependency
That's great. You don't have to use it. All dependencies are optional. Running `./configure` will auto detect installed dependency and auto enable them so if you don't want to install library X, don't and the program will still build. And if you have it installed, but don't want to link with it, you can disable a specific lib by passing `HAVE_X=0` ie `HAVE_ZIP=0` will never link against libzip even if it is installed.

## OK I have the image, but what do I do with it
The data is returned [RGBA32](https://en.wikipedia.org/wiki/RGBA_color_model#RGBA32) format which is what many displays like X readily accept. For X11, the functions you may be interested in are `X{Create,Init,Put,Destroy}Image` or for xcb `xcb_image_{create_native,xcb_image_put,destroy}`
