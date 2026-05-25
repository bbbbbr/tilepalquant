#pragma once

#include <string>
#include <cstring>
#include "image.h"

using namespace std;

#define RGBCOL(R,G,B) R,G,B

#define RAND_SEED_DEFAULT        0

#define ARG_SKIP_NONE            0
#define ARG_AT_INPUT_FILENAME    1
#define ARG_AFTER_INPUT_FILENAME 2

#define DITHER_PIXELS_4      4
#define DITHER_PIXELS_2      2
#define DITHER_WIDTH         2
#define DITHER_HEIGHT        2
#define DITHER_PATTERN_AR_SZ (DITHER_WIDTH * DITHER_HEIGHT)

#define RGBA_ALPHA_MAX       255

#define RGBA_RED    0
#define RGBA_GREEN  1
#define RGBA_BLUE   2
#define RGBA_ALPHA  3


struct Opts
{
    enum indexZeroValues {
        indexZeroUnique = 0,
        indexZeroShared,
        indexZeroTranspFromTransp,
        indexZeroTranspFromColor,
    };


    enum ditherModes {
        ditherOff = 0,
        ditherFast,
        ditherSlow
    };

    enum ditherPatternValues {
        ditherDiagonal4 = 0,
        ditherHorizontal4,
        ditherVertical4,
        ditherDiagonal2,
        ditherHorizontal2,
        ditherVertical2,

        ditherPatternsMax = ditherVertical2,
        ditherPatternsCount
    };
};

struct quantOptions {
    // const body = document.getElementById("body");
    // const imageSelector = document.getElementById("image_selector");
    unsigned int  tileWidth;
    unsigned int  tileHeight;
    unsigned int  numPalettes;
    unsigned int  colorsPerPalette;
    unsigned int  bitsPerChannel;
    float         fractionOfPixels;

    unsigned int  colorZeroBehaviour;
    rgbColor    colorZeroValue;
    rgbColor    sharedColorInput;
    rgbColor    transparentColorInput;


    unsigned int  ditherMethod;
    unsigned int  ditherPatternType;
    unsigned int  ditherPixels;
    uint8_t       ditherPattern[DITHER_WIDTH][DITHER_HEIGHT];
    float         ditherWeight;

    unsigned int  totalPaletteColors; // Note: derived from numPalettes * colorsPerPalette

    string        sourceImageFilename;
    string        outputImageFilename;


    // Options unique to the console port
    string        argsForLoggingToOutput;
    string        outputLogArgsFilename;

    unsigned int randomSeed;

    bool         use_metafile;
    bool         verbose;
    bool         verboseDebug;
    bool         exportPreviews;
};


#define TILE_WIDTH_MIN             1
#define TILE_WIDTH_MAX             512
#define TILE_HEIGHT_MIN            1
#define TILE_HEIGHT_MAX            512
#define NUM_PALETTES_MIN           1
#define NUM_PALETTES_MAX           64
#define COLORS_PER_PALETTE_MIN     2
#define COLORS_PER_PALETTE_MAX     256
#define BITS_PER_CHANNEL_MIN       2
#define BITS_PER_CHANNEL_MAX       8
#define FRACTION_OF_PIXELS_MIN     0.01
#define FRACTION_OF_PIXELS_MAX     10.0
#define DITHER_WEIGHT_MIN          0.01
#define DITHER_WEIGHT_MAX          1.0

// Option defaults
#define TILE_WIDTH_DEFAULT         8
#define TILE_HEIGHT_DEFAULT        8
#define NUM_PALETTES_DEFAULT       8
#define COLORS_PER_PALETTE_DEFAULT 4
#define BITS_PER_CHANNEL_DEFAULT   5

#define FRACTION_OF_PIXELS_DEFAULT    0.1
#define COLOR_ZERO_BEHAVIOUR_DEFAULT  Opts::indexZeroUnique
// Note: with C++20 designated inits could be used
#define COLOR_ZERO_RGB_DEFAULT        {RGBCOL(0, 0, 0)}
#define SHARED_COLOR_RGB_DEFAULT      {RGBCOL(0, 0, 0)}
#define TRANSPARENT_COLOR_RGB_DEFAULT {RGBCOL(255, 0, 255)}
#define DITHER_METHOD_DEFAULT         Opts::ditherOff
#define DITHER_PATTERN_DEFAULT        Opts::ditherDiagonal4
#define DITHER_WEIGHT_DEFAULT         0.50

int processArgs(int argc, char* argv[], quantOptions & options);

