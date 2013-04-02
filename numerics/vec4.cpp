// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include "vec4.h"
#include <assert.h>
#include "float.h"

#ifdef F_T_IS_DOUBLE
#define EPS DBL_EPSILON
#else
#define EPS FLT_EPSILON
#endif

const m4 m4_identity = { {
        v4(1, 0, 0, 0),
        v4(0, 1, 0, 0),
        v4(0, 0, 1, 0),
        v4(0, 0, 0, 1)
    }
};

const static m4v4 dV_dv = { {
        {{v4(0., 0., 0., 0.), v4(0., 0.,-1., 0.), v4( 0., 1., 0., 0.), v4(0., 0., 0., 0.)}},
        {{v4(0., 0., 1., 0.), v4(0., 0., 0., 0.), v4(-1., 0., 0., 0.), v4(0., 0., 0., 0.)}},
        {{v4(0.,-1., 0., 0.), v4(1., 0., 0., 0.), v4( 0., 0., 0., 0.), v4(0., 0., 0., 0.)}},
        {{v4(0., 0., 0., 0.), v4(0., 0., 0., 0.), v4( 0., 0., 0., 0.), v4(0., 0., 0., 0.)}}
    }
};

const static v4m4 dv_dV = { {
        {{v4(0., 0., 0., 0.),
          v4(0., 0.,-.5, 0.),
          v4(0., .5, 0., 0.),
          v4(0., 0., 0., 0.)}},

        {{v4(0., 0., .5, 0.),
          v4(0., 0., 0., 0.),
          v4(-.5, 0., 0.,0.),
          v4(0., 0., 0., 0.)}},
        
        {{v4(0.,-.5, 0., 0.),
          v4(.5, 0., 0., 0.),
          v4(0., 0., 0., 0.),
          v4(0., 0., 0., 0.)}},
        
        {{v4(0., 0., 0., 0.),
          v4(0., 0., 0., 0.),
          v4(0., 0., 0., 0.),
          v4(0., 0., 0., 0.)}}
    }
};

/*
    if(V2) {

    }
}
*/

m4 rodrigues(const v4 W, m4v4 *dR_dW)
{
    v4 W2 = W * W;

    f_t theta2 = sum(W2);
    //1/theta ?= 0
    if(theta2 <= 0.) {
        if(dR_dW) {
            *dR_dW = dV_dv;
        }
        return m4_identity;
    }
    f_t theta = sqrt(theta2);
    f_t costheta, sintheta, invtheta, sinterm, costerm;
    v4 dsterm_dW, dcterm_dW;
    bool small = theta2 * theta2 * (1./120.) <= EPS; //120 = 5!, next term of sine expansion
    if(small) {
        //Taylor expansion: sin = x - x^3 / 6; cos = 1 - x^2 / 2 + x^4 / 24
        sinterm = 1. - theta2 * (1./6.);
        costerm = 0.5 - theta2 * (1./24.);
    } else {
        invtheta = 1. / theta;
        sintheta = sin(theta);
        costheta = cos(theta);
        sinterm = sintheta * invtheta;
        costerm = (1. - costheta) / theta2;
    }
    m4 V = skew3(W);
    m4 V2;
    V2[0][0] = -W2[1] - W2[2];
    V2[1][1] = -W2[2] - W2[0];
    V2[2][2] = -W2[0] - W2[1];
    V2[2][1] = V2[1][2] = W[1]*W[2];
    V2[0][2] = V2[2][0] = W[0]*W[2];
    V2[1][0] = V2[0][1] = W[0]*W[1];
 
    if(dR_dW) {
        v4 ds_dW, dc_dW;
        if(small) {
            ds_dW = W * -(1./3.);
            dc_dW = W * -(1./12.);
        } else {
            v4
                ds_dt = (costheta - sinterm) * invtheta,
                dc_dt = (sinterm - 2*costerm) * invtheta,
                dt_dW = W * invtheta;
            ds_dW = ds_dt * dt_dW;
            dc_dW = dc_dt * dt_dW;
        }
        m4v4 dV2_dW;
        dV2_dW[0][0] = v4(0., -2*W[1], -2*W[2], 0.);
        dV2_dW[1][1] = v4(-2*W[0], 0., -2*W[2], 0.);
        dV2_dW[2][2] = v4(-2*W[0], -2*W[1], 0., 0.);
        //don't do multiple assignment to avoid gcc bug
        dV2_dW[2][1] = v4(0., W[2], W[1], 0);
        dV2_dW[1][2] = v4(0., W[2], W[1], 0);
        dV2_dW[0][2] = v4(W[2], 0., W[0], 0);
        dV2_dW[2][0] = v4(W[2], 0., W[0], 0);
        dV2_dW[1][0] = v4(W[1], W[0], 0., 0);
        dV2_dW[0][1] = v4(W[1], W[0], 0., 0);
        
        *dR_dW = dV_dv * sinterm + outer_product(ds_dW, V) + dV2_dW * costerm + outer_product(dc_dW, V2);
    }
    return m4_identity + V * sinterm + V2 * costerm;
}

v4 invrodrigues(const m4 R, v4m4 *dW_dR)
{
    f_t trc = trace3(R);
    //sin theta can be zero if:
    if(trc >= 3.) { //theta = 0, so sin = 0
        if(dW_dR) {
            *dW_dR = dv_dV;
        }
        return v4(0.);
    }
    if(trc <= -1. + EPS) {//theta = pi - discontinuity as axis flips; off-axis elements don't give a good vector
        assert(0 && "need to implement invrodrigues for theta = pi"); //I don't think we need to do this since we integrate with small time steps
        //pick the largest diagonal - then average off-diagonal elements
        // http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToAngle/
        if(R[0][0] > R[1][1] && R[0][0] > R[2][2]) { //x is largest
        } else if(R[1][1] > R[2][2]) { // y is largest
        } else { // z is largest
        }
    }

    v4 s = invskew3(R);
    f_t
        costheta = (trc - 1.0) / 2.0,
        theta = acos(costheta),
        sintheta = sin(theta);

    if(theta * theta / 6. < EPS) { //theta is small, so we have near-skew-symmetry and discontinuity
        //just use the off-diagonal elements
        if(dW_dR) *dW_dR = dv_dV;
        return s;
    }
    f_t invsintheta = 1. / sintheta,
        invsin2theta = invsintheta * invsintheta,
        thetaf = theta * invsintheta;

    if(dW_dR) {
        *dW_dR = v4m4();
        f_t
            dtheta_dtrc = -0.5 * invsintheta,
            dtf_dt = invsintheta - theta * costheta * invsin2theta;

        m4 dtheta_dR = m4_identity * dtheta_dtrc;

        *dW_dR = dv_dV * thetaf + outer_product(dtheta_dR, s*dtf_dt);
    }
    return s * thetaf;
}

