#include <stdio.h>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <math.h>

#include "common.h"
#include "options.h"
#include "image.h"
#include "tile.h"
#include "randomshuffle.h"
#include "quant.h"

quantOptions options;

static void updateProgress(float progress);
static void updateQuantizedImage(Image & image);
static void updatePalettes(const vector <vector <rgbColor>> & palettes, const bool doSorting);

static void movePalettesCloser(vector <vector <rgbColor>> & palettes, vector <Tile> & tiles, pixelEntry & pixel, float alpha);

static vector <vector <rgbColor>> reducePalettes(const vector <vector <rgbColor>> & palettes, unsigned int bitsPerChannel);
static vector <vector <rgbColor>> sortPalettes(const vector <vector <rgbColor>> palettes, const int startIndex);

static void reverse(vector <int> & a, int left, int right);
static double toLinear(double x);
static void toLinearColor(rgbColor & color);
static double toSrgb(double x);
static void toSrgbColor(rgbColor & color);
static double brightness(const rgbColor & color);
static vector <vector <rgbColor>> replaceWeakestColors(const vector <vector <rgbColor>> & palettes, vector <Tile> & tiles, float minColorFactor, float minPaletteFactor, bool replacePalettes);

static float meanSquareError(const vector <vector <rgbColor>> & palettes, const vector <Tile> & tiles);
static float meanSquareErrorDither(const vector <vector <rgbColor>> & palettes, const vector <Tile> & tiles);
static Candidate getClosestColor(const vector <rgbColor> & palette, const rgbColor & color);
static Candidate getClosestColorDither(const vector <rgbColor> & palette, const pixelEntry & pixel);
static double colorDistance(const rgbColor & a, const rgbColor & b);
static double paletteDistance(const vector <rgbColor> & palette, const Tile & tile);
static double paletteDistanceDither(const vector <rgbColor> & palette, const Tile & tile);
static int getClosestPaletteIndex(const vector <vector <rgbColor>> & palettes, const Tile & tile);
static Candidate closestPaletteDistance(const vector <vector <rgbColor>> & palettes, const Tile & tile);
static int getClosestPaletteIndexDither(const vector <vector <rgbColor>> & palettes, const Tile & tile);
static Candidate closestPaletteDistanceDither(const vector <vector <rgbColor>> & palettes, const Tile & tile);

// Truncate value to N bits
static rgbColor getColor(const Image & image, const unsigned int x, const unsigned int y);
static bool isPixelTransparent(const Image & image, const unsigned int x, const unsigned int y);
static bool isColorTransparent(const rgbColor & color);
static Tile extractTile(const Image & image, const unsigned int startX, const unsigned int startY, const int tile_id);
static void extractTiles(Image & image, vector <Tile> & tiles);
static bool equalColors(const rgbColor & c1, const rgbColor & c2);
static void extractAllPixels(vector <Tile> & tiles, vector <pixelEntry> & pixels);
static Image quantizeTiles(const vector <vector <rgbColor>> & palettes, const Image & image, const bool useDither);
static void addPngColors(vector <vector <rgbColor>> & palettes, vector <rgbColorU8> & pngPalette, int adjustedIndex);
static void colorQuantize1Color(vector <Tile> & tiles, vector <pixelEntry> & pixels, RandomShuffle & randomShuffle, vector <vector <rgbColor>> & palettes);
static void expandPalettesByOneColor(vector <vector <rgbColor>> & palettes, vector <Tile> & tiles, vector <pixelEntry> & pixels, RandomShuffle & randomShuffle);

static rgbColor cloneColor(const rgbColor & color);
static void copyColor(rgbColor & dest, const rgbColor & source);
static void addColor(rgbColor & c1, const rgbColor & c2);
static void addColor(rgbColor & c1, const rgbColor & c2);
static void subtractColor(rgbColor & c1, const rgbColor & c2);
static void scaleColor(rgbColor & color, const double scaleFactor);
static void clampColor(rgbColor & color, const double minValue, const double maxValue);
static uint8_t toNbitU8(uint8_t value, unsigned int n);
static double toNbit(double value, unsigned int n);
static void toNbitColor(rgbColor & color, unsigned int n);

static void moveColorCloser(rgbColor & color, rgbColor & pixelColor, float alpha);

static int maxIndexDbl(vector <double> values);
static int minIndexDbl(vector <double> values);

// Not part of original JS version
static Tile & getParentTile(vector <Tile> & tiles, const pixelEntry & pixel);
static double randRange0to1(void);
static int    indexOf(const vector <int> & vec, int matchValue);
static rgbColorU8 rgbColorToU8(const rgbColor & col);
static void   printPalettes(const vector <vector <rgbColor>> & palettes);

/*

                        onmessage = function (event) {
                            updateProgress(0);
                            const data = event.data;
                            quantizationOptions = data.quantizationOptions;
                            ditherPattern = ditherPatterns.get(options.ditherPattern);
                            const patternPixels2 = new Set([
                                DitherPattern.Diagonal2,
                                DitherPattern.Horizontal2,
                                DitherPattern.Vertical2,
                            ]);
                            if (patternPixels2.has(options.ditherPattern)) {
                                ditherPixels = 2;
                            }
                            quantizeImage(data.imageData);
                            updateProgress(100);
                            postMessage({ action: Action.DoneQuantization });
                        };

*/
// function updateProgress(progress) {
static void updateProgress(float progress) {
    // postMessage({ action: Action.UpdateProgress, progress: progress });
    const int prog_bar_size = 20;
    const int prog_reached = (int)((float)(prog_bar_size / 100.0) * progress);
    if (options.verbose) printf("* Progress: %%%0.0f |", progress);
    int i = 1;
    while (i++ < prog_reached)  printf("#");
    while (i++ < prog_bar_size) printf("-");
    printf("|\n");

}

static void updateQuantizedImage(Image & image) {
//    postMessage({ action: Action.UpdateQuantizedImage, imageData: image });
    // TODO: TEMP: DEBUG: export a PNG as progress
    if (options.verbose) printf("* Shim: UpdateQuantizedImage() - writing out test png of intermediate processed image1\n");
    saveImageRGBAToPNG(options, image);
}

// function updatePalettes(palettes, doSorting) {
static void updatePalettes(const vector <vector <rgbColor>> & palettes, const bool doSorting) {
    vector <vector <rgbColor>> pal = palettes;

    if (options.verbose) printf("updatePalettes()\n");

    int startIndex = 0;
    if ((options.colorZeroBehaviour == Opts::indexZeroTranspFromColor) ||
        (options.colorZeroBehaviour == Opts::indexZeroTranspFromTransp)) {
        startIndex = 1;
        // for (const palette of pal) {
        for (vector <rgbColor> palette : pal) {
            // palette.unshift(cloneColor(options.colorZeroValue));
            palette.insert(palette.begin(), cloneColor(options.colorZeroValue));
        }
    }
    if (options.colorZeroBehaviour == Opts::indexZeroShared) {
        startIndex = 1;
    }
    if (doSorting) {
        pal = sortPalettes(pal, startIndex);
    }
    // @ CURRENT LOC HERE
/*
    // TODO: Postmessage update handling... (is it needed?), Don't really need a preview of the palette image since it's embedded in the indexed PNG (at least for <= 256 colors)
    postMessage({
        action: Action.UpdatePalettes,
        palettes: pal,
        numPalettes: options.numPalettes,
        numColors: options.colorsPerPalette,
    });
*/
}

static void movePalettesCloser(vector <vector <rgbColor>> & palettes, vector <Tile> & tiles, const pixelEntry & pixel, float alpha) {
    int sharedColorIndex = -1;
    if (options.colorZeroBehaviour == Opts::indexZeroShared) {
        sharedColorIndex = 0;
    }
    int closestPaletteIndex = -1;
    int closestColorIndex = -1;
    rgbColor targetColor;
    if (options.ditherMethod == Opts::ditherSlow) {
        closestPaletteIndex = getClosestPaletteIndexDither(palettes, getParentTile(tiles, pixel));
        // [closestColorIndex, , targetColor]
        Candidate result = getClosestColorDither(palettes[closestPaletteIndex], pixel);
        closestColorIndex = result.colorIndex;
        targetColor       = result.comparedColor;
    }
    else {
        closestPaletteIndex = getClosestPaletteIndex(palettes, getParentTile(tiles, pixel));
        // [closestColorIndex]
        Candidate result = getClosestColor(palettes[closestPaletteIndex], pixel.color);
        closestColorIndex = result.colorIndex;
        targetColor = pixel.color;
    }
    if (closestColorIndex != sharedColorIndex) {
        moveColorCloser(palettes[closestPaletteIndex][closestColorIndex], targetColor, alpha);
    }
}


// function quantizeImage(image) {
int quantizeImage(quantOptions & quantizationOptions, Image & image) {

    options = quantizationOptions; // Assign to global to avoid passing it down so many levels
    Image reducedImageData = image;

    const bool useDither = options.ditherMethod != Opts::ditherOff;
    if (useDither) {
        // If using dither, don't apply bit depth reduction immediately
        //
        // Copy already happed automatically on var instantiation above
        // for (let i = 0; i < image.data.length; i++) {
        //     reducedImageData.data[i] = image.data[i];
        // }
        if (options.verbose) printf("Using Dither\n");
    }
    else {
        if (options.verbose) printf("No Dither\n");
        for (int i = 0; i < (int)image.data.size(); i++) {
            // TODO: Seems to expect each item in the array to be an RGB (OR RGBA ?) entry
            // If RGBA, why quantizing the Alpha channel?
            reducedImageData.data[i] = toNbitU8(image.data[i], options.bitsPerChannel);
        }
    }


    // const tiles = extractTiles(reducedImageData);
    vector <Tile> tiles;
    extractTiles(reducedImageData, tiles);

    if (options.verbose) {
      float avgPixelsPerTile = 0;
      for (const Tile & tile : tiles) {
         avgPixelsPerTile += tile.colors.size();
      }
      avgPixelsPerTile /= tiles.size();
      printf("Colors per tile: %0.2f\n", avgPixelsPerTile);
   }

    if (options.verbose) printf("extractAllPixels()\n");  // DEBUG
    vector <pixelEntry> pixels;
    extractAllPixels(tiles, pixels);

    if (options.verbose) printf("randomShuffle.init()\n");  // DEBUG
    RandomShuffle randomShuffle;
    randomShuffle.init(pixels.size());

    const bool showProgress = true;
    int iterations = (int)(options.fractionOfPixels * (float)pixels.size());  // TODO: Wonder if this could be an option knob for quality/speed tradeoff
    float alpha = 0.3;
    float finalAlpha = 0.05;

    // This appears to be turned off, but it would swap out the mean square function used based on dither setting
    // Not going to implement it for now since it's unused
    // const meanSquareErr = meanSquareError;
    #define meanSquareErrSelected meanSquareError
    if (options.ditherMethod == Opts::ditherSlow) {
        // meanSquareErr = meanSquareErrorDither;  // Commented out in JS source
        iterations /= 5;
        alpha = 0.1;
        finalAlpha = 0.02;
    }

    const float minColorFactor = 0.5;
    const float minPaletteFactor = 0.5;
    const int replaceIterations = 10;
    const bool useMin = true;
    unsigned int prog[] = {25, 65, 90, 100};  // TODO: What are these doing?
    if (options.ditherMethod == Opts::ditherOff) {
        prog[3] = 94;
    }
    if (options.verbose) printf("colorQuantize1Color()\n");  // DEBUG
    vector <vector <rgbColor>> palettes;
    colorQuantize1Color(tiles, pixels, randomShuffle, palettes);

    int startIndex = 2;
    if (options.colorZeroBehaviour == Opts::indexZeroShared) {
        startIndex += 1;
    }
    int endIndex = options.colorsPerPalette;
    if ((options.colorZeroBehaviour == Opts::indexZeroTranspFromColor) ||
         (options.colorZeroBehaviour == Opts::indexZeroTranspFromTransp)) {
        endIndex -= 1;
    }
    updateProgress(prog[0] / options.numPalettes);
    updatePalettes(palettes, false);

    if (showProgress) {
        Image reducedOutput = quantizeTiles(palettes, reducedImageData, false);
        updateQuantizedImage(reducedOutput);
    }
    for (int numColors = startIndex; numColors <= endIndex; numColors++) {
        expandPalettesByOneColor(palettes, tiles, pixels, randomShuffle);
        updateProgress((prog[0] * numColors) / options.colorsPerPalette);
        updatePalettes(palettes, false);
        if (showProgress) {
            Image reducedOutput = quantizeTiles(palettes, reducedImageData, false);
            updateQuantizedImage(reducedOutput);
        }
    }

    float minMse = meanSquareErrSelected(palettes, tiles);
    vector <vector <rgbColor>> minPalettes = palettes;
    for (int i = 0; i < replaceIterations; i++) {
        palettes = replaceWeakestColors(palettes, tiles, minColorFactor, minPaletteFactor, true);

        for (int iteration = 0; iteration < iterations; iteration++) {
            const pixelEntry nextPixel = pixels[randomShuffle.next()];
            movePalettesCloser(palettes, tiles, nextPixel, alpha);
        }
        const float mse = meanSquareErrSelected(palettes, tiles);
        if (mse < minMse) {
            minMse = mse;
            minPalettes = palettes;
        }
        updateProgress(prog[0] + ((prog[1] - prog[0]) * (i + 1)) / replaceIterations);
        updatePalettes(palettes, false);
        if (showProgress) {
            if (useMin && (i == (replaceIterations - 1))) {
                Image reducedOutput = quantizeTiles(minPalettes, reducedImageData, false);
                updateQuantizedImage(reducedOutput);
            }
            else {
                Image reducedOutput = quantizeTiles(palettes, reducedImageData, false);
                updateQuantizedImage(reducedOutput);
            }
        }
        if (options.verbose) printf("MSE: %0.0f", mse);
        // if (options.verbose) printf((performance.now() - t1).toFixed(0) + " ms");
    }

    if (useMin) {
        palettes = minPalettes;
    }

    if (!useDither)
        palettes = reducePalettes(palettes, options.bitsPerChannel);

    // @ CURRENT LOC HERE
    /*
    const int finalIterations = iterations * 10;
    let nextUpdate = iterations;
    for (let iteration = 0; iteration < finalIterations; iteration++) {
        const nextPixel = pixels[randomShuffle.next()];
        movePalettesCloser(palettes, tiles, nextPixel, finalAlpha);
        if (iteration >= nextUpdate) {
            nextUpdate += iterations;
            updateProgress(prog[1] + ((prog[2] - prog[1]) * iteration) / finalIterations);
            updatePalettes(palettes, false);
        }
    }
    console.log("Normal final: " + meanSquareError(palettes, tiles).toFixed(0));
    console.log("Dither final: " + meanSquareErrorDither(palettes, tiles).toFixed(0));
    updateProgress(prog[2]);
    updatePalettes(palettes, false);
    if (!useDither) {
        palettes = reducePalettes(palettes, options.bitsPerChannel);
        for (let i = 0; i < 3; i++) {
            palettes = kMeans(palettes, tiles);
            updateProgress(prog[2] + ((prog[3] - prog[2]) * (i + 1)) / 3);
            updatePalettes(palettes, false);
        }
    }
    palettes = reducePalettes(palettes, options.bitsPerChannel);
    updatePalettes(palettes, true);
    updateQuantizedImage(quantizeTiles(palettes, reducedImageData, useDither));
    console.log("> MSE: " + meanSquareError(palettes, tiles).toFixed(2));
    console.log(`> Time: ${((performance.now() - t0) / 1000).toFixed(2)} sec`);
    */

    return EXIT_SUCCESS;
}


// function reducePalettes(palettes, bitsPerChannel) {
static vector <vector <rgbColor>> reducePalettes(const vector <vector <rgbColor>> & palettes, unsigned int bitsPerChannel) {
    vector <vector <rgbColor>> result;
    for (const vector <rgbColor> & palette : palettes) {
        vector <rgbColor> pal;
        for (const rgbColor & color : palette) {
            rgbColor col = color;
            toNbitColor(col, bitsPerChannel);
            pal.push_back(col);
        }
        result.push_back(pal);
    }
    return result;
}


// function sortPalettes(palettes, startIndex) {
// Note: Not passing palettes as a reference is intentional
static vector <vector <rgbColor>> sortPalettes(const vector <vector <rgbColor>> palettes, const int startIndex) {
    const int pairIterations = 2000;
    const int tIterations = 10000;
    const int paletteIterations = 100000;
    const int upWeight = 2;
    const int numPalettes = palettes.size();
    const int numColors = palettes[0].size();

    if ((numColors == 2) && (startIndex == 1)) {
        return palettes;
    }

    // TODO: DEBUG
    if (options.verbose) {
        printf("sortPalettes()\n");
        printf("== sortPalettes (Start) ==\n");
        printPalettes(palettes);
    }

    // // paletteDist[i+1][j+1] stores distance between palette i and palette j
    // Creates a 2D array initialized with zeros
    // const paletteDist = zeros2(numPalettes + 2, numPalettes + 2);
    vector <vector <double>> paletteDist (numPalettes + 2,  vector <double>(numPalettes + 2, 0.0) );

    // colorIndex[p1][p2][i] stores the index of the closest color in p2 from color index i in p1
    // Creates a 3D array initialized with zeros
    // const colorIndex = zeros3(numPalettes, numPalettes, numColors);
    vector <vector <vector <int>>> colorIndex (numPalettes,  vector <vector <int>>(numPalettes, vector <int>(numColors,0) ) );

    for (int i = 0; i < numPalettes; i++) {
        for (int j = 0; j < numPalettes; j++) {
            for (int k = 0; k < numColors; k++) {
                colorIndex[i][j][k] = k;
            }
        }
    }
    for (int p1 = 0; p1 < numPalettes - 1; p1++) {
        for (int p2 = p1 + 1; p2 < numPalettes; p2++) {

            // index is a 1D array copied from colorIndex[n][n][..N..]
            vector <int> index = colorIndex[p1][p2];
            for (int iteration = 0; iteration < pairIterations; iteration++) {

                int i1 = startIndex + floor(randRange0to1() * (numColors - startIndex - 1));
                int i2 = i1 + 1 + floor(randRange0to1() * (numColors - i1 - 1));
                if (randRange0to1() < 0.5) {
                    // [i1, i2] = [i2, i1];
                    // Swap the two
                    const int tmp = i1;
                    i1 = i2;
                    i2 = tmp;
                }
                const rgbColor p1i1 = palettes[p1][i1];
                const rgbColor p1i2 = palettes[p1][i2];
                const rgbColor p2i1 = palettes[p2][index[i1]];
                const rgbColor p2i2 = palettes[p2][index[i2]];
                const double straightDist = colorDistance(p1i1, p2i1) + colorDistance(p1i2, p2i2);
                const double swappedDist = colorDistance(p1i1, p2i2) + colorDistance(p1i2, p2i1);
                if (swappedDist < straightDist) {
                    // [index[i1], index[i2]] = [index[i2], index[i1]];
                    // Swap the two
                    const int tmp = index[i1];
                    index[i1] = index[i2];
                    index[i2] = tmp;
                }
            }
            double sum = 0;
            for (int i = 0; i < numColors; i++) {
                const rgbColor p1i = palettes[p1][i];
                const rgbColor p2i = palettes[p2][index[i]];
                sum += colorDistance(p1i, p2i);
            }
            paletteDist[p1 + 1][p2 + 1] = sum;
            paletteDist[p2 + 1][p1 + 1] = sum;
        }
    }
    for (int p1 = 1; p1 < numPalettes; p1++) {
        for (int p2 = 0; p2 < p1; p2++) {
            // index and revIndex are arrays of indexes found at [n][n] in 3D array colorIndex
            const vector <int> index = colorIndex[p2][p1];
            vector <int> revIndex = colorIndex[p1][p2];
            for (int i = 0; i < numColors; i++) {
                // revIndex[i] = index.indexOf(i);
                revIndex[i] = indexOf(index, i);  // TODO: VALIDATE MATCHES EXPECTED BEHAVIOR
            }
        }
    }

    // const palIndex = [];
    vector <int> palIndex;
    for (int i = 0; i < numPalettes + 2; i++) {
        palIndex.push_back(i);
    }
    if (numPalettes > 2) {
        for (int iteration = 0; iteration < paletteIterations; iteration++) {
            const int index1 = MAX((int)1, (int)floor(randRange0to1() * numPalettes));
            const int index2 = MIN(numPalettes, index1 + (int)1 + (int)floor(randRange0to1() * numPalettes));
            const int i1b = palIndex[index1 - 1];
            const int i1 = palIndex[index1];
            const int i2 = palIndex[index2];
            const int i2b = palIndex[index2 + 1];
            const double straightDist = paletteDist[i1b][i1] + paletteDist[i2][i2b];
            const double swappedDist = paletteDist[i1b][i2] + paletteDist[i1][i2b];
            if (swappedDist < straightDist) {
                reverse(palIndex, index1, index2);
            }
        }
    }
    // const pal1 = palettes[palIndex[1] - 1];
    // const p1Index = [];
    vector <rgbColor> pal1 = palettes[palIndex[1] - 1];
    vector <int> p1Index;
    for (int i = 0; i < numColors + 2; i++) {
        p1Index.push_back(i);
    }

    // Creates a 2D array initialized with zeros // TODO: of type double probably
    // const p1Dist = zeros2(numColors + 2, numColors + 2);
    vector <vector <double>> p1Dist (numColors + 2,  vector <double>(numColors + 2, 0.0) );

    for (int i = 1; i <= numColors; i++) {
        for (int j = 1; j <= numColors; j++) {
            p1Dist[i][j] = colorDistance(pal1[i - 1], pal1[j - 1]);
        }
    }

    if (numColors > 2) {
        for (int iteration = 0; iteration < paletteIterations; iteration++) {
            const int index1 =max((int)1 + startIndex, (int)floor(randRange0to1() * numColors));
            const int index2 =min(numColors, index1 + (int)1 + (int)floor(randRange0to1() * numColors));
            const int i1b = p1Index[index1 - 1];
            const int i1 = p1Index[index1];
            const int i2 = p1Index[index2];
            const int i2b = p1Index[index2 + 1];
            const double straightDist = p1Dist[i1b][i1] + p1Dist[i2][i2b];
            const double swappedDist = p1Dist[i1b][i2] + p1Dist[i1][i2b];
            if (swappedDist < straightDist) {
                reverse(p1Index, index1, index2);
            }
        }
    }

    // Creates a 2D array initialized with zeros // TODO: of type double probably
    // const pIndex = zeros2(numPalettes, numColors);
    vector <vector <double>> pIndex (numPalettes,  vector <double>(numColors, 0.0) );
    for (int i = 1; i <= numColors; i++) {
        for (int j = 1; j <= numColors; j++) {
            pIndex[i][j] = p1Index[i + 1] - 1;
        }
    }

    for (int i = 1; i < numPalettes; i++) {
        for (int j = 0; j < numColors; j++) {
            const int p1 = palIndex[i] - 1;
            const int p2 = palIndex[i + 1] - 1;
            pIndex[i][j] = colorIndex[p1][p2][pIndex[i - 1][j]];
        }
    }
    if (numColors >= 4) {
        for (int i = 1; i < numPalettes; i++) {
            const int p1 = palIndex[i] - 1;
            const int p2 = palIndex[i + 1] - 1;
            int iteration = 0;
            while (iteration < tIterations) {
                const int index1 = MAX(startIndex, (int)floor(randRange0to1() * numColors));
                const int index2 = MAX(startIndex, (int)floor(randRange0to1() * numColors));
                if (index1 == index2)
                    continue;
                const int up1 = pIndex[i - 1][index1];
                const int i1 = pIndex[i][index1];
                const int left1 = pIndex[i][index1 - 1];
                const int right1 = pIndex[i][index1 + 1];
                const int up2 = pIndex[i - 1][index2];
                const int i2 = pIndex[i][index2];
                const int left2 = pIndex[i][index2 - 1];
                const int right2 = pIndex[i][index2 + 1];
                double straightDist = upWeight *
                    colorDistance(palettes[p2][i1], palettes[p1][up1]);
                if (left1 >= 0)
                    straightDist += colorDistance(palettes[p2][i1], palettes[p2][left1]);
                if (right1 < numColors)
                    straightDist += colorDistance(palettes[p2][i1], palettes[p2][right1]);
                straightDist +=
                    upWeight *
                        colorDistance(palettes[p2][i2], palettes[p1][up2]);
                if (left2 >= 0)
                    straightDist += colorDistance(palettes[p2][i2], palettes[p2][left2]);
                if (right2 < numColors)
                    straightDist += colorDistance(palettes[p2][i2], palettes[p2][right2]);
                double swappedDist = upWeight *
                    colorDistance(palettes[p2][i2], palettes[p1][up1]);
                if (left1 >= 0)
                    swappedDist += colorDistance(palettes[p2][i2], palettes[p2][left1]);
                if (right1 < numColors)
                    swappedDist += colorDistance(palettes[p2][i2], palettes[p2][right1]);
                swappedDist +=
                    upWeight *
                        colorDistance(palettes[p2][i1], palettes[p1][up2]);
                if (left2 >= 0)
                    swappedDist += colorDistance(palettes[p2][i1], palettes[p2][left2]);
                if (right2 < numColors)
                    swappedDist += colorDistance(palettes[p2][i1], palettes[p2][right2]);
                if (swappedDist < straightDist) {
                    // [pIndex[i][index1], pIndex[i][index2]] = [
                    //     pIndex[i][index2],
                    //     pIndex[i][index1],
                    // ];
                    // swap
                    const int tmp = pIndex[i][index1];
                    pIndex[i][index1] = pIndex[i][index2];
                    pIndex[i][index2] = tmp;
                }
                iteration++;
            }
        }
    }
    // const pals = [];
    vector <vector <rgbColor>> pals;
    for (int i = 0; i < numPalettes; i++) {
        const int p2 = palIndex[i + 1] - 1;
        // const pal = [];
        vector <rgbColor> pal;
        for (int j = 0; j < numColors; j++) {
            pal.push_back(palettes[p2][pIndex[i][j]]);
        }
        pals.push_back(pal);
    }
    // TODO: DEBUG
    if (options.verbose) {
        printf("== sortPalettes (End) ==\n");
        printPalettes(pals);
    }

    return pals;
}
/*
// Creates a 1D array populated with zeros // TODO: of type double probably
function zeroArray(len) {
    const result = [];
    for (let i = 0; i < len; i++) {
        result.push(0);
    }
    return result;
}

// Creates a 2D array populated with zeros // TODO: of type double probably
function zeros2(len1, len2) {
    const result = [];
    for (let i = 0; i < len1; i++) {
        result.push(zeroArray(len2));
    }
    return result;
}

// Creates a 3D array populated with zeros // TODO: of type double probably
function zeros3(len1, len2, len3) {
    const result = [];
    for (let i = 0; i < len1; i++) {
        result.push(zeros2(len2, len3));
    }
    return result;
}

*/
// function reverse(a, left, right) {
static void reverse(vector <int> & a, int left, int right) {
    const double middle = (left + right) / 2.0;
    while ((double)left < middle) {
        // [a[left], a[right]] = [a[right], a[left]];  // swap array items
        // TODO: does this need bounds checking?
        const int tmp = a[left];
        a[left] = a[right];
        a[right] = tmp;
        left++;
        right--;
    }
}

static double toLinear(double x) {
    return x * x;
}

static void toLinearColor(rgbColor & color) {
    color.ch.r = toLinear(color.ch.r);
    color.ch.g = toLinear(color.ch.g);
    color.ch.b = toLinear(color.ch.b);
}

static double toSrgb(double x) {
    return sqrt(x);
}

static void toSrgbColor(rgbColor & color) {
    color.ch.r = toSrgb(color.ch.r);
    color.ch.g = toSrgb(color.ch.g);
    color.ch.b = toSrgb(color.ch.b);
}

const double brightnessScale[] = {0.299, 0.587, 0.114};
// function brightness(color) {
static double brightness(const rgbColor & color) {
    double sum = 0;
    sum += (brightnessScale[RGB_R] * toLinear(color.ch.r));
    sum += (brightnessScale[RGB_G] * toLinear(color.ch.g));
    sum += (brightnessScale[RGB_B] * toLinear(color.ch.b));
    return sum;
}

// function replaceWeakestColors(palettes, tiles, minColorFactor, minPaletteFactor, replacePalettes) {
static vector <vector <rgbColor>> replaceWeakestColors(const vector <vector <rgbColor>> & palettes, vector <Tile> & tiles, float minColorFactor, float minPaletteFactor, bool replacePalettes) {
    const unsigned int colorZeroBehaviour = options.colorZeroBehaviour;
    const bool         useSlowDither      = options.ditherMethod == Opts::ditherSlow;

    // let closestPal = closestPaletteDistance;  // Function alias, going to use a simplistic implementation
    // if (useSlowDither) {
    //     closestPal = closestPaletteDistanceDither;
    // }

    vector <int> closestPaletteIndex(tiles.size());
    int maxPaletteIndex = 0;
    int minPaletteIndex = 0;
    vector <double> totalPaletteMse(palettes.size());
    vector <double> removedPaletteMse(palettes.size());

    if (palettes.size() > 1) {
        for (int j = 0; j < (int)tiles.size(); j++) {
            const Tile & tile = tiles[j];
            // Handle function alias
            // const [index, minDistance] = closestPal(palettes, tile);
            Candidate result;
            if (useSlowDither) { result = closestPaletteDistance(palettes, tile); }
            else               { result = closestPaletteDistanceDither(palettes, tile); }
            const int index = result.colorIndex;
            const double minDistance = result.colorDistance;
            totalPaletteMse[index] += minDistance;
            closestPaletteIndex[j] = index;

            vector <vector <rgbColor>> remainingPalettes;
            for (int i = 0; i < (int)palettes.size(); i++) {
                if (i != index) {
                    remainingPalettes.push_back(palettes[i]);
                }
            }
            if (remainingPalettes.size() > 0) {
                // Handle function alias
                // const [, minDistance2] = closestPal(remainingPalettes, tile);
                Candidate result;
                if (useSlowDither) { result = closestPaletteDistance(remainingPalettes, tile); }
                else               { result = closestPaletteDistanceDither(remainingPalettes, tile); }
                const double minDistance2 = result.colorDistance;
                removedPaletteMse[index] += minDistance2;
            }
        }
        maxPaletteIndex = maxIndexDbl(totalPaletteMse);
        minPaletteIndex = minIndexDbl(removedPaletteMse);
    }

    vector <vector <rgbColor>> result;
    if (palettes[0].size() > 1) {

        vector <vector <double>> totalColorMse;
        vector <vector <double>> secondColorMse;
        for (int j = 0; j < (int)palettes.size(); j++) {
            totalColorMse[j].resize(palettes[j].size(), 0.0);
            secondColorMse[j].resize(palettes[j].size(), 0.0);
        }

        for (int j = 0; j < (int)tiles.size(); j++) {
            const Tile & tile = tiles[j];
            const int minPaletteIndex = closestPaletteIndex[j];
            const vector <rgbColor> pal = palettes[minPaletteIndex];

            if (useSlowDither) {
                for (const pixelEntry pixel : tile.pixels) {
                    // const [minColorIndex, minDist] = getClosestColorDither(pal, pixel);
                    const Candidate result = getClosestColorDither(pal, pixel);
                    const int minColorIndex = result.colorIndex;
                    const double minDist    = result.colorDistance;
                    totalColorMse[minPaletteIndex][minColorIndex] += minDist;

                    vector <rgbColor> remainingColors;
                    for (int i = 0; i < (int)pal.size(); i++) {
                        if (i != minColorIndex) {
                            remainingColors.push_back(pal[i]);
                        }
                    }
                    // const [, secondDist] = getClosestColorDither(remainingColors, pixel);
                    const Candidate result2 = getClosestColorDither(remainingColors, pixel);
                    const double secondDist = result2.colorDistance;
                    secondColorMse[minPaletteIndex][minColorIndex] +=
                        secondDist;
                }
            }
            else {
                for (int i = 0; i < (int)tile.colors.size(); i++) {
                    const rgbColor color = tile.colors[i];
                    // const [minColorIndex, minDist] = getClosestColor(pal, color);
                    const Candidate result = getClosestColor(pal, color);
                    const int minColorIndex = result.colorIndex;
                    const double minDist = result.colorDistance;
                    totalColorMse[minPaletteIndex][minColorIndex] +=
                        minDist * tile.counts[i];

                    vector <rgbColor> remainingColors;
                    for (int i = 0; i < (int)pal.size(); i++) {
                        if (i != minColorIndex) {
                            remainingColors.push_back(pal[i]);
                        }
                    }
                    // const [, secondDist] = getClosestColor(remainingColors, color);
                    const Candidate result2 = getClosestColor(remainingColors, color);
                    const double secondDist = result2.colorDistance;
                    secondColorMse[minPaletteIndex][minColorIndex] +=
                        secondDist * tile.counts[i];
                }
            }
        }

        int sharedColorIndex = -1;
        if (options.colorZeroBehaviour == Opts::indexZeroShared) {
            sharedColorIndex = 0;
        }
        for (int palIndex = 0; palIndex < (int)palettes.size(); palIndex++) {
            const int maxColorIndex = maxIndexDbl(totalColorMse[palIndex]);
            const int minColorIndex = minIndexDbl(secondColorMse[palIndex]);
            const bool shouldReplaceMinColor = ((minColorIndex != maxColorIndex) &&
                                                (minColorIndex != sharedColorIndex) &&
                                                (secondColorMse[palIndex][minColorIndex] < (minColorFactor * totalColorMse[palIndex][maxColorIndex])));
            vector <rgbColor> colors;
            for (int i = 0; i < (int)palettes[palIndex].size(); i++) {
                if ((i == minColorIndex) && shouldReplaceMinColor) {
                    // console.log("replaced color in palette " + palIndex);
                    colors.push_back(palettes[palIndex][maxColorIndex]);
                }
                else {
                    colors.push_back(palettes[palIndex][i]);
                }
            }
            result.push_back(colors);
        }
    }
    else {
        for (int palIndex = 0; palIndex < (int)palettes.size(); palIndex++) {
            vector <rgbColor> colors;
            for (int i = 0; i < (int)palettes[palIndex].size(); i++) {
                colors.push_back(palettes[palIndex][i]);
            }
            result.push_back(colors);
        }
    }

    if (replacePalettes &&
        (minPaletteIndex != maxPaletteIndex) &&
        (removedPaletteMse[minPaletteIndex] < (minPaletteFactor * totalPaletteMse[maxPaletteIndex])) ) {

        if (options.verbose) printf("replaceWeakestColors(): replaced palette %d\n", minPaletteIndex);

        // while (result[minPaletteIndex].size() > 0)
        //     result[minPaletteIndex].pop();
        if (result[minPaletteIndex].size() > 0)
            result[minPaletteIndex].clear();

        for (const rgbColor & color : result[maxPaletteIndex]) {
            const rgbColor c = color;
            result[minPaletteIndex].push_back(c);
        }
    }
    return result;
}

/*
function kMeans(palettes, tiles) {
    const colorZeroBehaviour = options.colorZeroBehaviour;
    const counts = [];
    const sumColors = [];
    for (let i = 0; i < palettes.length; i++) {
        const c = [];
        const colors = [];
        for (let j = 0; j < palettes[i].length; j++) {
            c.push(0);
            colors.push([0, 0, 0]);
        }
        counts.push(c);
        sumColors.push(colors);
    }
    for (const tile of tiles) {
        if (options.dither == Dither.Slow) {
            const palIndex = getClosestPaletteIndexDither(palettes, tile);
            for (const pixel of tile.pixels) {
                const [colIndex, ,] = getClosestColorDither(palettes[palIndex], pixel);
                counts[palIndex][colIndex] += 1;
                addColor(sumColors[palIndex][colIndex], pixel.color);
            }
        }
        else {
            const palIndex = getClosestPaletteIndex(palettes, tile);
            for (let i = 0; i < tile.colors.length; i++) {
                const [colIndex] = getClosestColor(palettes[palIndex], tile.colors[i]);
                counts[palIndex][colIndex] += tile.counts[i];
                const color = cloneColor(tile.colors[i]);
                scaleColor(color, tile.counts[i]);
                addColor(sumColors[palIndex][colIndex], color);
            }
        }
    }
    let sharedColorIndex = -1;
    if (colorZeroBehaviour == ColorZeroBehaviour.Shared) {
        sharedColorIndex = 0;
    }
    for (let i = 0; i < sumColors.length; i++) {
        for (let j = 0; j < sumColors[i].length; j++) {
            if (counts[i][j] == 0 || j == sharedColorIndex) {
                sumColors[i][j] = cloneColor(palettes[i][j]);
            }
            else {
                scaleColor(sumColors[i][j], 1.0 / counts[i][j]);
            }
        }
    }
    return sumColors;
}
*/



// float meanSquareError(palettes, tiles) {
static float meanSquareError(const vector <vector <rgbColor>> & palettes, const vector <Tile> & tiles) {
    double totalDistance = 0;
    int count = 0;
    for (const Tile & tile : tiles) {
        const int palIndex = getClosestPaletteIndex(palettes, tile);
        for (int i = 0; i < (int)tile.colors.size(); i++) {
            // const [, minDistance] = getClosestColor(palettes[palIndex], tile.colors[i]);
            const Candidate result = getClosestColor(palettes[palIndex], tile.colors[i]);
            const double minDistance = result.colorDistance;
            totalDistance += minDistance * tile.counts[i];
            count += tile.counts[i];
        }
    }
    return (float)(totalDistance / (double)count);
}

// float meanSquareErrorDither(palettes, tiles) {
static float meanSquareErrorDither(const vector <vector <rgbColor>> & palettes, const vector <Tile> & tiles) {
    double totalDistance = 0;
    int count = 0;
    for (const Tile & tile : tiles) {
        const int palIndex = getClosestPaletteIndexDither(palettes, tile);
        for (const pixelEntry & pixel : tile.pixels) {
            // const [, minDistance] = getClosestColorDither(palettes[palIndex], pixel);
            const Candidate result = getClosestColorDither(palettes[palIndex], pixel);
            const double minDistance = result.colorDistance;
            totalDistance += minDistance;
            count += 1;
        }
    }
    return (float)(totalDistance / (double)count);
}

// RandomShuffle moved to a separate source file
//
// See: randomshuffle.h


// function getClosestColor(palette, color) {
static Candidate getClosestColor(const vector <rgbColor> & palette, const rgbColor & color) {
    int minIndex = (int)palette.size() - 1;
    double minDist = colorDistance(palette[minIndex], color);
    for (int i = (int)palette.size() - 2; i >= 0; i--) {
        const double dist = colorDistance(palette[i], color);
        if (dist < minDist) {
            minIndex = i;
            minDist = dist;
        }
    }
    // return [minIndex, minDist];
    const Candidate result = {(int)minIndex, minDist, /* Next two are shims for {color, bright} to avoid warning */ {0,0,0}, 0};
    return result;
}


// function getClosestColorDither(palette, pixel) {
static Candidate getClosestColorDither(const vector <rgbColor> & palette, const pixelEntry & pixel) {

    rgbColor error = {0, 0, 0};
    rgbColor linearPixel = cloneColor(pixel.color);
    toLinearColor(linearPixel);

    vector <Candidate> candidates;
    rgbColor c            = {0, 0, 0};
    rgbColor err          = {0, 0, 0};
    rgbColor reducedColor = {0, 0, 0};

    for (unsigned int i = 0; i < options.ditherPixels; i++) {
        copyColor(c, linearPixel);
        copyColor(err, error);
        scaleColor(err, options.ditherWeight);
        addColor(c, err);
        clampColor(c, 0, 255 * 255);
        toSrgbColor(c);
        Candidate result = getClosestColor(palette, c);
        int minColorIndex = result.colorIndex;
        const rgbColor minColor = palette[minColorIndex];

        result.comparedColor = c;
        result.brightness = brightness(minColor);
        candidates.push_back(result);

        copyColor(reducedColor, minColor);
        toNbitColor(reducedColor, options.bitsPerChannel);
        toLinearColor(reducedColor);
        addColor(error, linearPixel);
        subtractColor(error, reducedColor);
    }
    for (unsigned int i = 0; i < options.ditherPixels - 1; i++) {
        for (unsigned int j = i + 1; j < options.ditherPixels; j++) {
            if (candidates[i].brightness > candidates[j].brightness) {
                // [candidates[i], candidates[j]] = [candidates[j], candidates[i]];
                // Swap the two candidates
                Candidate tmp = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = tmp;

            }
        }
    }
    const int index = options.ditherPattern[pixel.x & 1][pixel.y & 1];
    // return [
    //     candidates[index].colorIndex,
    //     candidates[index].colorDistance,
    //     candidates[index].comparedColor,
    // ];
    return candidates[index];
}

static double colorDistance(const rgbColor & a, const rgbColor & b) {
    return 2 * pow((a.ch.r - b.ch.r), 2) + 4 * pow((a.ch.g - b.ch.g), 2) + pow((a.ch.b - b.ch.b), 2);
}

// function paletteDistance(palette, tile) {
static double paletteDistance(const vector <rgbColor> & palette, const Tile & tile) {
    double sum = 0;
    // TODO: Why making copies of these instead of just using the source ones?
    // const vector <rgbColor> colors = tile.colors;
    // const vector <int>   counts = tile.counts;
    for (int i = 0; i < (int)tile.colors.size(); i++) {
        const Candidate result = getClosestColor(palette, tile.colors[i]);
        double minDist = result.colorDistance;
        sum += tile.counts[i] * minDist;
    }
    return sum;
}

// function paletteDistanceDither(palette, tile) {
static double paletteDistanceDither(const vector <rgbColor> & palette, const Tile & tile) {
    double sum = 0;
    // for (const pixel of tile.pixels) {
    for (const pixelEntry & pixel : tile.pixels) {
        const Candidate result = getClosestColorDither(palette, pixel);
        double minDist = result.colorDistance;
        sum += minDist;
    }
    return sum;
}

// function getClosestPaletteIndex(palettes, tile) {
static int getClosestPaletteIndex(const vector <vector <rgbColor>> & palettes, const Tile & tile) {
    if (palettes.size() == 1)
        return 0;
    // const distances = palettes.map((palette) => paletteDistance(palette, tile));
    vector <double> distances;
    for (const vector <rgbColor> & palette : palettes) {
        distances.push_back(paletteDistance(palette, tile));
    }
    return minIndexDbl(distances);
}

// function closestPaletteDistance(palettes, tile) {
static Candidate closestPaletteDistance(const vector <vector <rgbColor>> & palettes, const Tile & tile) {
    // const distances = palettes.map((palette) => paletteDistance(palette, tile));
    vector <double> distances;
    for (const vector <rgbColor> & palette : palettes) {
        distances.push_back(paletteDistance(palette, tile));
    }
    const int index = minIndexDbl(distances);
    // TODO: This returns a uint and a double, more or less a candidate
    //       closestPal gets used as an alias for this, then used to call this. the return needs to
    // return [index, distances[index]];
    const Candidate result = {index, distances[index], /* Next two are shims for {color, bright} to avoid warning */ {0,0,0}, 0};
    return result;
}

// function getClosestPaletteIndexDither(palettes, tile) {
static int getClosestPaletteIndexDither(const vector <vector <rgbColor>> & palettes, const Tile & tile) {
    if (palettes.size() == 1)
        return 0;
    // const distances = palettes.map((palette) => paletteDistanceDither(palette, tile));
    vector <double> distances;
    for (const vector <rgbColor> & palette : palettes) {
        distances.push_back(paletteDistanceDither(palette, tile));
    }
    return minIndexDbl(distances);
}

// function closestPaletteDistanceDither(palettes, tile) {
static Candidate closestPaletteDistanceDither(const vector <vector <rgbColor>> & palettes, const Tile & tile) {
    // const distances = palettes.map((palette) => paletteDistanceDither(palette, tile));
    vector <double> distances;
    for (const vector <rgbColor> & palette : palettes) {
        distances.push_back(paletteDistanceDither(palette, tile));
    }
    const int index = minIndexDbl(distances);
    // TODO: This returns a uint and a double, more or less a candidate
    //       closestPal gets used as an alias for this, then used to call this. the return needs to
    // return [index, distances[index]];
    const Candidate result = {index, distances[index], /* Next two are shims for {color, bright} to avoid warning */ {0,0,0}, 0};
    return result;
}

// function getColor(image, x, y) {
// TODO: Inline?
static rgbColor getColor(const Image & image, const unsigned int x, const unsigned int y) {
    // Seems to operate on raw RGBA8888 image buffer
    const unsigned int index = RGBA8888_SZ * (x + (image.width * y));
    const rgbColor color = {
        (double)image.data[index + RGBA8_R],
        (double)image.data[index + RGBA8_G],
        (double)image.data[index + RGBA8_B],
    };
    return color;
}

// extractTile(image, startX, startY) {
// static Tile extractTile(Image & image, unsigned int startX, unsigned int startY, int tile_id) {
static Tile extractTile(const Image & image, const unsigned int startX, const unsigned int startY, const int tile_id) {
    // const { tileWidth, tileHeight, colorZeroBehaviour, colorZeroValue } = quantizationOptions;
    // See tile.h
    // const tile = {
    //     colors: [],
    //     counts: [],
    //     pixels: [],
    // };
    Tile tile;
    const unsigned int endX = MIN(startX + options.tileWidth, image.width);
    const unsigned int endY = MIN(startY + options.tileHeight, image.height);
    for (unsigned int y = startY; y < endY; y++) {
        for (unsigned int x = startX; x < endX; x++) {
            rgbColor color = getColor(image, x, y);
            // skip transparent pixels
            if (isColorTransparent(color) || isPixelTransparent(image, x, y)) {
                continue;
            }
            // TODO: This is trying to push an entry that contains a reference to the tile, rgbcolor, x and y, they all get used later
            // TODO: IMPORTANT!: Must push a reference to the parent tile eventually. or at least find a way to derive it
            // tile.pixels.push({ tile, color, x, y });
            const pixelEntry pixel = {tile_id, color, x, y};
            tile.pixels.push_back(pixel);

            // Look for matching colors, if present then increment the matching color index in counts[]
            // otherwise create a new colors/counts entry
            //
            // Replaces: colorIndex = tile.colors.findIndex((c) => equalColors(c, color));
            int colorIndex;
            bool foundMatch = false;
            for (int c = 0; c < (int)tile.colors.size(); c++) {
               if (equalColors(tile.colors[c], color)) { colorIndex = c; foundMatch = true; break;}
            }
            if (foundMatch) {
                tile.counts[colorIndex]++;
            }
            else {
                tile.colors.push_back(color);
                tile.counts.push_back(1); // New Color entry with counter value of "1"
            }
        }
    }
    return tile;
}

// TODO: Inline?
// isPixelTransparent(x, y) {
bool isPixelTransparent(const Image & image, const unsigned int x, const unsigned int y) {
    const unsigned int index = RGBA8888_SZ * (x + image.width * y);
    return ((options.colorZeroBehaviour == Opts::indexZeroTranspFromTransp) &&
        (image.data[index + RGBA_ALPHA] < RGBA_ALPHA_MAX));
}

// function isColorTransparent(color) {
bool isColorTransparent(const rgbColor & color) {
    return ((options.colorZeroBehaviour == Opts::indexZeroTranspFromColor) &&
        equalColors(color, options.colorZeroValue));
}

// function extractTiles(image) {
static void extractTiles(Image & image, vector <Tile> & tiles) {
    int totalPixels = 0;
    int tileCount = 0;
    int tile_id = 0;
    for (unsigned int y = 0; y < image.height; y += options.tileHeight) {
        for (unsigned int x = 0; x < image.width; x += options.tileWidth) {
            const Tile tile = extractTile(image, x, y, tile_id);
            // TODO: DEBUG TEST
            // if (options.verbose) printf("-> Extract tile @ %4u x %4u:  colors.sz=%3zu, pixels.sz = %3zu, tilenum=%zu vs px-tilenum=%zu\n",
            //                             x,y, tile.colors.size(), tile.pixels.size(),
            //                             tile_id, tile.pixels[0].parent_tile_id);
            // TODO: DEBUG TEST
           if (tile.colors.size() == 0) printf(" -----> Empty tile\n");
           if (tile.colors.size() == 0)
               continue;
            tiles.push_back(tile);
            tile_id++;
            // These are just for stats collecting in the original version
            totalPixels += (int)tile.pixels.size();
            tileCount++;
        }
    }


    // TODO: DEBUG TEST
        if (options.verbose) printf("****** Num Tiles = %zu ******* \n", tiles.size());
        // for (int i = 0; i < (int)tiles.size(); i++) {
        //     if (tiles[i].pixels.size() > 0) {
        //         if (options.verbose) printf("-> Saved Tile [%4zu]:  colors.sz=%3zu, pixels.sz = %3zu, tilenum=%zu vs px-tilenum=%zu\n",
        //                                     i, tiles[i].colors.size(), tiles[i].pixels.size(),
        //                                     i, tiles[i].pixels[0].parent_tile_id);
        //         }
        // }
    // TODO: END DEBUG TEST
    if (options.verbose) {
       const float avgPixelsPerTile = totalPixels / tileCount;
       printf("avg pixels per tile: %0.2f\n", avgPixelsPerTile);
   }

   // return tiles;  // Changed to created by caller and passed by reference
}

// function equalColors(c1, c2) {
static bool equalColors(const rgbColor & c1, const rgbColor & c2) {
    if (c1.ch.r != c2.ch.r) return false;
    if (c1.ch.g != c2.ch.g) return false;
    if (c1.ch.b != c2.ch.b) return false;
    return true;
}

// function extractAllPixels(tiles) {
static void extractAllPixels(vector <Tile> & tiles, vector <pixelEntry> & pixels) {
      // const pixels = [];
      for (const Tile & tile : tiles) {
         for (const pixelEntry & pixel : tile.pixels) {
            pixels.push_back(pixel);
         }
      }
      // return pixels; // Changed to created by caller and passed by reference
}

// function quantizeTiles(palettes, image, useDither) {
static Image quantizeTiles(const vector <vector <rgbColor>> & palettes, const Image & image, const bool useDither) {
    // const { tileWidth, tileHeight, bitsPerChannel, colorZeroBehaviour, colorZeroValue, numPalettes, colorsPerPalette, } = quantizationOptions;
    const bool imageIsReduced = options.ditherMethod != Opts::ditherOff;

    int adjustedIndex = 0;
    if ((options.colorZeroBehaviour == Opts::indexZeroTranspFromColor) ||
        (options.colorZeroBehaviour == Opts::indexZeroTranspFromTransp)) {
        adjustedIndex = 1;
    }

    // const reducedPalettes = structuredClone(palettes);
    vector <vector <rgbColor>> reducedPalettes = palettes;
    for (vector <rgbColor> pal : reducedPalettes) {
        for (rgbColor color : pal) {
            toNbitColor(color, options.bitsPerChannel);
        }
    }

    // const transparentColor = cloneColor(options.colorZeroValue);
    rgbColor transparentColor = options.colorZeroValue;
    if (imageIsReduced)
        toNbitColor(transparentColor, options.bitsPerChannel);
    rgbColor colorZero = cloneColor(options.colorZeroValue);
    toNbitColor(colorZero, options.bitsPerChannel);

    // int bmpWidth = ceil(image.width / 4) * 4;
    // const quantizedImage = {
    //     width: image.width,
    //     height: image.height,
    //     data: new Uint8ClampedArray(image.data.length),
    //     totalPaletteColors: options.numPalettes * options.colorsPerPalette,
    //     paletteData: new Uint8ClampedArray(1024),
    //     colorIndexes: new Uint8ClampedArray(bmpWidth * image.height),
    // };
    Image quantizedImage;
    quantizedImage.width = image.width;
    quantizedImage.height = image.height;
    quantizedImage.data.resize(image.data.size());
    quantizedImage.totalPaletteColors = options.numPalettes * options.colorsPerPalette;
    // Resize below not needed, colors added by .push_back() insertion instead,
    // quantizedImage.paletteData.resize( .... N .... ); // = new Uint8ClampedArray(1024);
    quantizedImage.colorIndexes.resize(image.width * image.height); // = new Uint8ClampedArray(bmpWidth * image.height),

    if ((options.numPalettes * options.colorsPerPalette) <= 256) {
        addPngColors(reducedPalettes, quantizedImage.paletteData, adjustedIndex);
    }
    else if (options.verbose) {printf("extractTile(): No preview PNG image, more than 256 colors\n"); }

    for (unsigned int startY = 0; startY < image.height; startY += options.tileHeight) {
        for (unsigned int startX = 0; startX < image.width; startX += options.tileWidth) {

            Tile tile = extractTile(image, startX, startY, TILE_ID_DISCARDABLE);
            vector <rgbColor> palette = reducedPalettes[0];

            int closestPaletteIndex = 0;
            if (tile.colors.size() > 0) {
                if (useDither) {
                    closestPaletteIndex = getClosestPaletteIndexDither(reducedPalettes, tile);
                }
                else {
                    closestPaletteIndex = getClosestPaletteIndex(reducedPalettes, tile);
                }
                palette = reducedPalettes[closestPaletteIndex];
            }
            const unsigned int endX = MIN(startX + options.tileWidth, image.width);
            const unsigned int endY = MIN(startY + options.tileHeight, image.height);

            for (unsigned int y = startY; y < endY; y++) {
                for (unsigned int x = startX; x < endX; x++) {

                    const size_t index = RGBA8888_SZ * (x + image.width * y);
                    // const bmpIndex = x + bmpWidth * (image.height - 1 - y);
                    const size_t pngIndex = (x + image.width * y);
                    // const color = [
                    //     image.data[index],
                    //     image.data[index + 1],
                    //     image.data[index + 2],
                    // ];
                    rgbColor color = getColor(image, x,y);

                    if (((options.colorZeroBehaviour == Opts::indexZeroTranspFromTransp) && (image.data[index + RGBA8_ALPHA] < 255)) ||
                        ((options.colorZeroBehaviour == Opts::indexZeroTranspFromColor)  && equalColors(color, transparentColor))) {
                        // Copy pixel from source image as transparent or as-is(treated as transparent) if applicable
                         quantizedImage.data[index + RGBA8_R] = image.data[index + RGBA8_R];
                        quantizedImage.data[index + RGBA8_G] = image.data[index + RGBA8_G];
                        quantizedImage.data[index + RGBA8_B] = image.data[index + RGBA8_B];
                        quantizedImage.data[index + RGBA8_ALPHA] = image.data[index + RGBA8_ALPHA];
                        // Set paletteIndex to color zero of closest palette (perhaps since it's typically transparent on consoles)
                        quantizedImage.colorIndexes[pngIndex] =
                            closestPaletteIndex * options.colorsPerPalette;
                    }
                    else {
                        int closestColorIndex = 0;
                        if (useDither) {
                            // [closestColorIndex] = getClosestColorDither(palette, {
                            //     color: color,
                            //     x: x,
                            //     y: y,
                            // });
                            const pixelEntry pixel = {TILE_ID_DISCARDABLE, color, x, y};
                            const Candidate result = getClosestColorDither(palette, pixel);
                            closestColorIndex = result.colorIndex;
                        }
                        else {
                            // [closestColorIndex] = getClosestColor(palette, color);
                            const Candidate result = getClosestColor(palette, color);
                            closestColorIndex = result.colorIndex;
                        }
                        const rgbColor paletteColor = palette[closestColorIndex]; // cloneColor(palette[closestColorIndex]);
                        quantizedImage.data[index + RGBA8_R] = (uint8_t)paletteColor.ch.r;
                        quantizedImage.data[index + RGBA8_G] = (uint8_t)paletteColor.ch.g;
                        quantizedImage.data[index + RGBA8_B] = (uint8_t)paletteColor.ch.b;
                        quantizedImage.data[index + RGBA8_ALPHA] = (uint8_t)ALPHA_FULLY_OPAQUE;
                        quantizedImage.colorIndexes[pngIndex] =
                            (closestPaletteIndex * options.colorsPerPalette) + closestColorIndex + adjustedIndex;
                    }
                }
            }
        }
    }
    return quantizedImage;
}

// Makes a uint8 copy of the palette for use with exporting a preview image
// function addBmpColors(palettes, bmpPalette) {
static void addPngColors(vector <vector <rgbColor>> & palettes, vector <rgbColorU8> & pngPalette, int adjustedIndex) {
    int i = 0;
    for (const vector <rgbColor> & pal : palettes) {
        if (adjustedIndex == 1) {
            pngPalette.push_back(rgbColorToU8(options.colorZeroValue));
            // bmpPalette[i] = colorZero[RGB_B];
            // bmpPalette[i + 1] = colorZero[RGB_g];
            // bmpPalette[i + 2] = colorZero[RGB_R];
            // i += 4;
        }
        for (const rgbColor & color : pal) {
            pngPalette.push_back(rgbColorToU8(color));
            // bmpPalette[i] = color[RGB_B];
            // bmpPalette[i + 1] = color[RGB_g];
            // bmpPalette[i + 2] = color[RGB_R];
            // i += 4;
        }
    }
}

// function colorQuantize1Color(tiles, pixels, randomShuffle) {
static void colorQuantize1Color(vector <Tile> & tiles, vector <pixelEntry> & pixels, RandomShuffle & randomShuffle, vector <vector <rgbColor>> & palettes) {
    int iterations = (int)(options.fractionOfPixels * (float)pixels.size());
    float  alpha = 0.3;
    if (options.ditherMethod == Opts::ditherSlow) {
        iterations /= 5;
        alpha = 0.1;
    }

    rgbColor avgColor = {0, 0, 0};
    for (const pixelEntry & pixel : pixels) {
        addColor(avgColor, pixel.color);
    }
    scaleColor(avgColor, 1.0 / (float)pixels.size());

    // const palettes = [[avgColor]];
    //
    // Creates a 3d array like:
    // palettes[1][1][3] with:
    //     [0][0][0]: 116.4280056423611 (r avg)
    //     [0][0][1]: 94.53263346354166 (g avg)
    //     [0][0][2]: 97.56797960069444 (b avg)
    // Equivalent to: <vector> <vector> rgbColor
    //
    // vector <rgbColor> newRow;
    // newRow.push_back(avgColor);
    // palettes.push_back(newRow);
    //
    // Equiv to above:
    palettes.push_back(vector <rgbColor> {avgColor} );

    if (options.colorZeroBehaviour == Opts::indexZeroShared) {
        palettes[0].push_back(avgColor);
        // Overwrite entry [0][0] with colorZeroValue
        palettes[0][0] = cloneColor(options.colorZeroValue);
    }

    // TODO: DEBUG
    if (options.verbose) {
        printf("== colorQuantize1Color ==\n");
        printPalettes((const vector <vector <rgbColor>>)palettes);
    }

    unsigned int splitIndex = 0;

    for (unsigned int  numPalettes = 2; numPalettes <= options.numPalettes; numPalettes++) {
        palettes.push_back(palettes[splitIndex]);
        for (int iteration = 0; iteration < iterations; iteration++) {
            const pixelEntry nextPixel = pixels[randomShuffle.next()];
            movePalettesCloser(palettes, tiles, nextPixel, alpha);
        }

        // const paletteDistance = zeroArray(numPalettes);
        vector <double> paletteDistances(numPalettes, 0.0);
        for (const Tile & tile : tiles) {
            // [palIndex, distance]
            const Candidate result = closestPaletteDistance(palettes, tile);
            const int palIndex = result.colorIndex;
            paletteDistances[palIndex] += result.colorDistance;
        }
        splitIndex = maxIndexDbl(paletteDistances);
    }
    // return palettes;  // Return handled via argument passed as reference
}

// function expandPalettesByOneColor(palettes, tiles, pixels, randomShuffle) {
static void expandPalettesByOneColor(vector <vector <rgbColor>> & palettes, vector <Tile> & tiles, vector <pixelEntry> & pixels, RandomShuffle & randomShuffle) {
    int iterations = (int)(options.fractionOfPixels * (float)pixels.size());
    float alpha = 0.3;
    if (options.ditherMethod == Opts::ditherSlow) {
        iterations /= 5;
        alpha = 0.1;
    }

    const int numColors = palettes[0].size() + 1;
    // const splitIndexes = zeroArray(palettes.length);
    vector <int> splitIndexes(palettes.size());
    if (numColors > 2) {
        // const totalColorDistances = [];
        // for (let i = 0; i < palettes.length; i++) {
        //     const totalColorDistance = zeroArray(numColors);
        //     totalColorDistances.push(totalColorDistance);
        // }
        // Creates a 2D array initialized with zeros
        vector <vector <double>> totalColorDistances (palettes.size(),  vector <double>(numColors) );

        for (const Tile & tile : tiles) {
            const int closestPaletteIndex = getClosestPaletteIndex(palettes, tile);
            const vector <rgbColor> palette = palettes[closestPaletteIndex];
            const vector <rgbColor> colors = tile.colors;
            const vector <int>      counts = tile.counts;

            for (int i = 0; i < (int)colors.size(); i++) {
                // const [minIndex, minDist] = getClosestColor(palette, colors[i]);
                const Candidate result = getClosestColor(palette, colors[i]);
                const int minIndex   = result.colorIndex;
                const double minDist = result.colorDistance;

                totalColorDistances[closestPaletteIndex][minIndex] +=
                    counts[i] * minDist;
            }
        }
        for (int i = 0; i < (int)palettes.size(); i++) {
            splitIndexes[i] = maxIndexDbl(totalColorDistances[i]);
        }
    }

    for (int i = 0; i < (int)palettes.size(); i++) {
        vector <rgbColor> colors = palettes[i];
        const int splitIndex           = splitIndexes[i];
        colors.push_back(colors[splitIndex]);
    }

    for (int iteration = 0; iteration < iterations; iteration++) {
        const pixelEntry nextPixel = pixels[randomShuffle.next()];
        movePalettesCloser(palettes, tiles, nextPixel, alpha);
    }
}
/*
function colorQuantize1Palette(pixels, randomShuffle, colorsPerPalette) {
    int iterations = (int)(options.fractionOfPixels * (float)pixels.size());
    if (options.dither == Dither.Slow) {
        iterations /= 5;
    }
    const errorStartIteration = iterations * 0.5;
    const alpha = 0.3;
    const colorZeroBehaviour = options.colorZeroBehaviour;
    if (colorZeroBehaviour == ColorZeroBehaviour.TransparentFromColor ||
        colorZeroBehaviour == ColorZeroBehaviour.TransparentFromTransparent) {
        colorsPerPalette -= 1;
    }
    // find average color
    const avgColor = [0, 0, 0];
    for (const pixel of pixels) {
        addColor(avgColor, pixel.color);
    }
    scaleColor(avgColor, 1.0 / pixels.length);
    let sharedColorIndex = -1;
    if (colorZeroBehaviour == ColorZeroBehaviour.Shared) {
        sharedColorIndex = 0;
    }
    const colors = [avgColor];
    let splitIndex = 0;
    for (let numColors = 2; numColors <= colorsPerPalette; numColors++) {
        if (numColors == 2 &&
            colorZeroBehaviour == ColorZeroBehaviour.Shared) {
            colors[0] = cloneColor(options.colorZeroValue);
            colors.push(avgColor);
        }
        else {
            colors.push(cloneColor(colors[splitIndex]));
        }
        const totalColorDistance = new Array(numColors);
        for (let i = 0; i < numColors; i++) {
            totalColorDistance[i] = 0.0;
        }
        for (let iteration = 0; iteration < iterations; iteration++) {
            const nextPixel = pixels[randomShuffle.next()];
            let minColorIndex = -1;
            let minColorDistance = -1;
            let targetColor;
            if (options.dither == Dither.Slow) {
                [minColorIndex, minColorDistance, targetColor] =
                    getClosestColorDither(colors, nextPixel);
            }
            else {
                [minColorIndex, minColorDistance] = getClosestColor(colors, nextPixel.color);
                targetColor = nextPixel.color;
            }
            if (minColorIndex !== sharedColorIndex) {
                moveColorCloser(colors[minColorIndex], targetColor, alpha);
            }
            if (iteration > errorStartIteration) {
                totalColorDistance[minColorIndex] += minColorDistance;
            }
        }
        splitIndex = maxIndex(totalColorDistance);
    }
    return colors;
}
*/

static rgbColor cloneColor(const rgbColor & color) {
    rgbColor result = {color.ch.r, color.ch.g, color.ch.b};
    return result;
}

static void copyColor(rgbColor & dest, const rgbColor & source) {
    dest.ch.r = source.ch.r;
    dest.ch.g = source.ch.g;
    dest.ch.b = source.ch.b;
}

static void addColor(rgbColor & c1, const rgbColor & c2) {
    c1.ch.r += c2.ch.r;
    c1.ch.g += c2.ch.g;
    c1.ch.b += c2.ch.b;
}

static void subtractColor(rgbColor & c1, const rgbColor & c2) {
    c1.ch.r -= c2.ch.r;
    c1.ch.g -= c2.ch.g;
    c1.ch.b -= c2.ch.b;
}

static void scaleColor(rgbColor & color, const double scaleFactor) {
    color.ch.r *= scaleFactor;
    color.ch.g *= scaleFactor;
    color.ch.b *= scaleFactor;
}

static void clampColor(rgbColor & color, const double minValue, const double maxValue) {
    color.ch.r = CLAMP(color.ch.r, minValue, maxValue);
    color.ch.g = CLAMP(color.ch.g, minValue, maxValue);
    color.ch.b = CLAMP(color.ch.b, minValue, maxValue);
}


// alpha = 255 / (2 ** n - 1)
const float alphaValues[] = {0, 255, 85, 36.42857, 17, 8.22581, 4.04762, 2.00787, 1};

static uint8_t toNbitU8(uint8_t value, unsigned int n) {
    // Expects N to be clamped per BITS_PER_CHANNEL_MIN/MAX
    const float alpha = alphaValues[n];
    return (uint8_t)round(round((float)value / alpha) * alpha);
}

static double toNbit(double value, unsigned int n) {
    // Expects N to be clamped per BITS_PER_CHANNEL_MIN/MAX
    const double alpha = alphaValues[n];
    return round(round(value / alpha) * alpha);
}

static void toNbitColor(rgbColor & color, unsigned int n) {
    color.ch.r = toNbit(color.ch.r, n);
    color.ch.g = toNbit(color.ch.g, n);
    color.ch.b = toNbit(color.ch.b, n);
}

static void moveColorCloser(rgbColor & color, rgbColor & pixelColor, float alpha) {
    color.ch.r = ((1 - alpha) * color.ch.r) + (alpha * pixelColor.ch.r);
    color.ch.g = ((1 - alpha) * color.ch.g) + (alpha * pixelColor.ch.g);
    color.ch.b = ((1 - alpha) * color.ch.b) + (alpha * pixelColor.ch.b);
}

static int maxIndexDbl(vector <double> values) {
    int maxI = 0;
    for (int i = 1; i < (int)values.size(); i++) {
        if (values[i] > values[maxI]) {
            maxI = i;
        }
    }
    return maxI;
}


// function minIndex(values) {
// Called with vector <unsigned int> and vector <rgbColor> (aka, palette)
// This one is for unsigned int  // TODO: do this the C++ way
static int minIndexDbl(vector <double> values) {
    int minI = 0;
    for (int i = 1; i < (int)values.size(); i++) {
        if (values[i] < values[minI]) {
            minI = i;
        }
    }
    return minI;
}

// Returns parent tile (from list of tiles) for a given pixel
// which may have been detached from the tile.
static Tile & getParentTile(vector <Tile> & tiles, const pixelEntry & pixel) {
    return tiles[pixel.parent_tile_id];
}

static double randRange0to1(void) {
    return (   (double)rand() / ((double)(RAND_MAX) + (double)(1)));
}

// The JS version of this returns -1, but preferred types have been int
// Unclear if designed behavior is relying on -1 so far
static int indexOf(const vector <int> & vec, int matchValue) {
    for (int index = 0; index < (int)vec.size(); index++) {
        if (vec[index] == matchValue) return index;
    }
    return 0;  // TODO: May need to switch to int and return -1 to signal failure (or some other method that works with expectations in the code)
}

static rgbColorU8 rgbColorToU8(const rgbColor & col) {
    rgbColorU8 newColor = {(uint8_t)col.ch.r,
                           (uint8_t)col.ch.g,
                           (uint8_t)col.ch.b};
    return (newColor);
}

static void printPalettes(const vector <vector <rgbColor>> & palettes) {
    printf("Num Palettes: %d\n", (int)palettes.size());
    for (int palId = 0; palId < (int)palettes.size(); palId++) {
        printf("--> Palette [%d] Size: %d \n", palId, (int)palettes[palId].size());
        for (int colorId = 0; colorId < (int)palettes[palId].size(); colorId++) {
            printf("  - Pal Color[%d][%d] = r:%0.2f, g:%0.2f, b:%0.2f\n",
                    palId, colorId, palettes[palId][colorId].ch.r, palettes[palId][colorId].ch.g, palettes[palId][colorId].ch.b);
        }
    }
}