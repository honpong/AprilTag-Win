// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include "vec4.h"
#include <assert.h>

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
    f_t eps=0.0;

    f_t theta2 = sum(W2);
    //1/theta ?= 0
    if(theta2 <= eps) {
        if(dR_dW) {
            *dR_dW = dV_dv;
        }
        return m4_identity;
    }
    f_t 
        theta = sqrt(theta2),
        invtheta = 1.0 / theta,
        sintheta = sin(theta),
        costheta = cos(theta),
        sinterm = sintheta * invtheta,
        costerm = (1-costheta) / theta2;

    m4 V = skew3(W);
    m4 V2;
    V2[0][0] = -W2[1] - W2[2];
    V2[1][1] = -W2[2] - W2[0];
    V2[2][2] = -W2[0] - W2[1];
    V2[2][1] = V2[1][2] = W[1]*W[2];
    V2[0][2] = V2[2][0] = W[0]*W[2];
    V2[1][0] = V2[0][1] = W[0]*W[1];
 
    if(dR_dW) {
        v4
            ds_dt = (costheta - sinterm) * invtheta,
            dc_dt = (sinterm - 2*costerm) * invtheta,
            dt_dW = W * invtheta,
            ds_dW = ds_dt * dt_dW,
            dc_dW = dc_dt * dt_dW;
        
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
    f_t trc = trace3(R), eps1 = 3.0, eps2 = -1.0;
    //sin theta can be zero if:
    if(trc >= eps1) { //theta = 0
        if(dW_dR) {
            *dW_dR = dv_dV;
        }
        return v4(0.);
    }
    assert(trc > eps2);
    
    f_t
        costheta = (trc - 1.0) / 2.0,
        theta = acos(costheta),
        invsin2theta = 1. / (1 - costheta * costheta),
        invsintheta = sqrt(invsin2theta),
        thetaf = theta * invsintheta;

    v4 
        s = invskew3(R);
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

