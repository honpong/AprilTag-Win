// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __QUATERNION_H
#define __QUATERNION_H

#include "vec4.h"

class quaternion_point {
 public:
 point(): data((v_intrinsic){ 0., 0., 0., 0.}) {}
 point(const f_t x, const f_t y, const f_t z): data((v_intrinsic) { 0., x, y, z }) {}    
 point(const v_intrinsic &other): data(other) {}
    //member access
    f_t & operator[](const int i) { assert(i < 3 && i >= 0); return ((f_t *)&data)[i]; }
    const f_t & operator[](const int i) const { assert(i < 3 && i >= 0); return ((f_t *)&data)[i]; }
    void print() const {
        fprintf(stderr, "[ %e, %e, %e ]", (*this)[0], (*this)[1], (*this)[2]);
    }

    v_intrinsic data;
};

class quaternion_vector {
};

class quaternion_rotation {
    quaternion_point rotate(const quaternion_point &p) {
    
    }
    void normalize() {
        q = q/norm(q);
    }
    quaternion &conjugate() const {
        return quaternion(q[0], -q[1], -q[2], -q[3]);
    }
 protected:
    v4 q;
};

quaternion operator*(const quaternion &x, const quaternion &y) {
    return quaternion(sum(x.q * v4(y[0], -y[1], -y[2], -y[3])),
                      sum(x.q * v4(y[1],  y[0],  y[3], -y[2])),
                      sum(x.q * v4(y[2], -y[3],  y[0],  y[1])),
                      sum(x.q * v4(y[3],  y[2], -y[1],  y[0])));
}

quaternion conjugate(const quaternion &x) {
    return quaternion(

class transformation {
    quaternion_rotation qr;
    quaternion_vector qt;
};
    
#endif
