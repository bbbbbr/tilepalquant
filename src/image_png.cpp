#include <stdio.h>
#include <vector>

#include "options.h"

#include "lodepng.h"
#include "image.h"

// Load png and return RGBA8888 buffer along with width and height of image
int loadImageRGBAFromPNG(quantOptions & options, Image & sourceImageRGBA) {

    //load and decode png
    vector<unsigned char> buffer;
    lodepng::load_file(buffer, options.sourceImageFilename);
    lodepng::State state;

    // Decode as 32 bit RGBA
    unsigned error = lodepng::decode(sourceImageRGBA.data,
                                     sourceImageRGBA.width, sourceImageRGBA.height,
                                     state, buffer);
    if (error) {
        printf("decoder error %s\n", lodepng_error_text(error));
        return EXIT_FAILURE;
    }

    // Validate image dimensions
    if (((sourceImageRGBA.width  % options.tileWidth)  != 0) ||
        ((sourceImageRGBA.height % options.tileHeight) != 0)) {
        printf("Error: Image size %d x %d isn't an even multiple of tile size %d x %d\n",
                sourceImageRGBA.width, sourceImageRGBA.height,
                options.tileWidth, options.tileHeight);
        return EXIT_FAILURE;
    }

            // const unsigned char* color = &image.data[(j * image.w + i) * RGBA32_SZ];
            // int color_int = RGBA32(color[RGBA8_R], color[RGBA8_G], color[RGBA8_B], color[RGBA8_ALPHA]);

            // for(unsigned int y = 0; y < sourceImageRGBA.h; y += sourceImageRGBA.tile_h * sy)
            // {
            //     for(unsigned int x = 0; x < sourceImageRGBA.w; x += sourceImageRGBA.tile_w * sx)
                // {

    if (options.verbose) {
        printf("PNG Loaded: %s: Image size %d x %d, tile size %d x %d\n",
                options.sourceImageFilename.c_str(),
                sourceImageRGBA.width, sourceImageRGBA.height, options.tileWidth, options.tileHeight);
    }

    return EXIT_SUCCESS;
}


// Load png and return RGBA8888 buffer along with width and height of image
int saveImageRGBAToPNG(quantOptions & options, Image & image) {

    int colorCount = image.paletteData.size(); // paletteData is an array of rgb888 (3x8 bits) colors
    if ((colorCount > 0) && (colorCount <= 256)) {

        // Note: ".totalPaletteColors" reflects the maximum possible output quantized palette size if all colors are populated, but in various initial iterations they may not be

        if (options.verbose) printf("PNG export: Index Mode\n");
        if (options.verbose) printf("PNG export: .paletteData.size() = %d, Calculated color count = %d\n", (int)image.paletteData.size(), colorCount);
        // if (options.verbose) printf("PNG export: .totalPaletteColors = %d, .paletteData.size() = %d, calculated count = %d\n", options.totalPaletteColors, (int)image.paletteData.size(), colorCount);

        lodepng::State png_state;

        // Loop through colors and add them to the png palette
        for (int c = 0; c < colorCount; c++) {
            if (options.verbose) printf("PNG export: adding color %d : %3hu, %3hu, %3hu\n", c,
                                image.paletteData[c].ch.r,  // r
                                image.paletteData[c].ch.g,  // g
                                image.paletteData[c].ch.b); // b

            lodepng_palette_add(&png_state.info_png.color,
                                image.paletteData[c].ch.r, // r
                                image.paletteData[c].ch.g, // g
                                image.paletteData[c].ch.b, // b
                                ALPHA_FULLY_OPAQUE);                        // alpha (fully opaque)
            lodepng_palette_add(&png_state.info_raw,
                                image.paletteData[c].ch.r, // r
                                image.paletteData[c].ch.g, // g
                                image.paletteData[c].ch.b   , // b
                                ALPHA_FULLY_OPAQUE);                        // alpha (fully opaque)
        }


        // lodepng options: going from RAW to indexed PNG
        png_state.info_raw.colortype = LCT_PALETTE;
        png_state.info_raw.bitdepth = 8;


        // Palette must be added both to input and output color mode, because in this
        // Sample both the raw image and the expected PNG image use that palette.
        png_state.info_png.color.colortype = LCT_PALETTE;
        png_state.info_png.color.bitdepth = 8;
        png_state.encoder.auto_convert = 0;  // False: Specify exactly what output PNG color mode we want

        // Encode and save
        // unsigned error = lodepng::encode(&p_png_image,
        //                        &png_size_bytes,
        //                        p_image.p_img_data,
        //                        p_image.width, p_image.height,
        //                        &png_state);

        std::vector<unsigned char> buffer;
        unsigned error = lodepng::encode(buffer, &image.colorIndexes[0], image.width, image.height, png_state);

        if (error) {
            if (options.verbose) printf("PNG export: encoder error %s\n", lodepng_error_text(error));
        }
        else {
            if (options.verbose) printf("PNG export: Writing output image to png file: %d x %d, to %s\n", image.width, image.height, options.outputImageFilename.c_str());
            // lodepng_save_file(png_image, png_size_bytes, filename_out);
            lodepng::save_file(buffer, options.outputImageFilename);
        }

        // Free resources
        lodepng_state_cleanup(&png_state);

    }
    else {
    // TODO: DEBUG: Note: The RGB output can be forced to show a more detailed preview image that doesn't have as many palette constraints yet applied
        if (options.verbose) printf("PNG export: Non-indexed (RGBA8888) Mode\n");

        //Test: output png after reading it
        unsigned error = lodepng::encode(options.outputImageFilename, image.data, image.width, image.height);

        if (error) {
            printf("PNG encoder error %s\n", lodepng_error_text(error));
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}