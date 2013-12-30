#ifndef FAST_H
#define FAST_H

#include <vector>
using namespace std;

typedef struct { float x, y, score, reserved; } xy; 
typedef unsigned char byte;

class fast_detector_9 {
 private:
    int pixel[16];
    int xsize, ysize, stride;
    float score_match(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error);

 public:
    vector<xy> features;
    vector<xy> &detect(const unsigned char *im, const unsigned char *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    vector<xy> &detect(const unsigned char *im, const unsigned char *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    xy track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, int x1, int y1, int x2, int y2, int b);
    void init(const int xsize, const int ysize, const int stride);
};

class fast_detector_10 {
 private:
    int pixel[16];
    int xsize, ysize, stride;
    float score_match(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error);

 public:
    vector<xy> features;
    vector<xy> &detect(const unsigned char *im, const unsigned char *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    vector<xy> &detect(const unsigned char *im, const unsigned char *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    xy track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, int x1, int y1, int x2, int y2, int b);
    void init(const int xsize, const int ysize, const int stride);
};


class fast_detector_11 {
 private:
    int pixel[16];
    int xsize, ysize, stride;
    float score_match(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error);

 public:
    vector<xy> features;
    vector<xy> &detect(const unsigned char *im, const unsigned char *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    vector<xy> &detect(const unsigned char *im, const unsigned char *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    xy track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, int x1, int y1, int x2, int y2, int b);
    void init(const int xsize, const int ysize, const int stride);
};

class fast_detector_12 {
 private:
    int pixel[16];
    int xsize, ysize, stride;
    float score_match(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error);

 public:
    vector<xy> features;
    vector<xy> &detect(const unsigned char *im, const unsigned char *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    vector<xy> &detect(const unsigned char *im, const unsigned char *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    xy track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, int x1, int y1, int x2, int y2, int b);
    void init(const int xsize, const int ysize, const int stride);
};

#endif
