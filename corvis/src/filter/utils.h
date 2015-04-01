#ifndef __UTILS_H__
#define __UTILS_H__

#include "quaternion.h"
#include "vec4.h"

static inline quaternion initial_orientation_from_gravity(const v4 gravity)
{
    v4 z(0., 0., 1., 0.);
    quaternion q = rotation_between_two_vectors(gravity, z);
    //we want camera to face positive y, so device faces negative y
    v4 y(0., -1., 0., 0.);
    v4 zt = quaternion_rotate(q, z);
    //project the transformed z vector onto the x-y plane
    zt[2] = 0.;
    f_t len = zt.norm();
    if(len > 1.e-6) // otherwise we're looking straight up or down, so don't make any changes
        {
            quaternion dq = rotation_between_two_vectors_normalized(zt / len, y);
            q = quaternion_product(dq, q);
        }
    return q;
}

#endif//__UTILS_H__
