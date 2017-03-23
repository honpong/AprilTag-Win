#include "transformation.h"

bool estimate_transformation(const aligned_vector<v3> & src, const aligned_vector<v3> & dst, transformation & transform)
{
    if(src.size() != dst.size() || src.size() < 3)
        return false;

    // calculate centroid
    v3 center_src = map(src).colwise().mean();
    v3 center_dst = map(dst).colwise().mean();

    // remove centroid
    m<Eigen::Dynamic, 3> centered_src = map(src) - center_src.transpose().replicate(src.size(),1);
    m<Eigen::Dynamic, 3> centered_dst = map(dst) - center_dst.transpose().replicate(dst.size(),1);

    // TODO: can incorporate weights here optionally, maybe use feature depth variance
    v3 S; m3 R = project_rotation(centered_dst.transpose() * centered_src, &S);
    transform = transformation(R, center_dst - R*center_src);
    return S(0) > F_T_EPS*10 && S(1) / S(0) > F_T_EPS*10; // not all the same point and not colinear
}

