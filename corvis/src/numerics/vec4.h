// Written by Eagle Jones
// Copyright (c) 2012-2015, RealityCap
// All rights reserved.
//

#ifndef __vec4_H
#define __vec4_H

extern "C" {
#include "../cor/cor_types.h"
#include <stdio.h>
#include <inttypes.h>
}

#include <ostream>
#include <vector>

//Don't use GPL-licensed pieces of eigen
#define EIGEN_MPL2_ONLY

//This disables internal asserts which slow eigen down quite a bit
#ifndef DEBUG
#define EIGEN_NO_DEBUG
#endif

//These could cause assertion failures, so only turn them on for debug builds
#ifdef DEBUG
//#define EIGEN_NO_AUTOMATIC_RESIZING
#endif

//This, or EIGEN_INITIALIZE_MATRICES_BY_NAN can be used to verify that we are initializing everything correctly
//#define EIGEN_INITIALIZE_MATRICES_BY_ZERO

#define EIGEN_DEFAULT_IO_FORMAT Eigen::IOFormat(Eigen::StreamPrecision, 0, " ", ";", "", "", "[", "]")
#define EIGEN_DEFAULT_TO_ROW_MAJOR
#include <initializer_list> // needed by "eigen_initializer_list.h"
#define EIGEN_MATRIX_PLUGIN "eigen_initializer_list.h"
#include <Eigen/Dense>

template <int Rows = Eigen::Dynamic>                  using v = Eigen::Matrix<f_t, Rows, 1>;
template <int Rows = Eigen::Dynamic, int Cols = Rows> using m = Eigen::Matrix<f_t, Rows, Cols>;
typedef Eigen::Matrix<f_t, 4, 1> v4;
typedef Eigen::Matrix<f_t, 3, 1> v3;
typedef Eigen::Matrix<f_t, 4, 4> m4;
typedef Eigen::Matrix<f_t, 3, 3> m3;
typedef Eigen::Matrix<f_t, 2, 1> v2, feature_t;
typedef Eigen::Quaternion<f_t> quaternion;

#include <vector>
template <typename T> using aligned_vector = std::vector<T, Eigen::aligned_allocator<T>>;
#include <list>
template <typename T> using aligned_list = std::list<T, Eigen::aligned_allocator<T>>;

template <class T, int R, int C> Eigen::Map<const Eigen::Matrix<T, R, C>, Eigen::Unaligned> m_map(const T (&a)[R][C]) { return &a[0][0]; }
template <class T, int R, int C> Eigen::Map<Eigen::Matrix<T, R, C>, Eigen::Unaligned> m_map(T (&a)[R][C]) { return &a[0][0]; }
template <class T, int N> Eigen::Map<const Eigen::Matrix<T, N, 1>, Eigen::Unaligned> v_map(const T (&a)[N]) { return a; }
template <class T, int N> Eigen::Map<Eigen::Matrix<T, N, 1>, Eigen::Unaligned> v_map(T (&a)[N]) { return a; }

static inline v3 v3_sqrt(const v3 &v) { return v3(sqrt(v[0]), sqrt(v[1]), sqrt(v[2])); }

#ifdef __ACCELERATE__
static inline v3 v3_from_vFloat(const vFloat &other)
{
    return v3(other[0], other[1], other[2]);
}

static inline vFloat vFloat_from_v3(const v3 &other)
{
    return (vFloat){(float)other[0], (float)other[1], (float)other[2], 0};
}
#endif

template <int N>
class stdev
{
public:
    v<N> sum, max, mean, M2, variance, stdev_;
    uint32_t count;
    stdev(): sum(v<N>::Zero()), mean(v<N>::Zero()), M2(v<N>::Zero()), variance(v<N>::Zero()), stdev_(v<N>::Zero()), max(0.), count(0) {}
    bool valid() { return count >= 2; }
    void data(const v<N> &x) {
        ++count;
        v<N> delta = x - mean;
        mean = mean + delta / (f_t)count;
        M2 = M2 + delta.cwiseProduct(x - mean);
        if(x.norm() > max.norm()) max = x;
        variance = M2 / (f_t)(count - 1);
        stdev_ = variance.array().sqrt();
    }
};

template<int N>
static inline std::ostream& operator<<(std::ostream &stream, const stdev<N> &v)
{
    return stream << "mean is: " << v.mean << ", stdev is: " << v.stdev_ << ", max is: " << v.max << std::endl;
}

class histogram
{
public:
    std::vector<unsigned int> v;
    histogram(unsigned int max): v(max, 0) {}
    void data(unsigned int x) { if(x >= v.size()) ++v[v.size()-1]; else ++v[x]; }
};

static inline std::ostream& operator<<(std::ostream &stream, const histogram &h)
{
    for(unsigned int i = 0; i < h.v.size(); ++i)
    {
        stream << i << " " << h.v[i] << "\n";
    }
    return stream;
}

class v3_lowpass {
public:
    v3 filtered;
    f_t constant;
    v3_lowpass(f_t rate, f_t cutoff) { constant  = 1 / (1 + rate/cutoff); }
    v3 sample(const v3 &data) { return filtered = filtered * (1 - constant) + data * constant; }
};

inline static m3 skew(const v3 &v)
{
    m3 V = m3::Zero();
    V(1, 2) = -(V(2, 1) = v[0]);
    V(2, 0) = -(V(0, 2) = v[1]);
    V(0, 1) = -(V(1, 0) = v[2]);
    return V;
}

inline static v3 invskew(const m3 &V)
{
    return v3((V(2, 1) - V(1, 2))/2,
              (V(0, 2) - V(2, 0))/2,
              (V(1, 0) - V(0, 1))/2);
}

#endif
