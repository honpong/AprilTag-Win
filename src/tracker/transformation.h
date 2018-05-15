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

bool estimate_transformation(const aligned_vector<v3> & src, const aligned_vector<v3> & dst, transformation & transform);
f_t  estimate_transformation(const aligned_vector<v3> &P, const aligned_vector<v2> &p, transformation &transform);
#include <random> // FIXME: remove these includes and use template parameters
#include <set>
f_t estimate_transformation(const aligned_vector<v3> &src, const aligned_vector<v2> &dst, transformation &transform, std::minstd_rand &gen,
                            int max_iterations = 20, f_t max_reprojection_error = .00001f, f_t confidence = .90f, unsigned min_matches = 6,
                            std::set<size_t> *inliers = nullptr);
f_t estimate_3d_point(const aligned_vector<v2> &src, const std::vector<transformation> &camera_poses, f_t &depth_m);
f_t estimate_fundamental(const aligned_vector<v2> &src, const aligned_vector<v2> &dst, m3 &fundamental, std::minstd_rand &gen,
                         int max_iterations = 40, f_t max_reprojection_error = 1, f_t confidence = .90f, unsigned min_matches = 8,
                         std::set<size_t> *inliers = nullptr);

class transformation_cov {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    transformation_cov() : cov(m<6,6>::Identity()) {};
    transformation_cov(const transformation& G_, const m<6,6>& cov_) : G(G_.Q, G_.T), cov(cov_) {};

    transformation G;
    m<6,6> cov;
};

static inline transformation_cov operator*(const transformation_cov& G1, const transformation_cov& G2)
{
    // G3 = G1*G2, cov3 = J1*cov1*J1^T + J2*cov2*J2^T
    // J1 = [I -(R1*t2)^; 0 I], J2 = [R1 0; 0 R1]
    m3 S = -skew(G1.G.Q * G2.G.T);
    m<6,6> P = G1.cov;

    // J1*cov1*J1^T
    P.block<3,3>(0,3) += S*P.block<3,3>(3,3);
    P.block<3,3>(0,0) += S*P.block<3,3>(3,0) + P.block<3,3>(0,3)*S.transpose();
    // P.block<3,3>(3,0) should be = P.block<3,3>(0,3).transpose() but it's going to be overwritten below
    // P.block<3,3>(3,3) does not change

    // ... + J2*cov2*J2^T
    const m<6,6>& Q = G2.cov;
    m3 R1 = G1.G.Q.toRotationMatrix();
    P.block<3,3>(0,0) += R1 * Q.topLeftCorner<3,3>() * R1.transpose();
    P.block<3,3>(0,3) += R1 * Q.topRightCorner<3,3>() * R1.transpose();
    P.block<3,3>(3,0) = P.block<3,3>(0,3).transpose();
    P.block<3,3>(3,3) += R1 * Q.bottomRightCorner<3,3>() * R1.transpose();

    return transformation_cov(G1.G*G2.G, P);
}

static inline transformation_cov invert(const transformation_cov& G12)
{
    // G21 = invert(G12), cov21 = Jinv*cov12*Jinv^T
    // Jinv = [-R21 t21^*R21; 0 -R21]
    transformation_cov G21(invert(G12.G), G12.cov);
    auto& P = G21.cov;

    const m3& R21 = G21.G.Q.toRotationMatrix();
    m3 S = -skew(G21.G.T) * R21;

    P.block<3,3>(0,0) = R21 * P.block<3,3>(0,0) * R21.transpose();
    P.block<3,3>(0,3) = R21 * P.block<3,3>(0,3);
    P.block<3,3>(0,0) = P.block<3,3>(0,0) + P.block<3,3>(0,3)*S.transpose() + S*P.block<3,3>(0,3).transpose() + S*P.block<3,3>(3,3)*S.transpose();

    P.block<3,3>(3,3) = R21 * P.block<3,3>(3,3);
    P.block<3,3>(0,3) = P.block<3,3>(0,3)*R21.transpose() + S*P.block<3,3>(3,3).transpose();
    P.block<3,3>(3,0) = P.block<3,3>(0,3).transpose();
    P.block<3,3>(3,3) = P.block<3,3>(3,3)*R21.transpose();

    return G21;
}

static inline transformation_cov compose(const transformation_cov &G1, const transformation_cov &G2, const m<6,6> &cov12) {
    // G3 = G1*G2, cov = [J1 J2] [cov1 cov12; cov12^T cov2] [J1^T; J2^T] = J1 cov1 J1^T + J2 cov2 J2^T + J1 cov12 J2^T + J2 cov21 J1^T
    // we compute the first sumands J1 cov1 J1^T + J2 cov2 J2^T
    transformation_cov G3 = G1*G2;

    // we add the reamining components of the covariance that depend on the correlation: ... + J1 cov12 J2^T + J2 cov21 J1^T
    m3 R1T = G1.G.Q.toRotationMatrix().transpose();
    m3 S = -skew(G1.G.Q * G2.G.T);
    m<6,6> J1_cov12_J2T;
    J1_cov12_J2T.block<3,3>(0,0) = (cov12.block<3,3>(0,0) + S * cov12.block<3,3>(3,0)) * R1T;
    J1_cov12_J2T.block<3,3>(0,3) = (cov12.block<3,3>(0,3) + S * cov12.block<3,3>(3,3)) * R1T;
    J1_cov12_J2T.block<3,3>(3,0) = cov12.block<3,3>(3,0) * R1T;
    J1_cov12_J2T.block<3,3>(3,3) = cov12.block<3,3>(3,3) * R1T;

    G3.cov += J1_cov12_J2T + J1_cov12_J2T.transpose();

    return G3;
}
