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
 See below for derivation:
 [0       0       0       -z1x2   -z1y2   -z1     y1x2    y1y2    y1 ]
 [z1x2    z1y2    z1      0       0       0       -x1x2   -x1y2   -x1]
 */
static void set_homography_correspondence(matrix &X, int n, float x1, float y1, float z1, float x2, float y2)
{
    /* Matlab, for dual projection case, not ours:
     % set up constraints for homography matrix
     function [rows] = makeRows(x1, x2);
     
     rows = [ 0 0 0    -x1'    x2(2)*x1';
     x1'  0 0 0    -x2(1)*x1'];
     */
    int row = n * 2;
    X(row, 0) = 0;
    X(row, 1) = 0;
    X(row, 2) = 0;
    X(row, 3) = -z1 * x2;
    X(row, 4) = -z1 * y2;
    X(row, 5) = -z1;
    X(row, 6) = y1 * x2;
    X(row, 7) = y1 * y2;
    X(row, 8) = y1;

    row++;
    X(row, 0) = z1 * x2;
    X(row, 1) = z1 * y2;
    X(row, 2) = z1;
    X(row, 3) = 0;
    X(row, 4) = 0;
    X(row, 5) = 0;
    X(row, 6) = -x1 * x2;
    X(row, 7) = -x1 * y2;
    X(row, 8) = -x1;
}

 /*
     Derivation follows: http://vision.ucla.edu//MASKS/MASKS-ch5.pdf section 5.3
     
     X1 = 3D position of corner in QR-centered coordinates
     R,T = transformation bringing camera coordinates to QR-centered coordinates
     p = unknown projective scale factor (z2)
     X2 = (x2, y2, 1) - normalized image coordinates of projected point
     
     X1 = R p X2 + T

     N = normal to plane where the 4 features lie (in camera coordinates)
     d = distance from camera to the plane (not to the point)
     N^t p X2 = d follows from above two definitions, so
     1/d N^t p X2 = 1
     
     Now we multiply T by 1, and subtitute in the above.
     
     X1 = R p X2 + T / d N^t p X2
     X1 = H p X2, where H = R + T / d N^t
     
     Take cross product, X1 x X1 = 0; express as X1^ X1, where X1^ is (X1 hat) the skew-symmetric matrix for cross product.
     
     X1^ = [0   -z1  y1]
           [x1    0 -x1]
           [-y1  x1   0]
     
     X1^ X1 = X1^ H p X2
     0 = X1^ H p X2; p is a non-zero scalar, so we can divide it out
     0 = X1^ H X2
     
     Now work out the coefficients
     
     H X2 = [ H0 H1 H2 ] [x2]   [H0x2+H1y2+H2]
            [ H3 H4 H5 ]*[y2] = [H3x2+H4y2+H5]
            [ H6 H7 H8 ] [ 1]   [H6x2+H7y2+H8]
     
                [0       0       0       -z1x2   -z1y2   -z1     y1x2    y1y2    y1 ]   [H0]
     X1^ H X2 = [z1x2    z1y2    z1      0       0       0       -x1x2   -x1y2   -x1] * [H1]
                [-y1x2   -y1y2   -y1     x1x2    x1y2    x1      0       0       0  ]   [H2]
                                                                                        [H3]
                                                                                        [H4]
                                                                                        [H5]
                                                                                        [H6]
                                                                                        [H7]
                                                                                        [H8]
     
     The X1X2 matrix only has rank 2, so we just use the first two constraints for each pixel.
     
     Decomposition of planar homography matrix should follow Masks derivation.

     */

    //Based on Matlab from http://cs.gmu.edu/%7Ekosecka/examples-code/homography2Motion.m
    //TODO - create test code for all of this.


void compute_planar_homography_one_sided(const v4 world_points[4], const feature_t calibrated[4])
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
        set_homography_correspondence(X, c, world_points[c][0], world_points[c][1], world_points[c][2], calibrated[c].x, calibrated[c].y);
    }
    
    //Matlab: [u,s,v] = svd(B);
    matrix U(8, 8);
    matrix S(1, 8);
    matrix Vt(9, 9);
    
    matrix_svd(X, U, S, Vt);
    
    //Matlab: h = v(:,9);
    //    Hest = transpose(reshape(h,[3,3]));
    m4 h = m4_identity;
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            h[i][j] = Vt(8, i*3+j);
        }
    }
    m4 Hest = transpose(h); //Check this - matlab version transposed?
    
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
    
    Ns[0] = (H - W1 * transpose(U1)) * N1;
    Ns[1] = (H - W2 * transpose(U2)) * N2;
    Ns[2] = -(H - W1 * transpose(U1)) * N1;
    Ns[3] = -(H - W2 * transpose(U2)) * N2;
    
    Ts[0] = N1;
    Ts[1] = N2;
    Ts[2] = -N1;
    Ts[3] = -N2;
    
    fprintf(stderr, "Solutions found:\n");
    for(int i = 0; i < 4; ++i)
    {
        Rs[i].print(); Ns[i].print(); Ts[i].print();
        fprintf(stderr, "\n\n");
    }
}

