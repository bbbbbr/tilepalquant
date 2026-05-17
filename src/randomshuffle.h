#pragma once

#include <string>
#include <cstring>

#include <stdio.h> // DEBUG

using namespace std;

struct RandomShuffle {
    vector <size_t> values;
    size_t currentIndex;  // TODO: size_t?

    void init(size_t n) {
        for (size_t i = 0; i < n; i++) {
            values.push_back(i);
            currentIndex = n - 1;
        }
    }

    void shuffle() {
        for (size_t i = 0; i < values.size(); i++) {
            // const index = i + floor(rand() * (values.size() - i));
            // TODO: why doesn't it swap with ANY index in the range, and instead only from a gradually shrinking section at the end?
            const size_t index = i + (size_t)floor(((double)rand() / ((double)(RAND_MAX)+(double)(1))) * (double)(values.size() - i));  // TODO: more idiomatic rand range?
            const size_t tmp = values[i];
            values[i] = values[index];
            values[index] = tmp;
        }
    }

    size_t next() {
        currentIndex += 1;
        if (currentIndex >= values.size()) {
            shuffle();
            currentIndex = 0;
        }
        return values[currentIndex];
    }
};

