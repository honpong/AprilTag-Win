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
#include "feature_descriptor.h"
#include "mapper.h"
#include "../cor/sensor_data.h"

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
    feature_single,
    feature_revived
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
    f_t outlier = 0;
    v4 initial;
    v4 current;
    f_t innovation_variance_x = 0, innovation_variance_y = 0, innovation_variance_xy = 0;
    uint64_t id;
    uint64_t groupid;
    v4 world = v4(0, 0, 0, 0);
    v4 Xcamera = v4(0, 0, 0, 0);
    feature_t image_velocity = {0,0};
    sensor_clock::duration dt = sensor_clock::duration(0);
    sensor_clock::duration last_dt = sensor_clock::duration(0);
    sensor_clock::time_point last_seen;

    sensor_clock::time_point found_time;

    descriptor descriptor;
    bool descriptor_valid{false};

    float last_variance;

    static f_t initial_depth_meters;
    static f_t initial_var;
    static f_t initial_process_noise;
    static f_t measurement_var;
    static f_t outlier_thresh;
    static f_t outlier_reject;
    static f_t max_variance;

    state_vision_feature(): state_leaf("feature") {};
    state_vision_feature(uint64_t feature_id, const feature_t & initial);
    bool should_drop() const;
    bool is_valid() const;
    bool is_good() const;
    void dropping_group();
    void drop();
    bool is_initialized() const { return status == feature_normal; }
    bool force_initialize();
//private:
    enum feature_flag status = feature_initializing;
    
    unsigned char patch[(tracker::half_patch_width * 2 + 1) * (tracker::half_patch_width * 2 + 1)];
    
    void reset() {
        index = -1;
        v.set_depth_meters(1.);
    }
    
    inline f_t from_row(const matrix &c, const int i) const
    {
        return index >= 0 ? c(i, index) : 0;
    }

    inline f_t &to_col(matrix &c, const int j) const
    {
        static f_t scratch;
        return index >= 0 ? c(index, j) : scratch;
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
        fprintf(stderr, "feature %" PRIu64 "d %f %f\n", id, v.v, variance());
    }

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class state_vision_group: public state_branch<state_node *> {
 public:
    state_vector Tr;
    state_quaternion Qr;

    state_branch<state_vision_feature *> features;
    list<uint64_t> neighbors;
    list<uint64_t> old_neighbors;
    int health;
    enum group_flag status;
    uint64_t id;

    state_vision_group(const state_vision_group &other);
    state_vision_group(uint64_t group_id);
    void make_empty();
    int process_features(const image_gray8 &image, mapper & map, bool map_enabled);
    int make_reference();
    int make_normal();
    static f_t ref_noise;
    static f_t min_feats;
    
    //cached data
    m4 dQrp_s_dW;
    m4 dTrp_ddT, dTrp_dQ_s;

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class state_vision: public state_motion {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_vector Tc;
    state_quaternion Qc;
    state_scalar focal_length;
    state_scalar center_x, center_y;
    state_scalar k1, k2, k3;
    int image_width, image_height;
    feature_t image_size() const { return feature_t {(f_t)image_width, (f_t)image_height}; }
    bool fisheye = false;
    uint64_t feature_counter;
    uint64_t group_counter;
    bool estimate_camera_intrinsics{false};
    bool estimate_camera_extrinsics{false};

    state_branch<state_vision_group *> groups;
    list<state_vision_feature *> features;
    
    state_vision(covariance &c);
    ~state_vision();
    int process_features(const image_gray8 &image, sensor_clock::time_point time);
    state_vision_feature *add_feature(const feature_t & initial);
    state_vision_group *add_group(sensor_clock::time_point time);

    state_vision_group *reference;

    uint64_t last_reference{0};
    v4 last_Tr;
    rotation_vector last_Wr;

    bool map_enabled{false};
    mapper map;
    transformation loop_offset;
    f_t lost_factor;
    bool loop_closed{false};
    
    feature_t undistort_feature(const feature_t &feat_d, f_t *ku_d_ = nullptr, f_t *dku_d_drd = nullptr, f_t *dku_d_dk1 = nullptr, f_t *dku_d_dk2 = nullptr, f_t *dku_d_dk3 = nullptr) const;
    feature_t distort_feature(const feature_t &feat_u, f_t *kd_u_ = nullptr, f_t *dkd_u_dru = nullptr, f_t *dkd_u_dk1 = nullptr, f_t *dkd_u_dk2 = nullptr, f_t *dkd_u_dk3 = nullptr) const;
    feature_t normalize_feature(const feature_t &feat) const;
    feature_t unnormalize_feature(const feature_t &feat) const;
    float median_depth_variance();
    
    virtual void reset();
    bool load_map(std::string json);

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
