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

static inline bool operator==(const quaternion &a, const quaternion &b)
{
    return a.w() == b.w() && a.x() == b.x() && a.y() == b.y() && a.z() == b.z();
}

static inline quaternion conjugate(const quaternion &q)
{
    return quaternion(q.w(), -q.x(), -q.y(), -q.z());
}

static inline quaternion quaternion_product(const quaternion &a, const quaternion &b) {
    return quaternion(a.w() * b.w() - a.x() * b.x() - a.y() * b.y() - a.z() * b.z(),
                      a.w() * b.x() + a.x() * b.w() + a.y() * b.z() - a.z() * b.y(),
                      a.w() * b.y() + a.y() * b.w() + a.z() * b.x() - a.x() * b.z(),
                      a.w() * b.z() + a.z() * b.w() + a.x() * b.y() - a.y() * b.x());
}

static inline m4 quaternion_product_left_jacobian(const quaternion &b) {
    return {{
        {b.w(), -b.x(), -b.y(), -b.z()},
        {b.x(), b.w(), b.z(), -b.y()},
        {b.y(), -b.z(), b.w(), b.x()},
        {b.z(), b.y(), -b.x(), b.w()}
    }};
}

static inline m4 quaternion_product_right_jacobian(const quaternion &a) {
    return {{
        {a.w(), -a.x(), -a.y(), -a.z()},
        {a.x(), a.w(), -a.z(), a.y()},
        {a.y(), a.z(), a.w(), -a.x()},
        {a.z(), -a.y(), a.x(), a.w()}
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
    /* original quaternion_rotate:
     v4 qv = qvec_cross(q, v);
     v4 qqv = qvec_cross(q, qv);
     return v + 2. * q.w() * qv + 2. * qqv;
     */
   /* v4 wqv = v4(q.w() * (q.y() * v[2] - q.z() * v[1]),
                q.w() * (q.z() * v[0] - q.x() * v[2]),
                q.w() * (q.x() * v[1] - q.y() * v[0]),
                0);
    
    v4 qqv = v4(q.y() * (q.x() * v[1] - q.y() * v[0]) - q.z() * (q.z() * v[0] - q.x() * v[2]),
                q.z() * (q.y() * v[2] - q.z() * v[1]) - q.x() * (q.x() * v[1] - q.y() * v[0]),
                q.x() * (q.z() * v[0] - q.x() * v[2]) - q.y() * (q.y() * v[2] - q.z() * v[1]),
                0);
    
    return v + 2. * (wqv + qqv);
    */
    
    f_t
    w = q.w(),
    x = q.x(),
    y = q.y(),
    z = q.z(),
    x1 = q.x()*v[1],
    x2 = q.x()*v[2],
    y0 = q.y()*v[0],
    y2 = q.y()*v[2],
    z0 = q.z()*v[0],
    z1 = q.z()*v[1];
    
    //wqv
    //w*(y2-z1)
    //w*(z0-x2)
    //w*(x1-y0)
    
    //qqv
    //y*(x1-y0) - z*(z0-x2)
    //z*(y2-z1) - x*(x1-y0)
    //x*(z0-x2) - y*(y2-z1)
    
    v4 rhs(w*(y2-z1) + y*(x1-y0) - z*(z0-x2),
           w*(z0-x2) + z*(y2-z1) - x*(x1-y0),
           w*(x1-y0) + x*(z0-x2) - y*(y2-z1),
           0.);
    
    return v + 2. * rhs;
}


static inline m4 quaternion_rotate_left_jacobian(const quaternion &q, const v4 &v) {
    /*
     m4 dwqv_dq = {{
     v4((q.y() * v[2] - q.z() * v[1]), 0., q.w() * v[2], -q.w() * v[1]),
     v4((q.z() * v[0] - q.x() * v[2]), -q.w() * v[2], 0., q.w() * v[0]),
     v4((q.x() * v[1] - q.y() * v[0]), q.w() * v[1], -q.w() * v[0], 0.),
     v4(0., 0., 0., 0.)
     }};
     m4 dqqv_dq = {{
     v4(0., q.y() * v[1] + q.z() * v[2], q.x() * v[1] - 2. * q.y() * v[0], q.x() * v[2] - 2. * q.z() * v[0]),
     v4(0., q.y() * v[0] - 2. * q.x() * v[1], q.z() * v[2] + q.x() * v[0], q.y() * v[2] - 2. * q.z() * v[1]),
     v4(0., q.z() * v[0] - 2. * q.x() * v[2], q.z() * v[1] - 2. * q.y() * v[2], q.x() * v[0] + q.y() * v[1]),
     v4(0., 0., 0., 0.)
     }};
     return 2. * dwqv_dq + 2. * dqqv_dq;
     */
    //dwqv_dq
    //y2 - z1, 0., w2, -w1
    //z0 - x2, -w2, 0., w0
    //x1 - y0, w1, -w0, 0.
    
    //dqqv_dq
    //0., y1 + z2, x1 - 2*y0, x2 - 2*z0
    //0., y0 - 2*x1, z2 + x0, y2 - 2*z1
    //0., z0 - 2*x2, z1 - 2*y2, x0 + y1

    f_t
    w0 = q.w()*v[0],
    w1 = q.w()*v[1],
    w2 = q.w()*v[2],
    x0 = q.x()*v[0],
    x1 = q.x()*v[1],
    x2 = q.x()*v[2],
    y0 = q.y()*v[0],
    y1 = q.y()*v[1],
    y2 = q.y()*v[2],
    z0 = q.z()*v[0],
    z1 = q.z()*v[1],
    z2 = q.z()*v[2];
    
    return 2. * (m4) {{
        {y2-z1, y1+z2, w2+x1-2*y0, -w1+x2-2*z0},
        {z0-x2, -w2+y0-2*x1, z2+x0, w0+y2-2*z1},
        {x1-y0, w1+z0-2*x2, -w0+z1-2*y2, x0+y1},
        {0., 0., 0., 0.}
    }};
}

//This is the same as the right jacobian of quaternion_rotate
static inline m4 to_rotation_matrix(const quaternion &q)
{
    /*
    m4 dwqv_dv = {{
        v4(0., -q.w() * q.z(), q.w() * q.y(), 0.),
        v4(q.w() * q.z(), 0., -q.w() * q.x(), 0.),
        v4(-q.w() * q.y(), q.w() * q.x(), 0., 0.),
        v4(0., 0., 0., 0.)
    }};
    m4 dqqv_dv = {{
        v4(-q.y() * q.y() - q.z() * q.z(), q.y() * q.x(), q.z() * q.x(), 0.),
        v4(q.x() * q.y(), -q.z() * q.z() - q.x() * q.x(), q.z() * q.y(), 0.),
        v4(q.x() * q.z(), q.y() * q.z(), -q.x() * q.x() - q.y() * q.y(), 0.),
        v4(0., 0., 0., 0.)
    }};
    m4 m3_identity = m4::Identity();
    m3_identity[3][3] = 0.;
    return m3_identity + 2. * dwqv_dv + 2. * dqqv_dv;
     */
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
        {1. - 2. * (yy+zz), 2. * (xy-wz), 2. * (xz+wy), 0.},
        {2. * (xy+wz), 1. - 2. * (xx+zz), 2. * (yz-wx), 0.},
        {2. * (xz-wy), 2. * (yz+wx), 1. - 2. * (xx+yy), 0.},
        {0., 0., 0., 1.}
    }};
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
        f_t s = 2. * sqrt(tr[0] + 1);
        res.w() = .25 * s;
        res.x() = (m(2, 1) - m(1, 2)) / s;
        res.y() = (m(0, 2) - m(2, 0)) / s;
        res.z() = (m(1, 0) - m(0, 1)) / s;
    } else if(tr[1] >= tr[2] && tr[1] >= tr[3]) {
        f_t s = 2. * sqrt(tr[1] + 1);
        res.w() = (m(2, 1) - m(1, 2)) / s;
        res.x() = .25 * s;
        res.y() = (m(1, 0) + m(0, 1)) / s;
        res.z() = (m(2, 0) + m(0, 2)) / s;
    } else if(tr[2] >= tr[3]) {
        f_t s = 2. * sqrt(tr[2] + 1.);
        res.w() = (m(0, 2) - m(2, 0)) / s;
        res.x() = (m(1, 0) + m(0, 1)) / s;
        res.y() = .25 * s;
        res.z() = (m(1, 2) + m(2, 1)) / s;
    } else {
        f_t s = 2. * sqrt(tr[3] + 1.);
        res.w() = (m(1, 0) - m(0, 1)) / s;
        res.x() = (m(0, 2) + m(2, 0)) / s;
        res.y() = (m(1, 2) + m(2, 1)) / s;
        res.z() = .25 * s;
    }
    return normalize(res);
}

static inline quaternion to_quaternion(const rotation_vector &v) {
    rotation_vector w(.5 * v.x(), .5 * v.y(), .5 * v.z());
    f_t th2, th = sqrt(th2=w.norm2()), C = cos(th), S = sinc(th,th2);
    return quaternion(C, S * w.x(), S * w.y(), S * w.z()); // e^(v/2)
}

static inline rotation_vector to_rotation_vector(const quaternion &q) {
    f_t denom = sqrt(q.x()*q.x() + q.y()*q.y() + q.z()*q.z());
    if(denom == 0.) return rotation_vector(q.x(), q.y(), q.z());
    f_t scale = 2. * acos(q.w()) / denom;
    return rotation_vector(q.x() * scale, q.y() * scale, q.z() * scale); // 2 log(q)
}

static inline rotation_vector to_rotation_vector(const m4 &R)
{
    return to_rotation_vector(to_quaternion(R));
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
    dQ_dQ = m4::Identity() + .5 * (m4) {{
        {0., -w[0], -w[1], w[2]},
        {w[0], 0., w[2], -w[1]},
        {w[1], -w[2], 0., w[0]},
        {w[2], w[1], -w[0], 0.}
    }};
    dQ_dw = .5 * (m4) {{
        {-Q.x(), -Q.y(), -Q.z(), 0.},
        {Q.w(), -Q.z(), Q.y(), 0.},
        {Q.z(), Q.w(), -Q.x(), 0.},
        {-Q.y(), Q.x(), Q.w(), 0.}
    }};
}

//Assumes a and b are already normalized
static inline quaternion rotation_between_two_vectors_normalized(const v4 &a, const v4 &b)
{
    quaternion res;
    f_t d = a.dot(b);
    v4 axis;
    if( d >= 1.) //the two vectors are aligned)
    {
        return quaternion();
    }
    else if(d < (-1. + 1.e-6)) //the two vector are (nearly) opposite, pick an arbitrary orthogonal axis
    {
        if(fabs(a[0]) > fabs(a[2])) //make sure we have a non-zero element
        {
            res = quaternion(0., -a[1], a[0], 0.);
        } else {
            res = quaternion(0., 0., -a[2], a[1]);
        }
    } else { // normal case
        f_t s = sqrt((1. + d) * 2.);
        
        v4 axis = cross(a, b);
        res = quaternion(.5 * s, axis[0] / s, axis[1] / s, axis[2] / s);
    }
    return normalize(res);
}

static inline quaternion rotation_between_two_vectors(const v4 &a, const v4 &b)
{
    //make sure the 3rd element is zero and normalize
    v4 an(a[0], a[1], a[2], 0.);
    v4 bn(b[0], b[1], b[2], 0.);
    return rotation_between_two_vectors_normalized(an.normalized(), bn.normalized());
}

#endif
