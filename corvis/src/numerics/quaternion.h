// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __QUATERNION_H
#define __QUATERNION_H

#include "vec4.h"


class quaternion: private v4 {
public:
    quaternion(const v_intrinsic &other): v4(other) {}
    quaternion(const f_t other0, const f_t other1, const f_t other2, const f_t other3): v4(other0, other1, other2, other3) {}

    using v4::data;
    using v4::operator[];
};

static inline quaternion operator*(const quaternion &a, const f_t other) { return quaternion(a.data * v4(other).data); }
static inline quaternion operator*(const f_t other, const quaternion &a) { return quaternion(a.data * v4(other).data); }
static inline quaternion operator+(const quaternion &a, const quaternion &other) { return quaternion(a.data + other.data); }

static inline quaternion quaternion_product(const quaternion &a, const quaternion &b) {
    return quaternion(a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
                      a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
                      a[0] * b[2] + a[2] * b[0] + a[3] * b[1] - a[1] * b[3],
                      a[0] * b[3] + a[3] * b[0] + a[1] * b[2] - a[2] * b[1]);
}

static inline m4 quaternion_product_left_jacobian(const quaternion &b) {
    return (m4) {{
        v4(b[0], -b[1], -b[2], -b[3]),
        v4(b[1], b[0], b[3], -b[2]),
        v4(b[2], -b[3], b[0], b[1]),
        v4(b[3], b[2], -b[1], b[0])
    }};
}

static inline m4 quaternion_product_right_jacobian(const quaternion &a) {
    return (m4) {{
        v4(a[0], -a[1], -a[2], -a[3]),
        v4(a[1], a[0], -a[3], a[2]),
        v4(a[2], a[3], a[0], -a[1]),
        v4(a[3], -a[2], a[1], a[0])
    }};
}

static inline quaternion quaternion_normalize(const quaternion &a) {
    return a * (1. / norm(a.data));
}

static inline v4 qvec_cross(const quaternion &a, const v4 &b) {
    return v4(a[2] * b[2] - a[3] * b[1],
              a[3] * b[0] - a[1] * b[2],
              a[1] * b[1] - a[2] * b[0],
              0);
}

static inline v4 quaternion_rotate(const quaternion &q, const v4 &v) {
    v4 u(q[1], q[2], q[3], 0.);
    v4 uv = qvec_cross(q, v);
    v4 uuv = qvec_cross(q, uv);
    return v + 2 * q[0] * uv + 2. * uuv;
}

static inline m4 quaternion_to_rotation_matrix(const quaternion &q)
{
    f_t
    bb = q[1]*q[1],
    cc = q[2]*q[2],
    dd = q[3]*q[3],
    ab = q[0]*q[1],
    ac = q[0]*q[2],
    ad = q[0]*q[3],
    bc = q[1]*q[2],
    bd = q[1]*q[3],
    cd = q[2]*q[3];
    
    return (m4) {{
        v4(1. - 2. * (cc+dd), 2. * (bc-ad), 2. * (bd+ac), 0.),
        v4(2. * (bc+ad), 1. - 2. * (bb+dd), 2. * (cd-ab), 0.),
        v4(2. * (bd-ac), 2. * (cd+ab), 1. - 2. * (bb+cc), 0.),
        v4(0., 0., 0., 1.)
    }};
}

static inline m4v4 quaternion_to_rotation_matrix_jacobian(const quaternion &q)
{
    return (m4v4) {{
        {{-2. * v4(0., 0., 2.*q[2], 2.*q[3]), 2. * v4(-q[3], q[2], q[1], -q[0]), 2. * v4(q[2], q[3], q[0], q[1]), v4(0.)}},
        {{2. * v4(q[3], q[2], q[1], q[0]), -2. * v4(0., 2.*q[1], 0., 2.*q[3]), 2. * v4(-q[1], -q[0], q[3], q[2]), v4(0.)}},
        {{2. * v4(-q[2], q[3], -q[0], q[1]), 2. * v4(q[1], q[0], q[3], q[2]), -2. * v4(0., 2.*q[1], 2.*q[2], 0.), v4(0.)}},
        {{v4(0.), v4(0.), v4(0.), v4(0.)}}
    }};
}

static inline quaternion rotvec_to_quaternion(const v4 &v) {
    f_t theta2 = sum(v * v);
    f_t theta = sqrt(theta2);
    f_t sinterm = (theta2 * theta2 < FLT_EPSILON) ? .5 + theta2 / 48. : sin(.5 * theta) / theta;
    return quaternion(cos(theta * .5), sinterm * v[0], sinterm * v[1], sinterm * v[2]);
}

static inline quaternion integrate_angular_velocity_quaternion(const quaternion &Q, const v4 &w)
{
    quaternion product(-Q[1] * w[0] - Q[2] * w[1] - Q[3] * w[2],
                        Q[0] * w[0] + Q[2] * w[2] - Q[3] * w[1],
                        Q[0] * w[1] + Q[3] * w[0] - Q[1] * w[2],
                        Q[0] * w[2] + Q[1] * w[1] - Q[2] * w[0]);
    return Q + .5 * product;
}

static inline void linearize_angular_integration_quaternion(const quaternion &Q, const v4 &w, m4 &dQ_dQ, m4 &dQ_dw)
{
    dQ_dQ = m4_identity + .5 *
    (m4){{
        v4(0., -w[0], -w[1], w[2]),
        v4(w[0], 0., w[2], -w[1]),
        v4(w[1], -w[2], 0., w[0]),
        v4(w[2], w[1], -w[0], 0.)
    }};
    dQ_dw = .5 * (m4) {{
        v4(-Q[1], -Q[2], -Q[3], 0.),
        v4(Q[0], -Q[3], Q[2], 0.),
        v4(Q[3], Q[0], -Q[1], 0.),
        v4(-Q[2], Q[1], Q[0], 0.)
    }};
}

#endif
