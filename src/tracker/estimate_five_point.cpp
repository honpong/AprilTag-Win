#include "estimate.h"
#include "ransac.h"

using rc::map;

static m<10,20> compute_template(const m3& E1, const m3& E2, const m3& E3, const m3& E4);

float estimate_five_point(const aligned_vector<v2>& src, const aligned_vector<v2>& dst, std::vector<Essential>& Ev)
{
    if (src.size() != dst.size() || src.size() < 5) {
        Ev.push_back(Essential());
        return std::numeric_limits<f_t>::infinity();
    }

    const Eigen::Index num_points = src.size();

    m<3, Eigen::Dynamic> p_src = map(src).transpose().colwise().homogeneous();
    m<3, Eigen::Dynamic> p_dst = map(dst).transpose().colwise().homogeneous();

    // fill in A
    m<Eigen::Dynamic, 9> P; P.resize(num_points,9);
    for(int i=0; i<num_points; ++i)
        Eigen::Map<m3>(P.row(i).data()) = p_dst.col(i) * p_src.col(i).transpose();

    // compute 4D null space
    Eigen::JacobiSVD<decltype(P)> svdP(P, Eigen::ComputeFullV); // U * S * V.transpose()
    m<9,1> e1 = svdP.matrixV().col(8);
    m<9,1> e2 = svdP.matrixV().col(7);
    m<9,1> e3 = svdP.matrixV().col(6);
    m<9,1> e4 = svdP.matrixV().col(5);
    m3 E1 = Eigen::Map<m3>(e1.data());
    m3 E2 = Eigen::Map<m3>(e2.data());
    m3 E3 = Eigen::Map<m3>(e3.data());
    m3 E4 = Eigen::Map<m3>(e4.data());

    // compute template
    m<10,20> T = compute_template(E1, E2, E3, E4);

    // create action matrix
    m<10,10> InvCreCb = -T.block<10,10>(0,0).colPivHouseholderQr().solve(T.block<10,10>(0,10));

    m<10,10> A = m<10,10>::Zero();
    A.topRows<6>() = InvCreCb.topRows<6>();
    A(6,0) = 1.f;
    A(7,1) = 1.f;
    A(8,2) = 1.f;
    A(9,6) = 1.f;

    // calculate solutions
    Eigen::EigenSolver<decltype(A)> eigA(A);
    auto lambda = eigA.eigenvalues();
    auto V = eigA.eigenvectors();
    for(int i=0; i<10; ++i) {
        if(std::abs(lambda[i].imag()) < 1e-9) {// real solution
            Essential E;
            v3 sol = V.block<3,1>(6,i).real() / V(9,i).real();
            E.E = sol[0]*E1 + sol[1]*E2 + sol[2]*E3 + E4;
            E.error = 0.f;
            for (int j=0; j<num_points; j++) {
                v3 line = E.E * p_src.col(j);
                line = line/line.head(2).norm();
                E.error += std::abs(line.dot(p_dst.col(j)));
            }
            E.error /= num_points;
            Ev.push_back(E);
        }
    }
    if(Ev.size()) {
        std::sort(Ev.begin(), Ev.end(), [](const Essential& E1, const Essential& E2) {
            return E1.error < E2.error; });
    } else {
        Ev.push_back(Essential());
    }

    return Ev[0].error;
}


struct estimated_five_point {
    aligned_vector<size_t> indices;
    const struct state {
        const aligned_vector<v2> &src;
        const aligned_vector<v2> &dst;
        const f_t threshold;
    } *state;
    estimated_five_point(const struct state &state_, aligned_vector<size_t>::iterator b, aligned_vector<size_t>::iterator e) : indices(b,e), state(&state_) {}
    bool computed = false;
    void compute_model() {
        if (computed) return; else computed = true;
        aligned_vector<v2> src; src.reserve(indices.size()); for (auto &i : indices) src.push_back(state->src[i]);
        aligned_vector<v2> dst; dst.reserve(indices.size()); for (auto &i : indices) dst.push_back(state->dst[i]);
        error = estimate_five_point(src, dst, Ev);
    }
    m3 essential() { // returns essential matrix with smallest reprojection error
        compute_model();
        return Ev[0].E;
    }
    f_t reprojection_error() { // returns smallest reprojection error
        compute_model();
        return Ev[0].error;
    }
    bool operator()(const struct state &state, aligned_vector<size_t>::iterator i) {
        compute_model();
        v3 line = Ev[0].E * state.src[*i].homogeneous();
        line = line/line.head(2).norm();
        f_t e = std::abs(line.dot(state.dst[*i].homogeneous()));
        return e < state.threshold;
    }
    operator bool() {
            return reprojection_error() < state->threshold;
    }
    bool operator<(estimated_five_point &o) {
        return o.reprojection_error() < state->threshold
        &&  (  indices.size() != o.indices.size()
             ? indices.size() < o.indices.size()
             : reprojection_error() > o.reprojection_error());
    }
protected:
    std::vector<Essential> Ev;
    f_t error = std::numeric_limits<f_t>::infinity();
};

f_t estimate_five_point(const aligned_vector<v2> &src, const aligned_vector<v2> &dst, m3 &E, std::minstd_rand &gen,
                         int max_iterations, f_t max_reprojection_error, f_t confidence, unsigned min_matches, std::set<size_t> *inliers)
{
    if (src.size() != dst.size() || src.size() < 6) // we require 6 points to disambiguate (ransac)
        return false;
    aligned_vector<size_t> indices(src.size()); for (size_t i=0; i<src.size(); i++) indices[i] = i;
    struct estimated_five_point::state state = { src, dst, max_reprojection_error };
    estimated_five_point best = ransac<6,estimated_five_point>(state, indices.begin(), indices.end(), gen, max_iterations, confidence, min_matches);
    E = best.essential();
    if (inliers) { inliers->clear(); for (auto i : best.indices) inliers->insert(i); }
    return best.reprojection_error();
}

// {e1, e2, e3, e4} = Array[#, {3, 3}, 0]& /@ {E1, E2, E3, E4};
// e = e1 x + e2 y + e3 z + e4;
// EE = {Det[e], 2 e.Transpose[e].e - Tr[e.Transpose[e]] e} // Flatten;
// TT = Values@CoefficientRules[#, {x, y, z}, "DegreeLexicographic"] & /@ EE;
// ex = MapThread[HoldForm[Set[#1, #2]] &, {Array[T, {10, 20}, 0], TT}, 2] // Flatten;
// c = StringJoin @@ ("    " <> ToString[#, CForm] <> ";\n" & /@ (ex /. {x_^3 -> HoldForm[x x x], x_^2 -> HoldForm[x x]}));
// With[{cpp = OpenWrite["compute-template.cpp"]}, WriteString[cpp, c]; Close[cpp]]

static m<10, 20> compute_template(const m3 &E1, const m3 &E2, const m3 &E3, const m3 &E4) {
    m<10,20> T;
#include "compute-template.cpp"
    ;
    return T;
}
