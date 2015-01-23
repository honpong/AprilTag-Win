//
//  homography.h
//  RC3DK
//
//  Created by Eagle Jones on 11/13/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef __RC3DK__homography__
#define __RC3DK__homography__

#include <vector>
using namespace std;

#include "../cor/cor_types.h"
#include "../numerics/vec4.h"

typedef struct _homography_decomposition {
    m4 R;
    v4 T;
    v4 N;
} homography_decomposition;

bool homography_compute_one_sided(const v4 world_points[4], const feature_t calibrated[4], m4 &R, v4 &T);
m4 homography_compute(const feature_t p1[4], const feature_t p2[4]);
vector<homography_decomposition> homography_decompose(const feature_t p1[4], const feature_t p2[4], const m4 & H);
bool homography_align_qr_ideal(const feature_t p2[4], float qr_size, bool use_markers, homography_decomposition & result);
void homography_ideal_to_qr(m4 & R, v4 & T);
bool homography_align_to_qr(const feature_t p2[4], float qr_size, m4 & R, v4 & T);

#endif /* defined(__RC3DK__homography__) */
