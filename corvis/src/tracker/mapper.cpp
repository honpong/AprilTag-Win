// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.
// Modified by Pedro Pinies and Lina Paz

#include <iostream>
#include <algorithm>
#include <assert.h>
#include <limits.h>
#include <queue>
#include "mapper.h"
#include "resource.h"
#include "state_vision.h"
#include "quaternion.h"
#include "fast_tracker.h"
#include "descriptor.h"

using namespace std;

mapper::mapper()
{
    // Load BoW vocabulary
    size_t voc_size = 0;
    const char* voc_file = load_vocabulary(voc_size);
    if (voc_size == 0 || voc_file == nullptr)
        std::cerr << "mapper: BoW vocabulary file not found" << std::endl;
    orb_voc = new orb_vocabulary();
    if(!orb_voc->loadFromMemory(voc_file, voc_size)) {
        delete orb_voc;
        orb_voc =  nullptr;
        std::cerr << "mapper: Cannot load BoW vocabulay" << std::endl;
    }
}


mapper::~mapper()
{
    reset();
}

void mapper::reset()
{
    log->debug("Map reset");
    nodes.clear();
    feature_id_offset = 0;
    node_id_offset = 0;
    unlinked = false;
    dbow_inverted_index.clear();
    features_dbow.clear();
}

map_edge &map_node::get_add_neighbor(mapper::nodeid neighbor)
{
    return edges.emplace(neighbor, map_edge{}).first->second;
}

void mapper::add_edge(nodeid id1, nodeid id2, const transformation& G12, bool loop_closure) {
    id1 += node_id_offset;
    id2 += node_id_offset;
    if(nodes.size() <= id1) nodes.resize(id1+1);
    if(nodes.size() <= id2) nodes.resize(id2+1);
    map_edge& edge12 = nodes[id1].get_add_neighbor(id2);
    edge12.loop_closure = loop_closure;
    edge12.G = G12;
    map_edge& edge21 = nodes[id2].get_add_neighbor(id1);
    edge21.loop_closure = loop_closure;
    edge21.G = invert(G12);
}

void mapper::add_node(nodeid id, const rc_Sensor camera_id)
{
    id += node_id_offset;
    if(nodes.size() <= id) nodes.resize(id + 1);
    nodes[id].id = id;
    nodes[id].camera_id = camera_id;
}

void map_node::set_feature(const uint64_t id, const v3 &pos, const float variance, const feature_type type)
{
    features[id].position = pos;
    features[id].variance = variance;
    features[id].id = id;
    features[id].type = type;
}

void mapper::set_feature(nodeid groupid, uint64_t id, const v3 &pos, const float variance, const bool is_new) {
    groupid += node_id_offset;
    id += feature_id_offset;
    nodes[groupid].set_feature(id, pos, variance);
    if (is_new) features_dbow[id] = groupid;
}

void mapper::set_node_transformation(nodeid id, const transformation & G)
{
    id += node_id_offset;
    nodes[id].global_transformation = G;
}

void mapper::node_finished(nodeid id)
{
    id += node_id_offset;
    nodes[id].status = node_status::finished;
    for (auto &word : nodes[id].frame->dbow_histogram)
        dbow_inverted_index[word.first].push_back(id); // Add this node to inverted index
}

vector<mapper::node_path> mapper::breadth_first_search(nodeid start, int maxdepth) {
    vector<node_path> neighbor_nodes;

    if(!initialized())
        return neighbor_nodes;

    queue<node_path> next;
    next.push(node_path{start, transformation()});

    //FIXME: use unordered_map<nodeid,struct{depth,parent}> to avoid storing this info in mapper
    nodes[start].parent = -2;
    nodes[start].depth = 0;
    while (!next.empty()) {
        node_path current_path = next.front();
        nodeid& u = current_path.first;
        transformation& Gu = current_path.second;
        next.pop();
        if(!maxdepth || nodes[u].depth < maxdepth) {
            for(auto edge : nodes[u].edges) {
                const nodeid& v = edge.first;
                const transformation& Gv = edge.second.G;

                if(nodes[v].parent == -1) {
                    nodes[v].depth = nodes[u].depth + 1;
                    nodes[v].parent = u;
                    next.push(node_path{v, Gu*Gv});
                }
            }
        }
        neighbor_nodes.push_back(current_path);
    }
    for(auto neighbor : neighbor_nodes) {
        nodes[neighbor.first].parent = -1;
    }
    return neighbor_nodes;
}

std::vector<std::pair<mapper::nodeid,float>> find_loop_closing_candidates(
    const std::shared_ptr<frame_t> current_frame,
    const aligned_vector<map_node> &nodes,
    const std::map<unsigned int, std::vector<mapper::nodeid>> &dbow_inverted_index,
    const orb_vocabulary* orb_voc
) {
    std::vector<std::pair<mapper::nodeid, float>> loop_closing_candidates;

    // find nodes sharing words with current frame
    std::map<mapper::nodeid,uint32_t> common_words_per_node;
    uint32_t max_num_shared_words = 0;
    for (auto word : current_frame->dbow_histogram) {
        auto word_i = dbow_inverted_index.find(word.first);
        if (word_i == dbow_inverted_index.end())
            continue;
        for (auto nid : word_i->second) {
            if (nodes[nid].status == node_status::finished) {
                common_words_per_node[nid]++;
                // keep maximum number of words shared with current frame
                if (max_num_shared_words < common_words_per_node[nid]) {
                    max_num_shared_words = common_words_per_node[nid];
                }
            }
        }
    }

    // if no common_words, return
    if (!max_num_shared_words)
        return loop_closing_candidates;

    // keep candidates with at least min_num_shared_words
    int min_num_shared_words = static_cast<int>(max_num_shared_words * 0.8f);
    std::vector<std::pair<mapper::nodeid, float> > dbow_scores;
//    assert(orb_voc->getScoringType() != DBoW2::ScoringType::L1_NORM);
    float best_score = 0.0f; // assuming L1 norm
    for (auto& node_candidate : common_words_per_node) {
        if (node_candidate.second > min_num_shared_words) {
            const map_node& node = nodes[node_candidate.first];
            float dbow_score = static_cast<float>(orb_voc->score(node.frame->dbow_histogram,
                                                                 current_frame->dbow_histogram));
            dbow_scores.push_back(std::make_pair(node_candidate.first, dbow_score));
            best_score = std::max(dbow_score, best_score);
        }
    }

    // sort candidates by dbow_score and age
    auto compare_dbow_scores = [](const std::pair<mapper::nodeid, float>& p1, const std::pair<mapper::nodeid, float>& p2) {
        return (p1.second > p2.second) || (p1.second == p2.second && p1.first < p2.first);
    };
    std::sort(dbow_scores.begin(), dbow_scores.end(), compare_dbow_scores);

    // keep good candidates according to a minimum score
    float min_score = std::max(best_score * 0.75f, 0.0f); // assuming L1 norm
    for (auto& dbow_score : dbow_scores) {
        if (dbow_score.second > min_score) {
            loop_closing_candidates.push_back(dbow_score);
        } else {
            break;
        }
    }
    return loop_closing_candidates;
}

static size_t calculate_orientation_bin(const orb_descriptor &a, const orb_descriptor &b, const size_t num_orientation_bins) {
    return static_cast<size_t>(((a - b) * (float)M_1_PI + 1) / 2 * num_orientation_bins + 0.5f) % num_orientation_bins;
}

static mapper::matches match_2d_descriptors(const std::shared_ptr<frame_t> candidate_frame, const std::shared_ptr<frame_t> current_frame,
                                            std::map<uint64_t, mapper::nodeid> &features_dbow) {
    //matches per orientationn increment between current frame and node candidate
    const int num_orientation_bins = 30;
    std::vector<mapper::matches> increment_orientation_histogram(num_orientation_bins);

    mapper::matches current_to_candidate_matches;

    if (candidate_frame->keypoints.size() > 0 && current_frame->keypoints.size() > 0) {
        auto it_candidate = candidate_frame->dbow_direct_file.begin();
        auto it_current = current_frame->dbow_direct_file.begin();

        while (it_candidate != candidate_frame->dbow_direct_file.end() &&
               it_current != current_frame->dbow_direct_file.end()) {
            if (it_current->first < it_candidate->first) {
                ++it_current;
            } else if (it_current->first > it_candidate->first) {
                ++it_candidate;
            } else {
                auto candidate_keypoint_indexes = it_candidate->second;
                auto current_keypoint_indexes = it_current->second;

                for (int current_point_idx : current_keypoint_indexes) {
                    int best_candidate_point_idx;
                    int best_distance = std::numeric_limits<int>::max();
                    int second_best_distance = std::numeric_limits<int>::max();
                    auto& current_keypoint = *current_frame->keypoints[current_point_idx];
                    for (int candidate_point_idx : candidate_keypoint_indexes) {
                        auto& candidate_keypoint = *candidate_frame->keypoints[candidate_point_idx];
                        int dist = orb_descriptor::distance(candidate_keypoint.descriptor,
                                                            current_keypoint.descriptor);
                        if (dist < best_distance
                            // Use only keypoints with 3D estimation
                            && features_dbow.find(candidate_keypoint.id) != features_dbow.end()) {
                            second_best_distance = best_distance;
                            best_distance = dist;
                            best_candidate_point_idx = candidate_point_idx;
                        }
                    }

                    // not match if more than 50 bits are different
                    if (best_distance <= 50 && (best_distance < second_best_distance * 0.6f)) {
                        auto& best_candidate_keypoint = *candidate_frame->keypoints[best_candidate_point_idx];
                        size_t bin = calculate_orientation_bin(best_candidate_keypoint.descriptor,
                                                               current_keypoint.descriptor,
                                                               num_orientation_bins);
                        increment_orientation_histogram[bin].push_back(mapper::match(current_point_idx, best_candidate_point_idx));
                    }
                }
                ++it_current;
                ++it_candidate;
            }
        }

        // Check orientations to prune wrong matches (just keep best 3)
        std::partial_sort(increment_orientation_histogram.begin(),
                          increment_orientation_histogram.begin()+3,
                          increment_orientation_histogram.end(),
                          [](const std::vector<std::pair<uint64_t, uint64_t>>&v1,
                          const std::vector<std::pair<uint64_t, uint64_t>>&v2) {
            return (v1.size() > v2.size());});

        for(int i=0; i<3; ++i) {
            current_to_candidate_matches.insert(current_to_candidate_matches.end(),
                                                increment_orientation_histogram[i].begin(),
                                                increment_orientation_histogram[i].end());
        }
    }

    return current_to_candidate_matches;
}

void mapper::estimate_pose(const aligned_vector<v3>& points_3d, const aligned_vector<v2>& points_2d, const rc_Sensor camera_id, transformation& G_candidateB_currentframeB, std::set<size_t>& inliers_set) {
    state_extrinsics* const extrinsics = camera_extrinsics[camera_id];
    transformation G_BC = transformation(extrinsics->Q.v, extrinsics->T.v);
    state_vision_intrinsics* const intrinsics = camera_intrinsics[camera_id];
    const f_t focal_px = intrinsics->focal_length.v * intrinsics->image_height;
    const f_t sigma_px = 3.0;
    const f_t max_reprojection_error = 2*sigma_px/focal_px;
    const int max_iter = 10; // 10
    const float confidence = 0.9; //0.9
    std::default_random_engine rng(-1);
    transformation G_currentframeC_candidateB;
    estimate_transformation(points_3d, points_2d, G_currentframeC_candidateB, rng, max_iter, max_reprojection_error, confidence, 5, &inliers_set);
    G_candidateB_currentframeB = invert(G_BC*G_currentframeC_candidateB);
}

bool mapper::relocalize(const camera_frame_t& camera_frame, std::vector<transformation>& vG_W_currentframe) {
    std::shared_ptr<frame_t> current_frame = camera_frame.frame;
    if (!current_frame->keypoints.size())
        return false;

    bool is_relocalized = false;
    const int min_num_inliers = 12;
    int best_num_inliers = 0;
    int i = 0;

    std::vector<std::pair<nodeid, float>> candidate_nodes =
        find_loop_closing_candidates(current_frame, nodes, dbow_inverted_index, orb_voc);
    const auto &keypoint_current = current_frame->keypoints;
    state_vision_intrinsics* const intrinsics = camera_intrinsics[camera_frame.camera_id];
    for (auto nid : candidate_nodes) {
        matches matches_node_candidate = match_2d_descriptors(nodes[nid.first].frame, current_frame, features_dbow);
        // Just keep candidates with more than a min number of mathces
        std::set<size_t> inliers_set;
        if(matches_node_candidate.size() >= min_num_inliers) {
            aligned_vector<v3> candidate_3d_points;
            aligned_vector<v2> current_2d_points;
            transformation G_candidate_currentframe;
            // Estimate pose from 3d-2d matches
            auto neighbors = breadth_first_search(nid.first, 1); // all points observed from this candidate node should be created at a node 1 edge away in the graph
            std::map<nodeid, transformation> G_candidate_neighbors;
            for (auto neighbor : neighbors)
                G_candidate_neighbors[neighbor.first] = std::move(neighbor.second);

            const auto &keypoint_candidates = nodes[nid.first].frame->keypoints;
            for (auto m : matches_node_candidate) {
                auto &candidate = *keypoint_candidates[m.second];
                uint64_t keypoint_id = candidate.id;
                nodeid nodeid_keypoint = features_dbow[keypoint_id];

                // NOTE: We use 3d features observed from candidate, this does not mean
                // these features belong to the candidate node (group)
                map_feature &mfeat = nodes[nodeid_keypoint].features[keypoint_id]; // feat is in body frame
                v3 p_candidate = G_candidate_neighbors[nodeid_keypoint] * mfeat.position;
                candidate_3d_points.push_back(p_candidate);
                //undistort keypoints at current frame
                auto& current = *keypoint_current[m.first];
                feature_t kp = {current.x, current.y};
                feature_t ukp = intrinsics->undistort_feature(intrinsics->normalize_feature(kp));
                current_2d_points.push_back(ukp);
            }
            estimate_pose(candidate_3d_points, current_2d_points, camera_frame.camera_id, G_candidate_currentframe, inliers_set);
            if(inliers_set.size() >= min_num_inliers) {
                is_relocalized = true;
//                vG_WC.push_back(G_WC);

                if(inliers_set.size() >  best_num_inliers) {
                    best_num_inliers = inliers_set.size();
                    vG_W_currentframe.clear();
                    const transformation& G_W_candidate = nodes[nid.first].global_transformation;
                    vG_W_currentframe.push_back(G_W_candidate*G_candidate_currentframe);
                    transformation G_candidate_closestnode = G_candidate_currentframe*invert(camera_frame.G_closestnode_frame);
                    add_edge(nid.first, camera_frame.closest_node,
                             G_candidate_closestnode, true);
                }
            }
        }

        if (!inliers_set.size())
            log->debug("{}/{}) candidate nid: {:3} score: {:.5f}, matches: {:2}",
                       i++, candidate_nodes.size(), nid.first, nid.second, matches_node_candidate.size());
        else
            log->info(" {}/{}) candidate nid: {:3} score: {:.5f}, matches: {:2}, EPnP inliers: {}",
                      i++, candidate_nodes.size(), nid.first, nid.second, matches_node_candidate.size(), inliers_set.size());
    }

    return is_relocalized;
}

using namespace rapidjson;

#define KEY_INDEX "index"
#define RETURN_IF_FAILED(R) {bool ret = R; if (!ret) return ret;}

#define KEY_NODE_EDGE_LOOP_CLOSURE "closure"
#define KEY_NODE_EDGE_TRANSLATION "T"
#define KEY_NODE_EDGE_QUATERNION "Q"
void map_edge::serialize(Value &json, Document::AllocatorType & allocator) {
    json.AddMember(KEY_NODE_EDGE_LOOP_CLOSURE, loop_closure, allocator);

    // add edge transformation
    Value translation_json(kArrayType);
    translation_json.PushBack(G.T[0], allocator);
    translation_json.PushBack(G.T[1], allocator);
    translation_json.PushBack(G.T[2], allocator);
    json.AddMember(KEY_NODE_EDGE_TRANSLATION, translation_json, allocator);

    Value rotation_json(kArrayType);
    rotation_json.PushBack(G.Q.w(), allocator);
    rotation_json.PushBack(G.Q.x(), allocator);
    rotation_json.PushBack(G.Q.y(), allocator);
    rotation_json.PushBack(G.Q.z(), allocator);
    json.AddMember(KEY_NODE_EDGE_QUATERNION, rotation_json, allocator);
}

void map_edge::deserialize(const Value &json, map_edge &edge) {
    edge.loop_closure = json[KEY_NODE_EDGE_LOOP_CLOSURE].GetBool();

    // get edge transformation
    transformation &G = edge.G;
    const Value & translation = json[KEY_NODE_EDGE_TRANSLATION];
    for (SizeType j = 0; j < G.T.size() && j < translation.Size(); j++) {
        G.T[j] = (float)translation[j].GetDouble();
    }
    const Value & rotation = json[KEY_NODE_EDGE_QUATERNION];
    G.Q.w() = (float)rotation[0].GetDouble();
    G.Q.x() = (float)rotation[1].GetDouble();
    G.Q.y() = (float)rotation[2].GetDouble();
    G.Q.z() = (float)rotation[3].GetDouble();
}

#define KEY_FEATURE_ID "id"
#define KEY_FEATURE_VARIANCE "variance"
#define KEY_FEATURE_POSITION "position"
void map_feature::serialize(Value &feature_json, Document::AllocatorType &allocator) {
    feature_json.AddMember(KEY_FEATURE_ID, id, allocator);
    feature_json.AddMember(KEY_FEATURE_VARIANCE, variance, allocator);

    Value pos_json(kArrayType);
    pos_json.PushBack(position[0], allocator);
    pos_json.PushBack(position[1], allocator);
    pos_json.PushBack(position[2], allocator);
    feature_json.AddMember(KEY_FEATURE_POSITION, pos_json, allocator);
}

bool map_feature::deserialize(const Value &json, map_feature &feature, uint64_t &max_loaded_featid) {
    feature.id = json[KEY_FEATURE_ID].GetUint64();
    if (feature.id > max_loaded_featid) max_loaded_featid = feature.id;

    feature.variance = (float)json[KEY_FEATURE_VARIANCE].GetDouble();
    const Value &pos_json = json[KEY_FEATURE_POSITION];
    RETURN_IF_FAILED(pos_json.IsArray())
        RETURN_IF_FAILED(pos_json.Size() == feature.position.size())
        for (SizeType j = 0; j < pos_json.Size(); j++) {
            feature.position[j] = (f_t)pos_json[j].GetDouble();
        }
    return true;
}

#define KEY_FRAME_FEAT "features"
#define KEY_FRAME_FEAT_ID "id"
#define KEY_FRAME_FEAT_X "x"
#define KEY_FRAME_FEAT_Y "y"
#define KEY_FRAME_FEAT_LEVEL "level"
#define KEY_FRAME_FEAT_DESC "descriptor"
#define KEY_FRAME_FEAT_DESC_RAW "raw"
#define KEY_FRAME_FEAT_DESC_SIN "sin"
#define KEY_FRAME_FEAT_DESC_COS "cos"
void frame_serialize(const std::shared_ptr<frame_t> frame, Value &json, Document::AllocatorType &allocator) {

    // add key point
    Value features_json(kArrayType);
    for (auto &fast_feat : frame->keypoints) {
        Value feat_json(kObjectType);
        feat_json.AddMember(KEY_FRAME_FEAT_ID, fast_feat->id, allocator);
        feat_json.AddMember(KEY_FRAME_FEAT_X, fast_feat->x, allocator);
        feat_json.AddMember(KEY_FRAME_FEAT_Y, fast_feat->y, allocator);
        //feat_json.AddMember(KEY_FRAME_FEAT_LEVEL, fast_feat->level, allocator);

        // add descriptor
        Value descriptor_json(kObjectType);
        descriptor_json.AddMember(KEY_FRAME_FEAT_DESC_SIN, fast_feat->descriptor.sin_, allocator);
        descriptor_json.AddMember(KEY_FRAME_FEAT_DESC_COS, fast_feat->descriptor.cos_, allocator);
        Value desc_raw_json(kArrayType);
        for (auto v : fast_feat->descriptor.descriptor)
            desc_raw_json.PushBack(v, allocator);
        descriptor_json.AddMember(KEY_FRAME_FEAT_DESC_RAW, desc_raw_json, allocator);
        feat_json.AddMember(KEY_FRAME_FEAT_DESC, descriptor_json, allocator);

        features_json.PushBack(feat_json, allocator);
    }
    json.AddMember(KEY_FRAME_FEAT, features_json, allocator);
}

bool frame_deserialize(const Value &json, std::shared_ptr<frame_t> &frame) {

    frame = std::make_shared<frame_t>();
    // get key points
    const Value &features_json = json[KEY_FRAME_FEAT];
    RETURN_IF_FAILED(features_json.IsArray())
    frame->keypoints.resize(features_json.Size(), nullptr);
    for (SizeType j = 0; j < features_json.Size(); j++) {
        // get descriptor values
        const Value &desc_json = features_json[j][KEY_FRAME_FEAT_DESC];
        const Value &desc_raw_json = desc_json[KEY_FRAME_FEAT_DESC_RAW];
        RETURN_IF_FAILED(desc_raw_json.IsArray())
        orb_descriptor::raw raw_desc;
        RETURN_IF_FAILED(raw_desc.size() == desc_raw_json.Size())
        for (SizeType d = 0; d < desc_raw_json.Size(); d++)
            raw_desc[d] = desc_raw_json[d].GetUint64();
        float desc_cos = (float)desc_json[KEY_FRAME_FEAT_DESC_COS].GetDouble();
        float desc_sin = (float)desc_json[KEY_FRAME_FEAT_DESC_SIN].GetDouble();
        //get feature values
        frame->keypoints[j] = std::make_shared<fast_tracker::fast_feature<orb_descriptor>>(
            features_json[j][KEY_FRAME_FEAT_ID].GetUint64(),
            (float)features_json[j][KEY_FRAME_FEAT_X].GetDouble(),
            (float)features_json[j][KEY_FRAME_FEAT_Y].GetDouble(),
            //features_json[j][KEY_FRAME_FEAT_LEVEL].GetInt(),
            orb_descriptor(raw_desc, desc_cos, desc_sin)
        );
    }
    return true;
}

#define KEY_NODE_ID "id"
#define KEY_FRAME_CAMERA_ID "camera_id"
#define KEY_NODE_EDGE "edges"
#define KEY_NODE_FEATURES "features"
#define KEY_NODE_FRAME "map_frame"
#define KEY_NODE_TRANSLATION "T"
#define KEY_NODE_QUATERNION "Q"
#define KEY_NODE_STATUS "status"
void map_node::serialize(Value &json, Document::AllocatorType & allocator) {
    json.AddMember(KEY_NODE_ID, id, allocator);
    json.AddMember(KEY_FRAME_CAMERA_ID, camera_id, allocator);
    // add edges
    Value edges_json(kArrayType);
    for (auto &edge : edges) {
        Value edge_json(kObjectType);
        edge.second.serialize(edge_json, allocator);
        edge_json.AddMember(KEY_INDEX, edge.first, allocator);
        edges_json.PushBack(edge_json, allocator);
    }
    json.AddMember(KEY_NODE_EDGE, edges_json, allocator);
    // add global transformation
    Value translation_json(kArrayType);
    translation_json.PushBack(global_transformation.T[0], allocator);
    translation_json.PushBack(global_transformation.T[1], allocator);
    translation_json.PushBack(global_transformation.T[2], allocator);
    json.AddMember(KEY_NODE_TRANSLATION, translation_json, allocator);

    Value rotation_json(kArrayType);
    rotation_json.PushBack(global_transformation.Q.w(), allocator);
    rotation_json.PushBack(global_transformation.Q.x(), allocator);
    rotation_json.PushBack(global_transformation.Q.y(), allocator);
    rotation_json.PushBack(global_transformation.Q.z(), allocator);
    json.AddMember(KEY_NODE_QUATERNION, rotation_json, allocator);
    // add map_frame
    Value map_frame_json = Value(kObjectType);
    frame_serialize(frame, map_frame_json, allocator);
    json.AddMember(KEY_NODE_FRAME, map_frame_json, allocator);

    Value features_json(kArrayType);
    for (auto &feat : features) {
        Value feature_json(kObjectType);
        feat.second.serialize(feature_json, allocator);
        feature_json.AddMember(KEY_INDEX, feat.first, allocator);
        features_json.PushBack(feature_json, allocator);
    }
    json.AddMember(KEY_NODE_FEATURES, features_json, allocator);
    json.AddMember(KEY_NODE_STATUS, (uint8_t)status, allocator);
}

bool map_node::deserialize(const Value &json, map_node &node, uint64_t &max_loaded_featid) {
    node.id = json[KEY_NODE_ID].GetUint64();
    node.camera_id = json[KEY_FRAME_CAMERA_ID].GetUint64();
    // get edges
    const Value & edges_json = json[KEY_NODE_EDGE];
    RETURN_IF_FAILED(edges_json.IsArray())
    for (SizeType j = 0; j < edges_json.Size(); j++) {
        const Value& edge_json = edges_json[j];
        uint64_t unordered_map_index = edge_json[KEY_INDEX].GetUint64();
        map_edge::deserialize(edge_json, node.edges[unordered_map_index]);
    }
    // get global transformation
    transformation &G = node.global_transformation;
    const Value & translation = json[KEY_NODE_TRANSLATION];
    RETURN_IF_FAILED(translation.IsArray())
        for (SizeType j = 0; j < G.T.size() && j < translation.Size(); j++) {
            G.T[j] = (float)translation[j].GetDouble();
        }
    const Value & rotation = json[KEY_NODE_QUATERNION];
    G.Q.w() = (float)rotation[0].GetDouble();
    G.Q.x() = (float)rotation[1].GetDouble();
    G.Q.y() = (float)rotation[2].GetDouble();
    G.Q.z() = (float)rotation[3].GetDouble();

    // get map frame
    RETURN_IF_FAILED(frame_deserialize(json[KEY_NODE_FRAME], node.frame))
    const Value & features_json = json[KEY_NODE_FEATURES];
    RETURN_IF_FAILED(features_json.IsArray())
    for (SizeType j = 0; j < features_json.Size(); j++) {
        const Value &feature_json = features_json[j];
        uint64_t map_index = feature_json[KEY_INDEX].GetUint64();
        RETURN_IF_FAILED(map_feature::deserialize(feature_json, node.features[map_index], max_loaded_featid))
    }
    node.status = static_cast<node_status>(json[KEY_NODE_STATUS].GetUint());
    return true;
}

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#define MAPPER_SERIALIZED_VERSION 2
#define KEY_VERSION "version"
#define KEY_NODEID "node_id"
#define KEY_MAP_DBOW_INVERTED_INDICES "inverted_indices"
#define KEY_MAP_DBOW_FEATURES "dbow_features"
#define KEY_NODES "nodes"
#define KEY_FEATURE_DESCRIPTOR "d"

void mapper::serialize(rapidjson::Value &map_json, rapidjson::Document::AllocatorType &allocator) {
    Value version(MAPPER_SERIALIZED_VERSION);
    map_json.SetObject();
    map_json.AddMember(KEY_VERSION, version, allocator);

    // add map nodes
    Value nodes_json(kArrayType);
    for (int i = 0; i < nodes.size(); i++) {
        Value node_json(kObjectType);
        nodes[i].serialize(node_json, allocator);
        nodes_json.PushBack(node_json, allocator);
    }
    map_json.AddMember(KEY_NODES, nodes_json, allocator);

    // add dbow_inverted_index
    Value dbow_inverted_indices_json(kArrayType);
    for (auto &featid_nodeid : dbow_inverted_index) {
        Value inverted_index_json(kObjectType);
        inverted_index_json.AddMember(KEY_INDEX, featid_nodeid.first, allocator);

        Value node_ids_json(kArrayType);
        for (auto &node_entry : featid_nodeid.second)
            node_ids_json.PushBack(node_entry, allocator);
        inverted_index_json.AddMember(KEY_NODEID, node_ids_json, allocator);
        dbow_inverted_indices_json.PushBack(inverted_index_json, allocator);
    }
    map_json.AddMember(KEY_MAP_DBOW_INVERTED_INDICES, dbow_inverted_indices_json, allocator);

    // add features_dbow
    Value features_dbow_json(kArrayType);
    for (auto &featid_nodeid : features_dbow) {
        Value featid_nodeid_json(kObjectType);
        featid_nodeid_json.AddMember(KEY_INDEX, featid_nodeid.first, allocator);
        featid_nodeid_json.AddMember(KEY_NODEID, featid_nodeid.second, allocator);
        features_dbow_json.PushBack(featid_nodeid_json, allocator);
    }
    map_json.AddMember(KEY_MAP_DBOW_FEATURES, features_dbow_json, allocator);
}

#define HANDLE_IF_FAILED(condition, handle_func) { bool ret = condition; if (!ret) {handle_func(); return false; } }

bool mapper::deserialize(const Value &map_json, mapper &map) {
    auto failure_handle = [&]() {map.reset(); map.log->critical("Failed to load map!");};

    int version = map_json[KEY_VERSION].GetInt();
    if (version != MAPPER_SERIALIZED_VERSION) {
        map.log->error("mapper version mismatch.  Found {}, but xpected {}", version, MAPPER_SERIALIZED_VERSION);
        return false;
    }
    const Value & nodes_json = map_json[KEY_NODES];
    HANDLE_IF_FAILED(nodes_json.IsArray(), failure_handle)

    map.reset();

    // get map nodes
    uint64_t max_feature_id = 0;
    uint64_t max_node_id = 0;
    map.nodes.resize(nodes_json.Size());
    for (SizeType i = 0; i < nodes_json.Size(); i++) {
        auto &cur_node = map.nodes[i];
        HANDLE_IF_FAILED(map_node::deserialize(nodes_json[i], cur_node, max_feature_id), failure_handle)
        cur_node.frame->calculate_dbow(map.orb_voc); // populate map_frame's dbow_histogram and dbow_direct_file
        if (max_node_id < cur_node.id) max_node_id = cur_node.id;
    }

    map.node_id_offset = max_node_id + 1;
    map.feature_id_offset = max_feature_id + 1;

    // get dbow_inverted_index
    const Value &dbow_inverted_indices_json = map_json[KEY_MAP_DBOW_INVERTED_INDICES];
    HANDLE_IF_FAILED(dbow_inverted_indices_json.IsArray(), failure_handle);
    for (SizeType j = 0; j < dbow_inverted_indices_json.Size(); j++) {
        auto map_key = dbow_inverted_indices_json[j][KEY_INDEX].GetUint64();
        const Value &node_ids_json = dbow_inverted_indices_json[j][KEY_NODEID];
        HANDLE_IF_FAILED(node_ids_json.IsArray(), failure_handle);
        auto &node_ids = map.dbow_inverted_index[map_key];
        for (SizeType k = 0; k < node_ids_json.Size(); k++)
            node_ids.push_back(node_ids_json[k].GetUint64());
    }

    // get features_dbow
    const Value &features_dbow_json = map_json[KEY_MAP_DBOW_FEATURES];
    HANDLE_IF_FAILED(features_dbow_json.IsArray(), failure_handle);
    for (SizeType j = 0; j < features_dbow_json.Size(); j++) {
        auto map_key = features_dbow_json[j][KEY_INDEX].GetUint64();
        map.features_dbow[map_key] = features_dbow_json[j][KEY_NODEID].GetUint64();
    }
    map.unlinked = true;
    map.log->info("Loaded map with {} nodes and {} features", map.node_id_offset, map.feature_id_offset);
    return true;
}

