//
//  quaternion.h
//  RC3DK
//
//  Created by Eagle Jones.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef __QUATERNION_H
#define __QUATERNION_H

#include "vec4.h"
#include "sinc.h"
#include "rotation_vector.h"

static inline std::ostream& operator<<(std::ostream &stream, const quaternion &q)
{
    return stream << "[" << q.w() << ", (" << q.x() << ", " << q.y() << ", " << q.z() << ")]";
}

static inline quaternion to_quaternion(const m3 &m)
{
    v4 tr;
    quaternion res;
    tr[0] = ( m(0, 0) + m(1, 1) + m(2, 2));
    tr[1] = ( m(0, 0) - m(1, 1) - m(2, 2));
    tr[2] = (-m(0, 0) + m(1, 1) - m(2, 2));
    tr[3] = (-m(0, 0) - m(1, 1) + m(2, 2));
    if(tr[0] >= tr[1] && tr[0]>= tr[2] && tr[0] >= tr[3])
    {
        f_t s = 2 * sqrt(tr[0] + 1);
        res.w() = s/4;
        res.x() = (m(2, 1) - m(1, 2)) / s;
        res.y() = (m(0, 2) - m(2, 0)) / s;
        res.z() = (m(1, 0) - m(0, 1)) / s;
    } else if(tr[1] >= tr[2] && tr[1] >= tr[3]) {
        f_t s = 2 * sqrt(tr[1] + 1);
        res.w() = (m(2, 1) - m(1, 2)) / s;
        res.x() = s/4;
        res.y() = (m(1, 0) + m(0, 1)) / s;
        res.z() = (m(2, 0) + m(0, 2)) / s;
    } else if(tr[2] >= tr[3]) {
        f_t s = 2 * sqrt(tr[2] + 1);
        res.w() = (m(0, 2) - m(2, 0)) / s;
        res.x() = (m(1, 0) + m(0, 1)) / s;
        res.y() = s/4;
        res.z() = (m(1, 2) + m(2, 1)) / s;
    } else {
        f_t s = 2 * sqrt(tr[3] + 1);
        res.w() = (m(1, 0) - m(0, 1)) / s;
        res.x() = (m(0, 2) + m(2, 0)) / s;
        res.y() = (m(1, 2) + m(2, 1)) / s;
        res.z() = s/4;
    }
    return res.normalized();
}

static inline quaternion to_quaternion(const rotation_vector &v) {
    rotation_vector w(v/2);
    f_t th2, th = sqrt(th2=w.norm2()), C = cos(th), Sc = sinc(th,th2);
    return quaternion(C, Sc * w.x(), Sc * w.y(), Sc * w.z()); // e^(v/2)
}

static inline rotation_vector to_rotation_vector(const quaternion &q_) {
    quaternion q = q_; if (q.w() < 0) q *= quaternion(-1,0,0,0); // return [0,pi] instead of [0,2pi]
    f_t S2, S = sqrt(S2=q.vec().squaredNorm()), C = q.w();
    f_t scale = 2 * atan2c(S,C,S2); // robust version of 2 asin(S)/S
    return rotation_vector(scale * q.vec()); // 2 log(q)
}

static inline rotation_vector to_rotation_vector(const m3 &R)
{
    return to_rotation_vector(to_quaternion(R));
}

//Assumes a and b are already normalized
static inline quaternion rotation_between_two_vectors_normalized(const v3 &a, const v3 &b)
{
    quaternion res;
    f_t d = a.dot(b);
    if(d < f_t(-1 + 1.e-6)) //the two vector are (nearly) opposite, pick an arbitrary orthogonal axis
    {
        if(fabs(a[0]) > fabs(a[2])) //make sure we have a non-zero element
        {
            res = quaternion(0., -a[1], a[0], 0.);
        } else {
            res = quaternion(0., 0., -a[2], a[1]);
        }
    } else { // normal case
        v3 axis = a.cross(b);
        res = quaternion(1+d, axis[0], axis[1], axis[2]);
    }
    return res.normalized();
}

static inline quaternion rotation_between_two_vectors(const v3 &a, const v3 &b)
{
    return rotation_between_two_vectors_normalized(a.normalized(), b.normalized());
}

#endif
