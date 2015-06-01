// Written by Eagle Jones
// Copyright (c) 2012-2015, RealityCap
// All rights reserved.
//

#include "vec4.h"
#include <assert.h>
#include "float.h"

v4 integrate_angular_velocity(const v4 &W, const v4 &w)
{
    f_t theta2 = W.dot(W);
    f_t gamma, eta;
    if (theta2 * theta2 <= 120 * F_T_EPS) {
        gamma = 2. - theta2 / 6.;
        eta = W.dot(w) * (-60. - theta2) / 360.;
    } else {
        f_t theta = sqrt(theta2),
            sinp = sin(.5 * theta),
            cosp = cos(.5 * theta),
            cotp = cosp / sinp,
            invtheta = 1. / theta;
        gamma = theta * cotp,
        eta = W.dot(w) * invtheta * (cotp - 2. * invtheta);
    }
    return W + .5 * (gamma * w - eta * W + cross(W, w));
}

void linearize_angular_integration(const v4 &W, const v4 &w, m4 &dW_dW, m4 &dW_dw)
{
    f_t theta2 = W.dot(W); //dtheta2_dW = 2 * W
    f_t gamma, eta;
    v4 dg_dW, de_dW, de_dw;
    if(theta2 * theta2 <= 120 * F_T_EPS) {
        gamma = 2. - theta2 / 6.;
        eta = W.dot(w) * (-60. - theta2) / 360.;
        dg_dW = W * -(1./3.);
        de_dW = W.dot(w) * W * (-1./180.) + w * (-60. - theta2) / 360.;
        de_dw = W * (-60. - theta2) / 360.;
    } else {
        f_t theta = sqrt(theta2),
            sinp = sin(.5 * theta),
            cosp = cos(.5 * theta),
            cotp = cosp / sinp,
            invtheta = 1. / theta;
        gamma = theta * cotp;
        eta = W.dot(w) * invtheta * (cotp - 2. * invtheta);
        v4 dt_dW = W * invtheta;
        f_t dcotp_dt = -.5 / (sinp * sinp);
        v4 dcotp_dW = dcotp_dt * dt_dW;
        v4 dinvtheta_dW = -1. / theta2 * dt_dW;
        f_t dg_dt = cotp + theta * dcotp_dt;
        dg_dW = dg_dt * dt_dW;
        de_dW = w * invtheta * (cotp - 2. * invtheta) + W.dot(w) * (dinvtheta_dW * (cotp - 2. * invtheta) + invtheta * (dcotp_dW -2. * dinvtheta_dW));
        de_dw = W * invtheta * (cotp - 2. * invtheta);
    }

    m4 m3_identity = m4::Identity();
    m3_identity(3, 3) = 0.;
    
    m4 dg_de_combined;
    dg_de_combined.row(0) = dg_dW * w[0] - de_dW * W[0];
    dg_de_combined.row(1) = dg_dW * w[1] - de_dW * W[1];
    dg_de_combined.row(2) = dg_dW * w[2] - de_dW * W[2];
    dg_de_combined.row(3) = dg_dW * w[3] - de_dW * W[3];
    
    m4 de_dw_dot_W;
    de_dw_dot_W.row(0) = de_dw * W[0];
    de_dw_dot_W.row(1) = de_dw * W[1];
    de_dw_dot_W.row(2) = de_dw * W[2];
    de_dw_dot_W.row(3) = de_dw * W[3];
    
    dW_dW = m4::Identity() + .5 * (dg_de_combined - (eta * m3_identity) - skew3(w));
    dW_dw = .5 * (gamma * m3_identity - de_dw_dot_W + skew3(W));
}
