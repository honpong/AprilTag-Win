#ifndef FAST_H
#define FAST_H

#include <vector>
#include "scaled_mask.h"
#include "tracker.h"

typedef struct { float x, y, score, reserved; } xy; 
typedef unsigned char byte;

class fast_detector_9 {
 private:
    int pixel[16];
    int xsize, ysize, stride, patch_stride, patch_win_half_width;

 public:
    std::vector<xy> features;
    std::vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    std::vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    template<typename Descriptor>
    xy track(const Descriptor& descriptor, const tracker::image& image, float predx, float predy, float radius, int b);
    void init(const int image_width, const int image_height, const int image_stride, const int tracker_patch_stride, const int tracker_patch_win_half_width);
};

class fast_detector_10 {
 private:
    int pixel[16];
    int xsize, ysize, stride, patch_stride, patch_win_half_width;

public:
    std::vector<xy> features;
    std::vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    std::vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    template<typename Descriptor>
    xy track(const Descriptor& descriptor, const tracker::image& image, float predx, float predy, float radius, int b);
    void init(const int image_width, const int image_height, const int image_stride, const int tracker_patch_stride, const int tracker_patch_win_half_width);
};


class fast_detector_11 {
 private:
    int pixel[16];
    int xsize, ysize, stride, patch_stride, patch_win_half_width;

 public:
    std::vector<xy> features;
    std::vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    std::vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    template<typename Descriptor>
    xy track(const Descriptor& descriptor, const tracker::image& image, float predx, float predy, float radius, int b);
    void init(const int image_width, const int image_height, const int image_stride, const int tracker_patch_stride, const int tracker_patch_win_half_width);
};

class fast_detector_12 {
 private:
    int pixel[16];
    int xsize, ysize, stride, patch_stride, patch_win_half_width;

 public:
    std::vector<xy> features;
    std::vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    std::vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    template<typename Descriptor>
    xy track(const Descriptor& descriptor, const tracker::image& image, float predx, float predy, float radius, int b);
    void init(const int image_width, const int image_height, const int image_stride, const int tracker_patch_stride, const int tracker_patch_win_half_width);
};

#endif
