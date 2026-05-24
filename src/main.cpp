#include <stdio.h>
#include <fstream>
#include <cstdint>
#include <cstdlib>

#include <string>

#include "options.h"
#include "image.h"
#include "image_png.h"
#include "quant.h"

int main(int argc, char* argv[]) {

    int errorCode = EXIT_SUCCESS;

    // Read all arguments
    quantOptions options;

    // Make sure we had no errors
    if ((errorCode = processArgs(argc, argv, options)) == EXIT_SUCCESS) {

        if (options.sourceImageFilename.size()) {
            Image sourceImageRGBA;
            if ((errorCode = loadImageRGBAFromPNG(options, sourceImageRGBA)) != EXIT_SUCCESS) {
                return errorCode;
            }

            if ((errorCode = quantizeImage(options, sourceImageRGBA)) != EXIT_SUCCESS) {
                return errorCode;
            }
        }

        // Test output
        // if ((errorCode = saveImageRGBAToPNG(&options, &sourceImageRGBA)) != EXIT_SUCCESS) {
        //     return errorCode;
        // }
    }

    return errorCode;
}
