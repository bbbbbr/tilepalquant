#pragma once

#include <string>
#include <cstring>

#include "options.h"
#include "image.h"
#include "image_png.h"

using namespace std;

struct Candidate {
    int       colorIndex;
    double    colorDistance;
    rgbColor  comparedColor;
    double    brightness;
};

int quantizeImage(quantOptions & quantizationOptions, Image & image);
