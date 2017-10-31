// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.
// Modified by Pedro Pinies and Lina Paz

#include <iostream>
#include <algorithm>
#include <assert.h>
#include <limits.h>
#include <queue>
#include <unordered_set>
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
    orb_voc.reset(new orb_vocabulary());
    if(!orb_voc->loadFromMemory(voc_file, voc_size)) {
        orb_voc.reset();
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
    current_node_id = std::numeric_limits<uint64_t>::max();
    unlinked = false;
    dbow_inverted_index.clear();
    features_dbow.clear();
}

map_edge &map_node::get_add_neighbor(mapper::nodeid neighbor)
{
    return edges.emplace(neighbor, map_edge{}).first->second;
}

void mapper::add_edge(nodeid id1, nodeid id2, const transformation& G12, bool loop_closure) {
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
    if(nodes.size() <= id) nodes.resize(id + 1);
    nodes[id].id = id;
    nodes[id].camera_id = camera_id;
}

void mapper::get_triangulation_geometry(const nodeid group_id, const tracker::feature_track& keypoint, aligned_vector<v2> &tracks_2d, std::vector<transformation> &camera_poses)
{
    tracks_2d.clear();
    camera_poses.clear();
    std::set<nodeid> search_nodes;
    for (auto &ag : keypoint.group_tracks) {
        search_nodes.insert(ag.group_id);
    }
    nodes_path ref_path = breadth_first_search(group_id, std::move(search_nodes));
    map_node &ref_node = nodes[group_id];
    const state_extrinsics* const ref_extrinsics = camera_extrinsics[ref_node.camera_id];
    transformation G_BR = transformation(ref_extrinsics->Q.v, ref_extrinsics->T.v);
    // accumulate cameras and 2d tracks to triangulate
    for (auto &ag : keypoint.group_tracks) {
        map_node &node = nodes[ag.group_id];
        const state_extrinsics* const extrinsics = camera_extrinsics[node.camera_id];
        const state_vision_intrinsics* const intrinsics = camera_intrinsics[node.camera_id];
        transformation G_BC = transformation(extrinsics->Q.v, extrinsics->T.v);
        // normalize/undistort 2d point track
        feature_t kpd = {ag.x, ag.y};
        feature_t kpn = intrinsics->undistort_feature(intrinsics->normalize_feature(kpd));
        tracks_2d.push_back(kpn);
        // read node transformation
        transformation G_CR = invert(ref_path[ag.group_id] * G_BC) * G_BR;
        camera_poses.push_back(G_CR);
    }
}

void mapper::add_triangulated_feature_to_group(const nodeid group_id, std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature, const f_t depth_m)
{
    map_node &ref_node = nodes[group_id];
    ref_node.add_feature(feature, depth_m, 1.e-3f*1.e-3f, feature_type::triangulated);
    features_dbow[feature->id] = group_id;
}

void map_node::add_feature(std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature,
                           const f_t depth_m, const float depth_variance_m2, const feature_type type) {
    map_feature mf;
    mf.depth = depth_m;
    mf.variance = depth_variance_m2;
    mf.feature = feature;
    mf.type = type;
    features.emplace(feature->id, mf);
}

void map_node::set_feature(const uint64_t id, const f_t depth_m, const float depth_variance_m2, const feature_type type)
{
    features[id].depth = depth_m;
    features[id].variance = depth_variance_m2;
    features[id].type = type;
}

void mapper::add_feature(nodeid groupid, std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature,
                         const f_t depth_m, const float depth_variance_m2, const feature_type type) {
    nodes[groupid].add_feature(feature, depth_m, depth_variance_m2, type);
    features_dbow[feature->id] = groupid;
}

void mapper::set_feature(nodeid groupid, uint64_t id, const f_t depth_m, const float depth_variance_m2,
                         const feature_type type) {
    nodes[groupid].set_feature(id, depth_m, depth_variance_m2, type);
}

v3 mapper::get_feature3D(nodeid node_id, uint64_t feature_id) {
    map_node& node = nodes[node_id];
    auto mf = node.features[feature_id];
    auto intrinsics = camera_intrinsics[node.camera_id];
    auto extrinsics = camera_extrinsics[node.camera_id];
    m3 Rbc = extrinsics->Q.v.toRotationMatrix();
    feature_t feature{mf.feature->x, mf.feature->y};
    v2 pn = intrinsics->undistort_feature(intrinsics->normalize_feature(feature));
    return Rbc*(mf.depth*pn.homogeneous()) + extrinsics->T.v;
}

void mapper::set_node_transformation(nodeid id, const transformation & G)
{
    nodes[id].global_transformation = G;
}

void mapper::node_finished(nodeid id)
{
    nodes[id].status = node_status::finished;
    for (auto &word : nodes[id].frame->dbow_histogram)
        dbow_inverted_index[word.first].push_back(id); // Add this node to inverted index
}

mapper::nodes_path mapper::breadth_first_search(nodeid start, int maxdepth)
{
    nodes_path neighbor_nodes;
    if(!initialized())
        return neighbor_nodes;

    queue<node_path> next;
    next.push(node_path{start, transformation()});

    typedef int depth;
    unordered_map<nodeid, depth> nodes_visited;
    nodes_visited[start] = 0;

    while (!next.empty()) {
        node_path current_path = next.front();
        nodeid& u = current_path.first;
        transformation& Gu = current_path.second;
        next.pop();
        if(!maxdepth || nodes_visited[u] < maxdepth) {
            for(auto edge : nodes[u].edges) {
                const nodeid& v = edge.first;
                if(nodes_visited.find(v) == nodes_visited.end()) {
                    nodes_visited[v] = nodes_visited[u] + 1;
                    const transformation& Gv = edge.second.G;
                    next.push(node_path{v, Gu*Gv});
                }
            }
        }
        neighbor_nodes.insert(current_path);
    }

    return neighbor_nodes;
}

mapper::nodes_path mapper::breadth_first_search(nodeid start, set<nodeid>&& searched_nodes)
{
    nodes_path searched_nodes_path;
    if(!initialized())
        return searched_nodes_path;

    queue<node_path> next;
    next.push(node_path{start, transformation()});

    unordered_set<nodeid> nodes_visited;
    nodes_visited.insert(start);

    while (!next.empty() && !searched_nodes.empty()) {
        node_path current_path = next.front();
        nodeid& u = current_path.first;
        transformation& Gu = current_path.second;
        next.pop();
        for(auto edge : nodes[u].edges) {
            const nodeid& v = edge.first;
            if(nodes_visited.find(v) == nodes_visited.end()) {
                nodes_visited.insert(v);
                const transformation& Gv = edge.second.G;
                next.push(node_path{v, Gu*Gv});
            }
        }

        if(searched_nodes.find(u) != searched_nodes.end()) {
            searched_nodes_path.insert(current_path);
            searched_nodes.erase(u);
        }
    }

    if(next.empty())
        assert(searched_nodes.empty()); // If all graph has been traversed searched_nodes should be empty

    return searched_nodes_path;
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
    size_t min_num_shared_words = static_cast<int>(max_num_shared_words * 0.8f);
    std::vector<std::pair<mapper::nodeid, float> > dbow_scores;
    static_assert(orb_vocabulary::scoring_type == DBoW2::ScoringType::L1_NORM,
                  "orb_vocabulary does not use L1 norm");
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
    static constexpr int num_orientation_bins = 30;
    std::vector<mapper::matches> increment_orientation_histogram(num_orientation_bins);

    mapper::matches current_to_candidate_matches;

    if (candidate_frame->keypoints.size() > 0 && current_frame->keypoints.size() > 0) {
        auto match = [&current_frame, &candidate_frame, &features_dbow, &increment_orientation_histogram](
                const std::vector<unsigned int>& current_keypoint_indexes,
                const std::vector<unsigned int>& candidate_keypoint_indexes) {
            for (unsigned int current_point_idx : current_keypoint_indexes) {
                int best_candidate_point_idx;
                int best_distance = std::numeric_limits<int>::max();
                int second_best_distance = std::numeric_limits<int>::max();
                auto& current_keypoint = *current_frame->keypoints[current_point_idx];
                for (unsigned int candidate_point_idx : candidate_keypoint_indexes) {
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
        };

        if (candidate_frame->dbow_direct_file.empty() && current_frame->dbow_direct_file.empty()) {
            // not using dbow direct file to prefilter matches
            auto fill_with_indices = [](size_t N) {
                std::vector<unsigned int> v;
                v.reserve(N);
                for (size_t i = 0; i < N; ++i) v.push_back(i);
                return v;
            };
            std::vector<unsigned int> current_keypoint_indexes = fill_with_indices(current_frame->keypoints.size());
            std::vector<unsigned int> candidate_keypoint_indexes = fill_with_indices(candidate_frame->keypoints.size());
            match(current_keypoint_indexes, candidate_keypoint_indexes);
        } else {
            // dbow direct file is used
            for (auto it_candidate = candidate_frame->dbow_direct_file.begin();
                 it_candidate != candidate_frame->dbow_direct_file.end(); ++it_candidate) {
                auto it_current = current_frame->dbow_direct_file.find(it_candidate->first);
                if (it_current != current_frame->dbow_direct_file.end()) {
                    const auto& candidate_keypoint_indexes = it_candidate->second;
                    const auto& current_keypoint_indexes = it_current->second;
                    match(current_keypoint_indexes, candidate_keypoint_indexes);
                }
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
    const size_t min_num_inliers = 12;
    size_t best_num_inliers = 0;
    size_t i = 0;

    std::vector<std::pair<nodeid, float>> candidate_nodes =
        find_loop_closing_candidates(current_frame, nodes, dbow_inverted_index, orb_voc.get());
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
            auto G_candidate_neighbors = breadth_first_search(nid.first, 1); // all points observed from this candidate node should be created at a node 1 edge away in the graph

            const auto &keypoint_candidates = nodes[nid.first].frame->keypoints;
            for (auto m : matches_node_candidate) {
                auto &candidate = *keypoint_candidates[m.second];
                uint64_t keypoint_id = candidate.id;
                nodeid nodeid_keypoint = features_dbow[keypoint_id];

                // NOTE: We use 3d features observed from candidate, this does not mean
                // these features belong to the candidate node (group)
                v3 p_candidate = G_candidate_neighbors[nodeid_keypoint] *
                        get_feature3D(nodeid_keypoint, keypoint_id); // feat is in body frame
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

std::unique_ptr<orb_vocabulary> mapper::create_vocabulary_from_map(int branching_factor, int depth_levels) const {
    std::unique_ptr<orb_vocabulary> voc;
    if (branching_factor > 1 && depth_levels > 0 && !nodes.empty()) {
        voc.reset(new orb_vocabulary);
        voc->train(nodes.begin(), nodes.end(),
                   [](const map_node& node) -> const std::vector<std::shared_ptr<fast_tracker::fast_feature<orb_descriptor>>>& {
                       return node.frame->keypoints;
                   },
                   [](const std::shared_ptr<fast_tracker::fast_feature<orb_descriptor>>& feature) -> const orb_descriptor::raw& {
                       return feature->descriptor.descriptor;
                   },
            branching_factor, depth_levels);
    }
    return voc;
}

void mapper::predict_map_features(const uint64_t camera_id_now, const transformation& G_Bcurrent_Bnow) {
    map_feature_tracks.clear();
    G_neighbors_now.clear();
    // predict features from all nodes that are 1 edge away from current_node_id in the map.
    // increasing this value will try to bring back groups that are not directly connected to current_node_id.
    const state_extrinsics* const extrinsics_now = camera_extrinsics[camera_id_now];
    const state_vision_intrinsics* const intrinsics_now = camera_intrinsics[camera_id_now];
    nodes_path neighbors = breadth_first_search(current_node_id, 1);
    transformation G_CB = invert(transformation(extrinsics_now->Q.v, extrinsics_now->T.v));

    transformation G_Bnow_Bcurrent = invert(G_Bcurrent_Bnow);
    for(const auto& neighbor : neighbors) {
        map_node& node_neighbor = nodes[neighbor.first];
        if(node_neighbor.status == node_status::normal)
            continue; // if node status is normal then node is already in the filter
        std::list<tracker::feature_track> tracks;
        const transformation& G_Bnow_Bneighbor = G_Bnow_Bcurrent*neighbor.second;
        transformation G_Cnow_Bneighbor = G_CB*G_Bnow_Bneighbor;
        for(const auto& f : node_neighbor.features) {
            // predict feature in current camera pose
            v3 p3dC = G_Cnow_Bneighbor * get_feature3D(neighbor.first, f.second.feature->id);
            if(p3dC.z() < 0)
                continue;
            feature_t kpn = p3dC.segment<2>(0)/p3dC.z();
            feature_t kpd = intrinsics_now->unnormalize_feature(intrinsics_now->distort_feature(kpn));
            if(kpd.x() < 0 || kpd.x() > intrinsics_now->image_width ||
               kpd.y() < 0 || kpd.y() > intrinsics_now->image_height)
                continue;

            // create feature track
            tracks.emplace_back(f.second.feature, kpd.x(), kpd.y(), 0);
            tracks.back().depth = f.second.depth;
        }
        map_feature_tracks.emplace_back(neighbor.first, std::move(tracks));
        G_neighbors_now[neighbor.first] = invert(G_Bnow_Bneighbor);
    }
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
#define KEY_FEATURE_DEPTH "depth"
#define KEY_FEATURE_INITIAL_XY "xy"
#define KEY_FRAME_FEAT_PATCH "patch"
#define KEY_FRAME_FEAT_DESC_RAW "raw"
void map_feature::serialize(Value &feature_json, Document::AllocatorType &allocator) {
    feature_json.AddMember(KEY_FEATURE_ID, feature->id, allocator);
    feature_json.AddMember(KEY_FEATURE_DEPTH, depth, allocator);
    feature_json.AddMember(KEY_FEATURE_VARIANCE, variance, allocator);

    // add feature initial coordinates
    Value xy_json(kArrayType);
    xy_json.PushBack(feature->x, allocator);
    xy_json.PushBack(feature->y, allocator);
    feature_json.AddMember(KEY_FEATURE_INITIAL_XY, xy_json, allocator);

    // add feature descriptor
    Value desc_json(kObjectType);
    Value desc_raw_json(kArrayType);
    for (auto v : feature->descriptor.descriptor)
        desc_raw_json.PushBack(v, allocator);

    desc_json.AddMember(KEY_FRAME_FEAT_DESC_RAW, desc_raw_json, allocator);
    feature_json.AddMember(KEY_FRAME_FEAT_PATCH, desc_json, allocator);
}

bool map_feature::deserialize(const Value &json, map_feature &feature, uint64_t &max_loaded_featid) {
    uint64_t id = json[KEY_FEATURE_ID].GetUint64();
    if (id > max_loaded_featid) max_loaded_featid = id;
    feature.depth = (float)json[KEY_FEATURE_DEPTH].GetDouble();

    // get feature initial coordinates
    const Value &xy_json = json[KEY_FEATURE_INITIAL_XY];
    RETURN_IF_FAILED(xy_json.IsArray())
            RETURN_IF_FAILED(xy_json.Size() == 2)
            v2 xy;
            for (SizeType j = 0; j < 2; ++j) {
                xy[j] = (f_t) xy_json[j].GetDouble();
            }
    // get descriptor values
    const Value &desc_json = json[KEY_FRAME_FEAT_PATCH];
    const Value &desc_raw_json = desc_json[KEY_FRAME_FEAT_DESC_RAW];
    RETURN_IF_FAILED(desc_raw_json.IsArray())
    std::array<unsigned char, patch_descriptor::L> raw_desc;
    RETURN_IF_FAILED(raw_desc.size() == desc_raw_json.Size())
    for (SizeType d = 0; d < desc_raw_json.Size(); d++)
        raw_desc[d] = static_cast<unsigned char>(desc_raw_json[d].GetUint64());

    feature.feature = std::make_shared<fast_tracker::fast_feature<patch_descriptor>>(id, xy[0], xy[1], raw_desc);
    return true;
}

#define KEY_FRAME_FEAT "features"
#define KEY_FRAME_FEAT_ID "id"
#define KEY_FRAME_FEAT_X "x"
#define KEY_FRAME_FEAT_Y "y"
#define KEY_FRAME_FEAT_LEVEL "level"
#define KEY_FRAME_FEAT_DESC "descriptor"
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
    node.status = node_status::finished;
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
    for (size_t i = 0; i < nodes.size(); i++) {
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
        cur_node.frame->calculate_dbow(map.orb_voc.get()); // populate map_frame's dbow_histogram and dbow_direct_file
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

