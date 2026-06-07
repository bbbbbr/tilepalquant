#pragma once
using namespace std;

#define VERSION "1.0.1"


template <typename T> T MIN(const T& value1, const T& value2)
{
  return value1 < value2 ? value1 : value2;
}

template <typename T> T MAX(const T& value1, const T& value2)
{
  return value1 > value2 ? value1 : value2;
}

template <typename T> T CLAMP(const T& value, const T& low, const T& high)
{
  return value < low ? low : (value > high ? high : value);
}
