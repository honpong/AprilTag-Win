/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2016-2017 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <stdbool.h>

#include "transformation.h"
#include <spdlog/spdlog.h>
#include "spdlog/sinks/null_sink.h"
#include "fast_tracker.h"
#include "rc_tracker.h"
#include "orb_descriptor.h"
#include "descriptor.h"
#include "mapper.h"
#include "bstream.h"

struct map_edge_v1 {
    edge_type type;
    transformation G;
};

struct map_feature_v1 {
    std::shared_ptr<log_depth> v;
    f_t v_var;
    std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature;
};

template<typename map_feature_v, typename map_edge_v, typename frame_v>
class map_node_t {
public:
    nodeid id;
    std::map<nodeid, map_edge_v> edges;
    std::set<nodeid> covisibility_edges;
    transformation global_transformation;
    uint64_t camera_id;
    uint64_t frames_active;
    std::shared_ptr<frame_v> frame;
    std::map<featureid, map_feature_v> features;
};

class map_stream_reader : public bstream_reader {
public:
    map_stream_reader(const rc_LoadCallback func_, void *handle_) : bstream_reader(func_, handle_) {};
    featureid max_loaded_featid{ 0 };
    map_stream_reader() = delete;
};

/// interface for loading a map file
class map_loader {
public:
    virtual bool deserialize(map_stream_reader &cur_stream) = 0;
    virtual void set(mapper &cur_map) = 0;
    virtual ~map_loader() {}
};

/// loading data into mapper data structures version v1.
template<template <class map_feature_v, class...> class map_node_v, class map_feature_v, class... TArgs>
class mapper_t : public map_loader {
public:
    mapper_t() {};
    std::unordered_map<nodeid, map_node_v<map_feature_v, TArgs...>> nodes;
    std::map<featureid, nodeid> features_dbow;
    aligned_map<std::string,mapper::stage> stages;
    virtual void set(mapper &cur_map) override {
        for (auto ele : nodes)
            assign((*cur_map.nodes)[ele.first], ele.second);
        *cur_map.features_dbow = move(features_dbow);
        *cur_map.stages = std::move(stages);
    }
    virtual bool deserialize(map_stream_reader &cur_stream) override {
        cur_stream.max_loaded_featid = 0;
        cur_stream >> nodes >> features_dbow;
        cur_stream >> stages;
        return cur_stream.good();
    }
};

template<typename map_node_v>
static inline void assign(map_node &node, map_node_v &loaded_node);

/// get a class instance of map_load corresponding to specified version.
map_loader *get_map_load(uint8_t version);
