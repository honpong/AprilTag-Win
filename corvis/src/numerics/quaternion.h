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
#include "rotation.h"

class quaternion {
public:
    quaternion(): data(1., 0., 0., 0.) {}
    quaternion(const f_t other0, const f_t other1, const f_t other2, const f_t other3): data(other0, other1, other2, other3) {}

    const f_t w() const { return data[0]; }
    const f_t x() const { return data[1]; }
    const f_t y() const { return data[2]; }
    const f_t z() const { return data[3]; }
    f_t &w() { return data[0]; }
    f_t &x() { return data[1]; }
    f_t &y() { return data[2]; }
    f_t &z() { return data[3]; }
    
private:
    v4 data;
};

static inline std::ostream& operator<<(std::ostream &stream, const quaternion &q)
{
    return stream << "[" << q.w() << ", (" << q.x() << ", " << q.y() << ", " << q.z() << ")]";
}

/*static inline quaternion operator*(const quaternion &a, const f_t other) { return quaternion(a.data * v4(other).data); }
static inline quaternion operator*(const f_t other, const quaternion &a) { return quaternion(a.data * v4(other).data); }
static inline quaternion operator+(const quaternion &a, const quaternion &other) { return quaternion(a.data + other.data); }*/

static inline bool operator==(const quaternion &a, const quaternion &b)
{
    return a.w() == b.w() && a.x() == b.x() && a.y() == b.y() && a.z() == b.z();
}

static inline quaternion quaternion_product(const quaternion &a, const quaternion &b) {
    return quaternion(a.w() * b.w() - a.x() * b.x() - a.y() * b.y() - a.z() * b.z(),
                      a.w() * b.x() + a.x() * b.w() + a.y() * b.z() - a.z() * b.y(),
                      a.w() * b.y() + a.y() * b.w() + a.z() * b.x() - a.x() * b.z(),
                      a.w() * b.z() + a.z() * b.w() + a.x() * b.y() - a.y() * b.x());
}

static inline m4 quaternion_product_left_jacobian(const quaternion &b) {
    return (m4) {{
        v4(b.w(), -b.x(), -b.y(), -b.z()),
        v4(b.x(), b.w(), b.z(), -b.y()),
        v4(b.y(), -b.z(), b.w(), b.x()),
        v4(b.z(), b.y(), -b.x(), b.w())
    }};
}

static inline m4 quaternion_product_right_jacobian(const quaternion &a) {
    return (m4) {{
        v4(a.w(), -a.x(), -a.y(), -a.z()),
        v4(a.x(), a.w(), -a.z(), a.y()),
        v4(a.y(), a.z(), a.w(), -a.x()),
        v4(a.z(), -a.y(), a.x(), a.w())
    }};
}

static inline quaternion normalize(const quaternion &a) {
    f_t norm = 1. / sqrt(a.w() * a.w() + a.x() * a.x() + a.y() * a.y() + a.z() * a.z());
    return quaternion(a.w() * norm, a.x() * norm, a.y() * norm, a.z() * norm);
}

static inline v4 qvec_cross(const quaternion &a, const v4 &b) {
    return v4(a.y() * b[2] - a.z() * b[1],
              a.z() * b[0] - a.x() * b[2],
              a.x() * b[1] - a.y() * b[0],
              0);
}

static inline v4 quaternion_rotate(const quaternion &q, const v4 &v) {
    v4 u(q.x(), q.y(), q.z(), 0.);
    v4 uv = qvec_cross(q, v);
    v4 uuv = qvec_cross(q, uv);
    return v + 2 * q.w() * uv + 2. * uuv;
}

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
    
    return (m4) {{
        v4(1. - 2. * (yy+zz), 2. * (xy-wz), 2. * (xz+wy), 0.),
        v4(2. * (xy+wz), 1. - 2. * (xx+zz), 2. * (yz-wx), 0.),
        v4(2. * (xz-wy), 2. * (yz+wx), 1. - 2. * (xx+yy), 0.),
        v4(0., 0., 0., 1.)
    }};
}

static inline m4v4 to_rotation_matrix_jacobian(const quaternion &q)
{
    return (m4v4) {{
        {{-2. * v4(0., 0., 2.*q.y(), 2.*q.z()), 2. * v4(-q.z(), q.y(), q.x(), -q.w()), 2. * v4(q.y(), q.z(), q.w(), q.x()), v4(0.)}},
        {{2. * v4(q.z(), q.y(), q.x(), q.w()), -2. * v4(0., 2.*q.x(), 0., 2.*q.z()), 2. * v4(-q.x(), -q.w(), q.z(), q.y()), v4(0.)}},
        {{2. * v4(-q.y(), q.z(), -q.w(), q.x()), 2. * v4(q.x(), q.w(), q.z(), q.y()), -2. * v4(0., 2.*q.x(), 2.*q.y(), 0.), v4(0.)}},
        {{v4(0.), v4(0.), v4(0.), v4(0.)}}
    }};
}

static inline quaternion rotvec_to_quaternion(const v4 &v) {
    f_t theta2 = sum(v * v);
    f_t theta = sqrt(theta2);
    f_t sinterm = (theta2 * theta2 < FLT_EPSILON) ? .5 + theta2 / 48. : sin(.5 * theta) / theta;
    return quaternion(cos(theta * .5), sinterm * v[0], sinterm * v[1], sinterm * v[2]);
}

static inline quaternion integrate_angular_velocity(const quaternion &Q, const v4 &w)
{
    return quaternion(Q.w() + .5 * (-Q.x() * w[0] - Q.y() * w[1] - Q.z() * w[2]),
                      Q.x() + .5 * (Q.w() * w[0] + Q.y() * w[2] - Q.z() * w[1]),
                      Q.y() + .5 * (Q.w() * w[1] + Q.z() * w[0] - Q.x() * w[2]),
                      Q.z() + .5 * (Q.w() * w[2] + Q.x() * w[1] - Q.y() * w[0]));
}

static inline void integrate_angular_velocity_jacobian(const quaternion &Q, const v4 &w, m4 &dQ_dQ, m4 &dQ_dw)
{
    dQ_dQ = m4_identity + .5 *
    (m4){{
        v4(0., -w[0], -w[1], w[2]),
        v4(w[0], 0., w[2], -w[1]),
        v4(w[1], -w[2], 0., w[0]),
        v4(w[2], w[1], -w[0], 0.)
    }};
    dQ_dw = .5 * (m4) {{
        v4(-Q.x(), -Q.y(), -Q.z(), 0.),
        v4(Q.w(), -Q.z(), Q.y(), 0.),
        v4(Q.z(), Q.w(), -Q.x(), 0.),
        v4(-Q.y(), Q.x(), Q.w(), 0.)
    }};
}

#endif
