#pragma once

#include <string>
#include <cstring>

#include "options.h"
#include "image.h"
#include "image_png.h"

using namespace std;

int quantizeImage(quantOptions * quantizationOptions, Image * image);