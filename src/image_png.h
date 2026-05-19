#pragma once
#include <vector>

using namespace std;

int loadImageRGBAFromPNG(quantOptions & options, Image & sourceImageRGBA);
int saveImageRGBAToPNG(quantOptions & options, Image & sourceImageRGBA);