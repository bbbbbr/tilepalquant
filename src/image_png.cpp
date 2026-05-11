#include <stdio.h>
#include <vector>

#include "options.h"

#include "lodepng.h"
#include "image.h"

// Load png and return RGBA8888 buffer along with width and height of image
int loadImageRGBAFromPNG(quantOptions * options, Image * sourceImageRGBA) {

    //load and decode png
    vector<unsigned char> buffer;
    lodepng::load_file(buffer, options->sourceImageFilename);
    lodepng::State state;

    // Decode as 32 bit RGBA
    unsigned error = lodepng::decode(sourceImageRGBA->data,
                                     sourceImageRGBA->width, sourceImageRGBA->height,
                                     state, buffer);
    if (error) {
        printf("decoder error %s\n", lodepng_error_text(error));
        return EXIT_FAILURE;
    }

    // Validate image dimensions
    if (((sourceImageRGBA->width  % options->tileWidth)  != 0) ||
        ((sourceImageRGBA->height % options->tileHeight) != 0)) {
        printf("Error: Image size %d x %d isn't an even multiple of tile size %d x %d\n",
                sourceImageRGBA->width, sourceImageRGBA->height,
                options->tileWidth, options->tileHeight);
        return EXIT_FAILURE;
    }

            // const unsigned char* color = &image.data[(j * image.w + i) * RGBA32_SZ];
            // int color_int = RGBA32(color[RGBA8_R], color[RGBA8_G], color[RGBA8_B], color[RGBA8_ALPHA]);

            // for(unsigned int y = 0; y < sourceImageRGBA.h; y += sourceImageRGBA.tile_h * sy)
            // {
            //     for(unsigned int x = 0; x < sourceImageRGBA.w; x += sourceImageRGBA.tile_w * sx)
                // {

    if (options->verbose) {
        printf("Loaded %s: Image size %d x %d, tile size %d x %d\n",
                options->sourceImageFilename.c_str(),
                sourceImageRGBA->width, sourceImageRGBA->height, options->tileWidth, options->tileHeight);
    }

    return EXIT_SUCCESS;
}


// Load png and return RGBA8888 buffer along with width and height of image
int saveImageRGBAToPNG(quantOptions * options, Image * sourceImageRGBA) {
    //Test: output png after reading it
    unsigned error = lodepng::encode(options->outputImageFilename, sourceImageRGBA->data,
                                     sourceImageRGBA->width, sourceImageRGBA->height);
    if (error) {
        printf("encoder error %s\n", lodepng_error_text(error));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}