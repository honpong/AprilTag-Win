#include "estimate.h"
#include "ransac.h"

using rc::map;

static m3 normalization_transform(const aligned_vector<v2>& points) {
     v2 mean = map(points).colwise().mean();
     v2 var = (((map(points).rowwise() - mean.transpose()).cwiseProduct(map(points).rowwise() - mean.transpose())).colwise().sum()/(points.size()-1));
     v2 n = (2 / var.array()).sqrt();
     return m3 {
         { n[0],  0  , -n[0]*mean[0] },
         {  0  , n[1], -n[1]*mean[1] },
         {  0  ,  0  ,      1        },
     };
}

f_t estimate_fundamental(const aligned_vector<v2> &src, const aligned_vector<v2> &dst, m3& fundamental)
{
    f_t reprojection_error = std::numeric_limits<f_t>::infinity();
    if (src.size() != dst.size() || src.size() < 8)
        return reprojection_error;

    const Eigen::Index P = src.size();

    // normalize points
    m3 Tsrc = normalization_transform(src);
    m3 Tdst = normalization_transform(dst);

    m<3, Eigen::Dynamic> p_src = Tsrc * map(src).transpose().colwise().homogeneous();
    m<3, Eigen::Dynamic> p_dst = Tdst * map(dst).transpose().colwise().homogeneous();

    // fill in A
    m<Eigen::Dynamic, 9> A; A.resize(P,9);
    for(int i=0; i<P; ++i)
        Eigen::Map<m3>(A.row(i).data()) = p_dst.col(i) * p_src.col(i).transpose();

    // calculating F
    Eigen::JacobiSVD<decltype(A)> svdA(A, Eigen::ComputeFullV); // U * S * V.transpose()
    m<9,1> V_lastcol = svdA.matrixV().rightCols(1);
    m3 F = Eigen::Map<m3>(V_lastcol.data());
    Eigen::JacobiSVD<decltype(F)> svdF(F, Eigen::ComputeFullU | Eigen::ComputeFullV); // U * S * V.transpose()
    v3 s = svdF.singularValues();
    s[2] = 0;
    F = svdF.matrixU()*s.asDiagonal()*svdF.matrixV().transpose();

    // unnormalize F
    F = Tdst.transpose()*F*Tsrc;

    // reprojection error
    f_t error_sum = 0;
    for (int i=0; i<P; i++) {
        v3 line = F * map(src).transpose().col(i).homogeneous();
        line = line/line.head(2).norm();
        error_sum += std::abs(line.dot(map(dst).transpose().col(i).homogeneous()));
    }
    reprojection_error = error_sum/P;
    fundamental = F;

    return reprojection_error;
}

struct estimated_fundamental {
    aligned_vector<size_t> indices;
    const struct state {
        const aligned_vector<v2> &src;
        const aligned_vector<v2> &dst;
        const f_t threshold;
    } *state;
    estimated_fundamental(const struct state &state_, aligned_vector<size_t>::iterator b, aligned_vector<size_t>::iterator e) : indices(b,e), state(&state_) {}
    bool computed = false;
    void compute_model() {
        if (computed) return; else computed = true;
        aligned_vector<v2> src; src.reserve(indices.size()); for (auto &i : indices) src.push_back(state->src[i]);
        aligned_vector<v2> dst; dst.reserve(indices.size()); for (auto &i : indices) dst.push_back(state->dst[i]);
        error = estimate_fundamental(src, dst, F);
    }
    m3 fundamental() {
        compute_model();
        return F;
    }
    f_t reprojection_error() {
        compute_model();
        return error;
    }
    bool operator()(const struct state &state, aligned_vector<size_t>::iterator i) {
        compute_model();
        v3 line = F * state.src[*i].homogeneous();
        line = line/line.head(2).norm();
        f_t e = std::abs(line.dot(state.dst[*i].homogeneous()));
        return e < state.threshold;
    }
    operator bool() {
        return reprojection_error() < state->threshold;
    }
    bool operator<(estimated_fundamental &o) {
        return o.reprojection_error() < state->threshold
        &&  (  indices.size() != o.indices.size()
             ? indices.size() < o.indices.size()
             : reprojection_error() > o.reprojection_error());
    }
protected:
    m3 F;
    f_t error = std::numeric_limits<f_t>::infinity();
};

f_t estimate_fundamental(const aligned_vector<v2> &src, const aligned_vector<v2> &dst, m3 &fundamental, std::minstd_rand &gen,
                         int max_iterations, f_t max_reprojection_error, f_t confidence, unsigned min_matches, std::set<size_t> *inliers)
{
    if (src.size() != dst.size() || src.size() < 8)
        return false;
    aligned_vector<size_t> indices(src.size()); for (size_t i=0; i<src.size(); i++) indices[i] = i;
    struct estimated_fundamental::state state = { src, dst, max_reprojection_error };
    estimated_fundamental best = ransac<8,estimated_fundamental>(state, indices.begin(), indices.end(), gen, max_iterations, confidence, min_matches);
    fundamental = best.fundamental();
    if (inliers) { inliers->clear(); for (auto i : best.indices) inliers->insert(i); }
    return best.reprojection_error();
}
