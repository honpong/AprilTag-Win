// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __MODEL_H
#define __MODEL_H

#include "cor_types.h"
#include "packet.h"
#include "vec4.h"
#include "matrix.h"
#include <vector>
#include <list>
#include <future>
#include <cinttypes>
#include "state.h"
#include "state_motion.h"
#include "tracker.h"
#include "platform/sensor_clock.h"
#include "feature_descriptor.h"
#include "mapper.h"
#include "sensor_data.h"

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
    void set_depth_meters(f_t initial_depth) { v = (initial_depth > 0) ? log(initial_depth) : 0; }
    log_depth(): v(0) {}
};

class state_vision_intrinsics: public state_branch<state_node *>
{
public:
    state_scalar    focal_length { "focal_length", constant };
    state_vector<2> center       { "center", constant };
    state_vector<4> k            { "k", constant };
    rc_CalibrationType type {rc_CALIBRATION_TYPE_UNKNOWN};
    int image_width, image_height;

    state_vision_intrinsics(bool estimate_)
    {
        estimate = estimate_;
        children.push_back(&focal_length);
        children.push_back(&center);
        children.push_back(&k);
    }

    feature_t image_size() const { return feature_t {(f_t)image_width, (f_t)image_height}; }
    feature_t undistort_feature(const feature_t &feat_d) const;
    feature_t distort_feature(const feature_t &featu_u) const;
    f_t get_undistortion_factor(const feature_t &feat_d, m<1,2> *dku_d_dfeat_d = nullptr, m<1,4> *dku_d_dk = nullptr) const;
    f_t   get_distortion_factor(const feature_t &feat_u, m<1,2> *dkd_u_dfeat_u = nullptr, m<1,4> *dkd_u_dk = nullptr) const;
    feature_t normalize_feature(const feature_t &feat) const;
    feature_t unnormalize_feature(const feature_t &feat) const;
};

class state_vision_group;
struct state_camera;

class state_vision_feature: public state_leaf<log_depth, 1> {
 public:
    tracker::feature_track track;
    f_t outlier = 0;
    v2 initial;
    f_t innovation_variance_x = 0, innovation_variance_y = 0, innovation_variance_xy = 0;
    state_vision_group &group;
    v3 body = v3(0, 0, 0);
    v3 node_body = v3(0, 0, 0);

    struct descriptor descriptor;
    bool descriptor_valid{false};

    bool depth_measured{false};

    static f_t initial_depth_meters;
    static f_t initial_var;
    static f_t initial_process_noise;
    static f_t outlier_thresh;
    static f_t outlier_reject;
    static f_t max_variance;

    state_vision_feature(const tracker::feature_track &track, state_vision_group &group);
    bool should_drop() const;
    bool is_valid() const;
    bool is_good() const;
    void dropping_group();
    void drop();
    bool is_initialized() const { return status == feature_normal; }
    bool force_initialize();
//private:
    enum feature_flag status = feature_initializing;
    
    void reset() {
        set_initial_variance(initial_var);
        v.set_depth_meters(initial_depth_meters);
        set_process_noise(initial_process_noise);
    }
    
    inline f_t from_row(const matrix &c, const int i) const
    {
        if(index < 0) return 0;
        if(index >= c.cols())
        {
            if((type == node_type::fake) && (i == index)) return initial_covariance(0, 0);
            else return 0;
        }
        if((i < 0) || (i >= c.rows())) return 0;
        else return c(i, index);
    }

    inline f_t &to_col(matrix &c, const int j) const
    {
        static f_t scratch;
        if((index < 0) || (index >= c.rows())) return scratch;
        else return c(index, j);
    }
    
    void perturb_variance() {
        if(index < 0 || index >= cov->size()) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
    }
    
    f_t variance() const {
        if(index < 0 || index >= cov->size()) return initial_covariance(0, 0);
        return (*cov)(index, index);
    }
    
    void copy_state_to_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        state[index] = v.v;
    }
    
    virtual void copy_state_from_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        v.v = state[index];
    }
    
    virtual void print_matrix_with_state_labels(matrix &state, node_type nt) const {
        if(type != nt) return;
        fprintf(stderr, "feature[%" PRIu64 "]: ", track.feature->id); state.row(index+0).print();
    }

    virtual std::ostream &print_to(std::ostream & s) const
    {
        return s << "f" << track.feature->id << ": " << v.v << "±" << std::sqrt(variance());
    }

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class state_vision_group: public state_branch<state_node *> {
 public:
    state_vector<3>  Tr { "Tr", dynamic};
    state_quaternion Qr { "Qr", dynamic};

    state_camera &camera;
    state_branch<state_vision_feature *> features;
    std::list<uint64_t> neighbors;
    std::list<uint64_t> old_neighbors;
    int health;
    enum group_flag status;
    uint64_t id;

    state_vision_group(const state_vision_group &other);
    state_vision_group(state_camera &camera, uint64_t group_id);
    void make_empty();
    int process_features();
    int make_reference();
    int make_normal();
    static f_t ref_noise;
    static f_t min_feats;
    
    //cached data
    m3 dQrp_s_dW;
    m3 dTrp_ddT, dTrp_dQ_s;

    virtual std::ostream &print_to(std::ostream & s) const
    {
        s << "g" << id << ": "; return state_branch<state_node*>::print_to(s);
    }

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct state_camera: state_branch<state_node*> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    state_extrinsics extrinsics;
    state_vision_intrinsics intrinsics;
    std::unique_ptr<tracker> feature_tracker;

    std::future<const std::vector<tracker::feature_track> & > detection_future;

    state_branch<state_vision_group *> groups;
    void update_feature_tracks(const rc_ImageData &image);
    void clear_features_and_groups();
    int feature_count() const;
    int process_features(mapper *map, spdlog::logger &log);
    void remove_group(state_vision_group *g, mapper *map);

    state_vision_group *detecting_group = nullptr; // FIXME on reset
    int detecting_space = 0;

    state_camera() : extrinsics("Qc", "Tc", false), intrinsics(false) {
        reset();
        children.push_back(&extrinsics);
        children.push_back(&intrinsics);
        children.push_back(&groups);
    }
};

class state_vision: public state_motion {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_branch<std::unique_ptr<state_camera>, std::vector<std::unique_ptr<state_camera>>> cameras;

    uint64_t group_counter;

    state_vision(covariance &c);
    ~state_vision();
    int process_features(state_camera &camera, const rc_ImageData &image, mapper *map);
    int feature_count() const;
    void clear_features_and_groups();
    state_vision_feature *add_feature(const tracker::feature_track &track_, state_vision_group &group);
    state_vision_group *add_group(state_camera &camera, mapper *map);
    transformation get_transformation() const;

    void update_map(const rc_ImageData &image, mapper *map);

    float median_depth_variance();
    
    virtual void enable_orientation_only(bool remap = true);
    virtual void reset();

protected:
    virtual void evolve_state(f_t dt);
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt);
    virtual void cache_jacobians(f_t dt);
};

typedef state_vision state;

#endif
