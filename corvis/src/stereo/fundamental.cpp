#include "fundamental.h"

#include <Eigen/SVD>

bool debug_eight_point_ransac = false;

m4 eight_point_F(v4 p1[], v4 p2[], int npts)
{
    // normalize points
    v4 center1 = v4(0., 0., 0., 0.);
    v4 center2 = v4(0., 0., 0., 0.);
    float norm1 = 0, norm2 = 0;
    for(int i = 0; i < npts; i++) {
        center1 += p1[i];
        center2 += p2[i];
    }
    center1 = center1 / npts;
    center2 = center2 / npts;
    for(int i = 0; i < npts; i++) {
        norm1 += (p1[i] - center1).dot(p1[i] - center1);
        norm2 += (p2[i] - center1).dot(p2[i] - center2);
    }
    norm1 = sqrt(2.*npts / norm1);
    norm2 = sqrt(2.*npts / norm2);

    m3 T1, T2;
    T1(0, 0) = norm1; T1(0, 1) = 0;     T1(0, 2) = -norm1*center1[0];
    T1(1, 0) = 0;     T1(1, 1) = norm1; T1(1, 2) = -norm1*center1[1];
    T1(2, 0) = 0;     T1(2, 1) = 0;     T1(2, 2) = 1;


    T2(0, 0) = norm2; T2(0, 1) = 0;     T2(0, 2) = -norm2*center2[0];
    T2(1, 0) = 0;     T2(1, 1) = norm2; T2(1, 2) = -norm2*center2[1];
    T2(2, 0) = 0;     T2(2, 1) = 0;     T2(2, 2) = 1;

    Eigen::Matrix<f_t, Eigen::Dynamic, 9> constraints(npts, 9);
    for(int i = 0; i < npts; i++) {
        v4 p1n = (p1[i] - center1) * norm1;
        v4 p2n = (p2[i] - center2) * norm2;
        constraints(i, 0) = p1n[0]*p2n[0];
        constraints(i, 1) = p1n[1]*p2n[0];
        constraints(i, 2) = p2n[0];
        constraints(i, 3) = p1n[0]*p2n[1];
        constraints(i, 4) = p1n[1]*p2n[1];
        constraints(i, 5) = p2n[1];
        constraints(i, 6) = p1n[0];
        constraints(i, 7) = p1n[1];
        constraints(i, 8) = 1;
    }


    // some columns of U are inverted compared to matlab
    Eigen::Matrix<f_t, 9, 9> Vt;

    Eigen::JacobiSVD<decltype(constraints)> svd(constraints, Eigen::ComputeFullV);
    Vt = svd.matrixV().transpose();
    
    m3 F;
    F(0, 0) = Vt(8, 0); F(0, 1) = Vt(8, 1); F(0, 2) = Vt(8, 2);
    F(1, 0) = Vt(8, 3); F(1, 1) = Vt(8, 4); F(1, 2) = Vt(8, 5);
    F(2, 0) = Vt(8, 6); F(2, 1) = Vt(8, 7); F(2, 2) = Vt(8, 8);

    Eigen::JacobiSVD<m3> svdF(F, Eigen::ComputeFullU | Eigen::ComputeFullV);
    m3 SE = svdF.singularValues().asDiagonal();
    SE(2,2) = 0;
    
    // TODO: there is probably a cleaner way to do this:
    // F = transpose(T2) * U * S * Vt * T1;
    F.noalias() = T2.transpose() * svdF.matrixU() * SE * svdF.matrixV().transpose() * T1;

    // F = F / norm(F) / sign(F(3,3))
    float Fnorm = F.norm();
    if(F(2,2) < 0)
      Fnorm = -Fnorm;
    F = F / Fnorm;

    m4 estimatedF = m4::Zero();
    for(int i = 0; i < 3; i++)
      for(int j = 0; j < 3; j++)
        estimatedF(i, j) = F(i,j);
    estimatedF(3, 3) = 1;

    return estimatedF;
}

extern bool line_line_intersect(v4 p1, v4 p2, v4 p3, v4 p4, v4 & pa, v4 & pb);

// note, R,T is from p1 to p2, not from p2 to p1 (as in world coordinates)
bool validate_RT(const m4 & R, const v4 & T, const v4 & p1, const v4 & p2)
{
    // TODO: decide if I should use this or intersection
    // positive depth constraint
    /*
    m4 skew_Rp1 = skew3(R*p1);
    m4 skew_p2 = skew3(p2);
    float alpha1 = -sum((skew_p2*T)*(skew_p2*R*p1))/pow(norm(skew_p2*T), 2);
    float alpha2 =  sum((skew_Rp1*p2)*(skew_Rp1*T))/pow(norm(skew_Rp1*p2), 2);
     */

    // actual intersection
    v4 o1 = T;
    v4 o2 = v4(0,0,0,0);
    v4 p12 = R*p1 + T;
    v4 pa, pb;
    bool success = line_line_intersect(o1, p12, o2, p2, pa, pb);
    if(!success) return false;

    // switch to camera relative coordinates
    pa = R.transpose()*(pa - T);

    //fprintf(stderr, "intersection says %d, alpha says %f %f\n", (pa[2] > 0 & pb[2] > 0), alpha1, alpha2);

    if(pa[2] < 0 || pb[2] < 0) return false;

    return true;
}

bool decompose_F(const m4 & F, float focal_length, float center_x, float center_y, const v4 & p1, const v4 & p2, m4 & R, v4 & T)
{
    m4 Kinv = m4::Zero();
    Kinv(0, 0) = 1./focal_length;
    Kinv(1, 1) = 1./focal_length;
    Kinv(0, 2) = -center_x/focal_length;
    Kinv(1, 2) = -center_y/focal_length;
    Kinv(2, 2) = 1;
    Kinv(3, 3) = 1;

    m4 K = m4::Zero();
    K(0, 0) = focal_length;
    K(1, 1) = focal_length;
    K(0, 2) = center_x;
    K(1, 2) = center_y;
    K(2, 2) = 1;
    K(3, 3) = 1;

    m4 E4 = K.transpose()*F*K;
    v4 p1p = Kinv*p1;
    v4 p2p = Kinv*p2;

    m3 E;

    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            E(i,j) = E4(i, j);

    m3 W;
    W(0,0) = 0; W(0,1) = -1; W(0,2) = 0;
    W(1,0) = 1; W(1,1) = 0;  W(1,2) = 0;
    W(2,0) = 0; W(2,1) = 0;  W(2,2) = 1;

    m3 Rzp = W;
    m3 Rzn;
    Rzn(0,0) = 0;  Rzn(0,1) = 1; Rzn(0,2) = 0;
    Rzn(1,0) = -1; Rzn(1,1) = 0; Rzn(1,2) = 0;
    Rzn(2,0) = 0;  Rzn(2,1) = 0; Rzn(2,2) = 1;

    m3 Z;
    Z(0,0) = 0; Z(0,1) = -1; Z(0,2) = 0;
    Z(1,0) = 1; Z(1,1) = 0;  Z(1,2) = 0;
    Z(2,0) = 0; Z(2,1) = 0;  Z(2,2) = 0;

    Eigen::JacobiSVD<m3> svd(E, Eigen::ComputeFullV | Eigen::ComputeFullU);

    m3 U = svd.matrixU();
    m3 V = svd.matrixV();
    float det_U = U.determinant();
    float det_V = V.determinant();
    if(det_U < 0 && det_V < 0) {
        U = -U;
        V = -V;
    }
    else if(det_U < 0 && det_V > 0) {
        U = -U * Rzn;
        V = V * Rzp;
    }
    else if(det_U > 0 && det_V < 0) {
        U = U*Rzn;
        V = -V*Rzp;
    }

    // T = u_3
    m4 R1(m4::Identity()), R2(m4::Identity());
    v4 T1 { U(0, 2), U(1,2), U(2,2), 0. };
    v4 T2 = -T1;

    auto temp = U * W * V.transpose();
    for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++) {
            R1(row, col) = temp(row,col);
        }
    auto temp2 = U * W.transpose() * V.transpose();
    for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++) {
            R2(row, col) = temp2(row,col);
        }

    int nvalid = 0;
    if(validate_RT(R1, T1, p1p, p2p)) {
        R = R1;
        T = T1;
        nvalid++;
    }

    if(validate_RT(R2, T1, p1p, p2p)) {
        R = R2;
        T = T1;
        nvalid++;
    }

    if(validate_RT(R1, T2, p1p, p2p)) {
        R = R1;
        T = T2;
        nvalid++;
    }

    if(validate_RT(R2, T2, p1p, p2p)) {
        R = R2;
        T = T2;
        nvalid++;
    }

    if(nvalid != 1) {
        fprintf(stderr, "Error: There should always be only one solution, but found %d", nvalid);
        return false;
    }

    return true;
}


int compute_inliers(const v4 from [], const v4 to [], int nmatches, const m4 & F, float thresh, bool inliers [])
{
    int ninliers = 0;
    for(int i = 0; i < nmatches; i++) {
        // Sampson distance x2*F*x1 / (sum((Fx1).^2) + sum((F'x2).^2))
        v4 el1 = F*from[i];
        v4 el2 = F.transpose()*to[i];
        float distance = to[i].dot(F*from[i]);
        distance = distance*distance;
        distance = distance / (el1[0]*el1[0] + el1[1]*el1[1] + el2[0]* el2[0] + el2[1]*el2[1]);

// distance in pixels
//        v4 line = F*from[i];
//        line = line / sqrt(line[0]*line[0] + line[1]*line[1]);
//        float distance = fabs(sum(to[i]*line));
        if(distance < thresh) {
            inliers[i] = true;
            ninliers++;
        }
        else
            inliers[i] = false;
    }
    return ninliers;
}

// Chooses k random elements from a list with n elements and puts them in inds
// Uses reservoir sampling
void choose_random(int k, int n, int * inds)
{
    int i;

    // Initialize reservoir
    for(i = 0; i < k; i++)
        inds[i] = i;

    for(; i < n; i++) {
        // Pick a random index from 0 to i.
        int j = rand() % (i+1);

        // If the randomly picked index is smaller than k, then replace
        // the element present at the index with new element
        if (j < k)
            inds[j] = i;
    }
}

// F is from reference_pts to target_pts
bool ransac_F(const vector<v4> & reference_pts, const vector<v4> target_pts, m4 & F)
{
    vector<v4> estimate_pts1;
    vector<v4> estimate_pts2;
    const int npoints = (int)reference_pts.size();
    bool inliers[npoints];
    float sampson_thresh = 0.5;

    int iterations = 2000;
    m4 bestF = F;
    int bestinliers = compute_inliers(&reference_pts[0], &target_pts[0], npoints, bestF, sampson_thresh, inliers);

    for(int i = 0; i < iterations; i++)
    {
        int points[8];
        // select 8 random points
        choose_random(8, npoints, points);

        // estimate F
        estimate_pts1.clear();
        estimate_pts2.clear();
        for(int m = 0; m < 8; m++) {
            estimate_pts1.push_back(reference_pts[points[m]]);
            estimate_pts2.push_back(target_pts[points[m]]);
        }
        m4 currentF = eight_point_F(&estimate_pts1[0], &estimate_pts2[0], (int)estimate_pts1.size());

        // measure fitness
        int currentinliers = compute_inliers(&reference_pts[0], &target_pts[0], npoints, currentF, sampson_thresh, inliers);
        if(currentinliers > bestinliers) {
            bestinliers = currentinliers;
            bestF = currentF;
        }
    }

    // measure fitness
    compute_inliers(&reference_pts[0], &target_pts[0], npoints, bestF, sampson_thresh, inliers);

    // estimate F
    estimate_pts1.clear();
    estimate_pts2.clear();
    for(int m = 0; m < npoints; m++) {
        if(inliers[m]) {
            estimate_pts1.push_back(reference_pts[m]);
            estimate_pts2.push_back(target_pts[m]);
        }
    }
    bestF = eight_point_F(&estimate_pts1[0], &estimate_pts2[0], (int)estimate_pts1.size());

    // measure fitness
    int final_inliers = compute_inliers(&reference_pts[0], &target_pts[0], npoints, bestF, sampson_thresh, inliers);
    if(debug_eight_point_ransac) {
        fprintf(stderr, "after refining %d matches with %lu inliers, got final inliers %d\n", npoints, estimate_pts1.size(), final_inliers);
    }

    F = bestF;

    return true;
}

//TODO: estimate_F doesnt agree with eight point F. This is now correct for F corresponding to X2 = R * X1 + T

m4 estimate_F(const camera &g, const stereo_frame &reference, const stereo_frame &target, m4 & dR, v4 & dT)
{
    /*
    x1_w = R1 * x1 + T1
    x2 = R2^t * (R1 * x1 + T1 - T2)
    
    R21 = R2^t * R1
    T21 = R2^t * (T1 - T2)
    */
    m4 R1w = to_rotation_matrix(reference.W);
    m4 R2w = to_rotation_matrix(target.W);
    dR = R2w.transpose()*R1w;
    dT = R2w.transpose() * (reference.T - target.T);

    // E21 is 3x3
    m4 E21 = skew3(dT) * dR;

    m4 Kinv;
    Kinv(0, 0) = 1./g.focal_length;
    Kinv(1, 1) = 1./g.focal_length;
    Kinv(0, 2) = -g.center_x/g.focal_length;
    Kinv(1, 2) = -g.center_y/g.focal_length;
    Kinv(2, 2) = 1;
    Kinv(3, 3) = 1;

    m4 F21 = Kinv.transpose()*E21*Kinv;

    // for numerical conditioning
    // F = F / norm(F) / sign(F(3,3))
    float Fnorm = F21.norm();

    if(F21(2, 2) < 0)
        Fnorm = -Fnorm;

    return F21 / Fnorm;
}


