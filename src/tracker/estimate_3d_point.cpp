#include "estimate.h"

using rc::map;

f_t estimate_3d_point(const aligned_vector<v2> &src, const std::vector<transformation> &camera_poses, f_t &depth_m)
{
    m<Eigen::Dynamic, 4> A; A.resize(2*camera_poses.size(),4);
    m<3,4> P;
    for (size_t i = 0; i< src.size(); ++i) {
        v2 p = src[i];
        const transformation& camera = camera_poses[i];
        P.block<3,3>(0,0) = camera.Q.toRotationMatrix();
        P.block<3,1>(0,3) = camera.T;
        A.block<2,4>(2*i,0) = p * P.row(2) - P.topRows(2);
    }
    Eigen::JacobiSVD<decltype(A)> msvd(A, Eigen::ComputeFullV);
    v3 pCref = msvd.matrixV().topRightCorner(3,1) / msvd.matrixV()(3,3);
    depth_m = pCref[2];

    // calculate reprojection error at each node
    aligned_vector<f_t> errors_point;
    for (size_t i = 0; i< camera_poses.size(); ++i) {
        const transformation& G_C_Cref = camera_poses[i];
        const feature_t& kpn = src[i];
        // predict 3d point in the camera reference
        v3 p3dC = G_C_Cref * pCref;
        v2 kpn_p = p3dC.segment<2>(0)/p3dC.z();
        errors_point.push_back((kpn - kpn_p).norm());
    }
    return map(errors_point).mean();
}

