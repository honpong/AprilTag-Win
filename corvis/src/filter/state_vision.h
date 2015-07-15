// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __MODEL_H
#define __MODEL_H

extern "C" {
#include "../cor/cor_types.h"
#include "../cor/packet.h"
}
#include "../numerics/vec4.h"
#include "../numerics/matrix.h"
#include <vector>
#include <list>
#include "state.h"
#include "state_motion.h"
#include "tracker.h"
#include "../cor/platform/sensor_clock.h"

#define estimate_camera_intrinsics 0
#define estimate_camera_extrinsics 0

using namespace std;

enum group_flag {
    group_empty = 0,
    group_initializing,
    group_normal,
    group_reference
};

enum feature_flag {
    feature_empty = 0,
    feature_gooddrop,
    feature_normal,
    feature_ready,
    feature_initializing,
    feature_reject,
    feature_single
};

class log_depth
{
protected:
    friend class state_vision_feature;
    f_t v;
public:
    f_t depth() { return exp(v); }
    f_t invdepth() { return exp(-v); }
    f_t invdepth_jacobian() { return -exp(-v); }
    f_t stdev_meters(f_t stdev) { return exp(v + stdev) - exp(v); }
    void set_depth_meters(f_t initial_depth) { v = (initial_depth > 0.) ? log(initial_depth) : 0.; }
    log_depth(): v(0.) {}
};

class state_vision_feature: public state_leaf<log_depth, 1> {
 public:
    f_t outlier;
    v4 initial;
    v4 current;
    feature_t prediction;
    f_t innovation_variance_x, innovation_variance_y, innovation_variance_xy;
    static uint64_t counter;
    uint64_t id;
    uint64_t groupid;
    v4 world;
    f_t depth;
    feature_t image_velocity;
    sensor_clock::duration dt;
    sensor_clock::duration last_dt;
    sensor_clock::time_point last_seen;

    sensor_clock::time_point found_time;

    bool user;

    static f_t initial_depth_meters;
    static f_t initial_var;
    static f_t initial_process_noise;
    static f_t measurement_var;
    static f_t outlier_thresh;
    static f_t outlier_reject;
    static f_t max_variance;

    state_vision_feature(): state_leaf("feature") {};
    state_vision_feature(f_t initialx, f_t initialy);
    bool should_drop() const;
    bool is_valid() const;
    bool is_good() const;
    void dropping_group();
    void drop();
    bool is_initialized() const { return status == feature_normal; }
    bool force_initialize();
//private:
    enum feature_flag status;
    
    unsigned char patch[(tracker::half_patch_width * 2 + 1) * (tracker::half_patch_width * 2 + 1)];
    
    void reset() {
        index = -1;
        v.set_depth_meters(1.);
    }
    
    f_t copy_cov_from_row(const matrix &c, const int i) const
    {
        if(index < 0) return 0.;
        return c(i, index);
    }
    
    void copy_cov_to_col(matrix &c, const int j, const f_t val) const
    {
        c(index, j) = val;
    }
    
    void copy_cov_to_row(matrix &c, const int j, const f_t val) const
    {
        c(j, index) = val;
    }
    
    void perturb_variance() {
        cov->cov(index, index) *= PERTURB_FACTOR;
    }
    
    f_t variance() const {
        if(index < 0) return initial_variance[0];
        return (*cov)(index, index);
    }
    
    void copy_state_to_array(matrix &state) {
        state[index] = v.v;
    }
    
    virtual void copy_state_from_array(matrix &state) {
        v.v = state[index];
    }
    
    virtual void print() {
        fprintf(stderr, "feature %lld %f %f\n", id, v.v, variance());
    }

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class state_vision_group: public state_branch<state_node *> {
 public:
    state_vector Tr;
    state_rotation_vector Wr;

    state_branch<state_vision_feature *> features;
    list<uint64_t> neighbors;
    list<uint64_t> old_neighbors;
    int health;
    enum group_flag status;
    static uint64_t counter;
    uint64_t id;

    state_vision_group(const state_vision_group &other);
    state_vision_group();
    void make_empty();
    int process_features();
    int make_reference();
    int make_normal();
    static f_t ref_noise;
    static f_t min_feats;
    
    //cached data
    m4 Rr;
    m4 dWrp_dWr, dWrp_ddW;
    m4 dTrp_ddT, dTrp_dWr, dTrp_dW;

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class state_vision: public state_motion {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_vector Tc;
    state_rotation_vector Wc;
    state_scalar focal_length;
    state_scalar center_x, center_y;
    state_scalar k1, k2, k3;
    int image_width, image_height;
    bool fisheye = false;

    state_branch<state_vision_group *> groups;
    list<state_vision_feature *> features;
    
    state_vision(covariance &c);
    ~state_vision();
    int process_features(sensor_clock::time_point time);
    state_vision_feature *add_feature(f_t initialx, f_t initialy);
    state_vision_group *add_group(sensor_clock::time_point time);

    float total_distance;
    v4 last_position;
    m4 camera_matrix;
    state_vision_group *reference;
    
    void fill_calibration(const feature_t &initial, f_t &r2, f_t &kr) const {
        r2 = initial.x * initial.x + initial.y * initial.y;

        if(fisheye) {
            f_t r = sqrt(r2);
            f_t ru = tan(r * k1.v) / (2. * tan(k1.v / 2.));
            kr = ru / r;
        }
        else kr = 1. + r2 * (k1.v + r2 * (k2.v + r2 * k3.v));
    }
    feature_t calibrate_feature(const feature_t &initial) const;
    feature_t uncalibrate_feature(const feature_t &normalized) const;
    feature_t project_feature(const feature_t &feat) const;
    feature_t unproject_feature(const feature_t &feat) const;
    
    virtual void reset();
    void reset_position();
    
protected:
    virtual void add_non_orientation_states();
    virtual void remove_non_orientation_states();
    virtual void evolve_state(f_t dt);
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt);
    virtual void cache_jacobians(f_t dt);
private:
    void project_new_group_covariance(const state_vision_group &vision_group);
    void clear_features_and_groups();
};

typedef state_vision state;

#endif
