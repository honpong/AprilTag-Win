// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include "vec4.h"
#include <assert.h>
#include "float.h"
#include "quaternion.h"

const m4v4 skew3_jacobian = { {
    {{ {0., 0., 0., 0.}, {0., 0.,-1., 0.}, { 0., 1., 0., 0.}, {0., 0., 0., 0.} }},
    {{ {0., 0., 1., 0.}, {0., 0., 0., 0.}, {-1., 0., 0., 0.}, {0., 0., 0., 0.} }},
    {{ {0.,-1., 0., 0.}, {1., 0., 0., 0.}, { 0., 0., 0., 0.}, {0., 0., 0., 0.} }},
    {{ {0., 0., 0., 0.}, {0., 0., 0., 0.}, { 0., 0., 0., 0.}, {0., 0., 0., 0.} }}
} };

const v4m4 invskew3_jacobian = { {
    {{
        {0., 0., 0., 0.},
        {0., 0.,-.5, 0.},
        {0., .5, 0., 0.},
        {0., 0., 0., 0.}
    }},
    
    {{
        {0., 0., .5, 0.},
        {0., 0., 0., 0.},
        {-.5, 0., 0.,0.},
        {0., 0., 0., 0.}
    }},
    
    {{
        {0.,-.5, 0., 0.},
        {.5, 0., 0., 0.},
        {0., 0., 0., 0.},
        {0., 0., 0., 0.}
    }},
    
    {{
        {0., 0., 0., 0.},
        {0., 0., 0., 0.},
        {0., 0., 0., 0.},
        {0., 0., 0., 0.}
    }}
} };

m4 rodrigues(const v4& W, m4v4 *dR_dW)
{
    v4 W2 = W * W;

    f_t theta2 = W2.sum();
    //1/theta ?= 0
    if(theta2 <= 0.) {
        if(dR_dW) {
            *dR_dW = skew3_jacobian;
        }
        return m4::Identity();
    }
    f_t theta = sqrt(theta2);
    f_t costheta, sintheta, invtheta, sinterm, costerm;
    v4 dsterm_dW, dcterm_dW;
    bool small = theta2 * theta2 <= 120 * F_T_EPS; //120 = 5!, next term of sine expansion
    if(small) {
        //Taylor expansion: sin = x - x^3 / 6; cos = 1 - x^2 / 2 + x^4 / 24
        sinterm = 1. - theta2 * (1./6.);
        costerm = 0.5 - theta2 * (1./24.);
        invtheta = 0.;
        costheta = 0.;
    } else {
        invtheta = 1. / theta;
        sintheta = sin(theta);
        costheta = cos(theta);
        sinterm = sintheta * invtheta;
        costerm = (1. - costheta) / theta2;
    }
    m4 V = skew3(W);
    m4 V2;
    V2(0, 0) = -W2[1] - W2[2];
    V2(1, 1) = -W2[2] - W2[0];
    V2(2, 2) = -W2[0] - W2[1];
    V2(2, 1) = V2(1, 2) = W[1]*W[2];
    V2(0, 2) = V2(2, 0) = W[0]*W[2];
    V2(1, 0) = V2(0, 1) = W[0]*W[1];
 
    if(dR_dW) {
        v4 ds_dW, dc_dW;
        if(small) {
            ds_dW = W * -(1./3.);
            dc_dW = W * -(1./12.);
        } else {
            v4
                ds_dt(v4::Constant((costheta - sinterm) * invtheta)),
                dc_dt(v4::Constant((sinterm - 2*costerm) * invtheta)),
                dt_dW = W * invtheta;
            ds_dW = ds_dt * dt_dW;
            dc_dW = dc_dt * dt_dW;
        }
        m4v4 dV2_dW;
        dV2_dW[0].row(0) = v4(0., -2*W[1], -2*W[2], 0.);
        dV2_dW[1].row(1) = v4(-2*W[0], 0., -2*W[2], 0.);
        dV2_dW[2].row(2) = v4(-2*W[0], -2*W[1], 0., 0.);
        //don't do multiple assignment to avoid gcc bug
        dV2_dW[2].row(1) = v4(0., W[2], W[1], 0);
        dV2_dW[1].row(2) = v4(0., W[2], W[1], 0);
        dV2_dW[0].row(2) = v4(W[2], 0., W[0], 0);
        dV2_dW[2].row(0) = v4(W[2], 0., W[0], 0);
        dV2_dW[1].row(0) = v4(W[1], W[0], 0., 0);
        dV2_dW[0].row(1) = v4(W[1], W[0], 0., 0);
        
        *dR_dW = skew3_jacobian * sinterm + outer_product(ds_dW, V) + dV2_dW * costerm + outer_product(dc_dW, V2);
    }
    return m4::Identity() + V * sinterm + V2 * costerm;
}

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
