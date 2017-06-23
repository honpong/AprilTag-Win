// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __MAPPER_H
#define __MAPPER_H

#include <list>
#include <vector>
#include <set>
#include <stdbool.h>

#include "transformation.h"
#include "vec4.h"
#include <spdlog/spdlog.h>
#include "spdlog/sinks/null_sink.h"
#include "tracker.h"
#include "sensor.h"
#include "rc_tracker.h"
#include "orb_descriptor.h"
#include "DBoW2/TemplatedVocabulary.h"

typedef DBoW2::TemplatedVocabulary<orb_descriptor::raw, orb_descriptor::raw> orb_vocabulary;

class state_vision_intrinsics;

class transformation_variance {
    public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    transformation transform;
    m4 variance;
};

struct map_edge {
    uint64_t neighbor;
    int64_t geometry; //positive/negative indicate geometric edge direction, 0 indicates a covisibility edge
    bool loop_closure;
};

struct map_feature {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    uint64_t id;
    // map_feature position is the position relative to a camera with
    // one of images axes oriented to match gravity (world z axis)
    v3 position;
    float variance;
    map_feature(const uint64_t id, const v3 &p, const float v): id(id), position(p), variance(v){}
};

struct map_frame {
    rc_Sensor camera_id; // to know which camera intrinsics
    std::vector<std::shared_ptr<tracker::feature>> keypoints; // Pyramid 2D features
    DBoW2::BowVector dbow_histogram;       // histogram describing image
    DBoW2::FeatureVector dbow_direct_file;  // direct file (at level 4)
};

enum class node_status { initializing, normal, finished };

struct map_node {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    uint64_t id;
    bool match_attempted{false};
    bool finished{false};
    std::list<map_edge> edges;
    map_edge &get_add_neighbor(uint64_t neighbor);
    int terms;
    std::list<map_feature *> features; //sorted by label
    bool add_feature(const uint64_t id, const v3 &p, const float v);

    transformation_variance global_transformation;

    // temporary variables used in breadth first
    int depth;
    int parent;
    transformation_variance transform;

    // relocalization
    map_frame frame;
    std::set<uint64_t> neighbors;
    std::map<uint64_t,map_feature*> features_reloc; // essentially we need a map instead of a list
    node_status status{node_status::initializing};

map_node(): terms(0), depth(0), parent(-1) {}
};


struct local_feature {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    v3 position;
    map_feature *feature;
};


class mapper {
 private:
    aligned_vector<map_node> nodes;
    friend struct map_node;
    aligned_vector<transformation_variance> geometry;
    transformation relative_transformation;
    uint64_t feature_count;

    void internal_set_geometry(uint64_t id1, uint64_t id2, const transformation_variance &transform, bool loop_closed);
    void set_special(uint64_t id, bool special);
    void set_geometry(uint64_t id1, uint64_t id2, const transformation_variance &transform);

    bool unlinked;
    uint64_t node_id_offset{0};
    uint64_t feature_id_offset{0};

 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    mapper();
    ~mapper();
    void reset();

    bool is_unlinked(uint64_t node_id) const { return (unlinked && node_id < node_id_offset); }
    void process_current_image(tracker *feature_tracker, const rc_Sensor camera_id, const tracker::image &image);
    void add_node(uint64_t node_id);
    void add_edge(uint64_t node_id1, uint64_t node_id2);
    void add_feature(uint64_t node_id, uint64_t feature_id, uint64_t feature_track_id, const v3 & position_m, float depth_variance_m2);

    const aligned_vector<map_node> & get_nodes() const { return nodes; };

    void update_feature_position(uint64_t node_id, uint64_t feature_id, const v3 &position_m, float depth_variance_m2);
    void node_finished(uint64_t node_id);
    void set_node_transformation(uint64_t id, const transformation & G);

    // Reading / writing
    bool serialize(std::string &json);
    static bool deserialize(const std::string &json, mapper & map);

    // Debugging
    void dump_map(FILE *file);
    void print_stats();

    std::unique_ptr<spdlog::logger> log = std::make_unique<spdlog::logger>("mapper",  std::make_shared<spdlog::sinks::null_sink_st> ());

    bool enabled = false;

    /// fetch the vocabulary file from resource and create orb vocabulary
    size_t voc_size = 0;
    const char* voc_file = nullptr;
    orb_vocabulary* orb_voc;

    typedef uint64_t nodeid;
    typedef std::pair<nodeid, nodeid> match;
    typedef std::vector<match> matches;

    std::vector<std::vector<nodeid>> dbow_inverted_index; // given a word it stores the nodes in which it was observed

    // temporary store current frame in case we add a new node
    map_frame current_frame;

    // for a tracker_id we associate the corresponding node in which it was detected and its feature_id
    std::map<uint64_t, std::pair<nodeid, uint64_t>> features_dbow;

    //we need the camera intrinsics
    std::vector<state_vision_intrinsics*> camera_intrinsics;
    std::vector<rc_CameraIntrinsics*> sensor_intrinsics;

    std::vector<std::pair<nodeid,float>> find_loop_closing_candidates();
    matches match_2d_descriptors(const nodeid &candidate_id);

    bool relocalize(std::vector<transformation>& vG_WC, const transformation& G_BC);
    void estimate_pose(const aligned_vector<v3>& points_3d, const aligned_vector<v2>& points_2d, transformation& G_WC, std::set<size_t>& inliers_set);

private:
    size_t calculate_orientation_bin(const orb_descriptor &a, const orb_descriptor &b, const size_t num_orientation_bins);
};


#endif
