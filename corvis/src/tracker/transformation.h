//
//  transformation.h
//  RC3DK
//
//  Created by Brian Fulkerson
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __TRANSFORMATION_H
#define __TRANSFORMATION_H

#include "vec4.h"
#include "rotation_vector.h"
#include "quaternion.h"

class transformation {
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
        transformation(): Q(quaternion::Identity()), T(v3(0,0,0)) {};
        transformation(const quaternion & Q_, const v3 & T_) : Q(Q_), T(T_) {};
        transformation(const rotation_vector & v, const v3 & T_) : T(T_) { Q = to_quaternion(v); };
        transformation(const m3 & m, const v3 & T_) : T(T_) { Q = to_quaternion(m); };
        transformation(f_t s, const transformation &G0, const transformation &G1) : Q(G0.Q.slerp(s, G1.Q)), T(G0.T + s * (G1.T-G0.T)) {}

        bool operator ==(const transformation &t2) const {
            return Q.isApprox(t2.Q) && T == t2.T;
        };

        quaternion Q;
        v3 T;
};

static inline std::ostream& operator<<(std::ostream &stream, const transformation &t)
{
    return stream << t.Q << ", " << t.T; 
}

static inline transformation invert(const transformation & t)
{
    return transformation(t.Q.conjugate(), t.Q.conjugate() * -t.T);
}

static inline v3 transformation_apply(const transformation & t, const v3 & apply_to)
{
    return t.Q * apply_to + t.T;
}

static inline v3 operator*(const transformation & t, const v3 & apply_to)
{
    return transformation_apply(t, apply_to);
}

static inline transformation compose(const transformation & t1, const transformation & t2)
{
    return transformation((t1.Q * t2.Q).normalized(), t1.T + t1.Q * t2.T);
}

static inline transformation operator*(const transformation &t1, const transformation & t2)
{
    return compose(t1, t2);
}

static bool estimate_transformation(const aligned_vector<v3> & src, const aligned_vector<v3> & dst, transformation & transform)
{
    v3 center_src = v3::Zero();
    v3 center_dst = v3::Zero();
    if(src.size() != dst.size()) return false;
    int N = (int)src.size();
    if(N < 3) return false;

    // calculate centroid
    for(auto v : src)
        center_src += v;
    center_src = center_src / (f_t)N;

    for(auto v : dst)
        center_dst += v;
    center_dst = center_dst / (f_t)N;

    // remove centroid
    Eigen::Matrix<f_t, Eigen::Dynamic, 3> X(N, 3);
    Eigen::Matrix<f_t, Eigen::Dynamic, 3> Y(N, 3);
    for(int i = 0; i < N; i++)
        for(int j = 0; j < 3; j++)
            X(i, j) = src[i][j] - center_src[j];
    for(int i = 0; i < N; i++)
        for(int j = 0; j < 3; j++)
            Y(i, j) = dst[i][j] - center_dst[j];

    // TODO: can incorporate weights here optionally, maybe use feature depth variance

    Eigen::JacobiSVD<m3> svd(Y.transpose() * X, Eigen::ComputeFullU | Eigen::ComputeFullV); // SVD the rotation
    m3 U = svd.matrixU(), Vt = svd.matrixV().transpose();
    v3 S = svd.singularValues();

    // all the same point, or colinear
    if(S(0) < F_T_EPS*10 || S(1) / S(0) < F_T_EPS*10)
        return false;

    m3 R = U * Vt;
    // If det(R) == -1, we have a flip instead of a rotation
    if(R.determinant() < 0) {
        // Vt(2,:) = -Vt(2,:)
        Vt(2,0) = -Vt(2,0);
        Vt(2,1) = -Vt(2,1);
        Vt(2,2) = -Vt(2,2);
        R = U * Vt;
    }

    transform = transformation(R, center_dst - R*center_src);

    return true;
}

#endif
