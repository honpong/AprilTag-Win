// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __HOMOGENEOUS_H
#define __HOMOGENEOUS_H

#include "vec4.h"

class point {
 public:
 point(): data((v_intrinsic){ 0., 0., 0., 1.}) {}
 point(const f_t x, const f_t y, const f_t z): data((v_intrinsic) { x, y, z, 1. }) {}    
 point(const v_intrinsic &other): data(other) {}
    //member access
    f_t & operator[](const int i) { assert(i < 3 && i >= 0); return ((f_t *)&data)[i]; }
    const f_t & operator[](const int i) const { assert(i < 3 && i >= 0); return ((f_t *)&data)[i]; }
    void print() const {
        fprintf(stderr, "[ %e, %e, %e ]", (*this)[0], (*this)[1], (*this)[2]);
    }

    v_intrinsic data;
};

class vect {
 public:
 vect(): data((v_intrinsic){ 0., 0., 0., 0.}) {}
 vect(const f_t x, const f_t y, const f_t z): data((v_intrinsic) { x, y, z, 0. }) {}    
 vect(const v_intrinsic &other): data(other) {}
    //member access
    f_t & operator[](const int i) { assert(i < 3 && i >= 0); return ((f_t *)&data)[i]; }
    const f_t & operator[](const int i) const { assert(i < 3 && i >= 0); return ((f_t *)&data)[i]; }
    void print() const {
        fprintf(stderr, "[ %e, %e, %e ]", (*this)[0], (*this)[1], (*this)[2]);
    }
    v_intrinsic data;
};

static inline vect operator-(const vect &first) { return vect(-first.data); }
static inline vect operator/(const vect &first, const f_t second) { return vect(first.data / v4(second).data); }


static inline point operator+(const point &first, const vect &second) {
    return point(first.data + second.data);
}

static inline vect operator-(const point &first, const point &second) {
    return vect(first.data - second.data);
}

static inline vect operator+(const vect &first, const vect &second) {
    return vect(first.data + second.data);
}

static inline f_t norm_2(const vect &v) {
    v4 a(v.data * v.data);
    return a[0] + a[1] + a[2];
}

static inline f_t norm(const vect &v) {
    return sqrt(norm_2(v));
}

#include <stdio.h>

class homogeneous:public m4 {
 public:
    homogeneous() {}
 homogeneous(const m4& initial): m4(initial) {}

    void set_rotation(const m4 &R) {
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 3; ++j) {
                data[i][j] = R[i][j];
            }
        }
    }

    void set_translation(const vect &T) {
        data[0][3] = T[0];
        data[1][3] = T[1];
        data[2][3] = T[2];
    }

    homogeneous get_rotation() const {
        homogeneous R = *this;
        for(int i = 0; i < 3; ++i) {
            R[i][3] = 0.;
            R[3][i] = 0.;
        }
        return R;
    }

    vect get_translation() const {
        return vect(data[0][3], data[1][3], data[2][3]);
    }
};

class homogeneous_transformation: public homogeneous {
 public:
    homogeneous_transformation(): homogeneous(m4::Identity()) { }
 homogeneous_transformation(const m4& initial): homogeneous(initial) {}
    homogeneous_transformation(const m4 &R, const vect &T) {
        set_rotation(R);
        set_translation(T);
        data[3] = v4(0., 0., 0., 1.);
    }
};

static inline point operator*(const homogeneous_transformation &G, const point &X) {
    return point(sum(G[0] * X.data), sum(G[1] * X.data), sum(G[2] * X.data));
}

static inline homogeneous_transformation inverse(const homogeneous_transformation &G) {
    homogeneous_transformation r;
    r[0] = v4(G[0][0], G[1][0], G[2][0], - (G[0][0]*G[0][3] + G[1][0]*G[1][3] + G[2][0]*G[2][3]));
    r[1] = v4(G[0][1], G[1][1], G[2][1], - (G[0][1]*G[0][3] + G[1][1]*G[1][3] + G[2][1]*G[2][3]));
    r[2] = v4(G[0][2], G[1][2], G[2][2], - (G[0][2]*G[0][3] + G[1][2]*G[1][3] + G[2][2]*G[2][3]));
    return r;
}
template <typename T> static inline void get_gl_matrix(const homogeneous_transformation &G, T gl[4][4]) {
    //gl matrices are column major.
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            gl[j][i] = G[i][j];
        }
    }
}

class homogeneous_variance: public homogeneous {
 public:
 homogeneous_variance(const m4& initial): homogeneous(initial) {}
    homogeneous_variance() {
        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 4; ++j) {
                data[i][j] = 0.;
            }
        }
    }
    homogeneous_variance(const m4 &R, const vect &T) {
        set_rotation(R);
        set_translation(T);
        data[3] = v4(0., 0., 0., 0.);
    }
};

class transformation_variance {
 public:
    homogeneous_transformation transform;
    homogeneous_variance variance;
 transformation_variance(): transform(), variance() {}
 transformation_variance(const m4 &R, const m4 &R_var, const vect &T, const vect &T_var): transform(R, T), variance(R_var, T_var) {}
 transformation_variance(const homogeneous_transformation &G, const homogeneous_variance &G_var): transform(G), variance(G_var) {}
};

static inline transformation_variance operator*(const transformation_variance &first, const transformation_variance &second) {
    //variance of product is a^2*var(b) + var(a)*b^2 + var(a)*var(b)
    m4 ft, st;
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            ft[i][j] = first.transform[i][j] * first.transform[i][j];
            st[i][j] = second.transform[i][j] * second.transform[i][j];
        }
    }
    return transformation_variance(homogeneous_transformation(first.transform * second.transform),
                                   homogeneous_variance(ft * second.variance +
                                                        first.variance * st +
                                                        first.variance * second.variance));
}

static inline transformation_variance inverse(const transformation_variance &tv) {
    transformation_variance R(transpose(tv.transform.get_rotation()), transpose(tv.variance.get_rotation()));
    homogeneous_transformation T_trn;
    T_trn.set_translation(-tv.transform.get_translation());
    homogeneous_variance T_var;
    T_var.set_translation(tv.variance.get_translation());
    transformation_variance T(T_trn, T_var);
    return R * T;
    /*    homogeneous_transformation G = inverse(tv.transform);
    m4 R_var = transpose(tv.variance.get_rotation());
    m4 R = G.get_rotation();
    v4 T = tv.transform.get_translation();
    v4 T_var = R_var * T + R * T_var + R_var * T_var;
    return transformation_variance(G, homogeneous_variance(R_var, T_var));*/
    //return transformation_variance(inverse(tv.transform), tv.variance);
}

/*class homogeneous {
 public:
    homogeneous() {
        R = m4::Identity();
        T = v4(0.);
    };

    homogeneous(const m4 &_R, const v4 &_T) {
        R = _R;
        T = _T;
    }

    void set_translation(const vect &_T) {
        T[0] = _T[0]; T[1] = _T[1]; T[2] = _T[2];
    }

    point get_position() {
        return point(T[0], T[1], T[2]);
    }
    m4 R;
    v4 T;
};

static inline homogeneous operator*(const homogeneous &first, const homogeneous &second) {
    return homogeneous(first.R * second.R, first.T + first.R * second.T);
}

static inline point operator*(const homogeneous &G, const point &X) {
    v4 pt(X[0], X[1], X[2], 0.);
    v4 res = G.R * pt + G.T;
    return point(res[0], res[1], res[2]);
}

static inline homogeneous inverse(const homogeneous &G) {
    m4 Rt = transpose(G.R);
    return homogeneous(Rt, -(Rt*G.T));
}

template <typename T> static inline void get_gl_matrix(const homogeneous &G, T gl[4][4]) {
    //gl matrices are column major.
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            gl[j][i] = G.R[i][j];
        }
        gl[i][3] = 0.;
        gl[3][i] = G.T[i];
    }
    gl[3][3] = 1.;
    }
*/
#endif
