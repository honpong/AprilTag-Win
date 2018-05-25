#include "transformation_cov.h"

transformation_cov operator*(const transformation_cov& G1, const transformation_cov& G2) {
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

transformation_cov invert(const transformation_cov& G12) {
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

transformation_cov compose(const transformation_cov &G1, const transformation_cov &G2, const m<6,6> &cov12) {
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
