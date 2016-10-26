#ifndef __UTILS_H__
#define __UTILS_H__

#include "quaternion.h"
#include "vec4.h"
static inline quaternion initial_orientation_from_gravity_facing(const v3 &world_up,     const v3 &body_up,
                                                                 const v3 &world_facing, const v3 &body_forward)
{
    quaternion q = rotation_between_two_vectors(body_up, world_up);
    v3 zt = q * body_forward;
    zt -= world_up * zt.dot(world_up); // remove the component of zt in the world_up direction
    f_t len = zt.norm();
    if(len > 1.e-6) // otherwise we're looking straight up or down, so don't make any changes
        {
            v3 a = (zt / len).cross(world_facing.normalized());
            f_t d =  (zt / len).dot(world_facing.normalized()), s = sqrt(2*(1 + d));
            quaternion dq = a.norm() > F_T_EPS ? quaternion(s/2, a[0]/s, a[1]/s, a[2]/s).normalized()
                                               : d > 0 ? quaternion::Identity() : quaternion(0,0,0,1);
            q = dq * q;
        }
    return q;
}

#endif//__UTILS_H__
