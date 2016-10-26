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

#include "cor_types.h"
#include "vec4.h"

typedef struct _homography_decomposition {
    m3 R;
    v3 T;
    v3 N;
} homography_decomposition;

m3 homography_compute(const feature_t p1[4], const feature_t p2[4]);
vector<homography_decomposition> homography_decompose(const feature_t p1[4], const feature_t p2[4], const m3 & H);
bool homography_align_qr_ideal(const feature_t p2[4], float qr_size_m, bool use_markers, int modules, homography_decomposition & result);
void homography_ideal_to_qr(m3 & R, v3 & T);
bool homography_align_to_qr(const feature_t p2[4], float qr_size_m, int modules, m3 & R, v3 & T);

#endif /* defined(__RC3DK__homography__) */
