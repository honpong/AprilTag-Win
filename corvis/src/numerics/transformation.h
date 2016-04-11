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
#include "matrix.h"
#include "rotation_vector.h"
#include "quaternion.h"

class transformation {
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
        transformation(): Q(quaternion()), T(v4(0,0,0,0)) {};
        transformation(const quaternion & Q_, const v4 & T_) : Q(Q_), T(T_) {};
        transformation(const rotation_vector & v, const v4 & T_) : T(T_) { Q = to_quaternion(v); };
        transformation(const m4 & m, const v4 & T_) : T(T_) { Q = to_quaternion(m); };

        quaternion Q;
        v4 T;
};

static inline std::ostream& operator<<(std::ostream &stream, const transformation &t)
{
    return stream << t.Q << ", " << t.T; 
}

static inline bool operator==(const transformation & a, const transformation &b)
{
    return a.T == b.T && a.Q == b.Q;
}

static inline transformation invert(const transformation & t)
{
    return transformation(conjugate(t.Q), conjugate(t.Q) * -t.T);
}

static inline v4 transformation_apply(const transformation & t, const v4 & apply_to)
{
    return t.Q * apply_to + t.T;
}

static inline v4 operator*(const transformation & t, const v4 & apply_to)
{
    return transformation_apply(t, apply_to);
}

static inline transformation compose(const transformation & t1, const transformation & t2)
{
    return transformation(t1.Q * t2.Q, t1.T + t1.Q * t2.T);
}

static inline transformation operator*(const transformation &t1, const transformation & t2)
{
    return compose(t1, t2);
}

static bool estimate_transformation(const std::vector<v4> & src, const std::vector<v4> & dst, transformation & transform)
{
    v4 center_src = v4::Zero();
    v4 center_dst = v4::Zero();
    if(src.size() != dst.size()) return false;
    int N = src.size();

    // calculate centroid
    for(auto v : src)
        center_src += v;
    center_src = center_src / N;

    for(auto v : dst)
        center_dst += v;
    center_dst = center_dst / N;

    // remove centroid
    matrix X(N, 3);
    matrix Y(N, 3);
    for(int i = 0; i < N; i++)
        for(int j = 0; j < 3; j++)
            X(i, j) = src[i][j] - center_src[j];
    for(int i = 0; i < N; i++)
        for(int j = 0; j < 3; j++)
            Y(i, j) = dst[i][j] - center_dst[j];

    // compute rotation
    matrix H(3, 3);
    matrix_product(H, Y, X, true /*transpose Y*/);
    // TODO: can incorporate weights here optionally, maybe use feature depth variance

    matrix U(3,3), S(1,3), Vt(3,3);
    if(!matrix_svd(H, U, S, Vt))
        return false;

    matrix R(3,3);
    matrix_product(R, U, Vt);
    // If det(R) == -1, we have a flip instead of a rotation
    if(matrix_3x3_determinant(R) < 0) {
        // Vt(2,:) = -Vt(2,:)
        Vt(2,0) = -Vt(2,0);
        Vt(2,1) = -Vt(2,1);
        Vt(2,2) = -Vt(2,2);
        matrix_product(R, U, Vt);
    }

    m4 R_out = m4::Zero();
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            R_out(i,j) = R(i,j);

    // compute translation
	v4 T = center_dst - R_out*center_src;
    transform = transformation(R_out, T);

    return true;
}

#endif
