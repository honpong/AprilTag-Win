#ifndef FAST_H
#define FAST_H

#include <vector>
using namespace std;

typedef struct { float x, y, score, reserved; } xy; 
typedef unsigned char byte;

class fast_detector {
 public:
    vector<xy> features;
    vector<xy> &detect(const unsigned char *im, const unsigned char *mask, int xsize, int ysize, int stride, int number_wanted, int bthresh);
};

#endif
