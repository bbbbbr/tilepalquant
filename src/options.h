#pragma once

using namespace std;

// TODO: Implement rgbColor
#define rgbColor int

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
    int      ditherPattern;
    float    ditherWeight;
    
    int      totalPaletteColors; // TODO: derived from numPalettes * colorsPerPalette
    
    string   sourceImageFilename;
    string   outputImageFilename;
    
    
    // Options unique to the console port
    string   argsForLoggingToOutput;
    string   outputLogArgsFilename;
    
    int      randomSeed;

    bool     use_metafile;
};


int processArgs(int argc, char* argv[], quantOptions * options);





