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

#include "transformation.h"
#include "vec4.h"
#include <spdlog/spdlog.h>
#include "spdlog/sinks/null_sink.h"
#include "fast_tracker.h"
#include "sensor.h"
#include "state.h"
#include "rc_tracker.h"
#include "orb_descriptor.h"
#include "rapidjson/document.h"
#include "DBoW2/TemplatedVocabulary.h"

typedef DBoW2::TemplatedVocabulary<orb_descriptor::raw, orb_descriptor::raw> orb_vocabulary;

class state_vision_intrinsics;
struct frame_t;
struct camera_frame_t;

struct map_edge {
    bool loop_closure = false;
    transformation G;
    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static void deserialize(const rapidjson::Value &json, map_edge &node);
};

enum class feature_type { tracked, triangulated };

struct map_feature {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    uint64_t id;
    // map_feature position is the position relative to a camera with
    // one of images axes oriented to match gravity (world z axis)
    v3 position;
    float variance;
    feature_type type;
    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static bool deserialize(const rapidjson::Value &json, map_feature &feature, uint64_t &max_loaded_featid);
};

enum class node_status {normal, finished};

struct map_node {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    uint64_t id;
    std::unordered_map<uint64_t, map_edge> edges; // key is neighbor_id
    map_edge &get_add_neighbor(uint64_t neighbor);
    void set_feature(const uint64_t id, const v3 &pos, const float variance, const feature_type type = feature_type::tracked);

    transformation global_transformation;

    // relocalization
    uint64_t camera_id;
    std::shared_ptr<frame_t> frame;
    std::map<uint64_t,map_feature> features;
    node_status status{node_status::normal};
    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static bool deserialize(const rapidjson::Value &json, map_node &node, uint64_t &max_loaded_featid);
};

class mapper {
 private:
    aligned_vector<map_node> nodes;
    friend struct map_node;
    bool unlinked{false};
    uint64_t node_id_offset{0};
    uint64_t feature_id_offset{0};

 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    mapper();
    ~mapper();
    void reset();

    typedef uint64_t nodeid;
    typedef std::pair<nodeid, nodeid> match;
    typedef std::vector<match> matches;
    typedef std::map<nodeid, transformation> nodes_path;

    bool is_unlinked(nodeid node_id) const { return (unlinked && node_id < node_id_offset); }
    bool initialized() const { return (current_node_id != std::numeric_limits<uint64_t>::max()); }
    void add_node(nodeid node_id, const rc_Sensor camera_id);
    void add_edge(nodeid node_id1, nodeid node_id2, const transformation &G12, bool loop_closure = false);
    void add_loop_closure_edge(nodeid node_id1, nodeid node_id2, const transformation &G12);
    void set_feature(nodeid node_id, uint64_t feature_id, const v3 & position_m, const float depth_variance_m2, const bool is_new = true);
    void get_triangulation_geometry(const tracker::feature_track& keypoint, aligned_vector<v2> &tracks_2d, std::vector<transformation> &camera_poses);
    void add_triangulated_feature_to_group(const nodeid group_id, const uint64_t feature_id, const v3& point_3d);
    nodes_path breadth_first_search(nodeid start, int maxdepth = 1);
    nodes_path breadth_first_search(nodeid start, std::set<nodeid>&& searched_nodes);

    const aligned_vector<map_node> & get_nodes() const { return nodes; }
    map_node& get_node(nodeid id) { return nodes[id]; }

    void node_finished(nodeid node_id);
    void set_node_transformation(nodeid id, const transformation & G);

    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static bool deserialize(const rapidjson::Value &json, mapper &map);

    std::unique_ptr<spdlog::logger> log = std::make_unique<spdlog::logger>("mapper",  std::make_shared<spdlog::sinks::null_sink_st> ());

    /// fetch the vocabulary file from resource and create orb vocabulary
    orb_vocabulary* orb_voc;

    std::map<unsigned int, std::vector<nodeid>> dbow_inverted_index; // given a word it stores the nodes in which it was observed

    // temporary store current frame in case we add a new node
    nodeid current_node_id = std::numeric_limits<uint64_t>::max();

    // for a feature id we associate the corresponding node in which it was detected
    std::map<uint64_t, nodeid> features_dbow;

    //we need the camera intrinsics
    std::vector<state_vision_intrinsics*> camera_intrinsics;
    std::vector<state_extrinsics*> camera_extrinsics;

    bool relocalize(const camera_frame_t& camera_frame, std::vector<transformation>& vG_W_currentframe);
    void estimate_pose(const aligned_vector<v3>& points_3d, const aligned_vector<v2>& points_2d, const rc_Sensor camera_id, transformation& G_candidateB_nowB, std::set<size_t>& inliers_set);
};

#endif
