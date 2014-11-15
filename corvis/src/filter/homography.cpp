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
 [x1      y1      1        0        0       0       x1x2    y1x2    x2 ]
 
 Note: Since we choose X1, this should never be degenerate
 */
static void set_homography_correspondence(matrix &X, int n, float x1, float y1, float x2, float y2)
{
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
bool check_solution(const m4 & R, const v4 & T, const feature_t p1[4], const feature_t p2[4])
{
    int nvalid = 0;
    for(int i = 0; i < 4; i++)
    {
        v4 x1 = v4(p1[i].x, p1[i].y, 1, 0);
        v4 x2 = v4(p2[i].x, p2[i].y, 1, 0);
        m4 skew_Rx1 = skew3(R*x1);
        m4 skew_x2 = skew3(x2);
        float alpha1 = -sum((skew_x2*T)*(skew_x2*R*x1))/pow(norm(skew_x2*T), 2);
        float alpha2 =  sum((skew_Rx1*x2)*(skew_Rx1*T))/pow(norm(skew_Rx1*x2), 2);
//        fprintf(stderr, "%f %f\n", alpha1, alpha2);
        if(alpha1 > 0 && alpha2 > 0)
            nvalid++;
    }
    if(nvalid == 4)
        return true;
//    fprintf(stderr, "nvalid %d\n", nvalid);
    return false;
}

//Based on Matlab from http://cs.gmu.edu/%7Ekosecka/examples-code/homography2Motion.m
bool compute_planar_homography(const feature_t p1[4], const feature_t p2[4], m4 & R, v4 & T)
{
    matrix X(8, 9);
    
    /* Matlab:
     NPOINTS = size(p,2);
     B = [];
     for i = 1:NPOINTS
     B = [B; makeRows(p(:,i), q(:,i))];
     end;
     */
    for(int c = 0; c < 4; ++c)
    {
        set_homography_correspondence(X, c, p1[c].x, p1[c].y, p2[c].x, p2[c].y);
    }
    
    //Matlab: [u,s,v] = svd(B);
    matrix U(8, 8);
    matrix S(1, 8);
    matrix Vt(9, 9);
    
    matrix_svd(X, U, S, Vt);
    
    //Matlab: h = v(:,9);
    //    Hest = transpose(reshape(h,[3,3]));
    // Hest is not transposed below because the transpose from matlab
    // is just to undo what reshape has done
    m4 Hest = m4_identity;
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            Hest[i][j] = Vt(8, i*3+j);
        }
    }
    
    //Matlab: [u,s,v] = svd(Hest);
    U.resize(3, 3);
    S.resize(1, 3);
    Vt.resize(3, 3);
    matrix Hest_tmp(3,3);
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            Hest_tmp(i, j) = Hest[i][j];
        }
    }
    
    //Matlab: [u,s,v] = svd(Hest);
    matrix_svd(Hest_tmp, U, S, Vt);
    
    //Matlab: H = Hest/s(2,2);
    m4 H = Hest * (1. / S[1]);
    
    //Matlab: [u,s,v] = svd(H'*H);
    m4 HtH = transpose(H) * H;
    matrix HtH_tmp(&(HtH.data[0][0]), 3, 3, 4, 4);
    matrix_svd(HtH_tmp, U, S, Vt);
    
    //Matlab: s1 = s(1,1); s2 = s(2,2); s3 = s(3,3); - ignore, just use S(1)
    
    //Matlab: if det(u) < 0 u = -u; end;
    //    v1 = u(:,1); v2 = u(:,2); v3 = u(:,3);
    
    m4 Utmp;
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            Utmp[i][j] = U(i, j);
        }
    }
    float usign = (determinant3(Utmp) < 0.) ? 1. : -1.;
    
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
    m4 U1;
    U1[0] = v2;
    U1[1] = u1;
    U1[2] = skew3(v2) * u1;
    U1 = transpose(U1);
    
    //Matlab: U2 = [v2, u2, skew(v2)*u2];
    m4 U2;
    U2[0] = v2;
    U2[1] = u2;
    U2[2] = skew3(v2) * u2;
    U2 = transpose(U2);
    
    //Matlab: W1 = [H*v2, H*u1, skew(H*v2)*H*u1];
    m4 W1;
    W1[0] = H * v2;
    W1[1] = H * u1;
    W1[2] = skew3(H * v2) * H * u1;
    W1 = transpose(W1);
    
    //Matlab: W2 = [H*v2, H*u2, skew(H*v2)*H*u2];
    m4 W2;
    W2[0] = H * v2;
    W2[1] = H * u2;
    W2[2] = skew3(H * v2) * H * u2;
    W2 = transpose(W2);
    
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
    m4 Rs[4]; v4 Ns[4]; v4 Ts[4];
    Rs[0] = W1 * transpose(U1);
    Rs[1] = W2 * transpose(U2);
    Rs[2] = W1 * transpose(U1);
    Rs[3] = W2 * transpose(U2);
    
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            Rs[i][3][j] = 0.;
        }
        Rs[i][3][3] = 1.;
    }
    
    Ts[0] = (H - Rs[0]) * N1;
    Ts[1] = (H - Rs[1]) * N2;
    Ts[2] = -(H - Rs[2]) * N1;
    Ts[3] = -(H - Rs[3]) * N2;
    
    Ns[0] = N1;
    Ns[1] = N2;
    Ns[2] = -N1;
    Ns[3] = -N2;
    
    R = m4_identity;
    T = v4(0,0,0,0);

    f_t maxdelta2 = .1; //This is sufficiently high that no valid solution should exceed it
    f_t mindelta2 = maxdelta2;
    
    for(int i = 0; i < 4; ++i)
    {
        // MaSKS advocates checking N'*e3 > 0, but
        // since e3 is [0, 0, 1] this is equivalent
        // TODO: this returns two solutions for the qr code case
        // unless we enforce that N is approximately [0, 0, 1]
        if(Ns[i][2] > 0.99 && check_solution(Rs[i], Ts[i], p1, p2))
        {
            f_t delta2 = 0.;
            for(int c = 0; c < 4; ++c)
            {
                v4 x = v4(p1[c].x, p1[c].y, 1., 0.);
                v4 xc = Rs[i] * x + Ts[i];
                f_t dx = xc[0] / xc[2] - p2[c].x;
                f_t dy = xc[1] / xc[2] - p2[c].y;
                delta2 += dx * dx + dy * dy;
            }
            if(delta2 < mindelta2)
            {
                mindelta2 = delta2;
                R = Rs[i];
                T = Ts[i];
            }
        }
    }
    if(mindelta2 < maxdelta2)
    {
        fprintf(stderr, "Final solution, error %e:\n", mindelta2);
        R.print(); T.print(); fprintf(stderr, "\n");
        return true;
    }
    return false;
}

// qr_width is width of one side in meters
bool compute_qr_homography(feature_t calibrated_points[4], float qr_width, m4 &R, v4 &T)
{
    feature_t ideal_points[4]; // fake image of a qr code qr_width meter on each side, located at z=1 on an image plane at z=1
    ideal_points[0] = (feature_t){.x = -.5f * qr_width, .y = .5f * qr_width};
    ideal_points[1] = (feature_t){.x = -.5f * qr_width, .y = -.5f * qr_width};
    ideal_points[2] = (feature_t){.x = .5f * qr_width,  .y = -.5f * qr_width};
    ideal_points[3] = (feature_t){.x = .5f * qr_width,  .y = .5f * qr_width};

    return compute_planar_homography(ideal_points, calibrated_points, R, T);
}
