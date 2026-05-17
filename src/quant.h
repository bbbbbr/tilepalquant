#pragma once

#include <string>
#include <cstring>

#include "options.h"
#include "image.h"
#include "image_png.h"

using namespace std;

struct Candidate {
    size_t    colorIndex;
    double    colorDistance;
    rgbColor  comparedColor;
    double    brightness;
};

int quantizeImage(quantOptions & quantizationOptions, Image & image);