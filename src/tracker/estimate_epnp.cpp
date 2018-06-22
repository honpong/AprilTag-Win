#include "estimate.h"
#include "ransac.h"

using rc::map;

template<Eigen::Index R, Eigen::Index C, typename DV>
static m<4,3> estimate_transformation_camera_control_points(const v<6> &r, const Eigen::EigenBase<DV> &V_)
{
    constexpr Eigen::Index B = R*(R+1)/2+R*(C-R); static_assert(R <= C, "The computation of B assumes R <= C (as does the computation of bb)");

    // Access to the first C (<=4) components of null space by control point
    auto V = [&V_](Eigen::Index a) { return V_.derived().rowwise().reverse().template block<3,C>(3*a,0); };

    // r_nm = | c_w_n - c_w_m |^2 = | c_c_n - c_c_m |^2 = | b_i (V_n_i - V_m_i) |^2 = L(nm,ij) b_i b_j = L_nm_ij b_ij
    m<6,B> L;
    for (int nm=0, n=0; n<4; n++) for (int m=n+1; m<4; m++, nm++) // for each non-equal ordered control point pair
        for (int ij=0, i=0; i<R; i++) for (int j=i; j<C; j++, ij++) // for each possibly-equal ordered b/nullspace vector pair within (R,C)
            L(nm,ij) = (i==j?1:2) * ( V(n).col(i) - V(m).col(i) ).dot( V(n).col(j) - V(m).col(j) );

    v<B> bb = L.householderQr().solve(r);

    // solve for b in bb[ij] = b[i] b[j]
    v<C> b;
    if (bb[0] < 0) bb = -bb;
    for (int ij=0, i=0; i<R; i++) for (int j=i; j<C; j++, ij++)
        if (i == j) // use the magnitude from the diagonal for stabilty, but the sign from the cross terms if already computed
            b[j] = bb[ij] > 0 ? std::copysign(sqrt(bb[ij]), i>0 ? b[j] : 0) : 0/*inconsistency*/;
        else
            b[j] = b[i] ? bb[ij] / b[i] : 0/*don't propagate inconsistency*/;

    // Refine b with a few iterations of Gauss-Newton
    for (int gn=0; gn<2; gn++) {
        m<B,C> dbb_db = m<B,C>::Zero();
        for (int ij=0, i=0; i<R; i++) for (int j=i; j<C; j++, ij++) {
            bb[ij] = b[i] * b[j];
            dbb_db(ij,j) = (i==j?2:1) * b[i];
        }
        b -= (L * dbb_db).householderQr().solve(L * bb - r);
    }

    // Use b to compute the control points in the camera frame
    m<4,3> c_c; for (int n=0; n<4; n++) c_c.row(n) = V(n) * b;
    return c_c;
}

f_t estimate_transformation(const aligned_vector<v3> &src, const aligned_vector<v2> &dst, transformation &transform)
{
    f_t reprojection_error = std::numeric_limits<f_t>::infinity();
    if (src.size() != dst.size() || src.size() < 4)
        return reprojection_error;

    const Eigen::Index P = src.size();

    m<3,4> c_w; // world control points

    // choose first world control point as the centroid
    auto center_src = c_w.col(0) = map(src).colwise().mean();

    // remove the centroid
    m<Eigen::Dynamic, 3> centered_src = map(src).rowwise() - center_src.transpose();
    Eigen::JacobiSVD<decltype(centered_src)> svd(centered_src, Eigen::ComputeFullV); // U * S * V.transpose()

    // choose the other three "simplex" control points as aligned with principal conponents of centered_src about the center;
    // Note: Compensate for each sv being scaled by sqrt of the number of items contributing to each col of V
    m3 principal_compoments = svd.matrixV() * (svd.singularValues()/std::sqrt(P)).asDiagonal();
    m3 principal_inverse    = (std::sqrt(P)*svd.singularValues().cwiseInverse()).asDiagonal() * svd.matrixV().transpose();
    c_w.rightCols(3) = principal_compoments.colwise() + center_src;

    // find barycentric coordinates a s.t. src = a * c_w w/ a.rowwise().sum() == 1
    m<Eigen::Dynamic, 4> a; a.resize(P,4);
    a.rightCols(3) = centered_src * principal_inverse.transpose();
    a.leftCols(1)  = m<>::Ones(P,1) - a.rightCols(3).rowwise().sum();

    assert(map(src).isApprox(a * c_w.transpose()));
    assert(a.rowwise().sum().isApprox(m<>::Ones(P,1)));

    m<Eigen::Dynamic, 12> M; M.resize(2*P,12);
    for (int i=0; i<P; i++)
        for (int j=0; j<4; j++)
            M.block<2,3>(2*i,3*j) << a(i,j), 0, a(i,j) * -dst[i].x(),
                                     0, a(i,j), a(i,j) * -dst[i].y();

    Eigen::JacobiSVD<decltype(M)> msvd(M, Eigen::ComputeFullV); // U * S * V.transpose()

    v<6> r;
    for (int nm=0, n=0; n<4; n++) for (int m=n+1; m<4; m++, nm++) // for each non-equal control point pair
        r[nm] = (c_w.col(n) - c_w.col(m)).squaredNorm();

    aligned_vector<v3> p_c(P);
    for (int N=0; N<3; N++)
    {
        m<4,3> c_c;
        switch(N) {
            case 0: c_c = estimate_transformation_camera_control_points<2,3>(r, msvd.matrixV()); break; // [ B11 B12 B13; 0  B22 B23 ]
            case 1: c_c = estimate_transformation_camera_control_points<2,2>(r, msvd.matrixV()); break; // [ B11 B12; 0  B22 ]
            case 2: c_c = estimate_transformation_camera_control_points<1,4>(r, msvd.matrixV()); break; // [ B11 B12 B13 B14 ]
        }

        // Use the control points in the camera frame to compute all the points in the camera frame
        map(p_c) = a * c_c;

        // Assume that if the first z is negative, then they all are
        if (map(p_c)(0,2) < 0) map(p_c) = -map(p_c); // c_c = -c_c

        transformation G; estimate_transformation(src, p_c, G);

        f_t error_sum = 0;
        for (int i=0; i<P; i++) {
            v3 xyz = G.Q.toRotationMatrix() * map(src).transpose().col(i) + G.T;
            error_sum += (xyz.head<2>()/xyz.z() - map(dst).transpose().col(i)).norm();
        }
        if (reprojection_error > error_sum/P) {
            reprojection_error = error_sum/P;
            transform = G;
        }
    }
    return reprojection_error;
}

struct estimated_transformation {
    aligned_vector<size_t> indices;
    const struct state {
        const aligned_vector<v3> &src;
        const aligned_vector<v2> &dst;
        const f_t threshold;
    } *state;
    estimated_transformation(const struct state &state_, aligned_vector<size_t>::iterator b, aligned_vector<size_t>::iterator e) : indices(b,e), state(&state_) {}
    bool computed = false;
    void compute_model() {
        if (computed) return; else computed = true;
        aligned_vector<v3> src; src.reserve(indices.size()); for (auto &i : indices) src.push_back(state->src[i]);
        aligned_vector<v2> dst; dst.reserve(indices.size()); for (auto &i : indices) dst.push_back(state->dst[i]);
        error = estimate_transformation(src, dst, G);
        R_cached = G.Q.toRotationMatrix();
    }
    transformation transform() {
        compute_model();
        return G;
    }
    f_t reprojection_error() {
        compute_model();
        return error;
    }
    bool operator()(const struct state &state, aligned_vector<size_t>::iterator i) {
        compute_model();
        v3 xyz = R_cached * state.src[*i] + G.T;
        f_t e = (xyz.head<2>()/xyz.z() - state.dst[*i]).norm();
        return e < state.threshold;
    }
    operator bool() {
        return reprojection_error() < state->threshold;
    }
    bool operator<(estimated_transformation &o) {
        return o.reprojection_error() < state->threshold
        &&  (  indices.size() != o.indices.size()
             ? indices.size() < o.indices.size()
             : reprojection_error() > o.reprojection_error());
    }
protected:
    transformation G; m3 R_cached;
    f_t error = std::numeric_limits<f_t>::infinity();
};

f_t estimate_transformation(const aligned_vector<v3> &src, const aligned_vector<v2> &dst, transformation &transform, std::minstd_rand &gen,
                            int max_iterations, f_t max_reprojection_error, f_t confidence, unsigned min_matches, std::set<size_t> *inliers)
{
    if (src.size() != dst.size() || src.size() < 6)
        return false;
    aligned_vector<size_t> indices(src.size()); for (size_t i=0; i<src.size(); i++) indices[i] = i;
    struct estimated_transformation::state state = { src, dst, max_reprojection_error };
    estimated_transformation best = ransac<6,estimated_transformation>(state, indices.begin(), indices.end(), gen, max_iterations, confidence, min_matches);
    transform = best.transform();
    if (inliers) { inliers->clear(); for (auto i : best.indices) inliers->insert(i); }
    return best.reprojection_error();
}