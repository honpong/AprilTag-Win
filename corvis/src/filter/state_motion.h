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

class state_imu_intrinsics: public state_branch<state_node *>
{
public:
    state_vector w_bias {"w_bias"};
    state_vector a_bias {"a_bias"};

    state_imu_intrinsics()
    {
        children.push_back(&w_bias);
        children.push_back(&a_bias);
    }
};

class state_motion_orientation: public state_root {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_quaternion Q;
    state_vector w;
    state_vector dw;
    state_imu_intrinsics imu_intrinsics;
    state_scalar g;
    
    state_motion_orientation(covariance &c): state_root(c), Q("Q"), w("w"), dw("dw"), g("g") {
        Q.dynamic = true;
        w.dynamic = true;
        children.push_back(&Q);
        children.push_back(&w);
        children.push_back(&dw);
        children.push_back(&imu_intrinsics);
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

    bool orientation_initialized = false;
    quaternion initial_orientation;

protected:
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt);
    virtual void evolve_state(f_t dt);
    virtual void cache_jacobians(f_t dt);
    m3 Rt;
    v3 dW;
    m3 JdW_s, dQp_s_dW;
private:
    f_t gravity_magnitude = (f_t)9.80665;
};

class state_motion: public state_motion_orientation {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    friend class observation_accelerometer;
public:
    state_vector T;
    state_vector V;
    state_vector a;

    float total_distance = 0;
    v3 last_position = v3::Zero();

    state_motion(covariance &c): state_motion_orientation(c), T("T"), V("V"), a("a")
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
        total_distance = 0.;
    }
    
    void reset_position()
    {
        T.v = v3::Zero();
        total_distance = 0.;
        last_position = v3::Zero();
    }

    virtual void enable_orientation_only();
    virtual void disable_orientation_only();
    virtual void enable_bias_estimation();
    virtual void disable_bias_estimation();
protected:
    bool estimate_bias = true;;
    bool orientation_only = false;
    virtual void add_non_orientation_states();
    virtual void remove_non_orientation_states();
    virtual void evolve_state(f_t dt);
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt);
    virtual void cache_jacobians(f_t dt);
    v3 dT;
};

#endif /* defined(__RC3DK__state_motion__) */
