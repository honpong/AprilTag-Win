// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __MAPPER_H
#define __MAPPER_H

#include <list>
#include <vector>
#include <stdbool.h>

#include "../numerics/transformation.h"
#include "../numerics/vec4.h"
#include "feature_descriptor.h"
#include "dictionary.h"

using namespace std;

class transformation_variance {
    public:
    transformation transform;
    m4 variance;
};

struct map_edge {
    uint64_t neighbor;
    int64_t geometry; //positive/negative indicate geometric edge direction, 0 indicates a covisibility edge
};

struct map_feature {
    uint64_t id;
    v4 position;
    float variance;
    uint32_t label;
    descriptor d;
    map_feature(const uint64_t id, const v4 &p, const float v, const uint32_t l, const descriptor & d);
};

struct map_node {
    uint64_t id;
    static size_t histogram_size;
    bool match_attempted{false};
    bool finished{false};
    list<map_edge> edges;
    map_edge &get_add_neighbor(uint64_t neighbor);
    int terms;
    list<map_feature *> features; //sorted by label
    int depth; //used in traversal
    int parent;
    transformation_variance transform;
    transformation_variance global_orientation;
map_node(): terms(0), depth(0), parent(-1) {}
    bool add_feature(const uint64_t id, const v4 &p, const float v, const uint32_t l, const descriptor & d);
};

struct map_match {
    uint64_t id;
    float score;
};

struct local_feature {
    v4 position;
    map_feature *feature;
};

struct match_pair {
    local_feature first, second;
    float score;
};

class mapper {
 protected:
    vector<map_node> nodes;
    vector<transformation_variance> geometry;
    vector<uint64_t> document_frequency;
    uint64_t reference;
    transformation relative_transformation;
    list<v4> local_features;
    bool unlinked;
    list<uint64_t> origins;
    uint64_t feature_count;
    dictionary feature_dictionary;

    //vector<vector <reverse_entry> > reverse;
    //unused
    float one_to_one_idf_score(const list<map_feature *> &hist1, const list<map_feature *> &hist2);

    void diffuse_matches(vector<float> &matches, vector<map_match> &diffusion, int max, int unrecent);
    void joint_histogram(int node, list<map_feature *> &histogram);

    float tf_idf_score(const list<map_feature *> &hist1, const list<map_feature *> &hist2);
    void tf_idf_match(vector<float> &scores, const list<map_feature *> &histogram);

    int new_check_for_matches(uint64_t id1, uint64_t id2, transformation_variance &relpos, int min_inliers);
    int check_for_matches(uint64_t id1, uint64_t id2, transformation_variance &relpos, int min_inliers);
    int estimate_translation(uint64_t id1, uint64_t id2, v4 &result, int min_inliers, const transformation &pre_transform, const list<match_pair> &matches, const list<match_pair> &neighbor_matches);
    int brute_force_rotation(uint64_t id1, uint64_t id2, transformation_variance &trans, int threshhold, float min, float max);
    void localize_neighbor_features(uint64_t id, list<local_feature> &features);
    void breadth_first(int start, int maxdepth, void(mapper::*callback)(map_node &));
    void internal_set_geometry(uint64_t id1, uint64_t id2, const transformation_variance &transform);
    void set_special(uint64_t id, bool special);

 public:
    uint64_t group_id_offset;
    mapper();
    void add_edge(uint64_t id1, uint64_t id2);
    void set_relative_transformation(const transformation &T);
    void set_geometry(uint64_t id1, uint64_t id2, const transformation_variance &transform);
    // uses diffuse_matches and tf_idf_match
    bool get_matches(uint64_t id, vector<map_match> &matches, int max, int suppression);
    void set_reference(uint64_t id);
    /*    vector<map_match> *new_query(const vector<int> &histogram, size_t K);
    void delete_query(vector<map_match> *query);
    void add_matches(vector<int> &matches, const vector<int> &histogram);*/
    void dump_map(const char *filename);
    void train_dictionary() const;
    void node_finished(uint64_t id, const transformation &global_orientation);
    bool no_search;
    void print_stats();
    // return the number of features stored in a node
    int num_features(uint64_t group_id);
    // search all the groups for a similar one
    void match_group(uint64_t group_id);

    void add_node(uint64_t group_id);
    void add_feature(uint64_t groupid, uint64_t id, v4 pos, float variance, const descriptor & d);
    uint32_t project_feature(const descriptor & d);
};

#endif
