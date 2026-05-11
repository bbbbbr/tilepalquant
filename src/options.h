#pragma once

#include <string>
#include <cstring>

using namespace std;

// TODO: Implement rgbColor
#define rgbColor int

#define RAND_SEED_DEFAULT        0

#define ARG_SKIP_NONE            0
#define ARG_AT_INPUT_FILENAME    1
#define ARG_AFTER_INPUT_FILENAME 2

#define DITHER_PIXELS_4      4
#define DITHER_PIXELS_2      2
#define DITHER_WIDTH         2
#define DITHER_HEIGHT        2
#define DITHER_PATTERN_AR_SZ (DITHER_WIDTH * DITHER_HEIGHT)

// TODO: CAPS_CAPS?
//
struct Opts
{
    enum indexZeroValues {
        indexZeroUnique = 0,
        indexZeroShared,
        indexZeroTranspFromTransp,
        indexZeroTranspFromColor,
    };


    // enum colorValues = {
    //     defaultColor,          // TODO: RGB(0,0,0)
    //     sharedColor,           // TODO: RGB() from user
    //     transparentColorShim,  // TODO: Shim for transparentFromTransparent
    //     transparentColor,      // TODO: RGB() from user
    // };

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
    int      tileWidth;
    int      tileHeight;
    int      numPalettes;
    int      colorsPerPalette;
    int      bitsPerChannel;
    float    fractionOfPixels;

    int      colorZeroBehaviour;
    rgbColor colorZeroRGB; // TODO: = hexToColor(colorInput.value);
    rgbColor sharedColorRGB;
    rgbColor transparentColorRGB;


    int      ditherMethod;
    int      ditherPatternType;
    int      ditherPixels;
    uint8_t  ditherPattern[DITHER_WIDTH][DITHER_HEIGHT];
    float    ditherWeight;

    int      totalPaletteColors; // TODO: derived from numPalettes * colorsPerPalette

    string   sourceImageFilename;
    string   outputImageFilename;


    // Options unique to the console port
    string   argsForLoggingToOutput;
    string   outputLogArgsFilename;

    int      randomSeed;

    bool     use_metafile;

    bool     verbose;
};


// Option defaults
#define TILE_WIDTH_DEFAULT         8
#define TILE_HEIGHT_DEFAULT        8
#define NUM_PALETTES_DEFAULT       8
#define COLORS_PER_PALETTE_DEFAULT 4
#define BITS_PER_CHANNEL_DEFAULT   5

#define FRACTION_OF_PIXELS_DEFAULT    0.50
#define COLOR_ZERO_BEHAVIOUR_DEFAULT  Opts::indexZeroUnique
#define COLOR_ZERO_RGB_DEFAULT        ; // TODO: hexToColor(colorInput.value); RGB0,0,0)
#define SHARED_COLOR_RGB_DEFAULT      ;
#define TRANSPARENT_COLOR_RGB_DEFAULT ;
#define DITHER_METHOD_DEFAULT         Opts::ditherOff
#define DITHER_PATTERN_DEFAULT        Opts::ditherDiagonal4
#define DITHER_WEIGHT_DEFAULT         0.50

int processArgs(int argc, char* argv[], quantOptions * options);

