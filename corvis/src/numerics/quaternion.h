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
#include "rotation.h"
#include "rotation_vector.h"

class quaternion {
public:
    quaternion(): data(1, 0, 0, 0) {}
    quaternion(const f_t other0, const f_t other1, const f_t other2, const f_t other3): data(other0, other1, other2, other3) {}
    
    f_t w() const { return data[0]; }
    f_t x() const { return data[1]; }
    f_t y() const { return data[2]; }
    f_t z() const { return data[3]; }
    f_t &w() { return data[0]; }
    f_t &x() { return data[1]; }
    f_t &y() { return data[2]; }
    f_t &z() { return data[3]; }

    const quaternion operator*(const quaternion &q) const {
        return quaternion(w() * q.w() - x() * q.x() - y() * q.y() - z() * q.z(),
                          x() * q.w() + w() * q.x() - z() * q.y() + y() * q.z(),
                          y() * q.w() + z() * q.x() + w() * q.y() - x() * q.z(),
                          z() * q.w() - y() * q.x() + x() * q.y() + w() * q.z());
    }

private:
    v4 data;
};

static inline std::ostream& operator<<(std::ostream &stream, const quaternion &q)
{
    return stream << "[" << q.w() << ", (" << q.x() << ", " << q.y() << ", " << q.z() << ")]";
}

static inline bool operator==(const quaternion &a, const quaternion &b)
{
    return a.w() == b.w() && a.x() == b.x() && a.y() == b.y() && a.z() == b.z();
}

static inline quaternion conjugate(const quaternion &q)
{
    return quaternion(q.w(), -q.x(), -q.y(), -q.z());
}

static inline quaternion normalize(const quaternion &a) {
    f_t norm = 1 / sqrt(a.w() * a.w() + a.x() * a.x() + a.y() * a.y() + a.z() * a.z());
    return quaternion(a.w() * norm, a.x() * norm, a.y() * norm, a.z() * norm);
}

//This is the same as the right jacobian of quaternion_rotate
static inline m4 to_rotation_matrix(const quaternion &q)
{
    f_t
    xx = q.x()*q.x(),
    yy = q.y()*q.y(),
    zz = q.z()*q.z(),
    wx = q.w()*q.x(),
    wy = q.w()*q.y(),
    wz = q.w()*q.z(),
    xy = q.x()*q.y(),
    xz = q.x()*q.z(),
    yz = q.y()*q.z();

    //Need explicit naming of return type to work around compiler bug
    //https://connect.microsoft.com/VisualStudio/Feedback/Details/1515546
    return m4 {
        {1 - 2 * (yy+zz),     2 * (xy-wz),     2 * (xz+wy), 0},
        {    2 * (xy+wz), 1 - 2 * (xx+zz),     2 * (yz-wx), 0},
        {    2 * (xz-wy),     2 * (yz+wx), 1 - 2 * (xx+yy), 0},
        {    0,               0,               0,           1}
    };
}

// Assumes q is unit
static inline const v4 operator*(const quaternion &q, const v4 &v)
{
    return to_rotation_matrix(q) * v;
}

static inline quaternion to_quaternion(const m4 &m)
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
    return normalize(res);
}

static inline quaternion to_quaternion(const rotation_vector &v) {
    rotation_vector w(v.x()/2, v.y()/2, v.z()/2);
    f_t th2, th = sqrt(th2=w.norm2()), C = cos(th), S = sinc(th,th2);
    return quaternion(C, S * w.x(), S * w.y(), S * w.z()); // e^(v/2)
}

static inline rotation_vector to_rotation_vector(const quaternion &q) {
    f_t denom = sqrt(q.x()*q.x() + q.y()*q.y() + q.z()*q.z());
    if(denom == 0.) return rotation_vector(q.x(), q.y(), q.z());
    f_t scale = 2 * acos(q.w()) / denom;
    return rotation_vector(q.x() * scale, q.y() * scale, q.z() * scale); // 2 log(q)
}

static inline rotation_vector to_rotation_vector(const m4 &R)
{
    return to_rotation_vector(to_quaternion(R));
}

//Assumes a and b are already normalized
static inline quaternion rotation_between_two_vectors_normalized(const v4 &a, const v4 &b)
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
        f_t s = sqrt(2 * (1 + d));

        v4 axis = cross(a, b);
        res = quaternion(s / 2, axis[0] / s, axis[1] / s, axis[2] / s);
    }
    return normalize(res);
}

static inline quaternion rotation_between_two_vectors(const v4 &a, const v4 &b)
{
    //make sure the 3rd element is zero and normalize
    v4 an(a[0], a[1], a[2], 0);
    v4 bn(b[0], b[1], b[2], 0);
    return rotation_between_two_vectors_normalized(an.normalized(), bn.normalized());
}

#endif
