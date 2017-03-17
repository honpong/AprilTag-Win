#include "transformation.h"

bool estimate_transformation(const aligned_vector<v3> & src, const aligned_vector<v3> & dst, transformation & transform)
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

