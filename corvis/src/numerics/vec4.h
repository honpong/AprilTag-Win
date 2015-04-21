// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __vec4_H
#define __vec4_H

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#endif
extern "C" {
#ifndef __APPLE__
#include <xmmintrin.h>
#endif
#include "../cor/cor_types.h"
#include "stdio.h"
}

#include <ostream>

//Don't use GPL-licensed pieces of eigen
#define EIGEN_MPL2_ONLY

//This disables internal asserts which slow eigen down quite a bit
#ifndef DEBUG
#define EIGEN_NO_DEBUG
#endif

//These could cause assertion failures, so only turn them on for debug builds
#ifdef DEBUG
#define EIGEN_NO_AUTOMATIC_RESIZING
#endif

//This, or EIGEN_INITIALIZE_MATRICES_BY_NAN can be used to verify that we are initializing everything correctly
//#define EIGEN_INITIALIZE_MATRICES_BY_ZERO

#define EIGEN_DEFAULT_TO_ROW_MAJOR
#define EIGEN_MATRIX_PLUGIN "../../../numerics/eigen_initializer_list.h"
#include "../Eigen/Dense"

typedef Eigen::Matrix<f_t, 4, 1> v4;
typedef Eigen::Matrix<f_t, 4, 4> m4;

static inline v4 v4_sqrt(const v4 &v) { return v4(sqrt(v[0]), sqrt(v[1]), sqrt(v[2]), sqrt(v[3])); }

static inline v4 v4_from_vFloat(const vFloat &other)
{
    return v4(other[0], other[1], other[2], other[3]);
}

static inline vFloat vFloat_from_v4(const v4 &other)
{
    return (vFloat){(float)other[0], (float)other[1], (float)other[2], (float)other[3]};
}

static inline v4 cross(const v4 &a, const v4 &b) {
    return v4(a[1] * b[2] - a[2] * b[1],
              a[2] * b[0] - a[0] * b[2],
              a[0] * b[1] - a[1] * b[0],
              0);
}

class stdev_vector
{
public:
    v4 sum, mean, M2;
    f_t max;
    v4 variance, stdev;
    uint64_t count;
    stdev_vector(): sum(v4::Zero()), mean(v4::Zero()), M2(v4::Zero()), max(0.), variance(v4::Zero()), stdev(v4::Zero()), count(0) {}
    void data(const v4 &x) {
        ++count;
        v4 delta = x - mean;
        mean = mean + delta / count;
        M2 = M2 + delta.cwiseProduct(x - mean);
        if(x.norm() > max) max = x.norm();
        variance = M2 / (count - 1);
        stdev = v4(sqrt(variance[0]), sqrt(variance[1]), sqrt(variance[2]), sqrt(variance[3]));
    }
};

static inline std::ostream& operator<<(std::ostream &stream, const stdev_vector &v)
{
    return stream << "mean is: " << v.mean << ", stdev is: " << v.stdev << ", max is: " << v.max << std::endl;
}

class stdev_scalar
{
public:
    double sum, mean, M2;
    f_t max;
    f_t variance, stdev;
    uint64_t count;
    stdev_scalar(): sum(0.), mean(0.), M2(0.), max(0.), variance(0.), stdev(0.), count(0) {}
    void data(const f_t &x) {
        ++count;
        double delta = x - mean;
        mean = mean + delta / count;
        M2 = M2 + delta * (x - mean);
        if(x > max) max = x;
        variance = M2 / (count - 1);
        stdev = sqrt(variance);
    }
};

static inline std::ostream& operator<<(std::ostream &stream, const stdev_scalar &s)
{
    return stream << "mean is: " << s.mean << ", stdev is: " << s.stdev << ", max is: " << s.max << std::endl;
}

class v4_lowpass {
public:
    v4 filtered;
    f_t constant;
    v4_lowpass(f_t rate, f_t cutoff) { constant  = 1. / (1. + rate/cutoff); }
    v4 sample(const v4 &data) { return filtered = filtered * (1. - constant) + data * constant; }
};

static inline v4 relative_rotation(const v4 &first, const v4 &second)
{
    v4 rv = cross(first.normalized(), second.normalized());
    f_t sina = rv.norm();
    return rv / sina * asin(sina);
}

inline static m4 skew3(const v4 &v)
{
    m4 V = m4::Zero();
    V(1, 2) = -(V(2, 1) = v[0]);
    V(2, 0) = -(V(0, 2) = v[1]);
    V(0, 1) = -(V(1, 0) = v[2]);
    return V;
}

inline static v4 invskew3(const m4 &V)
{
    return v4(.5 * (V(2, 1) - V(1, 2)),
              .5 * (V(0, 2) - V(2, 0)),
              .5 * (V(1, 0) - V(0, 1)),
              0);
}

inline static f_t determinant_minor(const m4 &m, const int a, const int b)
{
    return (m(1, a) * m(2, b) - m(1, b) * m(2, a));
}

inline static f_t determinant3(const m4 &m)
{
    return m(0, 0) * determinant_minor(m, 1, 2)
    - m(0, 1) * determinant_minor(m, 0, 2)
    + m(0, 2) * determinant_minor(m, 0, 1);
}

v4 integrate_angular_velocity(const v4 &W, const v4 &w);
void linearize_angular_integration(const v4 &W, const v4 &w, m4 &dW_dW, m4 &dW_dw);

#endif
