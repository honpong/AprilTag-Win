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

using namespace std;

class state_position: public state_root {
 public:
    state_vector T;
    state_vector W;
    state_position() { children.push_back(&T); children.push_back(&W); }
};

//this needs to be adjusted if this model changes or is reordered - these are the N first states that are updated by integration
#define MOTION_STATES 15

class state_motion: public state_position {
 public:
    state_vector w;
    state_vector V;
    state_vector a;
    state_vector dw;
    state_vector da;
    state_vector a_bias;
    state_vector w_bias;
    state_motion() { children.push_back(&w); children.push_back(&V); children.push_back(&a); children.push_back(&dw); children.push_back(&da); children.push_back(&a_bias); children.push_back(&w_bias); }
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
    v4 Tr, Wr; //for initialization
    static uint64_t counter;
    uint64_t id;
    uint64_t groupid;
    uint16_t local_id;
    v4 world;
    v4 local;
    v4 relative;
    f_t depth;

    uint64_t found_time;

    uint8_t intensity;

    enum feature_flag status;
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
    void make_reject();
};

class state_vision_group: public state_branch<state_node *> {
 public:
    state_vector Tr;
    state_vector Wr;
    state_branch<state_vision_feature *> features;
    list<uint64_t> neighbors;
    list<uint64_t> old_neighbors;
    int health;
    enum group_flag status;
    static uint64_t counter;
    uint64_t id;

    state_vision_group(const state_vision_group &other);
    state_vision_group(v4 Tr_i, v4 Wr_i);
    void make_empty();
    int process_features();
    int make_reference();
    int make_normal();
    static f_t ref_noise;
    static f_t min_feats;
    static f_t min_health;
};

class state_vision: public state_motion_gravity {
 public:
    state_vector Tc;
    state_vector Wc;
    state_scalar focal_length;
    state_scalar center_x, center_y;
    state_scalar k1, k2, k3;

    state_branch<state_vision_group *> groups;
    list<state_vision_feature *> features;
    state_vision(bool estimate_calibration);
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
    v4 last_Tr, last_Wr;
    mapbuffer *mapperbuf;
    v4 initial_orientation;
    feature_t projected_orientation_marker;
    v4 virtual_tape_start;
    float median_depth;
    void get_relative_transformation(const v4 &T, const v4 &W, v4 &rel_T, v4 &rel_W);
    void set_geometry(state_vision_group *g, uint64_t time);
    void fill_calibration(feature_t &initial, f_t &r2, f_t &r4, f_t &r6, f_t &kr);
    feature_t calibrate_feature(const feature_t &initial);
};

typedef state_vision state;

#endif
