#pragma once

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
