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

class state_motion_orientation: public state_root {
public:
    state_rotation_vector W;
    state_vector w;
    state_vector dw;
    state_vector w_bias;
    state_vector a_bias;
    state_scalar g;
    
    state_motion_orientation(covariance &c): state_root(c) {
        W.dynamic = true;
        w.dynamic = true;
        children.push_back(&W);
        children.push_back(&w);
        children.push_back(&dw);
        children.push_back(&w_bias);
        children.push_back(&a_bias);
        //children.push_back(&g);
    }
protected:
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt);
    virtual void evolve_state(f_t dt);
    void evolve_covariance(f_t dt);
    void cache_jacobians(f_t dt);
private:
    m4 dWp_dW, dWp_dw, dWp_ddw;
};

class state_motion: public state_motion_orientation {
public:
    state_vector T;
    state_vector V;
    state_vector a;
    state_vector da;
    
    state_motion(covariance &c): state_motion_orientation(c), orientation_only(false)
    {
        T.dynamic = true;
        V.dynamic = true;
        a.dynamic = true;
        children.push_back(&T);
        children.push_back(&V);
        children.push_back(&a);
        children.push_back(&da);
    }
    virtual void enable_orientation_only();
    virtual void disable_orientation_only();
    virtual void evolve(f_t dt);
protected:
    bool orientation_only;
    virtual void add_non_orientation_states();
    virtual void remove_non_orientation_states();
private:
    void evolve_orientation_only(f_t dt);
    void evolve_covariance_orientation_only(f_t dt);
    virtual void evolve_state(f_t dt);
    void project_motion_covariance(matrix &dst, const matrix &src, f_t dt);
};

#endif /* defined(__RC3DK__state_motion__) */
