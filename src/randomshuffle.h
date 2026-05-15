#pragma once

#include <string>
#include <cstring>

using namespace std;

struct RandomShuffle {
    vector <unsigned int> values;
    unsigned int currentIndex;
    void init(unsigned int n) {
        for (unsigned int i = 0; i < n; i++) {
            values.push_back(i);
            currentIndex = n - 1;
        }
    }
    void shuffle() {
        for (unsigned int i = 0; i < values.size(); i++) {
            const unsigned int index = i + std::floor(std::rand() * (values.size() - i));
            const unsigned int tmp = values[i];
            values[i] = values[index];
            values[index] = tmp;
        }
    }
    unsigned int next() {
        currentIndex += 1;
        if (currentIndex >= values.size()) {
            shuffle();
            currentIndex = 0;
        }
        return values[currentIndex];
    }
};

