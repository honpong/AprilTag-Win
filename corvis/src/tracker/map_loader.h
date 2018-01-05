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

class log_depth;
struct frame_t;
class bstream_reader;
struct map_edge;
struct map_node;
class mapper;

struct map_edge_v1 {
    bool loop_closure = false;
    transformation G;
};

struct map_feature_v1 {
    std::shared_ptr<log_depth> v;
    std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature;
    static uint64_t max_loaded_featid;
};


struct map_node_v1 {
    uint64_t id;
    std::map<uint64_t, map_edge_v1> edges;
    transformation global_transformation;
    uint64_t camera_id;
    std::shared_ptr<frame_t> frame;
    std::map<uint64_t,map_feature_v1> features;
};

/// interface for loading a map file
class map_loader {
public:
    virtual bool b_deserialize(bstream_reader &cur_stream) = 0;
    virtual void set(mapper &cur_map) = 0;
};

/// loading data into mapper data structures version v1.
class mapper_v1 : public map_loader {
public:
    mapper_v1() {};
    typedef uint64_t nodeid;
    std::unordered_map<nodeid, map_node_v1> nodes;
    friend struct map_node;
    std::map<uint64_t, std::vector<nodeid>> dbow_inverted_index;
    std::map<uint64_t, nodeid> features_dbow;
    virtual bool b_deserialize(bstream_reader &cur_stream) override;
    virtual void set(mapper &cur_map) override;
};

/// get a class instance of map_load corresponding to specified version.
map_loader *get_map_load(uint8_t version);
