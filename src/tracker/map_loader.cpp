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

static bstream_reader & operator >> (bstream_reader & cur_stream, transformation &transform) {
    cur_stream >> transform.T[0] >> transform.T[1] >> transform.T[2];
    cur_stream >> transform.Q.w() >> transform.Q.x() >> transform.Q.y() >> transform.Q.z();
    return cur_stream;
}

typedef class map_node_t<map_feature_v1, map_edge_v1, frame_t> map_node_v1;
typedef class mapper_t<map_node_t, map_feature_v1, map_edge_v1, frame_t> mapper_v1;


static bstream_reader & operator >> (bstream_reader& content, orb_descriptor &orb) {
    orb_descriptor::raw raw; float cos_, sin_;
    content >>  (array_&)raw >> cos_ >> sin_;
    orb = orb_descriptor(raw,cos_,sin_);
    return content;
}

static bstream_reader & operator >> (bstream_reader& content, patch_descriptor &patch) {
    array<uint8_t, patch_descriptor::L> patch_raw;
    content >> patch_raw;
    patch = patch_descriptor(patch_raw);
    return content;
}

static bstream_reader & operator >> (bstream_reader& content, shared_ptr<fast_tracker::fast_feature<patch_orb_descriptor>> &feat) {
    featureid id = 0;
    patch_descriptor patch;
    content >> id >> patch;
    feat = make_shared<fast_tracker::fast_feature<patch_orb_descriptor>>(id, patch);
    content >> feat->descriptor.orb_computed;
    if (feat->descriptor.orb_computed)
        content >> feat->descriptor.orb;
    return content;
}

static bstream_reader & operator >> (bstream_reader& content, v2 &vec) {
    return content >> vec[0] >> vec[1];
}

static bstream_reader & operator >> (bstream_reader &content, shared_ptr<frame_t> frame) {
    uint64_t frame_ts = 0;
    content >> frame->keypoints >> frame->keypoints_xy;
    uint8_t has_image = 0;
    content >> has_image; //has image data
    if (has_image) {
        uint32_t col = 0, row = 0, stride = 0;
        content >> col >> row >> stride;
        vector<char> img_data;
        img_data.resize(row * stride);
        content.read(img_data.data(), row * stride);
#ifdef RELOCALIZATION_DEBUG
        frame->image = cv::Mat(row, col, CV_8UC1, (uint8_t*)img_data.data(), stride).clone();
#endif
    }
    content >> frame_ts;
    frame->timestamp = sensor_clock::micros_to_tp(frame_ts);
    return content;
}

static bstream_reader & operator >> (bstream_reader &content, mapper::stage &stage) {
    return content >> stage.closest_id >> stage.Gr_closest_stage;
}

static bstream_reader & operator >> (bstream_reader &content, map_edge_v1 &edge) {
    uint8_t e_type = 0;
    content >> e_type;
    edge.type = (edge_type)e_type;
    return content >> edge.G;
}

static bstream_reader & operator >> (bstream_reader &content, map_feature_v1 &feat) {
    feat.v = make_shared<log_depth>();
    content >> feat.v->initial[0] >> feat.v->initial[1] >> feat.v->v >> feat.v_var >> feat.feature;
    auto map_reader = static_cast<map_stream_reader *>(&content); // should be dynamic_cast
    map_reader->max_loaded_featid = std::max(feat.feature->id, map_reader->max_loaded_featid);
    return content;
}

static bstream_reader &  operator >> (bstream_reader &content, map_node_v1 &node) {
    content >> node.id >> node.camera_id >> node.frames_active >> node.edges;
    if (!content.good()) return content;
    uint8_t has_frame = 0;
    content >> has_frame;
    if (has_frame) {
        node.frame = std::allocate_shared<frame_t>(Eigen::aligned_allocator<frame_t>());
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
    node.camera_id = loaded_node.camera_id;
    node.frames_active = loaded_node.frames_active;
    node.frame = loaded_node.frame;
    node.covisibility_edges = loaded_node.covisibility_edges;
    node.features.clear();
    for (auto &feat : loaded_node.features) {
        node.features.emplace(piecewise_construct, forward_as_tuple(feat.first),
            forward_as_tuple(feat.second.v, feat.second.feature, feat.second.v_var, feature_type::tracked));
    }
    loaded_node.features.clear();
}

map_loader *get_map_load(uint8_t version) {
    switch (version) {
    case 2: return new mapper_v1();
    default: return nullptr;
    }
}
