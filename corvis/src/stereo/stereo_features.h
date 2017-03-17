#ifndef __STEREO_FEATURES_H
#define __STEREO_FEATURES_H

#include "sift.h"
#include "vec4.h"

#include <vector>

typedef struct _sift_keypoint
{
    feature_t pt;
    float sigma;
    vl_sift_pix d [128] ;
} sift_keypoint;

typedef struct _sift_match
{
    int i1, i2;
    float score;
} sift_match;

std::vector<sift_match> ubc_match(std::vector<sift_keypoint> k1, std::vector<sift_keypoint>k2, void(*progress_callback)(float)=NULL, float progress_start = 0, float progress_end = 1);
std::vector<sift_keypoint> sift_detect(uint8_t * image, int width, int height, int noctaves, int nlevels, int o_min, void(*progress_callback)(float)=NULL, float progress_start = 0, float progress_end = 1);

#endif
