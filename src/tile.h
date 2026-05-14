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
    vector <unsigned char> colors; // TODO: what type
    vector <unsigned char> counts;
    vector <pixelEntry> pixels;

    // unsigned char pal;

    //     Tile(size_t size = 0) : data(size), pal(0) {}
    //     bool operator==(const Tile& t) const
    //     {
    // //        return data == t.data && pal == t.pal; // probably, sometimes we need to take palette into account?
    //         return data == t.data;
    //     }

    //     const Tile& operator=(const Tile& t)
    //     {
    //         data = t.data;
    //         pal = t.pal;
    //         return *this;
    //     }
};

