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

struct map_edge {
    bool loop_closure = false;
    transformation G;
    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static void deserialize(const rapidjson::Value &json, map_edge &node);
};

struct map_feature {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    uint64_t id;
    // map_feature position is the position relative to a camera with
    // one of images axes oriented to match gravity (world z axis)
    v3 position;
    float variance;
    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static bool deserialize(const rapidjson::Value &json, map_feature &feature, uint64_t &max_loaded_featid);
};

struct map_frame {
    rc_Sensor camera_id; // to know which camera intrinsics
    std::vector<std::shared_ptr<fast_tracker::fast_feature<orb_descriptor>>> keypoints;
    DBoW2::BowVector dbow_histogram;       // histogram describing image
    DBoW2::FeatureVector dbow_direct_file;  // direct file (at level 4)
    void dbow_init(const orb_vocabulary *orb_voc);
    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static bool deserialize(const rapidjson::Value &json, map_frame &frame);
};

enum class node_status {normal, finished};

struct map_node {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    uint64_t id;
    std::unordered_map<uint64_t, map_edge> edges; // key is neighbor_id
    map_edge &get_add_neighbor(uint64_t neighbor);
    void set_feature(const uint64_t id, const v3 &pos, const float variance);

    transformation global_transformation;

    // relocalization
    map_frame frame;
    std::map<uint64_t,map_feature> features;
    node_status status{node_status::normal};
    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static bool deserialize(const rapidjson::Value &json, map_node &node, uint64_t &max_loaded_featid);

    //temporary variables used in breath first
    int parent{-1};
    int depth{0};
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
    typedef std::pair<nodeid, transformation> node_path;

    bool is_unlinked(nodeid node_id) const { return (unlinked && node_id < node_id_offset); }
    void process_keypoints(const std::vector<tracker::feature_track*> &keypoints, const rc_Sensor camera_id, const tracker::image &image);
    void add_node(nodeid node_id);
    void add_edge(nodeid node_id1, nodeid node_id2, const transformation &G12, bool loop_closure = false);
    void add_loop_closure_edge(nodeid node_id1, nodeid node_id2, const transformation &G12);
    void set_feature(nodeid node_id, uint64_t feature_id, const v3 & position_m, const float depth_variance_m2, const bool is_new = true);
    std::vector<node_path> breadth_first_search(nodeid start, int maxdepth = 1);

    const aligned_vector<map_node> & get_nodes() const { return nodes; };

    void node_finished(nodeid node_id);
    void set_node_transformation(nodeid id, const transformation & G);

    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static bool deserialize(const rapidjson::Value &json, mapper &map);

    std::unique_ptr<spdlog::logger> log = std::make_unique<spdlog::logger>("mapper",  std::make_shared<spdlog::sinks::null_sink_st> ());

    /// fetch the vocabulary file from resource and create orb vocabulary
    orb_vocabulary* orb_voc;

    std::map<unsigned int, std::vector<nodeid>> dbow_inverted_index; // given a word it stores the nodes in which it was observed

    // temporary store current frame in case we add a new node
    map_frame current_frame;
    nodeid current_node_id = 0;

    // for a feature id we associate the corresponding node in which it was detected
    std::map<uint64_t, nodeid> features_dbow;

    //we need the camera intrinsics
    std::vector<state_vision_intrinsics*> camera_intrinsics;
    std::vector<state_extrinsics*> camera_extrinsics;

    bool relocalize(std::vector<transformation>& vG_WC, const transformation& G_WBk);
};

#endif
