#pragma once

#include <string>
#include <cstring>

#include <stdio.h> // DEBUG

using namespace std;

struct RandomShuffle {
    vector <int> values;
    int currentIndex;

    void init(int n) {
        for (int i = 0; i < n; i++) {
            values.push_back(i);
            currentIndex = n - 1;
        }
    }

    void shuffle() {
        for (int i = 0; i < (int)values.size(); i++) {
            // const index = i + floor(rand() * (values.size() - i));
            // TODO: why doesn't it swap with ANY index in the range, and instead only from a gradually shrinking section at the end?
            const int index = i + (int)floor(((double)rand() / ((double)(RAND_MAX) + (double)(1))) * (double)(values.size() - i));  // TODO: more idiomatic rand range?
            const int tmp = values[i];
            values[i] = values[index];
            values[index] = tmp;
        }
    }

    int next() {
        currentIndex += 1;
        if (currentIndex >= (int)values.size()) {
            shuffle();
            currentIndex = 0;
        }
        return values[currentIndex];
    }
};

