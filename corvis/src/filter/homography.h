//
//  homography.h
//  RC3DK
//
//  Created by Eagle Jones on 11/13/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef __RC3DK__homography__
#define __RC3DK__homography__

#include "../cor/cor_types.h"
#include "../numerics/vec4.h"


void compute_planar_homography(const feature_t p1[4], const feature_t p2[4], m4 & R, v4 & T);
void compute_qr_homography(feature_t calibrated_points[4], float qr_width);

#endif /* defined(__RC3DK__homography__) */
