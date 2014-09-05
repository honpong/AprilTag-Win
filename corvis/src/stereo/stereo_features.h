#ifndef __STEREO_FEATURES_H
#define __STEREO_FEATURES_H

#include "sift.h"
#include "../cor/cor_types.h"

#include <vector>
using namespace std;

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

vector<sift_match> ubc_match(vector<sift_keypoint> k1, vector<sift_keypoint>k2);
vector<sift_keypoint> sift_detect(uint8_t * image, int width, int height, int noctaves, int nlevels, int o_min);

#endif
