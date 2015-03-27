#ifndef __UTILS_H__
#define __UTILS_H__

#include "quaternion.h"
#include "vec4.h"

static inline quaternion initial_orientation_from_gravity_facing(const v4 gravity, const v4 facing)
{
    v4 z(0., 0., 1., 0.);
    quaternion q = rotation_between_two_vectors(gravity, z);
    v4 zt = quaternion_rotate(q, -z/*camera in accelerometer frame*/);
    //project the transformed z vector onto the x-y plane
    zt[2] = 0.;
    f_t len = norm(zt);
    if(len > 1.e-6) // otherwise we're looking straight up or down, so don't make any changes
        {
            quaternion dq = rotation_between_two_vectors_normalized(zt / len, normalize(facing));
            q = quaternion_product(dq, q);
        }
    return q;
}

static inline quaternion initial_orientation_from_gravity(const v4 gravity) {
    return initial_orientation_from_gravity_facing(gravity, v4(0,1,0,0)); // camera faces +y
}

#endif//__UTILS_H__
