// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.
// Modified by Pedro Pinies and Lina Paz

#include <iostream>
#include <algorithm>
#include <assert.h>
#include <limits.h>
#include <mutex>
#include <queue>
#include <unordered_set>
#include "mapper.h"
#include "resource.h"
#include "state_vision.h"
#include "quaternion.h"
#include "fast_tracker.h"
#include "descriptor.h"
#include "Trace.h"

#ifdef RELOCALIZATION_DEBUG
#include <opencv2/imgproc.hpp>
#include "debug/visual_debug.h"
#define IMAGE_SHOW_MAP
//#define SHOW_ALL_CANDIDATES
#endif

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
    triangulated_tracks.clear();
    if(current_node) return;
    log->debug("Map reset");
    feature_id_offset = 0;
    node_id_offset = 0;
    unlinked = false;
    nodes.critical_section([&](){ nodes->clear(); });
    features_dbow.critical_section([&](){ features_dbow->clear(); });
    dbow_inverted_index.clear();
    partially_finished_nodes.clear();
}

map_edge &map_node::get_add_neighbor(mapper::nodeid neighbor)
{
    return edges.emplace(neighbor, map_edge{}).first->second;
}

void mapper::add_edge(nodeid id1, nodeid id2, const transformation& G12, edge_type type) {
    nodes.critical_section(&mapper::add_edge_no_lock, this, id1, id2, G12, type);
}

void mapper::add_edge_no_lock(nodeid id1, nodeid id2, const transformation &G12, edge_type type) {
    map_edge& edge12 = nodes->at(id1).get_add_neighbor(id2);
    edge12.G = G12;
    map_edge& edge21 = nodes->at(id2).get_add_neighbor(id1);
    edge21.G = invert(G12);
    if(type != edge_type::original) {
        edge12.type = type;
        edge21.type = type;
    }
}

void mapper::add_covisibility_edge(nodeid id1, nodeid id2) {
    nodes.critical_section(&mapper::add_covisibility_edge_no_lock, this, id1, id2);
}

void mapper::add_covisibility_edge_no_lock(nodeid id1, nodeid id2) {
    nodes->at(id1).covisibility_edges.insert(id2);
    nodes->at(id2).covisibility_edges.insert(id1);
}

void mapper::remove_edge(nodeid id1, nodeid id2) {
    nodes.critical_section(&mapper::remove_edge_no_lock, this, id1, id2);
}

void mapper::remove_edge_no_lock(nodeid id1, nodeid id2) {
    nodes->at(id1).edges.erase(id2);
    nodes->at(id2).edges.erase(id1);
}

void mapper::add_node(nodeid id, const rc_Sensor camera_id) {
    nodes.critical_section([&]() {
        (*nodes)[id].id = id;
        (*nodes)[id].camera_id = camera_id;
    });
}

void mapper::initialize_track_triangulation(const tracker::feature_track& track, const nodeid node_id) {
    // init state
    if(triangulated_tracks.find(track.feature->id) != triangulated_tracks.end())
        return;
    std::shared_ptr<log_depth> state_d = std::make_shared<log_depth>();
    v2 xd = {track.x, track.y};
    state_d->initial = xd;
    triangulated_track tt(node_id, state_d);
    triangulated_tracks.emplace(track.feature->id, std::move(tt));
}

void mapper::finish_lost_tracks(const tracker::feature_track& track) {
    auto tp = triangulated_tracks.find(track.feature->id);
    if (tp != triangulated_tracks.end()) {
        auto f = std::static_pointer_cast<fast_tracker::fast_feature<DESCRIPTOR>>(track.feature);
        if (tp->second.track_count > MIN_FEATURE_TRACKS && tp->second.parallax > MIN_FEATURE_PARALLAX) {
            std::unique_lock<std::mutex> lock1(nodes.mutex(), std::defer_lock);
            std::unique_lock<std::mutex> lock2(features_dbow.mutex(), std::defer_lock);
            std::lock(lock1, lock2);
            add_triangulated_feature_to_group(tp->second.reference_nodeid, f, tp->second.state);
        }
        triangulated_tracks.erase(track.feature->id);
    } else {
        log->debug("{}/{}) Not enougth support/parallax to add trianguled point with id: {}", track.feature->id);
    }
}

void mapper::remove_node_features(nodeid id) {
    for(auto& f : nodes->at(id).features)
        features_dbow->erase(f.first);
}

void mapper::add_triangulated_feature_to_group(const nodeid group_id, const std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>>& feature,
                                               const std::shared_ptr<log_depth>& v) {
    map_node &ref_node = nodes->at(group_id);
    ref_node.add_feature(feature, v, feature_type::triangulated);
    (*features_dbow)[feature->id] = group_id;
}

void mapper::update_3d_feature(const tracker::feature_track& track, const transformation &&G_Bnow_Bcurrent, const rc_Sensor camera_id_now) {

    auto tp = triangulated_tracks.find(track.feature->id);

    const state_vision_intrinsics* const intrinsics_reference = camera_intrinsics[get_node(tp->second.reference_nodeid).camera_id];
    const state_vision_intrinsics* const intrinsics_now = camera_intrinsics[camera_id_now];
    const state_extrinsics* const extrinsics_reference = camera_extrinsics[get_node(tp->second.reference_nodeid).camera_id];
    const state_extrinsics* const extrinsics_now = camera_extrinsics[camera_id_now];

    const f_t focal_px = intrinsics_now->focal_length.v * intrinsics_now->image_height;
    const f_t sigma2 = 10 / (focal_px*focal_px);

    // distance # edges traversed
    auto distance = [](const map_edge& edge) { return 1; };
    // select node if it is the searched node
    auto is_node_searched = [&tp](const node_path& path) { return path.id == tp->second.reference_nodeid; };
    // finish search when node is found
    auto finish_search = is_node_searched;

    nodes_path searched_node = dijkstra_shortest_path(node_path{current_node->id, transformation(), 0},
                                                      distance, is_node_searched, finish_search);
    assert(searched_node.size() == 1);
    auto& G_Bcurrent_Breference = searched_node.front().G;
    transformation G_CBnow = invert(transformation(extrinsics_now->Q.v, extrinsics_now->T.v));
    transformation G_BCreference = transformation(extrinsics_reference->Q.v, extrinsics_reference->T.v);

    transformation G_Ck_Ck_1 = G_CBnow * G_Bnow_Bcurrent * G_Bcurrent_Breference * G_BCreference;

    // calculate point prediction on current camera frame
    std::shared_ptr<log_depth>& state = tp->second.state;
    v2 xun = intrinsics_reference->undistort_feature(intrinsics_reference->normalize_feature(state->initial));
    v3 xun_k_1 = xun.homogeneous();
    float P = tp->second.cov;
    v3 pk_1 = xun_k_1 * state->depth();
    m3 Rk_k_1 = G_Ck_Ck_1.Q.toRotationMatrix();
    v3 pk = Rk_k_1 * pk_1 + G_Ck_Ck_1.T;
    // features are in front of the camera
    if(pk.z() < 0.f)
        return;
    v2 hk = pk.segment<2>(0)/pk.z();
    v2 zdk = {track.x,track.y};
    v2 zuk = intrinsics_now->undistort_feature(intrinsics_now->normalize_feature((zdk)));
    v2 inn_k = zuk-hk;

    // compute jacobians
    m<2,3> dhk_dpk = {{1/pk[2], 0,       -pk[0]/(pk[2]*pk[2])},
                      {0,       1/pk[2], -pk[1]/(pk[2]*pk[2])}};
    m3 dpk_dpk_1 = Rk_k_1;
    v3 dpk_1_dv = pk_1;
    v2 H = dhk_dpk * dpk_dpk_1 * dpk_1_dv;

    // update state and variance
    m<2,2> R = {{sigma2,0},{0,sigma2}};
    m<1,2> PH_t = P*H.transpose();
    m<2,2> S = H*PH_t + R;
    Eigen::LLT<Eigen::Matrix2f> Sllt = S.llt();
    m<1,2> K =  Sllt.solve(PH_t.transpose()).transpose();

    // check mahalanobis distance to remove outliers
    tp->second.parallax = std::acos(xun_k_1.dot(zuk.homogeneous())/(xun_k_1.norm() *zuk.homogeneous().norm()));
    if (inn_k.dot(Sllt.solve(inn_k)) > 5.99f) {
        return;
    }
    tp->second.track_count++;

    // update state
    state->v = state->v + K*inn_k;
    P -= K * PH_t.transpose();
    tp->second.cov = P;
}

void map_node::add_feature(std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature,
                           std::shared_ptr<log_depth> v, const feature_type type) {
    map_feature mf;
    mf.v = v;
    mf.feature = feature;
    mf.type = type;
    features.emplace(feature->id, mf);
}

void map_node::set_feature_type(const uint64_t id, const feature_type type)
{
    features[id].type = type;
}

void mapper::add_feature(nodeid groupid, std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature,
                         std::shared_ptr<log_depth> v, const feature_type type) {
    nodes.critical_section([&]() {
        nodes->at(groupid).add_feature(feature, v, type);
    });
    features_dbow.critical_section([&]() {
        (*features_dbow)[feature->id] = groupid;
    });
}

void mapper::set_feature_type(nodeid groupid, uint64_t id, const feature_type type) {
    nodes->at(groupid).set_feature_type(id, type);
}

v3 mapper::get_feature3D(nodeid node_id, uint64_t feature_id) const {
    const map_node& node = nodes->at(node_id);
    auto mf = node.features.at(feature_id);
    auto intrinsics = camera_intrinsics[node.camera_id];
    auto extrinsics = camera_extrinsics[node.camera_id];
    m3 Rbc = extrinsics->Q.v.toRotationMatrix();
    v2 pn = intrinsics->undistort_feature(intrinsics->normalize_feature(mf.v->initial));
    return Rbc*(mf.v->depth()*pn.homogeneous()) + extrinsics->T.v;
}

void mapper::set_node_transformation(nodeid id, const transformation & G)
{
    nodes.critical_section([&]() {
        nodes->at(id).global_transformation = G;
    });
}

void mapper::finish_node(nodeid id, bool compute_dbow_inverted_index) {
    auto& node = nodes->at(id);
    nodes.critical_section([&]() {
        node.status = node_status::finished;
    });
    if (compute_dbow_inverted_index) partially_finished_nodes.emplace(id);
}

void mapper::index_finished_nodes() {
    for (auto it = partially_finished_nodes.begin(); it != partially_finished_nodes.end(); ) {
        nodeid id = *it;
        bool update_index = false, remove_node = false;
        auto node_it = nodes->find(id);
        if (node_it == nodes->end()) {
            remove_node = true;
        } else if (node_it->second.frame) {
            update_index = true;
            remove_node = true;
        }
        if (update_index) {
            for (auto &word : node_it->second.frame->dbow_histogram)
                dbow_inverted_index[word.first].push_back(id);
        }
        if (remove_node) {
            it = partially_finished_nodes.erase(it);
        } else {
            ++it;
        }
    }
}

void mapper::remove_node(nodeid id)
{
    // if node removed is current_node
    bool update_current_node = false;
    if(current_node->id == id)
        update_current_node = true;

    std::lock_guard<std::mutex> lock(nodes.mutex());
    std::map<uint64_t, map_edge> edges = nodes->at(id).edges;
    std::set<nodeid> neighbors;
    assert(edges.size());
    for(auto& edge : edges) {
        if(update_current_node) {
            current_node = &nodes->at(edge.first);
            if(current_node->status == node_status::normal) // prefer an active node as current_node
                update_current_node = false;
        }
        neighbors.insert(edge.first);
        remove_edge_no_lock(id, edge.first);
    }
    features_dbow.critical_section(&mapper::remove_node_features, this, id);
    nodes->erase(id);
    partially_finished_nodes.erase(id);

    // if only 1 neighbor, node removed (id) is a loose end of the graph
    if(neighbors.size() > 1) {
        auto searched_neighbors = neighbors;
        // distance # edges traversed
        auto distance = [](const map_edge& edge) { return 1; };
        // select node if it is one of the searched nodes
        auto is_node_searched = [&searched_neighbors](const node_path& path) {
            auto it = searched_neighbors.find(path.id);
            if( it != searched_neighbors.end()) {
                searched_neighbors.erase(it);
                return true;
            } else
                return false;
            };
        // finish search when all searched nodes are found
        auto finish_search = [&searched_neighbors](const node_path& path) { return searched_neighbors.empty(); };

        // this could be more efficient if we don't calculate transformations in dijkstra
        std::set<nodeid> connected_neighbors;
        for(auto &path : dijkstra_shortest_path(node_path{*neighbors.begin(), transformation(), 0}, distance, is_node_searched, finish_search))
            connected_neighbors.insert(path.id);

        // are neighbors disconnected after removing node id ?
        if(connected_neighbors.size() < neighbors.size()) {
            std::vector<nodeid> disconnected_neighbors;
            std::set_difference(neighbors.begin(), neighbors.end(), connected_neighbors.begin(), connected_neighbors.end(), std::back_inserter(disconnected_neighbors));

            transformation G_id_connected = edges[*connected_neighbors.begin()].G;
            transformation G_id_disconnected = edges[disconnected_neighbors.front()].G; // pick one of the disconnected nodes
            add_edge_no_lock(*connected_neighbors.begin(), disconnected_neighbors.front(), invert(G_id_connected)*G_id_disconnected);
        }
    }
}

mapper::nodes_path mapper::dijkstra_shortest_path(const node_path& start, std::function<float(const map_edge& edge)> distance, std::function<bool(const node_path& path)> is_node_searched,
                                                  std::function<bool(const node_path& path)> finish_search)
{
    auto cmp = [](const node_path& path1, const node_path& path2) {
      return path1.distance > path2.distance;
    };
    priority_queue<node_path, std::vector<node_path>, decltype(cmp)> next(cmp);

    next.push(start);
    unordered_set<nodeid> nodes_done;
    nodes_path searched_nodes;
    bool stop = false;
    while(!stop && !next.empty()) {
        node_path path = next.top();
        next.pop();
        const nodeid& u = path.id;
        if(nodes_done.find(u) == nodes_done.end()) {
            nodes_done.insert(u);
            if(is_node_searched(path)) {
                searched_nodes.push_back(path);
                stop = finish_search(path);
            }
            const transformation& G_start_u = path.G;
            const float& distance_u = path.distance;
            for(auto edge : nodes->at(u).edges) {
                const nodeid& v = edge.first;
                const transformation& G_u_v = edge.second.G;
                f_t distance_uv = distance(edge.second);
                next.emplace(v, G_start_u*G_u_v, distance_u + distance_uv);
            }
        }
    }

    return searched_nodes;
}

std::vector<std::pair<mapper::nodeid,float>> mapper::find_loop_closing_candidates(
    const std::shared_ptr<frame_t>& current_frame)
{
    std::vector<std::pair<mapper::nodeid, float>> loop_closing_candidates;
    // find nodes sharing words with current frame
    std::map<mapper::nodeid,uint32_t> common_words_per_node;
    uint32_t max_num_shared_words = 0;
    for (auto word : current_frame->dbow_histogram) {
        auto word_i = dbow_inverted_index.find(word.first);
        if (word_i == dbow_inverted_index.end()) continue;
        std::vector<mapper::nodeid> nodeids = word_i->second;
        nodes.critical_section([&]() {
            for (size_t i = 0; i < nodeids.size(); ) {
                mapper::nodeid nid = nodeids[i];
                auto it = nodes->find(nid);
                bool remove = (it == nodes->end() || it->second.status != node_status::finished);
                if (remove) {
                    std::swap(nodeids[i], nodeids.back());
                    nodeids.pop_back();
                } else {
                    ++i;
                }
            }
        });
        for (auto nid : nodeids) {
            common_words_per_node[nid]++;
            // keep maximum number of words shared with current frame
            if (max_num_shared_words < common_words_per_node[nid]) {
                max_num_shared_words = common_words_per_node[nid];
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
    std::vector<std::pair<mapper::nodeid, std::shared_ptr<frame_t>>> candidate_frames;
    candidate_frames.reserve(common_words_per_node.size());
    nodes.critical_section([&]() {
        for (auto& node_candidate : common_words_per_node) {
            if (node_candidate.second > min_num_shared_words) {
                auto it = nodes->find(node_candidate.first);
                if (it != nodes->end())
                    candidate_frames.emplace_back(node_candidate.first, it->second.frame);
            }
        }
    });
    for (auto& node_candidate : candidate_frames) {
        float dbow_score = static_cast<float>(orb_voc->score(node_candidate.second->dbow_histogram,
                                                             current_frame->dbow_histogram));
        dbow_scores.push_back(std::make_pair(node_candidate.first, dbow_score));
        best_score = std::max(dbow_score, best_score);
    }

    // sort candidates by dbow_score
    auto compare_dbow_scores = [](const std::pair<mapper::nodeid, float>& p1, const std::pair<mapper::nodeid, float>& p2) {
        return (p1.second > p2.second);
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

mapper::matches mapper::match_2d_descriptors(const std::shared_ptr<frame_t>& candidate_frame, const std::shared_ptr<frame_t>& current_frame) {
    //matches per orientationn increment between current frame and node candidate
    static constexpr int num_orientation_bins = 30;
    std::vector<mapper::matches> increment_orientation_histogram(num_orientation_bins);

    mapper::matches current_to_candidate_matches;

    if (candidate_frame->keypoints.size() > 0 && current_frame->keypoints.size() > 0) {
        auto match = [&current_frame, &candidate_frame, &increment_orientation_histogram](
                const std::vector<size_t>& current_keypoint_indexes,
                const std::vector<size_t>& candidate_keypoint_indexes) {
            std::unordered_map<size_t, std::pair<size_t, int>> matches;  // candidate -> current, distance
            for (auto current_point_idx : current_keypoint_indexes) {
                size_t best_candidate_point_idx = 0;
                auto best_distance = std::numeric_limits<float>::max();
                auto second_best_distance = std::numeric_limits<float>::max();
                auto& current_keypoint = *current_frame->keypoints[current_point_idx];
                for (auto candidate_point_idx : candidate_keypoint_indexes) {
                    auto& candidate_keypoint = *candidate_frame->keypoints[candidate_point_idx];
                    float dist = current_keypoint.descriptor.distance_reloc(candidate_keypoint.descriptor,
                                                                            current_keypoint.descriptor);
                    if (dist < best_distance) {
                        second_best_distance = best_distance;
                        best_distance = dist;
                        best_candidate_point_idx = candidate_point_idx;
                    } else if (dist < second_best_distance) {
                        second_best_distance = dist;
                    }
                }

                if (best_distance < second_best_distance * 0.8f) {
                    auto it = matches.find(best_candidate_point_idx);
                    if (it == matches.end()) {
                        matches.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(best_candidate_point_idx),
                                        std::forward_as_tuple(current_point_idx, best_distance));
                    } else if (best_distance < it->second.second) {
                        it->second = {current_point_idx, best_distance};
                    }
                }
            }
            for (const auto& m : matches) {
                size_t best_candidate_point_idx = m.first;
                size_t current_point_idx = m.second.first;
                auto& current_keypoint = *current_frame->keypoints[current_point_idx];
                auto& best_candidate_keypoint = *candidate_frame->keypoints[best_candidate_point_idx];
                size_t bin = calculate_orientation_bin(best_candidate_keypoint.descriptor.orb,
                                                       current_keypoint.descriptor.orb,
                                                       num_orientation_bins);
                increment_orientation_histogram[bin].emplace_back(current_point_idx, best_candidate_point_idx);
            }
        };

        if (candidate_frame->dbow_direct_file.empty() && current_frame->dbow_direct_file.empty()) {
            // not using dbow direct file to prefilter matches
            auto indexes_all = [](size_t N) {
                std::vector<size_t> v;
                v.reserve(N);
                for (size_t i = 0; i < N; ++i) v.push_back(i);
                return v;
            };
            auto indexes_with_3d = [this](
                    const std::vector<std::shared_ptr<fast_tracker::fast_feature<patch_orb_descriptor>>>& keypoints) {
                std::vector<size_t> v;
                v.reserve(keypoints.size());
                features_dbow.critical_section([&]() {
                    for (size_t i = 0; i < keypoints.size(); ++i) {
                        if (features_dbow->find(keypoints[i]->id) != features_dbow->end()) {
                            v.push_back(i);
                        }
                    }
                });
                return v;
            };
            std::vector<size_t> current_keypoint_indexes = indexes_all(current_frame->keypoints.size());
            std::vector<size_t> candidate_keypoint_indexes = indexes_with_3d(candidate_frame->keypoints);
            match(current_keypoint_indexes, candidate_keypoint_indexes);
        } else {
            // dbow direct file is used
            auto indexes_with_3d = [this](
                    const std::vector<std::shared_ptr<fast_tracker::fast_feature<patch_orb_descriptor>>>& keypoints,
                    const std::vector<size_t>& keypoint_indices) {
                std::vector<size_t> v;
                v.reserve(keypoint_indices.size());
                features_dbow.critical_section([&]() {
                    for (size_t i : keypoint_indices) {
                        if (features_dbow->find(keypoints[i]->id) != features_dbow->end()) {
                            v.push_back(i);
                        }
                    }
                });
                return v;
            };
            for (auto it_candidate = candidate_frame->dbow_direct_file.begin();
                 it_candidate != candidate_frame->dbow_direct_file.end(); ++it_candidate) {
                auto it_current = current_frame->dbow_direct_file.find(it_candidate->first);
                if (it_current != current_frame->dbow_direct_file.end()) {
                    auto candidate_keypoint_indexes = indexes_with_3d(candidate_frame->keypoints, it_candidate->second);
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
    const float confidence = 0.9f; //0.9
    std::minstd_rand rng(-1);
    transformation G_currentframeC_candidateB;
    estimate_transformation(points_3d, points_2d, G_currentframeC_candidateB, rng, max_iter, max_reprojection_error, confidence, 5, &inliers_set);
    G_candidateB_currentframeB = invert(G_BC*G_currentframeC_candidateB);
}

map_relocalization_info mapper::relocalize(const camera_frame_t& camera_frame) {
    // note: mapper::relocalize can run in parallel to other mapper functions
    const std::shared_ptr<frame_t>& current_frame = camera_frame.frame;
    map_relocalization_info reloc_info;
    reloc_info.frame_timestamp = current_frame->timestamp;
    if (!current_frame->keypoints.size()) return reloc_info;

    const size_t min_num_inliers = 12;
    size_t best_num_inliers = 0;
    size_t i = 0;

    START_EVENT(SF_FIND_CANDIDATES, 0);
    std::vector<std::pair<nodeid, float>> candidate_nodes = find_loop_closing_candidates(current_frame);
    END_EVENT(SF_FIND_CANDIDATES, candidate_nodes.size());
    reloc_info.rstatus = relocalization_status::find_candidates;

    const auto &keypoint_xy_current = current_frame->keypoints_xy;
    state_vision_intrinsics* const intrinsics = camera_intrinsics[camera_frame.camera_id];
    for (const auto& nid : candidate_nodes) {
        bool is_relocalized_in_candidate = false;
        std::shared_ptr<frame_t> candidate_node_frame;
        transformation candidate_node_global_transformation;

        bool ok = nodes.critical_section([&]() {
            auto it = nodes->find(nid.first);
            if (it != nodes->end()) {
                candidate_node_frame = it->second.frame;
                candidate_node_global_transformation = it->second.global_transformation;
                return true;
            }
            return false;
        });
        if (!ok) continue;

        START_EVENT(SF_MATCH_DESCRIPTORS, 0);
        matches matches_node_candidate = match_2d_descriptors(candidate_node_frame, current_frame);
        END_EVENT(SF_MATCH_DESCRIPTORS, matches_node_candidate.size());
        reloc_info.rstatus = relocalization_status::match_descriptors;

        // Just keep candidates with more than a min number of mathces
        std::set<size_t> inliers_set;
        if(matches_node_candidate.size() >= min_num_inliers) {
            aligned_vector<v3> candidate_3d_points;
            aligned_vector<v2> current_2d_points;
            transformation G_candidate_currentframe;
            // Estimate pose from 3d-2d matches
            std::unordered_map<nodeid, transformation> G_candidate_neighbors;
            ok = nodes.critical_section([&](){
                auto it_node = nodes->find(nid.first);
                if (it_node != nodes->end()) {
                    auto covisible_neighbors = it_node->second.covisibility_edges; // some of trhe points observed from this candidate node should be found in the covisible nodes
                    // distance # edges traversed
                    auto distance = [](const map_edge& edge) { return 1; };
                    // select node if it is one of the covisible nodes
                    auto is_node_searched = [&covisible_neighbors](const node_path& path) {
                        auto it_neighbor = covisible_neighbors.find(path.id);
                        if( it_neighbor != covisible_neighbors.end()) {
                            covisible_neighbors.erase(it_neighbor);
                            return true;
                        } else
                            return false;
                        };
                    // finish search when all covisible nodes are found
                    auto finish_search = [&covisible_neighbors](const node_path& path) { return covisible_neighbors.empty(); };

                    G_candidate_neighbors[nid.first] = transformation();
                    for(auto &path : dijkstra_shortest_path(node_path{nid.first, transformation(), 0}, distance, is_node_searched, finish_search))
                        G_candidate_neighbors[path.id] = path.G;
                    return true;
                }
                return false;
            });
            if (!ok) continue;

            const auto &keypoint_candidates = candidate_node_frame->keypoints;
            std::vector<std::tuple<uint64_t, uint64_t, nodeid>> candidate_features;  // current_feature_id, candidate_feature_id, node_containing_candidate_feature_id
            features_dbow.critical_section([&]() {
                for (auto m : matches_node_candidate) {
                    auto &candidate = *keypoint_candidates[m.second];
                    uint64_t keypoint_id = candidate.id;
                    auto it = features_dbow->find(keypoint_id);
                    if (it != features_dbow->end()) {
                        candidate_features.emplace_back(m.first, it->first, it->second);
                    }
                }
            });
            nodes.critical_section([&]() {
                for (auto& f : candidate_features) {
                    uint64_t current_feature_id = std::get<0>(f);
                    uint64_t keypoint_id = std::get<1>(f);
                    nodeid nodeid_keypoint = std::get<2>(f);
                    if (nodes->find(nodeid_keypoint) != nodes->end()) {
                        // NOTE: We use 3d features observed from candidate, this does not mean
                        // these features belong to the candidate node (group)
                        auto it_G = G_candidate_neighbors.find(nodeid_keypoint);
                        if(it_G == G_candidate_neighbors.end()) {// TODO: all features observed from the candidate node should be represented wrt one of the covisible nodes but it does not happen ....
                            continue;
                        }
                        v3 p_candidate = it_G->second *
                                get_feature3D(nodeid_keypoint, keypoint_id); // feat is in body frame
                        candidate_3d_points.push_back(p_candidate);
                        //undistort keypoints at current frame
                        feature_t ukp = intrinsics->undistort_feature(intrinsics->normalize_feature(keypoint_xy_current[current_feature_id]));
                        current_2d_points.push_back(ukp);
                    }
                }
            });
            inliers_set.clear();
            START_EVENT(SF_ESTIMATE_POSE,0);
            estimate_pose(candidate_3d_points, current_2d_points, camera_frame.camera_id, G_candidate_currentframe, inliers_set);
            END_EVENT(SF_ESTIMATE_POSE,inliers_set.size());
            reloc_info.rstatus = relocalization_status::estimate_EPnP;
            if(inliers_set.size() >= min_num_inliers) {
                reloc_info.is_relocalized = true;
                is_relocalized_in_candidate = true;
                if(inliers_set.size() > best_num_inliers) {
                    transformation G_candidate_closestnode = G_candidate_currentframe*invert(camera_frame.G_closestnode_frame);
                    ok = nodes.critical_section([&]() {
                        if (nodes->find(nid.first) != nodes->end() && nodes->find(camera_frame.closest_node) != nodes->end()) {
                            add_edge_no_lock(nid.first, camera_frame.closest_node, G_candidate_closestnode, edge_type::relocalization);
                            add_covisibility_edge_no_lock(nid.first, camera_frame.closest_node);
                            return true;
                        }
                        return false;
                    });
                    if (ok) {
                        best_num_inliers = inliers_set.size();
                        reloc_info.candidates.clear();
                        reloc_info.candidates.emplace_back(G_candidate_currentframe, candidate_node_global_transformation, candidate_node_frame->timestamp);
                    }
                }
            }
        }
        if (!inliers_set.size())
            log->debug("{}/{}) candidate nid: {:3} score: {:.5f}, matches: {:2}",
                       i++, candidate_nodes.size(), nid.first, nid.second, matches_node_candidate.size());
        else
            log->info(" {}/{}) candidate nid: {:3} score: {:.5f}, matches: {:2}, EPnP inliers: {}",
                      i++, candidate_nodes.size(), nid.first, nid.second, matches_node_candidate.size(), inliers_set.size());

        #ifndef SHOW_ALL_CANDIDATES
            if (!is_relocalized_in_candidate)
                continue;
        #endif

        #if defined(RELOCALIZATION_DEBUG) && defined(IMAGE_SHOW_MAP)
            std::string image_name = "Num candidates: " + std::to_string(candidate_nodes.size()) + ", DBoW candidate: " + std::to_string(nid.first);
            cv::Mat color_candidate_image, color_current_image;
            cv::cvtColor(candidate_node_frame->image, color_candidate_image, CV_GRAY2BGRA);
            cv::cvtColor(current_frame->image, color_current_image, CV_GRAY2BGRA);
            cv::Mat current_reprojection_image = color_current_image.clone();
            cv::Mat candidate_reprojection_image = color_candidate_image.clone();
            const auto &keypoint_xy_candidates = candidate_node_frame->keypoints_xy;

            cv::Mat flipped_color_candidate_image;
            cv::flip(color_candidate_image, flipped_color_candidate_image, 1);
            cv::putText(flipped_color_candidate_image,
                        "candidate image " + std::to_string(nid.first) + " " +  std::to_string(nid.second),
                        cv::Point(30,30),cv::FONT_HERSHEY_COMPLEX,0.8,cv::Scalar(255,0,0,255));
            cv::putText(flipped_color_candidate_image,"last node added " + std::to_string(nodes->size()),
                        cv::Point(30,60),cv::FONT_HERSHEY_COMPLEX,0.8,cv::Scalar(255,0,0,255));
            cv::putText(flipped_color_candidate_image,"number of matches " + std::to_string(matches_node_candidate.size()),
                        cv::Point(30,90),cv::FONT_HERSHEY_COMPLEX,0.8,cv::Scalar(255,0,0,255));
            cv::putText(flipped_color_candidate_image,"number of inliers " + std::to_string(inliers_set.size()),
                        cv::Point(30,120),cv::FONT_HERSHEY_COMPLEX,0.8,cv::Scalar(255,0,0,255));

            if(reloc_info.is_relocalized) {
                cv::putText(color_current_image,"RELOCALIZED",
                cv::Point(30,30),cv::FONT_HERSHEY_COMPLEX,0.8,cv::Scalar(0,255,0,255));
            } else {
                cv::putText(color_current_image,"RELOCALIZATION FAILED",
                cv::Point(30,30),cv::FONT_HERSHEY_COMPLEX,0.8,cv::Scalar(255,0,0,255));
            }

            int rows = color_current_image.rows;
            int cols = color_current_image.cols;
            int type = color_current_image.type();
            cv::Mat compound(rows, 2*cols, type);
            color_current_image.copyTo(compound(cv::Rect(0, 0, cols, rows)));
            flipped_color_candidate_image.copyTo(compound(cv::Rect(cols, 0, cols, rows)));
            int matchidx=0;
            for (auto point_match : matches_node_candidate) {
                auto& p1 = keypoint_xy_current[point_match.first];
                auto& p2 = keypoint_xy_candidates[point_match.second];
                cv::Scalar color{0,0,255,255};
                cv::Scalar color_line{0,0,255,255};
                if(inliers_set.count(matchidx)) {
                    color = cv::Scalar(0,255,0,255);
                    color_line = cv::Scalar(255,0,0,255);
                }
                cv::circle(compound,cv::Point(p1[0],p1[1]),3,color,2);
                cv::circle(compound,cv::Point(compound.cols-p2[0],p2[1]),3,color,2);
                cv::line(compound,cv::Point(p1[0],p1[1]),cv::Point(compound.cols-p2[0],p2[1]),color_line,2);
                matchidx++;
            }

            visual_debug::batch batch;
            batch.add(compound, "Relocalized");

            for(auto &pc : keypoint_xy_current) {
                cv::circle(color_current_image, cv::Point(pc[0],pc[1]),3,cv::Scalar{255,0,0,255},2);
            }
            batch.add(color_current_image, "current image keypoints");

            for(auto &pc : keypoint_xy_candidates) {
                cv::circle(color_candidate_image, cv::Point(pc[0],pc[1]),3,cv::Scalar{255,0,0,255},2);
            }
            batch.add(color_candidate_image, "candidate image keypoints");

            visual_debug::send(batch);
        #endif
    }
    return reloc_info;
}

std::unique_ptr<orb_vocabulary> mapper::create_vocabulary_from_map(int branching_factor, int depth_levels) const {
    std::unique_ptr<orb_vocabulary> voc;
    if (branching_factor > 1 && depth_levels > 0 && !nodes->empty()) {
        voc.reset(new orb_vocabulary);
        voc->train(nodes->begin(), nodes->end(),
                   [](const std::pair<nodeid, map_node>& it) -> const std::vector<std::shared_ptr<fast_tracker::fast_feature<patch_orb_descriptor>>>& {
                       return it.second.frame->keypoints;
                   },
                   [](const std::shared_ptr<fast_tracker::fast_feature<patch_orb_descriptor>>& feature) -> const orb_descriptor::raw& {
                       return feature->descriptor.orb.descriptor;
                   },
            branching_factor, depth_levels);
    }
    return voc;
}

void mapper::predict_map_features(const uint64_t camera_id_now, const transformation& G_Bcurrent_Bnow) {
    map_feature_tracks.clear();
    if(!current_node)
        return;
    // distance # edges traversed
    auto distance = [](const map_edge& edge) { return 1; };
    // select node if it is finished and within 2 meters from current pose
    auto is_node_searched = [this](const node_path& path) {
      if( (get_node(path.id).status == node_status::finished) && (path.G.T.norm() < 2.f) )
          return true;
      else
          return false;
    };
    // search all graph
    auto finish_search = [](const node_path& path) { return false; };

    nodes_path neighbors = dijkstra_shortest_path(node_path{current_node->id, invert(G_Bcurrent_Bnow), 0},
                                                  distance, is_node_searched, finish_search);

    std::sort(neighbors.begin(), neighbors.end(), [](const node_path& path1, const node_path& path2){
        return path1.G.T.norm() < path2.G.T.norm();
    });

    if(neighbors.size() > 4)
        neighbors.resize(4);
    const state_extrinsics* const extrinsics_now = camera_extrinsics[camera_id_now];
    const state_vision_intrinsics* const intrinsics_now = camera_intrinsics[camera_id_now];
    transformation G_CB = invert(transformation(extrinsics_now->Q.v, extrinsics_now->T.v));

    for(const auto& neighbor : neighbors) {
        map_node& node_neighbor = nodes->at(neighbor.id);
        std::vector<map_feature_track> tracks;
        const transformation& G_Bnow_Bneighbor = neighbor.G;
        transformation G_Cnow_Bneighbor = G_CB*G_Bnow_Bneighbor;
        for(const auto& f : node_neighbor.features) {
            // predict feature in current camera pose
            v3 p3dC = G_Cnow_Bneighbor * get_feature3D(neighbor.id, f.second.feature->id);
            if(p3dC.z() < 0)
                continue;
            feature_t kpn = p3dC.segment<2>(0)/p3dC.z();
            feature_t kpd = intrinsics_now->unnormalize_feature(intrinsics_now->distort_feature(kpn));
            if(kpd.x() < 0 || kpd.x() > intrinsics_now->image_width ||
               kpd.y() < 0 || kpd.y() > intrinsics_now->image_height)
                continue;

            // create feature track
            tracker::feature_track track(f.second.feature, INFINITY, INFINITY, 0.0f);
            track.pred_x = kpd.x();
            track.pred_y = kpd.y();
            tracks.emplace_back(std::move(track), f.second.v);
        }
        map_feature_tracks.emplace_back(neighbor.id, invert(G_Bnow_Bneighbor), std::move(tracks));
    }
}

/// --------------------- S T R E A M I N G   O F   M A P   S T R U C T U R E S ------------------//
#define MAPPER_SERIALIZED_VERSION 2

#include "map_loader.h"
#include "bstream.h"

static bstream_writer & operator << (bstream_writer& content, const v2 &vec) {
    return content << vec[0] << vec[1];
}

static bstream_writer &operator << (bstream_writer &content, const map_edge &edge) {
    return content << static_cast<uint8_t>(edge.type) << edge.G;
}

static bstream_writer & operator << (bstream_writer& content, const std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> &feat) {
    content << feat->id << feat->descriptor.orb.sin_ << feat->descriptor.orb.cos_;
    content << (std::array<uint8_t, orb_descriptor::L> &)feat->descriptor.orb.descriptor;
    return content << (std::array<uint8_t, patch_descriptor::L> &)feat->descriptor.patch.descriptor;
}

static bstream_writer & operator << (bstream_writer &content, const map_feature &feat) {
    content << feat.v->initial[0] << feat.v->initial[1] << feat.v->v << feat.feature->id;// skip saving patch descriptor as generated data
    return content;
}

static bstream_writer & operator << (bstream_writer &content, const std::shared_ptr<frame_t> &frame) {
    return content << frame->keypoints << frame->keypoints_xy;
}

static bstream_writer & operator << (bstream_writer &content, const map_node &node) {
    content << node.id << node.camera_id << node.edges << node.global_transformation;
    content << (uint8_t)(node.frame != nullptr);
    if (node.frame) content << node.frame;
    return content << node.covisibility_edges << node.features;
}

static const char magic_file_format_num[5] = { 'R', 'C', 'M', '\0' }; //R C Map File

bool mapper::serialize(rc_SaveCallback func, void *handle) const {
    if (!validate_map_compatible(*nodes)) {
        log->error("map is not compatible for saving.");
        return false;
    }
    bstream_writer cur_stream(func, handle);
    cur_stream.write(magic_file_format_num, sizeof(magic_file_format_num));
    cur_stream << (uint8_t)MAPPER_SERIALIZED_VERSION;
    cur_stream << *nodes << dbow_inverted_index << *features_dbow;
    cur_stream.end_stream();
    if (!cur_stream.good()) log->error("map was not saved successfully.");
    return cur_stream.good();
}

bool mapper::deserialize(rc_LoadCallback func, void *handle, mapper &cur_map) {
    bstream_reader cur_stream(func, handle);

    char format_num[sizeof(magic_file_format_num)] = {};
    cur_stream.read(format_num, sizeof(format_num));
    if (strcmp(magic_file_format_num, format_num)) {
        cur_map.log->error("file format is not supported.");
        return false;
    }
    uint8_t version = 0;
    cur_stream >> version;
    std::unique_ptr<map_loader> loaded_map(get_map_load(version));
    if (!loaded_map) {
        cur_map.log->error("mapper version {} is not supported.", version);
        return false;
    }
    if (!loaded_map->deserialize(cur_stream)) {
        cur_map.log->error("failed to load map content.");
        return false;
    }
    loaded_map->set(cur_map);
    loaded_map.reset();

    uint64_t max_node_id = 0;
    for (auto &ele : *cur_map.nodes) {
        if (ele.second.frame)
            ele.second.frame->calculate_dbow(cur_map.orb_voc.get()); // populate map_frame's dbow_histogram and dbow_direct_file
        ele.second.status = node_status::finished;
        if (max_node_id < ele.second.id) max_node_id = ele.second.id;
    }
    cur_map.unlinked = true;
    cur_map.node_id_offset = max_node_id + 1;
    cur_map.feature_id_offset = map_feature_v1::max_loaded_featid + 1;
    cur_map.log->info("Loaded map with {} nodes and {} features", cur_map.node_id_offset, cur_map.feature_id_offset);
    return true;
}

