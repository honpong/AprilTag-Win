#ifndef FAST_H
#define FAST_H

#include <vector>
using namespace std;

typedef struct { float x, y, score, reserved; } xy; 
typedef unsigned char byte;

class fast_detector {
 private:
    int pixel[16];
    int xsize, ysize, stride;
    float score_match(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error);
    float score_match_ring(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error);

 public:
    vector<xy> features;
    vector<xy> &detect(const unsigned char *im, const unsigned char *mask, int number_wanted, int bthresh);
    vector<xy> &detect_threshold(const unsigned char *im, const unsigned char *mask, int b);
    xy track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, int x1, int y1, int x2, int y2, int b);
    fast_detector(const int xsize, const int ysize, const int stride);
};

#endif
