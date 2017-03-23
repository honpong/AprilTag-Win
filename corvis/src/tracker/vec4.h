// Written by Eagle Jones
// Copyright (c) 2012-2015, RealityCap
// All rights reserved.
//

#ifndef __vec4_H
#define __vec4_H

#include "cor_types.h"


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
#undef I // We don't use the C header <complex.h>, but Eigen does when you use LAPACKe, so remove this conficting definition

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

template <typename T, int R, int C> Eigen::Map<const Eigen::Matrix<T, R, C>, Eigen::Unaligned> map(const T (&a)[R][C]) { return decltype(map(a)) { &a[0][0] }; }
template <typename T, int R, int C> Eigen::Map<      Eigen::Matrix<T, R, C>, Eigen::Unaligned> map(      T (&a)[R][C]) { return decltype(map(a)) { &a[0][0] }; }

template <typename T, int N>        Eigen::Map<const Eigen::Matrix<T, N, 1>, Eigen::Unaligned> map(const T (&a)[N])    { return decltype(map(a)) { &a[0] }; }
template <typename T, int N>        Eigen::Map<      Eigen::Matrix<T, N, 1>, Eigen::Unaligned> map(      T (&a)[N])    { return decltype(map(a)) { &a[0] }; }

template <typename T, int N>        Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, N>, Eigen::Aligned, Eigen::OuterStride<N>>
    map(const aligned_vector<Eigen::Matrix<T, N, 1>> &a) { return decltype(map(a)) { &a[0][0], static_cast<Eigen::Index>(a.size()), N }; }
template <typename T, int N>        Eigen::Map<      Eigen::Matrix<T, Eigen::Dynamic, N>, Eigen::Aligned, Eigen::OuterStride<N>>
    map(      aligned_vector<Eigen::Matrix<T, N, 1>> &a) { return decltype(map(a)) { &a[0][0], static_cast<Eigen::Index>(a.size()), N }; }

inline static m3 skew(const v3 &v)
{
    m3 V = m3::Zero();
    V(1, 2) = -(V(2, 1) = v[0]);
    V(2, 0) = -(V(0, 2) = v[1]);
    V(0, 1) = -(V(1, 0) = v[2]);
    return V;
}

inline static m3 project_rotation(m3 M, v3 *S=nullptr)
{
    Eigen::JacobiSVD<m3> svd(M, Eigen::ComputeFullU | Eigen::ComputeFullV);
    m3 U = svd.matrixU(), Vt = svd.matrixV().transpose();
    if (S) *S = svd.singularValues();
    return U * v3{ 1, 1, (U*Vt).determinant() }.asDiagonal() * Vt;
}

#endif
