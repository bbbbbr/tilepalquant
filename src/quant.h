#pragma once

#include <string>
#include <cstring>

#include "options.h"
#include "image.h"
#include "image_png.h"

using namespace std;

// vector tileSet

int quantizeImage(quantOptions & quantizationOptions, Image & image);