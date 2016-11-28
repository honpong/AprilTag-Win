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
#include "transformation.h"

class state_imu_intrinsics: public state_branch<state_node *>
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
public:
    state_vector w_bias {"w_bias", constant};
    state_vector a_bias {"a_bias", constant};

    state_imu_intrinsics()
    {
        children.push_back(&w_bias);
        children.push_back(&a_bias);
    }
    bool estimate_bias = true;
    void enable_bias_estimation();
    void disable_bias_estimation();
};

struct state_imu: public state_branch<state_node *> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    state_extrinsics extrinsics;
    state_imu_intrinsics intrinsics;
    state_imu() : extrinsics("Qa", "Ta", false) {
        children.push_back(&intrinsics);
        children.push_back(&extrinsics);
    }
};

class state_motion_orientation: public state_root {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_quaternion Q { "Q",  dynamic };
    state_vector     w { "w",  dynamic };
    state_vector    dw { "dw", dynamic };
    state_vector   ddw {"ddw", fake };
    state_scalar     g { "g",  constant };
    state_imu imu;

    state_motion_orientation(covariance &c): state_root(c) {
        children.push_back(&Q);
        children.push_back(&w);
        children.push_back(&dw);
        children.push_back(&ddw);
        children.push_back(&imu);
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
    state_vector  T {  "T", dynamic };
    state_vector  V {  "V", dynamic };
    state_vector  a {  "a", dynamic };
    state_vector da { "da", fake };

    float total_distance = 0;
    transformation loop_offset;

    v3 last_position = v3::Zero();

    state_motion(covariance &c): state_motion_orientation(c) {
        children.push_back(&T);
        children.push_back(&V);
        children.push_back(&a);
        children.push_back(&da);
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

    virtual void enable_orientation_only(bool remap_ = true);
    virtual void disable_orientation_only(bool remap_ = true);
    virtual void enable_bias_estimation(bool remap_ = true);
    virtual void disable_bias_estimation(bool remap_ = true);

    void copy_from(const state_motion & other);

protected:
    bool orientation_only = false;
    virtual void add_non_orientation_states();
    virtual void remove_non_orientation_states();
    virtual void evolve_state(f_t dt);
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt);
    virtual void cache_jacobians(f_t dt);
    v3 dT;
};

#endif /* defined(__RC3DK__state_motion__) */
