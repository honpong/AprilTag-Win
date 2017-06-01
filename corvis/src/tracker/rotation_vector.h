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
    rotation_vector(): data(v3::Zero()) {}
    rotation_vector(const f_t other0, const f_t other1, const f_t other2): data(other0, other1, other2) {}
    
    v3 raw_vector() const { return data; }
    
    f_t x() const { return data[0]; }
    f_t y() const { return data[1]; }
    f_t z() const { return data[2]; }
    f_t &x() { return data[0]; }
    f_t &y() { return data[1]; }
    f_t &z() { return data[2]; }

    inline rotation_vector operator-() const { return rotation_vector(-data); }
    inline f_t norm2() const { return data[0]*data[0] + data[1]*data[1] + data[2]*data[2]; }
    inline rotation_vector operator*(f_t s) const { return rotation_vector(data * s); }
    inline rotation_vector operator/(f_t s) const { return rotation_vector(data / s); }
private:
    rotation_vector(const v3 &v) : data(v) {}
    v3 data;
};

static inline rotation_vector operator*(f_t s, rotation_vector &v) { return v * s; }

static inline std::ostream& operator<<(std::ostream &stream, const rotation_vector &v)
{
    return stream  << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
}

//TODO: regularize so that opposite axis/angle and/or 2*pi shells evaluate to equal
static inline bool operator==(const rotation_vector &a, const rotation_vector &b)
{
    return a.x() == b.x() && a.y() == b.y() && a.z() == b.z();
}

//TODO: move these here so that they can be inlined
m3 to_rotation_matrix(const rotation_vector &v); // e^\hat{v}
m3 to_spatial_jacobian(const rotation_vector &v); // \unhat{(d e^\hat{ v})  e^\hat{-v}} == to_spatial_jacobian(v) dv
m3 to_body_jacobian(const rotation_vector &v); // \unhat{   e^\hat{-v} d e^\hat{ v}} ==    to_body_jacobian(v) dv

#endif
