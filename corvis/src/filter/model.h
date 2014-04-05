// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __MODEL_H
#define __MODEL_H

extern "C" {
#include "cor_types.h"
#include "packet.h"
}
#include "../numerics/vec4.h"
#include "../numerics/matrix.h"
#include <vector>
#include <list>
#include "state.h"
#include "state_motion.h"

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
    v4 calibrated;
    feature_t prediction;
    //v4 innovation;
    v4 Tr; //for initialization
    rotation_vector Wr;
    static uint64_t counter;
    uint64_t id;
    uint64_t groupid;
    v4 world;
    v4 local;
    v4 relative;
    f_t depth;
    feature_t image_velocity;

    uint64_t found_time;

    uint8_t intensity;

    bool user;

    static f_t initial_depth_meters;
    static f_t initial_var;
    static f_t initial_process_noise;
    static f_t measurement_var;
    static f_t outlier_thresh;
    static f_t outlier_reject;
    static f_t max_variance;
    static f_t min_add_vis_cov;

    state_vision_feature() {};
    state_vision_feature(f_t initialx, f_t initialy);
    bool make_normal();
    bool should_drop() const;
    bool is_valid() const;
    bool is_good() const;
    void dropping_group();
    void drop();
    bool is_initialized() const { return status == feature_normal; }
    bool force_initialize();
//private:
    enum feature_flag status;
    
    void reset() {
        index = -1;
        v.set_depth_meters(1.);
    }
    
    f_t copy_cov_from_row(const matrix &cov, const int i) const
    {
        if(index < 0) return 0.;
        return cov(i, index);
    }
    
    void copy_cov_to_col(matrix &cov, const int j, const f_t v) const
    {
        cov(index, j) = v;
    }
    
    void copy_cov_to_row(matrix &cov, const int j, const f_t v) const
    {
        cov(j, index) = v;
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
    state_vision_group(const state_vector &T, const state_rotation_vector &W);
    void make_empty();
    int process_features();
    int make_reference();
    int make_normal();
    static f_t ref_noise;
    static f_t min_feats;
};

class state_vision: public state_motion {
 public:
    state_vector Tc;
    state_rotation_vector Wc;
    state_scalar focal_length;
    state_scalar center_x, center_y;
    state_scalar k1, k2, k3;

    state_branch<state_vision_group *> groups;
    list<state_vision_feature *> features;
    state_vision(bool estimate_calibration, covariance &c);
    ~state_vision();
    int process_features(uint64_t time);
    state_vision_feature *add_feature(f_t initialx, f_t initialy);
    state_vision_group *add_group(uint64_t time);

    bool estimate_calibration;
    float total_distance;
    v4 last_position;
    m4 camera_matrix;
    state_vision_group *reference;
    void fill_calibration(feature_t &initial, f_t &r2, f_t &r4, f_t &r6, f_t &kr) const;
    feature_t calibrate_feature(const feature_t &initial);
    
    void project_new_group_covariance(const state_vision_group &g);
    
    void enable_orientation_only();
    void disable_orientation_only();

};

typedef state_vision state;

#endif
