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
#include "state_size.h"

class state_imu_intrinsics: public state_branch<state_node *>
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    state_vector<3> w_bias {"w_bias", constant};
    state_vector<3> a_bias {"a_bias", constant};

    state_imu_intrinsics()
    {
        children.push_back(&w_bias);
        children.push_back(&a_bias);
    }

    void copy_from(const state_imu_intrinsics &other) {
        if (other.estimate) enable_estimation(); else disable_estimation();
        w_bias = other.w_bias;
        a_bias = other.a_bias;
    }

    virtual bool unmap() {
        if (state_branch<state_node *>::unmap()) {
            a_bias.save_initial_variance();
            w_bias.save_initial_variance();
            return true;
        }
        return false;
    }
};

struct state_imu: public state_branch<state_node *> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    state_extrinsics extrinsics;
    state_imu_intrinsics intrinsics;
    state_imu() : extrinsics("Qa", "Ta", false) {
        children.push_back(&intrinsics);
        children.push_back(&extrinsics);
    }
    void copy_from(const state_imu &other) {
        extrinsics.copy_from(other.extrinsics);
        intrinsics.copy_from(other.intrinsics);
    }
};

class state_velocimeter_intrinsics: public state_branch<state_node *>
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
public:
    void copy_from(const state_velocimeter_intrinsics &other) {
        if (other.estimate) enable_estimation(); else disable_estimation();
    }
};

struct state_velocimeter: public state_branch<state_node *> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    state_extrinsics extrinsics;
    state_velocimeter_intrinsics intrinsics;
    state_velocimeter() : extrinsics("Qv", "Tv", false) {
        children.push_back(&intrinsics);
        children.push_back(&extrinsics);
    }
    void copy_from(const state_velocimeter &other) {
        extrinsics.copy_from(other.extrinsics);
        intrinsics.copy_from(other.intrinsics);
    }
};

class state_motion_orientation: public state_root {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

    state_quaternion  Q { "Q",  dynamic };
    state_vector<3>   w { "w",  dynamic };
    state_vector<3>  dw { "dw", dynamic };
    state_vector<3> ddw {"ddw", fake };

    state_branch<state_node *> non_orientation;
    state_branch<std::unique_ptr<state_imu>, std::vector<std::unique_ptr<state_imu>>> imus;
    state_branch<std::unique_ptr<state_velocimeter>, std::vector<std::unique_ptr<state_velocimeter>>> velocimeters;

    state_motion_orientation(covariance &c, matrix &FP): state_root(c, FP) {
        children.push_back(&ddw);
        children.push_back(&dw);
        children.push_back(&w);
        children.push_back(&Q);
        non_orientation.children.push_back(&imus);
        non_orientation.children.push_back(&velocimeters);
        children.push_back(&non_orientation);
    }

    void copy_from(const state_motion_orientation &other) {
        for (size_t i=imus.children.size(); i<other.imus.children.size(); i++)
            imus.children.emplace_back(std::make_unique<state_imu>());

        for (auto i = std::make_pair(imus.children.begin(),other.imus.children.begin()); i.first != imus.children.end() && i.second != other.imus.children.end(); ++i.first, ++i.second)
            (**i.first).copy_from(**i.second);

        Q = other.Q;
        w = other.w;
        dw = other.dw;
        ddw = other.ddw;

        orientation_initialized = other.orientation_initialized;

        state_root::copy_from(other);
    }

    virtual void reset()
    {
        orientation_initialized = false;
        state_root::reset();
    }

    bool orientation_initialized = false;

protected:
    virtual void evolve_state(f_t dt);
    virtual void cache_jacobians(f_t dt);
    m3 Rt;
    v3 dW;
    m3 JdW_s, dQp_s_dW;
};

class state_motion: public state_motion_orientation {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    friend class observation_accelerometer;

    state_vector<3>  T {  "T", dynamic };
    state_vector<3>  V {  "V", dynamic };
    state_vector<3>  a {  "a", dynamic };
    state_vector<3> da { "da", fake };

    f_t total_distance = 0;
    transformation loop_offset;

    v3 last_position = v3::Zero();

    void integrate_distance() {
        f_t dT = (T.v - last_position).norm();
        if(dT > .01f) {
            total_distance += dT;
            last_position = T.v;
        }
    }

    state_motion(covariance &c, matrix &FP): state_motion_orientation(c, FP) {
        non_orientation.children.push_back(&da);
        non_orientation.children.push_back(&a);
        non_orientation.children.push_back(&V);
        non_orientation.children.push_back(&T);
        remap();
        assert(fake_statesize == FAKESTATESIZE && statesize == BASEMOTIONSTATESIZE);
    }

    void copy_from(const state_motion &other) {
        if(other.non_orientation.estimate) disable_orientation_only(false);
        else                               enable_orientation_only(false);

        T = other.T;
        V = other.V;
        a = other.a;
        da = other.da;

        loop_offset = other.loop_offset;

        state_motion_orientation::copy_from(other);
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

protected:
    virtual void evolve_state(f_t dt);
    template<int N>
    int project_motion_covariance(matrix &dst, const matrix &src, f_t dt, int i) const;
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt) const;
    virtual void cache_jacobians(f_t dt);
    v3 dT;
};

#endif /* defined(__RC3DK__state_motion__) */