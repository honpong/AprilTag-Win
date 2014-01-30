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

class state_vision_feature: public state_scalar {
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
    uint16_t local_id;
    v4 world;
    v4 local;
    v4 relative;
    f_t depth;
    feature_t image_velocity;

    uint64_t found_time;

    uint8_t intensity;

    bool user;

    static f_t initial_rho;
    static f_t initial_var;
    static f_t initial_process_noise;
    static f_t measurement_var;
    static f_t outlier_thresh;
    static f_t outlier_reject;
    static f_t max_variance;
    static f_t min_add_vis_cov;

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
    state_vision_group(v4 Tr_i, rotation_vector Wr_i);
    void make_empty();
    int process_features();
    int make_reference();
    int make_normal();
    static f_t ref_noise;
    static f_t min_feats;
    static f_t min_health;
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
    float orientation;
    v4 camera_orientation;
    float total_distance;
    v4 last_position;
    m4 camera_matrix;
    state_vision_group *reference;
    uint64_t last_reference;
    v4 last_Tr;
    rotation_vector last_Wr;
    v4 initial_orientation;
    feature_t projected_orientation_marker;
    v4 virtual_tape_start;
    float median_depth;
    void fill_calibration(feature_t &initial, f_t &r2, f_t &r4, f_t &r6, f_t &kr) const;
    feature_t calibrate_feature(const feature_t &initial);
    
    void project_new_group_covariance(matrix &dst, const matrix &src, const state_vision_group &g);
    void propagate_new_group(const state_vision_group &g);

};

typedef state_vision state;

#endif
