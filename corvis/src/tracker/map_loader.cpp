/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2016-2017 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#include <iostream>
#include <mutex>
#include "map_loader.h"
#include "state_vision.h"


using namespace std;

typedef class map_node_t<map_feature_v1, map_edge_v1, frame_t> map_node_v1;
typedef class mapper_t<map_node_t, map_feature_v1, map_edge_v1, frame_t> mapper_v1;

static bstream_reader & operator >> (bstream_reader& content, shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> &feat) {
    uint64_t id = 0;
    float sin_ = 0, cos_ = 0;
    content >> id >> sin_ >> cos_;
    orb_descriptor::raw des;
    content >> (array<uint8_t, orb_descriptor::L> &)des;
    orb_descriptor loaded_des(des, cos_, sin_);
    feat = make_shared<fast_tracker::fast_feature<DESCRIPTOR>>(id, orb_descriptor(loaded_des));
    return content;
}

static bstream_reader & operator >> (bstream_reader& content, v2 &vec) {
    return content >> vec[0] >> vec[1];
}

static bstream_reader & operator >> (bstream_reader &content, shared_ptr<frame_t> frame) {
    return content >> frame->keypoints >> frame->keypoints_xy;
}

static bstream_reader & operator >> (bstream_reader &content, map_edge_v1 &edge) {
    uint8_t e_type = 0;
    content >> e_type;
    edge.type = (edge_type)e_type;
    return content >> edge.G;
}

uint64_t map_feature_v1::max_loaded_featid = 0;

static bstream_reader & operator >> (bstream_reader &content, map_feature_v1 &feat) {
    feat.v = make_shared<log_depth>();
    content >> feat.v->initial[0] >> feat.v->initial[1] >> feat.v->v;
    uint64_t id = 0;
    array<uint8_t, patch_descriptor::L> raw_desc;
    content >> id >> raw_desc;
    if (map_feature_v1::max_loaded_featid < id)  map_feature_v1::max_loaded_featid = id;
    feat.feature = make_shared<fast_tracker::fast_feature<DESCRIPTOR>>(id, patch_descriptor(raw_desc));
    return content;
}

static bstream_reader &  operator >> (bstream_reader &content, map_node_v1 &node) {
    content >> node.id >> node.camera_id >> node.edges >> node.global_transformation;
    if (!content.good()) return content;
    uint8_t has_frame = 0;
    content >> has_frame;
    if (has_frame) {
        node.frame = make_shared<frame_t>();
        content >> node.frame;
    }
    return content >> node.covisibility_edges >> node.features;
}

/// assign/transfer values to map_node and also clear container elements in map_node_v1,
/// hence, state of map_node_v1 element is not defined after the function.
template<>
void assign<map_node_v1>(map_node &node, map_node_v1 &loaded_node) {
    node.id = loaded_node.id;
    node.edges.clear();
    for (auto &ed : loaded_node.edges) {
        node.edges.emplace(piecewise_construct, forward_as_tuple(ed.first),
            forward_as_tuple(ed.second.type, ed.second.G));
    }
    loaded_node.edges.clear();
    node.global_transformation = loaded_node.global_transformation;
    node.camera_id = loaded_node.camera_id;
    node.frame = loaded_node.frame;
    node.features.clear();
    for (auto &feat : loaded_node.features) {
        node.features.emplace(piecewise_construct, forward_as_tuple(feat.first),
            forward_as_tuple(feat.second.v, feat.second.feature));
    }
    loaded_node.features.clear();
}


map_loader *get_map_load(uint8_t version) {
    switch (version) {
    case 2: return new mapper_v1();
    default: return nullptr;
    }
}
