// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __MAPPER_H
#define __MAPPER_H

#include <list>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <stdbool.h>
#include <utility>

#include "transformation.h"
#include "vec4.h"
#include <spdlog/spdlog.h>
#include "spdlog/sinks/null_sink.h"
#include "fast_tracker.h"
#include "sensor.h"
#include "state.h"
#include "rc_tracker.h"
#include "orb_descriptor.h"
#include "DBoW2/TemplatedVocabulary.h"
#include "descriptor.h"
#include "concurrent.h"
#ifdef RELOCALIZATION_DEBUG
#include <opencv2/core/mat.hpp>
#include "debug/visual_debug.h"
#endif

typedef DBoW2::TemplatedVocabulary<orb_descriptor::raw, DBoW2::L1_NORM> orb_vocabulary;
typedef uint64_t nodeid;
typedef uint64_t featureid;
typedef size_t featureidx;

class state_vision_intrinsics;
class log_depth;

struct frame_t {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    sensor_clock::time_point timestamp;
    std::vector<std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>>> keypoints;
    std::vector<v2> keypoints_xy;
    DBoW2::BowVector dbow_histogram;       // histogram describing image

    void reserve(size_t sz) {
        keypoints.reserve(sz);
        keypoints_xy.reserve(sz);
    }

    void add_track(const tracker::feature_position &t, int width_px, int height_px) {
        if (fast_tracker::is_trackable<orb_descriptor::border_size>((int)t.x, (int)t.y, width_px, height_px)) {
            keypoints.emplace_back(std::make_shared<fast_tracker::fast_feature<patch_orb_descriptor>>(t.feature->id, patch_descriptor{}));
            keypoints_xy.emplace_back(t.x, t.y);
        }
    }

    inline void calculate_dbow(const orb_vocabulary *orb_voc) {
        // copy pyramid descriptors to a vector of descriptors
        constexpr int levelsup = std::numeric_limits<int>::max();  // consider all the tree
        auto get_descriptor = [](const decltype(keypoints)::value_type &kp) -> const orb_descriptor::raw & {
                return kp->descriptor.orb.descriptor;
        };
        dbow_histogram = orb_voc->transform(keypoints.begin(), keypoints.end(), get_descriptor, levelsup);
    }
#ifdef RELOCALIZATION_DEBUG
    cv::Mat image; // for debugging purposes
#endif
};

struct camera_frame_t {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    rc_Sensor camera_id;
    std::shared_ptr<frame_t> frame;
    nodeid closest_node;
    transformation G_closestnode_frame;
};

enum class edge_type { new_edge, dead_reckoning, filter, composition, map, relocalization};

struct map_edge {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    edge_type type = edge_type::new_edge;
    transformation G;
    map_edge() = default;
    map_edge(edge_type type_, const transformation &G_) : type(type_), G(G_) {};
};

enum class feature_type { tracked, triangulated };
enum class relocalization_status {begining, find_candidates, match_descriptors, estimate_EPnP};

struct map_feature {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    map_feature() = default;
    map_feature(std::shared_ptr<log_depth> v_, std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feat_) : v(std::move(v_)), feature(std::move(feat_)) {}
    map_feature(std::shared_ptr<log_depth> v_, std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feat_, f_t v_var_, feature_type type_) : v(std::move(v_)), v_var(v_var_), type(type_), feature(std::move(feat_)) {}
    std::shared_ptr<log_depth> v;
    f_t v_var; // feature variance
    feature_type type{ feature_type::tracked };
    std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature;
};

enum class node_status {reference, normal, finished};

struct map_node {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    nodeid id;
    aligned_map<nodeid, map_edge> edges; // key is neighbor_id; we use a map structure to gurantee same order when traversing edges.
    aligned_map<nodeid, map_edge> relocalization_edges;
    std::set<nodeid> covisibility_edges;
    void get_add_neighbor(nodeid neighbor, const transformation& G, const edge_type type);
    void add_feature(std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature, std::shared_ptr<log_depth> v, f_t v_var, const feature_type type);
    void set_feature_type(const featureid id, f_t v_var, const feature_type type);

    transformation global_transformation; // dead reckoning pose

    // relocalization
    uint64_t camera_id;
    std::shared_ptr<frame_t> frame;
    aligned_map<featureid,map_feature> features;
    node_status status{node_status::normal};
    uint64_t frames_active = 0;
};

struct map_relocalization_info {
    bool is_relocalized = false;
    sensor_clock::time_point frame_timestamp;
    relocalization_status rstatus{relocalization_status::begining};
    struct candidate {
        nodeid node_id;
        transformation G_node_frame;
        transformation G_world_node;
        sensor_clock::time_point node_timestamp;
        candidate() {}
        candidate(nodeid id, const transformation &g_node_frame, const transformation &g_world_node, sensor_clock::time_point node_ts)
            : node_id(id), G_node_frame(g_node_frame), G_world_node(g_world_node), node_timestamp(node_ts) {}
    };
    aligned_vector<candidate> candidates;
    size_t size() const { return candidates.size(); }
};

struct map_relocalization_edge : public map_edge {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    nodeid id1;
    nodeid id2;
    map_relocalization_edge(nodeid _id1, nodeid _id2, const transformation& _G12, edge_type _type) :
        map_edge(_type, _G12), id1(_id1), id2(_id2) {}
};

struct map_relocalization_result {
    map_relocalization_info info;
    aligned_vector<map_relocalization_edge> edges;
};

class mapper {
 public:
    typedef std::pair<featureidx, featureidx> match;
    typedef std::vector<match> matches;
    struct node_path {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        node_path(){}
        node_path(nodeid id_, const transformation &G_, f_t distance_) : id(id_), G(G_), distance(distance_) {}
        nodeid id;
        transformation G;
        f_t distance; // metric used in dijkstra to find shortest path
    };
    typedef aligned_vector<node_path> nodes_path;

    struct stage {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        nodeid closest_id = std::numeric_limits<nodeid>::max();
        transformation Gr_closest_stage;
        nodeid current_id = std::numeric_limits<nodeid>::max();
        transformation G_current_closest;
        stage() {}
        stage(nodeid closest_id_, const transformation &Gr_closest_stage_)
            : closest_id(closest_id_), Gr_closest_stage(Gr_closest_stage_) {}
        struct output {
            const char *name = nullptr;
            transformation G_world_stage;
            output() {}
            output(const char *name_, const transformation &G_world_stage_)
                : name(name_), G_world_stage(G_world_stage_) {}
        };
    };

 protected:
    // Note about concurrency:
    // The concurrent functions are:
    // relocalize: (read-access) in background thread,
    // all other functions: (read/write) in main thread.
    //
    // So, functions that write on concurrent member objects
    // must use thread-safe access.
    // The relocalize function (and the functions it invokes)
    // must use thread-safe access.
    // Other functions that only read concurrent member objects
    // do not need to lock mutexes.

    concurrent<aligned_unordered_map<nodeid, map_node>> nodes;
    friend struct map_node;
    template<template <class map_feature_v, class...> class map_node_v, class map_feature_v, class... TArgs>
    friend class mapper_t;
    bool unlinked{false};
    nodeid node_id_offset{0};
    featureid feature_id_offset{0};

    // for a feature id we associate the corresponding node in which it was detected
    concurrent<std::map<featureid, nodeid>> features_dbow;

public:
    concurrent<aligned_map<std::string,stage>> stages;

    void set_stage(std::string &&name, nodeid closest_id, const transformation &Gr_closest_stage) {
        stages.critical_section([&]() {
            (*stages)[std::move(name)] = stage{closest_id, Gr_closest_stage};
        });
    }
    bool get_stage(const std::string &name, stage &stage, nodeid current_id, const transformation &G_world_current, stage::output &current_stage);
    bool get_stage(bool next, const char *name, nodeid current_id, const transformation &G_world_current, stage::output &current_stage) {
        return stages.critical_section([&]() {
            auto it = name ? stages->find(name) : stages->begin();
            if (it == stages->end() || (name && next && ++it == stages->end()))
                return false;
            bool ok;
            do ok = get_stage(it->first, it->second, current_id, G_world_current, current_stage);
            while (next && !ok && ++it != stages->end());
            return ok;
        });
    }
    void reset_stages() {
        stages.critical_section([&]() {
           stages->clear();
        });
    }
private:
    // private functions that lock mutexes internally
    std::vector<std::pair<nodeid,float>> find_loop_closing_candidates(
        const std::shared_ptr<frame_t>& current_frame) const;

    mapper::matches match_2d_descriptors(const std::shared_ptr<frame_t>& candidate_frame, state_vision_intrinsics* const candidate_intrinsics,
                                         const std::shared_ptr<frame_t>& current_frame, state_vision_intrinsics* const current_intrinsics) const;

    // private functions that are used after acquiring some of the mutexes
    void remove_edge_no_lock(nodeid node_id1, nodeid node_id2);
    void add_edge_no_lock(nodeid node_id1, nodeid node_id2, const transformation &G12, edge_type type);
    void add_covisibility_edge_no_lock(nodeid node_id1, nodeid node_id2);

 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    mapper();
    ~mapper();
    void reset();
    void clean_map_after_filter_reset();
    void close_map(nodeid id);

    bool is_map_unlinked() const { return unlinked; }
    bool is_unlinked(nodeid node_id) const { return (unlinked && node_id < node_id_offset); }
    bool link_map(const map_relocalization_edge& edge);
    void add_node(nodeid node_id, const rc_Sensor camera_id);
    void add_edge(nodeid node_id1, nodeid node_id2, const transformation &G12, edge_type type);
    void add_relocalization_edges(const aligned_vector<map_relocalization_edge>& edges);
    void add_covisibility_edge(nodeid node_id1, nodeid node_id2);
    void remove_edge(nodeid node_id1, nodeid node_id2);
    void add_feature(nodeid node_id, std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature,
                     std::shared_ptr<log_depth> v, f_t v_var, const feature_type type = feature_type::tracked);
    bool move_feature(featureid feature_id, nodeid src_node_id, nodeid dst_node_id, const transformation* G_Bdst_Bsrc_ptr = nullptr);
    void remove_feature(nodeid node_id, featureid feature_id);
    void remove_feature(featureid feature_id);
    void remove_node(nodeid node_id);
    void set_feature_type(nodeid node_id, featureid feature_id, f_t v_var, const feature_type type = feature_type::tracked);
    v3 get_feature3D(nodeid node_id, featureid feature_id) const; // returns feature wrt node body frame
    mapper::nodes_path dijkstra_shortest_path(const node_path &start, std::function<float(const map_edge& edge)> distance, std::function<bool(const node_path &)> is_node_searched,
                                              std::function<bool(const node_path &)> finish_search) const;
    bool find_relative_pose(nodeid source, nodeid target, transformation &G) const;
    nodes_path find_neighbor_nodes(const node_path& start, const uint64_t camera_id_now);

    const aligned_unordered_map<nodeid, map_node> &get_nodes() const { return *nodes; }
    map_node& get_node(nodeid id) { return nodes->at(id); }
    const map_node* fetch_node(nodeid id) const { auto it = nodes->find(id); return it != nodes->end() ? &it->second : nullptr; }
    bool node_in_map(nodeid id) const { return nodes->find(id) != nodes->end(); }
    bool feature_in_map(featureid id, nodeid* nid = nullptr) const;
    bool is_root(nodeid node_id) const { return node_id == node_id_offset; }
    nodeid get_node_id_offset() const { return node_id_offset; }
    void set_node_id_offset(nodeid offset) { node_id_offset = offset; }
    featureid get_feature_id_offset() { return feature_id_offset; }
    bool edge_in_map(nodeid id1, nodeid id2, edge_type& type) const;
    rc_SessionId get_node_session(nodeid id) const {
        return (id < node_id_offset ? rc_SESSION_PREVIOUS_SESSION : rc_SESSION_CURRENT_SESSION);
    }

    void finish_node(nodeid node_id);
    void set_node_transformation(nodeid id, const transformation & G);
    void set_node_frame(nodeid id, std::shared_ptr<frame_t> frame);

    bool serialize(rc_SaveCallback func, void *handle) const;
    static bool deserialize(rc_LoadCallback func, void *handle, mapper &map);

    std::unique_ptr<orb_vocabulary> create_vocabulary_from_map(int branching_factor, int depth_levels) const;

    std::unique_ptr<spdlog::logger> log = std::make_unique<spdlog::logger>("mapper",  std::make_shared<spdlog::sinks::null_sink_st> ());

    /// fetch the vocabulary file from resource and create orb vocabulary
    std::unique_ptr<orb_vocabulary> orb_voc;

    // temporary pointer to reference node
    map_node* reference_node = nullptr; // points to node corresponding to latest active reference group in the filter

    //we need the camera intrinsics
    std::vector<state_vision_intrinsics*> camera_intrinsics;
    std::vector<state_extrinsics*> camera_extrinsics;

    map_relocalization_result relocalize(const camera_frame_t& camera_frame) const;
    bool estimate_pose(const aligned_vector<v3>& points_3d, const aligned_vector<v2>& points_2d, const rc_Sensor camera_id, transformation& G_candidateB_nowB, std::set<size_t>& inliers_set) const;

    // reuse map features in filter
    struct map_feature_track {
        tracker::feature_track track;
        std::shared_ptr<log_depth> v;
        f_t v_var; // feature variance
        map_feature_track(tracker::feature_track &&track_, std::shared_ptr<log_depth> v_, f_t v_var_)
            : track(std::move(track_)), v(std::move(v_)), v_var(v_var_) {}
        map_feature_track(map_feature_track &&) = default;
        map_feature_track & operator=(map_feature_track &&) = default;
        map_feature_track(const map_feature_track &) = delete;
        map_feature_track & operator=(const map_feature_track &) = delete;
    };

    struct node_feature_track {
        nodeid group_id;
        transformation G_neighbor_now;
        std::vector<map_feature_track> tracks;
        size_t found = 0;
        node_feature_track(nodeid id_, const transformation &G, std::vector<map_feature_track> &&tracks_)
            : group_id(id_), G_neighbor_now(G), tracks(std::move(tracks_)) {}
        node_feature_track(node_feature_track &&) = default;
        node_feature_track & operator=(node_feature_track &&) = default;
        node_feature_track(const node_feature_track &) = delete;
        node_feature_track & operator=(const node_feature_track &) = delete;
    };
    std::vector<node_feature_track> map_feature_tracks;
    void predict_map_features(const uint64_t camera_id_now, const nodes_path &neighbors, const size_t min_gorup_map_add);

    stdev<1> map_prune_stats;
};

#endif
