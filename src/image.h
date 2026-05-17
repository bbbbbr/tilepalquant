#pragma once
#include <vector>

using namespace std;

#define RGB_SZ      3             // 3 values per pixel
#define RGB888_SZ   3             // 3 bytes per pixel
#define RGBA8888_SZ 4             // 4 bytes per pixel
#define RGBA32_SZ   (RGBA8888_SZ) // RGBA 8:8:8:8 is 4 bytes per pixel

// ABGR:8888 (in 8 bit array format, OR RGBA:32 packed int on little-endian systems when accessed as bytes)
#define ABGR8_R              3 //
#define ABGR8_G              2 //
#define ABGR8_B              1 //
#define ABGR8_ALPHA          0 // Alpha channel in [0]

// RGB:888 (in 8 bit array format)
#define RGB_R              0 //
#define RGB_G              1 //
#define RGB_B              2 //

// RGBA:8888 (in 8 bit array format)
#define RGBA8_R              0 // Alpha channel is [0]
#define RGBA8_G              1 // Alpha channel in [1]
#define RGBA8_B              2 // Alpha channel in [2]
#define RGBA8_ALPHA          3 // Alpha channel in [3]

#define RGBA32(R,G,B,A) ((R << 24) | (G << 16) | (B << 8) | A)
#define RGB24(R,G,B)    ((R << 16) | (G << 8) | B)

#define ALPHA_FULLY_TRANSPARENT       0  // Full alpha channel transparency
#define RGBA32_TRANSPARENT_WHITE      (RGBA32(255,255,255,ALPHA_FULLY_TRANSPARENT))  // White, full transparency

// #define MAX(A,B) ((A)>(B)?(A):(B))

struct rgbColor {
    union {
        double chan[RGB_SZ];
        struct {
            double r;
            double g;
            double b;
        } ch;
    };
};


struct Image {
    vector< unsigned char > data; //data in indexed format
    unsigned int width;
    unsigned int height;

    // size_t colors_per_pal;  // Number of colors per palette (ex: CGB has 4 colors per palette x 8 palettes total)
    // size_t total_color_count; // Total number of colors across all palettes (palette_count x colors_per_pal)
    // unsigned char * palette = NULL; //palette colors in RGBA (1 color == 4 bytes)
    // unsigned char * source_tileset_palette = NULL;  // Mostly used for ensuring source tileset and primary image palettes match sufficiently
};
