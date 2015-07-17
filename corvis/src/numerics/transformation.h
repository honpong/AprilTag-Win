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

#endif
