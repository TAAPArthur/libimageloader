# An abstraction around image loading

This project is meant to provide a common interface for loading various image formats provided by various backbends. I noticed a lot of projects duplicated the same code to interface with real image loaders (libjpeg, libpng, etc) and thought that was a waste as it is generally uninteresting to the actual goal of that project. Other populate abstraction layers like Imlib2 are just huge and bloated. This aims to be a really thin wrapper.

The main goal is for this to be used as a backend for image viewers. It is not mean to scale, convert, resize or render images.

# Make & install
```
./configure
make
make install
```

# Features
* Load many formats including png, jpeg, and gif
* Load from directories
* Load remote images (requires libcurl)
* Load zip archives which includes comic book and epub formats (images only) (requires libzip or miniz)
* Load from pipe
* All dependencies can be compiled out

# Backends
* [spng](https://libspng.org/) (png)
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) (jpeg, png, tga, bmp, psd, gif, hdr, pic, pnm)
* [miniz](https://github.com/richgel999/miniz) (zip, cbz, epub)
* [libzip](https://libzip.org/) (zip, cbz, epub)
* [libcurl](https://curl.se/) (remote images)
* [Imlib2](https://docs.enlightenment.org/api/imlib2/html/) (peg, gif, ppm, pgm, xpm, png, tiff and eim)

More backends can be added given they follow the requirement of being able to be integrated in under 50 lines, don't have dependencies, and don't allocate memory unless they are used and release all memory when no longer needed. Imlib2 and libcurl break these last requirements. Being thread safe is a plus which libcurl also breaks.

# Converting
Keep in mind that if you convert from another backend like Imlib2, you'll lose features. This library doesn't support rendering to an X display, scaling nor converting an image. You give this library a path/fd and it gives you the image width/height and decoded image data.You have to do the rendering/scaling yourself. Note [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) has an easy api to do resizing.
