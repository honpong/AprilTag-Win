//
//  homography.cpp
//  RC3DK
//
//  Created by Eagle Jones on 11/13/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include "homography.h"
#include "../numerics/matrix.h"

/*
 [0       0       0       -x1      -y1     -1       x1y2    y1y2    y2 ]
 [x1      y1      1        0        0       0      -x1x2   -y1x2   -x2 ]
 */
static void homography_set_correspondence(Eigen::Matrix<f_t, 8, 9> &X, int n, float x1, float y1, float x2, float y2)
{
    /* Matlab:
     % set up constraints for homography matrix
     function [rows] = makeRows(x1, x2);
     
     rows = [ 0 0 0    -x1'    x2(2)*x1';
                x1'  0 0 0    -x2(1)*x1'];
     */

    int row = n * 2;
    X(row, 0) = 0;
    X(row, 1) = 0;
    X(row, 2) = 0;
    X(row, 3) = -x1;
    X(row, 4) = -y1;
    X(row, 5) = -1;
    X(row, 6) = y2 * x1;
    X(row, 7) = y2 * y1;
    X(row, 8) = y2 *  1;
    row++;
    
    X(row, 0) = x1;
    X(row, 1) = y1;
    X(row, 2) = 1;
    X(row, 3) = 0;
    X(row, 4) = 0;
    X(row, 5) = 0;
    X(row, 6) = -x2 * x1;
    X(row, 7) = -x2 * y1;
    X(row, 8) = -x2 *  1;

}

/* Positive depth check adapted from essentialDiscrete.m
 */
bool homography_check_solution(const m4 &R, const v4 &T, const feature_t p1[4], const feature_t p2[4])
{
    int nvalid = 0;
    for(int i = 0; i < 4; i++)
    {
        v4 x1 = v4(p1[i].x(), p1[i].y(), 1, 0);
        v4 x2 = v4(p2[i].x(), p2[i].y(), 1, 0);
        m4 skew_Rx1 = skew3(R*x1);
        m4 skew_x2 = skew3(x2);
        float alpha1 = (float)(-(skew_x2*T).dot(skew_x2*R*x1)/pow((skew_x2*T).norm(), 2));
        float alpha2 =  (float)((skew_Rx1*x2).dot(skew_Rx1*T)/pow((skew_Rx1*x2).norm(), 2));
        //fprintf(stderr, "alpha1 %f alpha2 %f\n", alpha1, alpha2);
        if(alpha1 > 0 && alpha2 > 0)
            nvalid++;
    }
    return nvalid == 4;
}

m3 homography_estimate_from_constraints(const Eigen::Matrix<f_t, 8, 9> &X)
{
    //Matlab: [u,s,v] = svd(B);
    Eigen::JacobiSVD<Eigen::Matrix<f_t, 8, 9>> svdx(X, Eigen::ComputeFullV);
    auto V = svdx.matrixV();
    //Matlab: h = v(:,9);
    //    Hest = transpose(reshape(h,[3,3]));
    // Hest is not transposed below because the transpose from matlab
    // is just to undo what reshape has done
    m3 Hest;
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            Hest(i, j) = V(i*3+j, 8);
        }
    }
    
    //Matlab: [u,s,v] = svd(Hest);
    Eigen::JacobiSVD<decltype(Hest)> svdh(Hest);
    
    //Matlab: H = Hest/s(2,2);
    Hest /= svdh.singularValues()[1];
    return Hest;
}

void homography_factorize(const m4 &H, m4 Rs[4], v4 Ts[4], v4 Ns[4])
{
    matrix U(3, 3);
    matrix S(1, 3);
    matrix Vt(3, 3);
    //Matlab: [u,s,v] = svd(H'*H);
    m4 HtH = H.transpose() * H;
    matrix HtH_tmp(HtH.data(), 3, 3, 4, 4);
    matrix_svd(HtH_tmp, U, S, Vt);
    
    //Matlab: s1 = s(1,1); s2 = s(2,2); s3 = s(3,3); - ignore, just use S(1)
    
    //Matlab: if det(u) < 0 u = -u; end;
    //    v1 = u(:,1); v2 = u(:,2); v3 = u(:,3);
    
    m4 Utmp;
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            Utmp(i, j) = U(i, j);
        }
    }
    //float usign = (determinant3(Utmp) < 0.) ? 1. : -1.;
    
    v4 v1(U(0, 0), U(1, 0), U(2, 0), 0.);
    v4 v2(U(0, 1), U(1, 1), U(2, 1), 0.);
    v4 v3(U(0, 2), U(1, 2), U(2, 2), 0.);
    
    if(determinant3(Utmp) < 0.)
    {
        v1 = -v1;
        v2 = -v2;
        v3 = -v3;
    }
    
    //Matlab: u1 = (v1*sqrt(1-s3) + v3*sqrt(s1 -1))/sqrt(s1 - s3);
    v4 u1 = (v1 * sqrt(1 - S[2]) + v3 * sqrt(S[0] - 1)) / sqrt(S[0] - S[2]);
    
    //Matlab: u2 = (v1*sqrt(1-s3) - v3*sqrt(s1 -1))/sqrt(s1 - s3);
    v4 u2 = (v1 * sqrt(1 - S[2]) - v3 * sqrt(S[0] - 1)) / sqrt(S[0] - S[2]);
    
    //Matlab: U1 = [v2, u1, skew(v2)*u1];
    m4 Ut = m4::Zero();
    Ut.row(0) = v2;
    Ut.row(1) = u1;
    Ut.row(2) = skew3(v2) * u1;
    m4 U1 = Ut.transpose();
    
    //Matlab: U2 = [v2, u2, skew(v2)*u2];
    Ut.row(0) = v2;
    Ut.row(1) = u2;
    Ut.row(2) = skew3(v2) * u2;
    m4 U2 = Ut.transpose();
    
    //Matlab: W1 = [H*v2, H*u1, skew(H*v2)*H*u1];
    Ut.row(0) = H * v2;
    Ut.row(1) = H * u1;
    Ut.row(2) = skew3(H * v2) * H * u1;
    m4 W1 = Ut.transpose();
    
    //Matlab: W2 = [H*v2, H*u2, skew(H*v2)*H*u2];
    Ut.row(0) = H * v2;
    Ut.row(1) = H * u2;
    Ut.row(2) = skew3(H * v2) * H * u2;
    m4 W2 = Ut.transpose();
    
    //Matlab: N1 = skew(v2)*u1;
    v4 N1 = skew3(v2) * u1;
    //Matlab: N2 = skew(v2)*u2;
    v4 N2 = skew3(v2) * u2;
    
    /* Matlab:
     Sol(:,:,1) = [W1*U1', (H - W1*U1')*N1, N1];
     Sol(:,:,2) = [W2*U2', (H - W2*U2')*N2, N2];
     Sol(:,:,3) = [W1*U1', -(H - W1*U1')*N1, -N1];
     Sol(:,:,4) = [W2*U2', -(H - W2*U2')*N2, -N2];
     */
    Rs[0] = W1 * U1.transpose();
    Rs[1] = W2 * U2.transpose();
    Rs[2] = W1 * U1.transpose();
    Rs[3] = W2 * U2.transpose();
    
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            Rs[i](3, j) = 0.;
        }
        Rs[i](3, 3) = 1.;
    }
    
    Ts[0] = (H - Rs[0]) * N1;
    Ts[1] = (H - Rs[1]) * N2;
    Ts[2] = -(H - Rs[2]) * N1;
    Ts[3] = -(H - Rs[3]) * N2;
    
    Ns[0] = N1;
    Ns[1] = N2;
    Ns[2] = -N1;
    Ns[3] = -N2;
}

// Assumes p1 used to construct H was N = [0 0 1]', d = 1
// 
// H = R + 1/d T N'
// 
// since N = [0 0 1]' and d = 1 by definition
// 
// H = R + [0 0 T]
// 
// Let Ri = the ith column of R, Hi = ith column of H
// 
// H = [R1 R2 R3+T]
//
// R1 = H1 & R2 = H2
//
// R3 = cross(R1, R2)
//
// R = [R1 R2 R3]
//
// HR = H - R
//
// T = HR3
homography_decomposition homography_decompose_direct(const m4 & H)
{
    homography_decomposition d;
    d.N = v4(0, 0, 1, 0);

    v4 r0(H(0, 0), H(1, 0), H(2, 0), 0);
    v4 r1(H(0, 1), H(1, 1), H(2, 1), 0);
    v4 r2 = cross(r0, r1);
    m4 Rt;
    Rt.row(0) = r0;
    Rt.row(1) = r1;
    Rt.row(2) = r2;
    Rt.row(3) = v4(0, 0, 0, 1);
    // since we constructed R with each row corresponding to a column
    // from the derivation, we need to transpose it
    d.R = Rt.transpose();

    d.T = v4(H(0, 2) - d.R(0, 2), H(1, 2) - d.R(1, 2), H(2, 2) - d.R(2, 2), 0);

    return d;
}

void print_decomposition(const homography_decomposition & d)
{
    /*
    fprintf(stderr, "R = "); d.R.print(); fprintf(stderr, "\n");
    fprintf(stderr, "T = "); d.T.print(); fprintf(stderr, "\n");
    fprintf(stderr, "N = "); d.N.print(); fprintf(stderr, "\n");
    */
}

vector<homography_decomposition> homography_positive_depth(const feature_t p1[4], const feature_t p2[4], const m4 & H, const m4 Rs[4], const v4 Ts[4], const v4 Ns[4])
{
    vector<homography_decomposition> results;
    for(int i = 0; i < 4; ++i)
    {
        homography_decomposition d {Rs[i], Ts[i], Ns[i]};
        print_decomposition(d);
        // TODO checking Ns[i][2] should be equivalent to this
        bool valid = homography_check_solution(d.R, d.T, p1, p2);
        if(valid)
            results.push_back(d);
    }
    return results;
}

bool homography_should_flip_sign(const m4 & H, const feature_t p1[4], const feature_t p2[4])
{
    int poscount = 0;
    for(int i = 0; i < 4; i++) {
        v4 x1(p1[i].x(), p1[i].y(), 1, 0);
        v4 x2(p2[i].x(), p2[i].y(), 1, 0);
        if(x2.dot(H*x1) >= 0)
            poscount++;
        else
            poscount--;
    }

    return poscount < 0;
}

//Based on Matlab from http://cs.gmu.edu/%7Ekosecka/examples-code/homography2Motion.m
m4 homography_compute(const feature_t p1[4], const feature_t p2[4])
{
    Eigen::Matrix<f_t, 8, 9> X;
    
    /* Matlab:
     NPOINTS = size(p,2);
     B = [];
     for i = 1:NPOINTS
     B = [B; makeRows(p(:,i), q(:,i))];
     end;
     */
    for(int c = 0; c < 4; ++c)
    {
        //Note: Since we choose X1, this should never be degenerate
        homography_set_correspondence(X, c, p1[c].x(), p1[c].y(), p2[c].x(), p2[c].y());
    }
    m4 H = m4::Identity();
    H.block(0, 0, 3, 3) = homography_estimate_from_constraints(X);
    if(homography_should_flip_sign(H, p1, p2))
        H = -H;

    return H;
}

vector<homography_decomposition> homography_decompose(const feature_t p1[4], const feature_t p2[4], const m4 & H)
{
    m4 Rs[4];
    v4 Ts[4], Ns[4];
    homography_factorize(H, Rs, Ts, Ns);
    return homography_positive_depth(p1, p2, H, Rs, Ts, Ns);
}

void fill_ideal_points(feature_t ideal[4], float qr_size_m, bool use_markers, int modules)
{
    if(use_markers) {
        // Ideal points are the QR code viewed from the bottom with
        // +z going out of the code toward the viewer
        // libzxing detects the center of the three markers and the center of the
        // lower right marker (LR) which is closer to the center of the code than
        // the rest
        // modules are width/height of the qr code in bars
        // v2 = 25, v3 = 29, v4 = 33, v10 = 57 (4 * version + 17)
        feature_t offset = {0.5,0.5};
        ideal[0] = {          f_t(3.5),           f_t(3.5)}; // UL
        ideal[1] = {          f_t(3.5), modules - f_t(3.5)}; // LL
        ideal[2] = {modules - f_t(6.5), modules - f_t(6.5)}; // LR
        ideal[3] = {modules - f_t(3.5),           f_t(3.5)}; // UR
        for(int i = 0; i < 4; i++)
            ideal[i] = (ideal[i]/modules - offset)*qr_size_m;
    }
    else {
        ideal[0] = {f_t(-0.5)*qr_size_m, f_t(-0.5)*qr_size_m}; // UL
        ideal[1] = {f_t(-0.5)*qr_size_m, f_t( 0.5)*qr_size_m}; // LL
        ideal[2] = {f_t( 0.5)*qr_size_m, f_t( 0.5)*qr_size_m}; // LR
        ideal[3] = {f_t( 0.5)*qr_size_m, f_t(-0.5)*qr_size_m}; // UR
    }

}

#define USE_EASY_DECOMPOSITION 1
bool homography_align_qr_ideal(const feature_t p2[4], float qr_size_m, bool use_markers, int modules, homography_decomposition & result)
{
    feature_t p1[4];
    fill_ideal_points(p1, qr_size_m, use_markers, modules);

    m4 H = homography_compute(p1, p2);

#if USE_EASY_DECOMPOSITION

    result = homography_decompose_direct(H);
    print_decomposition(result);
    return true;

#else

    vector<homography_decomposition> decompositions = homography_decompose(p1, p2, H);
    // A good solution should have N close to [0 0 1]
    float minN2 = 0.98;
    float bestN = minN2;
    homography_decomposition best;
    for(int i = 0; i < decompositions.size(); i++) {
        if(decompositions[i].N[2] > bestN) {
            bestN = decompositions[i].N[2];
            best = decompositions[i];
            print_decomposition(best);
        }
    }
    if(bestN > minN2) {
        result = best;
        return true;
    }
    return false;

#endif
}

void compose_with(m4 & R, v4 & T, const m4 & withR, const v4 & withT)
{
    T = R*withT + T;
    R = R*withR;
}

void homography_ideal_to_qr(m4 & R, v4 & T)
{
    // Riq = 180 degree rotation around X axis
    m4 Riq = m4::Identity();
    Riq(1, 1) = -1; Riq(2, 2) = -1;
    v4 Tiq = v4(0, 0, 1, 0);
    compose_with(R, T, Riq, Tiq);
}

bool homography_align_to_qr(const feature_t p2[4], float qr_size_m, int modules, m4 & R, v4 & T)
{
    homography_decomposition result;
    bool use_markers = true;
    if(homography_align_qr_ideal(p2, qr_size_m, use_markers, modules, result)) {
        R = result.R;
        T = result.T;
        homography_ideal_to_qr(R, T);
        return true;
    }
    return false;
}
