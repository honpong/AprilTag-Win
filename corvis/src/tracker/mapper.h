// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __MAPPER_H
#define __MAPPER_H

#include <list>
#include <vector>
#include <stdbool.h>

#include "transformation.h"
#include "vec4.h"
#include "feature_descriptor.h"
#include "dictionary.h"
#include <spdlog/spdlog.h>
#include "spdlog/sinks/null_sink.h"

using namespace std;

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
    uint32_t label;
    Eigen::VectorXf dvec;
    map_feature(const uint64_t id, const v3 &p, const float v, const uint32_t l, const descriptor & d);
};

struct map_node {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    uint64_t id;
    static size_t histogram_size;
    bool match_attempted{false};
    bool finished{false};
    list<map_edge> edges;
    map_edge &get_add_neighbor(uint64_t neighbor);
    int terms;
    list<map_feature *> features; //sorted by label
    bool add_feature(const uint64_t id, const v3 &p, const float v, const uint32_t l, const descriptor & d);

    transformation_variance global_transformation;

    // temporary variables used in breadth first
    int depth;
    int parent;
    transformation_variance transform;

map_node(): terms(0), depth(0), parent(-1) {}
};

struct map_match {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    uint64_t from;
    uint64_t to;
    float score;
    transformation g;
};

struct local_feature {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    v3 position;
    map_feature *feature;
};

struct match_pair {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    local_feature first, second;
    float score;
};

class mapper {
 private:
    aligned_vector<map_node> nodes;
    friend struct map_node;
    aligned_vector<transformation_variance> geometry;
    vector<uint64_t> document_frequency;
    transformation relative_transformation;
    uint64_t feature_count;
    dictionary feature_dictionary;

    void diffuse_matches(uint64_t id, vector<float> &matches, aligned_vector<map_match> &diffusion, int max, int unrecent);
    void joint_histogram(int node, list<map_feature *> &histogram);

    float tf_idf_score(const list<map_feature *> &hist1, const list<map_feature *> &hist2);
    void tf_idf_match(vector<float> &scores, const list<map_feature *> &histogram);

    float refine_transformation(const transformation_variance &base, transformation_variance &dR, transformation_variance &dT, const aligned_vector<match_pair> &neighbor_matches);
    int check_for_matches(uint64_t id1, uint64_t id2, transformation_variance &relpos, int min_inliers);
    int estimate_translation(uint64_t id1, uint64_t id2, v3 &result, int min_inliers, const transformation &pre_transform, const aligned_vector<match_pair> &matches, const aligned_vector<match_pair> &neighbor_matches);
    void localize_neighbor_features(uint64_t id, aligned_list<local_feature> &features);
    void breadth_first(int start, int maxdepth, void(mapper::*callback)(map_node &));
    void internal_set_geometry(uint64_t id1, uint64_t id2, const transformation_variance &transform, bool loop_closed);
    void set_special(uint64_t id, bool special);
    bool get_matches(uint64_t id, map_match &match, int max, int suppression);
    transformation get_relative_transformation(uint64_t id1, uint64_t id2);
    void set_geometry(uint64_t id1, uint64_t id2, const transformation_variance &transform);
    uint32_t project_feature(const descriptor & d);
    int pick_transformation_ransac(const aligned_vector<match_pair> &neighbor_matches,  transformation_variance & tv);
    int ransac_transformation(uint64_t id1, uint64_t id2, transformation_variance &proposal);
    int estimate_transform_with_inliers(const aligned_vector<match_pair> & matches, transformation_variance & tv);
    void rebuild_map_from_node(int id);

    bool unlinked;
    uint64_t node_id_offset{0};
    uint64_t feature_id_offset{0};

 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    mapper();
    ~mapper();
    void reset();

    bool is_unlinked(uint64_t node_id) const { return (unlinked && node_id < node_id_offset); }
    void add_node(uint64_t node_id);
    void add_edge(uint64_t node_id1, uint64_t node_id2);
    // Descriptor must have a norm of 1
    void add_feature(uint64_t node_id, uint64_t feature_id, const v3 & position_m, float depth_variance_m2, const descriptor & feature_descriptor);

    const aligned_vector<map_node> & get_nodes() const { return nodes; };

    void update_feature_position(uint64_t node_id, uint64_t feature_id, const v3 &position_m, float depth_variance_m2);
    void node_finished(uint64_t node_id);
    void set_node_transformation(uint64_t id, const transformation & G);

    bool find_closure(int max, int suppression, transformation &offset);

    // Training
    void train_dictionary() const;

    // Reading / writing
    bool serialize(std::string &json);
    static bool deserialize(const std::string &json, mapper & map);

    // Debugging
    void dump_map(FILE *file);
    void print_stats();

    std::unique_ptr<spdlog::logger> log = std::make_unique<spdlog::logger>("mapper",  make_shared<spdlog::sinks::null_sink_st> ());

    bool enabled = false;
};


#endif