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

void compute_planar_homography_one_sided(const v4 world_points[4], const feature_t calibrated[4]);

#endif /* defined(__RC3DK__homography__) */
