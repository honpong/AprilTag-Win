#ifndef __DETECTOR_FAST_H
#define __DETECTOR_FAST_H

#include <vector>
using namespace std;

extern "C" {
#include "cor.h"
}

void detect_fast(const uint8_t * im, const uint8_t * mask, int width, int height, vector<feature_t> & keypts, int number_wanted);

#endif
