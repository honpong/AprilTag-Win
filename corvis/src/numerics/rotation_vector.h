//
//  rotation_vector.h
//  RC3DK
//
//  Created by Eagle Jones on 1/19/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef RC3DK_rotation_vector_h
#define RC3DK_rotation_vector_h

#include "vec4.h"

class rotation_vector {
public:
    rotation_vector(): data(0.) {}
    rotation_vector(const f_t other0, const f_t other1, const f_t other2): data(other0, other1, other2, 0.) {}
    
    v4 raw_vector() { return data; }
    
    const f_t x() const { return data[0]; }
    const f_t y() const { return data[1]; }
    const f_t z() const { return data[2]; }
    f_t &x() { return data[0]; }
    f_t &y() { return data[1]; }
    f_t &z() { return data[2]; }

    inline const rotation_vector operator-() const { return rotation_vector(-data); }
    inline const f_t norm2() const { return data[0]*data[0] + data[1]*data[1] + data[2]*data[2]; }
private:
    rotation_vector(const v4 &v) : data(v) {}
    v4 data;
};

static inline std::ostream& operator<<(std::ostream &stream, const rotation_vector &v)
{
    return stream  << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
}

//TODO: regularize so that opposite axis/angle and/or 2*pi shells evaluate to equal
static inline bool operator==(const rotation_vector &a, const rotation_vector &b)
{
    return a.x() == b.x() && a.y() == b.y() && a.z() == b.z();
}

static inline m4 to_rotation_matrix(const rotation_vector &v)
{
    return rodrigues(v4(v.x(), v.y(), v.z(), 0.), NULL);
}

static inline m4v4 to_rotation_matrix_jacobian(const rotation_vector &v)
{
    m4v4 res;
    rodrigues(v4(v.x(), v.y(), v.z(), 0.), &res);
    return res;
}

static inline rotation_vector integrate_angular_velocity(const rotation_vector &V, const v4 &w)
{
    v4 res = integrate_angular_velocity(v4(V.x(), V.y(), V.z(), 0.), w);
    return rotation_vector(res[0], res[1], res[2]);
}

static inline void integrate_angular_velocity_jacobian(const rotation_vector &V, const v4 &w, m4 &dV_dV, m4 &dV_dw)
{
    linearize_angular_integration(v4(V.x(), V.y(), V.z(), 0.), w, dV_dV, dV_dw);
}

#endif
