#pragma once
#include <vector>

using namespace std;

struct Tile;

struct pixelEntry {
    // parent_tile_id replaces the reference to a pixel's parent tile used in the JS version
    size_t   parent_tile_id;
    rgbColor color;
    unsigned int x;
    unsigned int y;
};


// Ref: extractTile(image, startX, startY) {
// ...
// const tile = {
//     colors: [],
//     counts: [],
//     pixels: [],
// };
struct Tile
{
    vector <rgbColor>   colors;  // GOING TO HAVE TO MAKE ALL COLORS DOUBLES I THINK
    vector <size_t>     counts;
    vector <pixelEntry> pixels;
};

