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
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_rotation_vector W;
    state_vector w;
    state_vector dw;
    state_vector w_bias;
    state_vector a_bias;
    state_scalar g;
    
    state_motion_orientation(covariance &c): state_root(c), W("W"), w("w"), dw("dw"), w_bias("w_bias"), a_bias("a_bias"), g("g"), orientation_initialized(false), gravity_magnitude(9.80665) {
        W.dynamic = true;
        w.dynamic = true;
        children.push_back(&W);
        children.push_back(&w);
        children.push_back(&dw);
        children.push_back(&w_bias);
        children.push_back(&a_bias);
        //children.push_back(&g);
        g.v = gravity_magnitude;
    }
    
    virtual void reset()
    {
        orientation_initialized = false;
        state_root::reset();
        g.v = gravity_magnitude;
    }

    void compute_gravity(double latitude, double altitude);

    bool orientation_initialized;
    quaternion initial_orientation;
    
protected:
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt);
    virtual void evolve_state(f_t dt);
    void evolve_covariance(f_t dt);
    virtual void cache_jacobians(f_t dt);
    m4 Rt, Rt_dR_dW;
    v4 dW;
private:
    f_t gravity_magnitude;
    m4 dWp_dW, dWp_ddW;
};

class state_motion: public state_motion_orientation {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    friend class observation_accelerometer;
public:
    state_vector T;
    state_vector V;
    state_vector a;
    
    state_motion(covariance &c): state_motion_orientation(c), T("T"), V("V"), a("a"), estimate_bias(true), orientation_only(false)
    {
        T.dynamic = true;
        V.dynamic = true;
        children.push_back(&T);
        children.push_back(&V);
        children.push_back(&a);
    }
    
    virtual void reset()
    {
        disable_orientation_only();
        state_motion_orientation::reset();
    }
    
    virtual void enable_orientation_only();
    virtual void disable_orientation_only();
    virtual void evolve(f_t dt);
    virtual void enable_bias_estimation();
    virtual void disable_bias_estimation();
protected:
    bool estimate_bias;
    bool orientation_only;
    virtual void add_non_orientation_states();
    virtual void remove_non_orientation_states();
    virtual void evolve_state(f_t dt);
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt);
    virtual void cache_jacobians(f_t dt);
    v4 dT;
private:
    void evolve_orientation_only(f_t dt);
    void evolve_covariance_orientation_only(f_t dt);
};

#endif /* defined(__RC3DK__state_motion__) */
