#include <stdio.h>
#include <fstream>
#include <cstdint>
#include <cstdlib>

#include <string>

#include "options.h"
#include "image.h"
#include "image_png.h"
#include "quant.h"

// TODO
// PNGImage sourceImage;
// PNGImage quantizedImage;

int main(int argc, char* argv[]) {

    int errorCode = EXIT_SUCCESS;

    // Read all arguments
    quantOptions options;

    // Make sure we had no errors
    if ((errorCode = processArgs(argc, argv, &options)) == EXIT_SUCCESS) {

        Image sourceImageRGBA;
        if ((errorCode = loadImageRGBAFromPNG(options, sourceImageRGBA)) != EXIT_SUCCESS) {
            return errorCode;
        }

        if ((errorCode = quantizeImage(options, sourceImageRGBA)) != EXIT_SUCCESS) {
            return errorCode;
        }

        // Test output
        // if ((errorCode = saveImageRGBAToPNG(&options, &sourceImageRGBA)) != EXIT_SUCCESS) {
        //     return errorCode;
        // }
    }

    return errorCode;
}












/*
 TODO

    // Display processing updates and refresh image download links along the way
    // Most of this can be skipped
        else if (data.action === Action.UpdateQuantizedImage) {
            const imageData = data.imageData;
            const quantizedImageData = new window.ImageData(imageData.width, imageData.height);
            for (let i = 0; i < imageData.data.length; i++) {
                quantizedImageData.data[i] = imageData.data[i];
            }
            quantizedImage.width = imageData.width;
            quantizedImage.height = imageData.height;
            const ctx = quantizedImage.getContext("2d");
            ctx.putImageData(quantizedImageData, 0, 0);
            if (imageData.totalPaletteColors > 256) {
                quantizedImageDownload.href = quantizedImage.toDataURL();
            }
            else {
                quantizedImageDownload.href = bmpToDataURL(imageData.width, imageData.height, imageData.paletteData, imageData.colorIndexes);
            }
        }

        else if (data.action === Action.UpdatePalettes) {
            const palettes = data.palettes;
            const paletteDisplayHeight = 16;
            const paletteDisplayWidth = Math.min(16, Math.ceil(512 / data.numColors));
            palettesImage.width = data.numColors * paletteDisplayWidth;
            palettesImage.height = data.numPalettes * paletteDisplayHeight;
            const palCtx = palettesImage.getContext("2d");
            for (let j = 0; j < palettes.length; j += 1) {
                for (let i = 0; i < palettes[j].length; i += 1) {
                    palCtx.fillStyle = `rgb(
                        ${Math.round(palettes[j][i][0])},
                        ${Math.round(palettes[j][i][1])},
                        ${Math.round(palettes[j][i][2])})`;
                    palCtx.fillRect(i * paletteDisplayWidth, j * paletteDisplayHeight, paletteDisplayWidth, paletteDisplayHeight);
                }
            }
            palettesImageDownload.href = palettesImage.toDataURL();
        }
    };


    // Process the image
    worker.postMessage({
        action: Action.StartQuantization,
        imageData: imageDataFrom(sourceImage),
        quantizationOptions: {
            tileWidth: parseInt(tileWidthInput.value, radix),
            tileHeight: parseInt(tileHeightInput.value, radix),
            numPalettes: parseInt(numPalettesInput.value, radix),
            colorsPerPalette: parseInt(colorsPerPaletteInput.value, radix),
            bitsPerChannel: parseInt(bitsPerChannelInput.value, radix),
            fractionOfPixels: parseFloat(fractionOfPixelsInput.value),
            colorZeroBehaviour: colorZeroBehaviour,
            colorZeroValue: colorZeroValue,
            dither: ditherMethod,
            ditherWeight: parseFloat(ditherWeightInput.value),
            ditherPattern: ditherPattern,
        },
    });
});

function imageDataFrom(img) {
    const canvas = document.createElement("canvas");
    const context = canvas.getContext("2d");
    canvas.width = img.width;
    canvas.height = img.height;
    context.drawImage(img, 0, 0);
    return context.getImageData(0, 0, img.width, img.height);
}

*/
