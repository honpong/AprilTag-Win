//
//  state_motion.h
//  RC3DK
//
//  Created by Eagle Jones on 1/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef __RC3DK__state_motion__
#define __RC3DK__state_motion__

#include <iostream>
#include "state.h"

class state_motion: public state_root {
public:
    state_vector T;
    state_rotation_vector W;
    state_vector V;
    state_vector a;
    state_vector da;
    state_vector w;
    state_vector dw;
    state_vector a_bias;
    state_vector w_bias;
    state_motion()
    {
        T.dynamic = true; W.dynamic = true;
        children.push_back(&T); children.push_back(&W);
        w.dynamic = true;
        V.dynamic = true;
        a.dynamic = true;
        children.push_back(&V); children.push_back(&a); children.push_back(&da); children.push_back(&w); children.push_back(&dw); children.push_back(&a_bias); children.push_back(&w_bias);
    }
    void evolve_orientation_only(f_t dt);
    void evolve(f_t dt);
private:
    void evolve_state_orientation_only(f_t dt);
    void evolve_covariance_orientation_only(f_t dt);
    void project_motion_covariance_orientation_only(matrix &dst, const matrix &src, f_t dt, const m4 &dWp_dW, const m4 &dWp_dw, const m4 &dWp_ddw);
    void evolve_state(f_t dt);
    void evolve_covariance(f_t dt);
    void project_motion_covariance(matrix &dst, const matrix &src, f_t dt, const m4 &dWp_dW, const m4 &dWp_dw, const m4 &dWp_ddw);
};

class state_motion_derivative {
public:
    v4 V, a, da, w, dw;
    state_motion_derivative(const state_motion &state): V(state.V.v), a(state.a.v), da(state.da.v), w(state.w.v), dw(state.dw.v) {}
    state_motion_derivative() {}
};

class state_motion_gravity: public state_motion {
public:
    state_scalar g;
    state_motion_gravity() { } //children.push_back(&g); }
};


#endif /* defined(__RC3DK__state_motion__) */
