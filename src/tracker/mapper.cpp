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
#include "estimate.h"
#include "fast_tracker.h"
#include "descriptor.h"
#include "Trace.h"

#ifdef RELOCALIZATION_DEBUG
#include <opencv2/imgproc.hpp>
#include "debug/visual_debug.h"
#endif

using namespace std;

// Constant relocalization parameters
constexpr static f_t sigma_px = 3.0;
constexpr static int max_iter = 40; // 40
constexpr static float confidence = 0.9f; //0.9
constexpr static size_t min_num_inliers = 12;

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
    feature_id_offset = 0;
    node_id_offset = 0;
    unlinked = false;
    nodes.critical_section([&](){ nodes->clear(); });
    features_dbow.critical_section([&](){ features_dbow->clear(); });
    reset_stages();
}

void mapper::clean_map_after_filter_reset() {
    nodes.critical_section([&]() {
        for(auto& node : *nodes)
            node.second.status = node_status::finished;
    });
}

void map_node::get_add_neighbor(nodeid neighbor, const transformation& G, const edge_type type)
{
    if(type != edge_type::relocalization) {
        auto& edge = edges.emplace(neighbor, map_edge{}).first->second;
        edge.G = G;
        if((type != edge_type::filter) || ((std::underlying_type<edge_type>::type)type > (std::underlying_type<edge_type>::type)edge.type))
            edge.type = type;
    } else {
        auto& edge = relocalization_edges.emplace(neighbor, map_edge{}).first->second;
        edge.G = G;
        edge.type = type;
    }
}

void mapper::add_edge(nodeid id1, nodeid id2, const transformation& G12, edge_type type) {
    nodes.critical_section(&mapper::add_edge_no_lock, this, id1, id2, G12, type);
}

void mapper::add_relocalization_edges(const aligned_vector<map_relocalization_edge>& edges) {
    nodes.critical_section([&]() {
        for (auto& edge : edges) {
            if (nodes->find(edge.id1) != nodes->end() && nodes->find(edge.id2) != nodes->end()) {
                add_edge_no_lock(edge.id1, edge.id2, edge.G, edge.type);
//                add_covisibility_edge_no_lock(edge.id1, edge.id2);
            }
        }
    });
}

void mapper::add_edge_no_lock(nodeid id1, nodeid id2, const transformation &G12, edge_type type) {
    nodes->at(id1).get_add_neighbor(id2, G12, type);
    nodes->at(id2).get_add_neighbor(id1, invert(G12), type);
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

bool mapper::edge_in_map(nodeid id1, nodeid id2, edge_type& type) const {
    return nodes.critical_section([&]() {
        auto it1 = nodes->find(id1);
        if(it1 == nodes->end())
            return false;
        auto it2 = it1->second.edges.find(id2);
        if(it2 != it1->second.edges.end()) {
            type = it2->second.type;
            return true;
        } else {
            return false;
        }
    });
}

bool mapper::feature_in_map(featureid id, nodeid* nid) const {
    auto it = features_dbow->find(id);
    if (it != features_dbow->end()) {
        if (nid) *nid = it->second;
        return true;
    }
    return false;
}

void mapper::remove_node_features(nodeid id) {
    for(auto& f : nodes->at(id).features)
        features_dbow->erase(f.first);
}

void map_node::add_feature(std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature,
                           std::shared_ptr<log_depth> v, f_t v_var, const feature_type type) {
    auto fid = feature->id;
    features.emplace(std::piecewise_construct,
                     std::forward_as_tuple(fid),
                     std::forward_as_tuple(std::move(v), std::move(feature), v_var, type));
}

void map_node::set_feature_type(const featureid id, f_t v_var, const feature_type type)
{
    assert(features.find(id) != features.end());
    features.at(id).v_var = v_var;
    features.at(id).type = type;
}

void mapper::add_feature(nodeid groupid, std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature,
                         std::shared_ptr<log_depth> v, f_t v_var, const feature_type type) {
    assert(feature && features_dbow->find(feature->id) == features_dbow->end());
    auto it = nodes->find(groupid);
    if(it != nodes->end()) {
        critical_section(nodes, features_dbow, [&]() {
            (*features_dbow)[feature->id] = groupid;
            it->second.add_feature(std::move(feature), std::move(v), v_var, type);
        });
    }
}

bool mapper::move_feature(featureid feature_id, nodeid src_node_id, nodeid dst_node_id, const transformation* G_Bdst_Bsrc_ptr) {
    map_node& node_src = nodes->at(src_node_id);
    map_node& node_dst = nodes->at(dst_node_id);
    auto node_src_features_it = node_src.features.find(feature_id);
    auto features_dbow_it = features_dbow->find(feature_id);
    auto intrinsics_dst = camera_intrinsics[node_dst.camera_id];
    auto extrinsics_dst = camera_extrinsics[node_dst.camera_id];
    transformation G_CB = invert(transformation(extrinsics_dst->Q.v, extrinsics_dst->T.v));
    transformation G_Bdst_Bsrc;
    if(G_Bdst_Bsrc_ptr) {
        G_Bdst_Bsrc = *G_Bdst_Bsrc_ptr;
    } else {
        assert(find_relative_pose(dst_node_id, src_node_id, G_Bdst_Bsrc));
    }

    v3 p3dC = G_CB * G_Bdst_Bsrc * get_feature3D(src_node_id, feature_id);
    if(p3dC.z() < 0)
        return false;
    feature_t kpd = intrinsics_dst->project_feature(p3dC);

    critical_section(nodes, features_dbow, [&]() {
        map_feature& f = node_src_features_it->second;
        f.v->set_depth_meters(p3dC.z());
        f.v->initial = kpd;
        features_dbow_it->second = dst_node_id;
        node_dst.features.emplace(std::piecewise_construct, std::forward_as_tuple(feature_id), std::forward_as_tuple(std::move(f)));
        node_src.features.erase(node_src_features_it);
    });

    return true;
}

void mapper::remove_feature(nodeid groupid, featureid fid) {
    critical_section(nodes, features_dbow, [&]() {
        features_dbow->erase(fid);
        nodes->at(groupid).features.erase(fid);
    });
}

void mapper::remove_feature(featureid fid) {
    auto it = features_dbow->find(fid);
    if (it != features_dbow->end()) {
        critical_section(nodes, features_dbow, [&]() {
            nodes->at(it->second).features.erase(fid);
            features_dbow->erase(it);
        });
    }
}

void mapper::set_feature_type(nodeid group_id, featureid id, f_t v_var, const feature_type type) {
    nodes.critical_section([&]() { nodes->at(group_id).set_feature_type(id, v_var, type); });
}

v3 mapper::get_feature3D(nodeid node_id, featureid feature_id) const {
    const map_node& node = nodes->at(node_id);
    auto mf = node.features.at(feature_id);
    auto intrinsics = camera_intrinsics[node.camera_id];
    auto extrinsics = camera_extrinsics[node.camera_id];
    m3 Rbc = extrinsics->Q.v.toRotationMatrix();
    v3 pn = intrinsics->unproject_feature(mf.v->initial);
    return Rbc*(mf.v->depth()*pn) + extrinsics->T.v;
}

void mapper::set_node_transformation(nodeid id, const transformation & G)
{
    nodes.critical_section([&]() {
        nodes->at(id).global_transformation = G;
    });
}

void mapper::set_node_frame(nodeid id, std::shared_ptr<frame_t> frame) {
    nodes.critical_section([&]() {
        auto it = nodes->find(id);
        if (it != nodes->end()) {
            assert(!it->second.frame);
            it->second.frame = std::move(frame);
        }
    });
}

void mapper::finish_node(nodeid id) {
    auto& node = nodes->at(id);
    nodes.critical_section([&]() {
        node.status = node_status::finished;
    });
}

void mapper::remove_node(nodeid id)
{
    auto node_it = nodes->find(id);

    // move node stages to nearest neighbor
    stages.critical_section([&, this]() {
        for(auto stage_it = stages->begin(); stage_it != stages->end(); ) {
            stage& st = stage_it->second;
            if(st.closest_id == node_it->first) {
                auto neighbor_it = [node_it]() {
                    const auto &edges = node_it->second.edges;
                    auto best_edge_it = edges.begin();
                    f_t best_distance = std::numeric_limits<f_t>::infinity();
                    for(auto edge_it = edges.begin(); edge_it != edges.end(); ++edge_it) {
                        f_t sq_distance = edge_it->second.G.T.dot(edge_it->second.G.T);
                        if(sq_distance < best_distance) {
                            best_edge_it = edge_it;
                            best_distance = sq_distance;
                        }
                    }
                    return best_edge_it;
                }();
                if(neighbor_it != node_it->second.edges.end()) {
                    st.closest_id = neighbor_it->first;
                    st.Gr_closest_stage = invert(neighbor_it->second.G) * st.Gr_closest_stage;
                    st.current_id = std::numeric_limits<nodeid>::max();
                } else {
                    // the node doesn't have neighbors, we have to delete stages
                    stage_it = stages->erase(stage_it);
                    continue;
                }
            }
            ++stage_it;
        }
    });

    aligned_map<nodeid, map_edge> edges = node_it->second.edges;
    std::set<nodeid> neighbors;
    for(auto& edge : edges) {
        neighbors.insert(edge.first);
        remove_edge(id, edge.first);
    }

    for(const auto& reloc_edge : node_it->second.relocalization_edges) {
        auto& neighbor = nodes->at(reloc_edge.first);
        neighbor.relocalization_edges.erase(node_it->first);
    }

    nodes.critical_section([&]() {
        for (nodeid covisible_neighbor : node_it->second.covisibility_edges) {
            nodes->at(covisible_neighbor).covisibility_edges.erase(node_it->first);
        }
    });

    features_dbow.critical_section([&]() {
        for(auto& feature : node_it->second.features)
            features_dbow->erase(feature.first);
    });

    // this is run in case all groups are dropped
    if(reference_node->id == id) {
        auto& edge = *edges.begin();
        auto& neighbor = nodes->at(edge.first);
        if(neighbor.status == node_status::finished) {
            neighbor.global_transformation = reference_node->global_transformation * edge.second.G;
        }
        reference_node = &neighbor;
    }
    nodes.critical_section([&]() { nodes->erase(node_it); });

    // if only 1 neighbor, removed node (id) is a loose end of the graph so we don't need to reconnect anything
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

        std::set<nodeid> connected_neighbors;
        while (connected_neighbors.size() < neighbors.size()) {
            // this could be more efficient if we don't calculate transformations in dijkstra
            for(auto &path : dijkstra_shortest_path(node_path{*neighbors.begin(), transformation(), 0}, distance, is_node_searched, finish_search))
                connected_neighbors.insert(path.id);

            // are neighbors disconnected after removing node id ?
            if(connected_neighbors.size() < neighbors.size()) {
                std::vector<nodeid> disconnected_neighbors;
                std::set_difference(neighbors.begin(), neighbors.end(), connected_neighbors.begin(), connected_neighbors.end(), std::back_inserter(disconnected_neighbors));
                transformation G_id_connected = edges[*connected_neighbors.begin()].G;
                edge_type type_connected = edges[*connected_neighbors.begin()].type;
                transformation G_id_disconnected = edges[disconnected_neighbors.front()].G; // pick one of the disconnected nodes
                edge_type type_disconnected = edges[disconnected_neighbors.front()].type;

                edge_type type = edge_type::composition;
                if(type_connected == edge_type::dead_reckoning || type_disconnected == edge_type::dead_reckoning)
                    type = edge_type::dead_reckoning;

                add_edge(*connected_neighbors.begin(), disconnected_neighbors.front(), invert(G_id_connected)*G_id_disconnected, type);
            }
        }
    }
}

bool mapper::find_relative_pose(nodeid source, nodeid target, transformation& G) const {
    // returns transformation G_Bsource_Btarget
    assert(nodes->find(source) != nodes->end());
    assert(nodes->find(target) != nodes->end());
    auto distance = [](const map_edge& edge) { return edge.G.T.norm(); };
    auto is_node_searched = [target](const node_path& path) { return path.id == target; };
    auto finish_search = is_node_searched;

    nodes_path paths = dijkstra_shortest_path(node_path{source, transformation(), 0}, distance, is_node_searched, finish_search);
    if(!paths.empty()) {
        G = paths[0].G;
        return true;
    }
    return false;
}

mapper::nodes_path mapper::find_neighbor_nodes(const node_path& start, const uint64_t camera_id_now) {
    const state_extrinsics* const extrinsics_now = camera_extrinsics[camera_id_now];
    transformation G_CB = invert(transformation(extrinsics_now->Q.v, extrinsics_now->T.v));

    // distance # edges traversed
    auto distance = [](const map_edge& edge) { return edge.G.T.norm(); };
    // select node if it is finished and within 2 meters from current pose
    auto is_node_searched = [this, &G_CB](const node_path& path) {
      f_t cos_z = v3{0,0,1}.dot(G_CB.Q*path.G.Q*G_CB.Q.conjugate()*v3{0,0,1});
      // check if candidate is in a 2m radius and points to the same half-space direction cos(M_PI/2) = 0
      if( (get_node(path.id).status == node_status::finished) && (path.G.T.norm() < 2) && (cos_z > 0) )
          return true;
      else
          return false;
    };
    // search all graph
    auto finish_search = [](const node_path& path) { return false; };
    START_EVENT(SF_DIJKSTRA, 0);
    nodes_path neighbors = dijkstra_shortest_path(start, distance, is_node_searched, finish_search);

    std::sort(neighbors.begin(), neighbors.end(), [](const node_path& path1, const node_path& path2){
        return path1.G.T.norm() < path2.G.T.norm();
    });
    END_EVENT(SF_DIJKSTRA, neighbors.size());
    //performance tradeof:
    //too many neighbors -> long loop time
    //too few -> we might end up not adding features at all
    if(neighbors.size() > 8)
        neighbors.resize(8);

    return neighbors;
}

mapper::nodes_path mapper::dijkstra_shortest_path(const node_path& start, std::function<float(const map_edge& edge)> distance, std::function<bool(const node_path& path)> is_node_searched,
                                                  std::function<bool(const node_path& path)> finish_search) const
{
    auto cmp = [](const node_path& path1, const node_path& path2) {
      return path1.distance > path2.distance;
    };
    priority_queue<node_path, nodes_path, decltype(cmp)> next(cmp);

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
                if(nodes_done.find(v) == nodes_done.end()) {
                    const transformation& G_u_v = edge.second.G;
                    f_t distance_uv = distance(edge.second);
                    next.emplace(v, G_start_u*G_u_v, distance_u + distance_uv);
                }
            }
        }
    }

    return searched_nodes;
}

std::vector<std::pair<nodeid,float>> mapper::find_loop_closing_candidates(
    const std::shared_ptr<frame_t>& current_frame) const
{
    std::unordered_set<nodeid> discarded_nodes;
    {
        // find nodes with 3d features in common with current frame so that we don't try to relocalize against them
        critical_section(nodes, features_dbow, [&]() {
            for (auto& keypoint : current_frame->keypoints) {
                auto it = features_dbow->find(keypoint->id);
                if (it != features_dbow->end()) {
                    const map_node& node = nodes->at(it->second);
                    discarded_nodes.insert(node.id);
                    discarded_nodes.insert(node.covisibility_edges.begin(), node.covisibility_edges.end());
                }
            }
        });
    }

    constexpr size_t max_candidates = 10;
    std::vector<std::pair<nodeid, float>> dbow_scores;
    {
        std::unordered_map<nodeid, std::shared_ptr<frame_t>> candidate_frames;
        nodes.critical_section([&]() {
            for (auto& it : *nodes) {
                if (it.second.status == node_status::finished && it.second.frame && !discarded_nodes.count(it.first)) {
                    candidate_frames.emplace(it.first, it.second.frame);
                }
            }
        });
        if (!candidate_frames.empty()) {
            const auto better_item = [](const std::pair<nodeid, float>& p1, const std::pair<nodeid, float>& p2) {
                switch (orb_vocabulary::scoring_type) {
                case DBoW2::ScoringType::KL:
                case DBoW2::ScoringType::CHI_SQUARE:  // the lower the better
                    return p1.second < p2.second;
                case DBoW2::ScoringType::L1_NORM:
                case DBoW2::ScoringType::L2_NORM:
                case DBoW2::ScoringType::BHATTACHARYYA:  // the higher the better
                    return p1.second > p2.second;
                }
            };
            dbow_scores.reserve(std::min(candidate_frames.size(), max_candidates));
            auto it = candidate_frames.begin();
            for (; dbow_scores.size() < max_candidates && it != candidate_frames.end(); ++it) {
                dbow_scores.emplace_back(it->first, orb_voc->score(it->second->dbow_histogram, current_frame->dbow_histogram));
            }
            if (it != candidate_frames.end()) {
                std::make_heap(dbow_scores.begin(), dbow_scores.end(), better_item);
                for (; it != candidate_frames.end(); ++it) {
                    std::pair<nodeid, float> candidate {it->first, orb_voc->score(it->second->dbow_histogram, current_frame->dbow_histogram)};
                    if (better_item(candidate, dbow_scores.front())) {
                        std::pop_heap(dbow_scores.begin(), dbow_scores.end(), better_item);
                        dbow_scores.back() = candidate;
                        std::push_heap(dbow_scores.begin(), dbow_scores.end(), better_item);
                    }
                }
                std::sort_heap(dbow_scores.begin(), dbow_scores.end(), better_item);
            } else {
                std::sort(dbow_scores.begin(), dbow_scores.end(), better_item);
            }
        }
    }
    return dbow_scores;
}

static size_t calculate_orientation_bin(const orb_descriptor &a, const orb_descriptor &b, const size_t num_orientation_bins) {
    return static_cast<size_t>(((a - b) * (float)M_1_PI + 1) / 2 * num_orientation_bins + 0.5f) % num_orientation_bins;
}

mapper::matches mapper::match_2d_descriptors(const std::shared_ptr<frame_t>& candidate_frame, state_vision_intrinsics* const candidate_intrinsics,
                                             const std::shared_ptr<frame_t>& current_frame, state_vision_intrinsics* const current_intrinsics) const {
    //matches per orientationn increment between current frame and node candidate
    static constexpr int num_orientation_bins = 30;
    std::vector<mapper::matches> increment_orientation_histogram(num_orientation_bins);

    mapper::matches current_to_candidate_matches;

    if (candidate_frame->keypoints.size() > 0 && current_frame->keypoints.size() > 0) {

        auto match_fundamental = [&current_frame, &candidate_frame, &increment_orientation_histogram]() {
            std::unordered_map<size_t, std::pair<size_t, int>> matches;  // candidate -> current, distance
            for (size_t current_point_idx = 0; current_point_idx < current_frame->keypoints.size(); ++current_point_idx) {
                size_t best_candidate_point_idx = 0;
                auto best_distance = std::numeric_limits<float>::max();
                auto second_best_distance = std::numeric_limits<float>::max();
                auto& current_keypoint = *current_frame->keypoints[current_point_idx];
                for (size_t candidate_point_idx = 0; candidate_point_idx < candidate_frame->keypoints.size(); ++candidate_point_idx) {
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

        mapper::matches fundamental_current_to_candidate_matches;
        match_fundamental();

        // Check orientations to prune wrong matches (just keep best 3)
        std::partial_sort(increment_orientation_histogram.begin(),
                          increment_orientation_histogram.begin()+3,
                          increment_orientation_histogram.end(),
                          [](const std::vector<std::pair<featureidx, featureidx>>&v1,
                             const std::vector<std::pair<featureidx, featureidx>>&v2) {
            return (v1.size() > v2.size());});

        for(int i=0; i<3; ++i) {
            fundamental_current_to_candidate_matches.insert(fundamental_current_to_candidate_matches.end(),
                                                            increment_orientation_histogram[i].begin(),
                                                            increment_orientation_histogram[i].end());
        }


        if(fundamental_current_to_candidate_matches.size() >= min_num_inliers) {
            aligned_vector<v2> points_current;
            points_current.reserve(fundamental_current_to_candidate_matches.size());
            aligned_vector<v2> points_candidate;
            points_candidate.reserve(fundamental_current_to_candidate_matches.size());
            for(auto& current_candidate_match : fundamental_current_to_candidate_matches) {
                v2& pd_current = current_frame->keypoints_xy[current_candidate_match.first];
                v2& pd_candidate = candidate_frame->keypoints_xy[current_candidate_match.second];
                v2 p_current = current_intrinsics->unnormalize_feature(
                            current_intrinsics->undistort_feature(current_intrinsics->normalize_feature(pd_current)));
                v2 p_candidate = candidate_intrinsics->unnormalize_feature(
                            candidate_intrinsics->undistort_feature(candidate_intrinsics->normalize_feature(pd_candidate)));
                points_current.push_back(p_current);
                points_candidate.push_back(p_candidate);
            }
            std::minstd_rand rng(-1);
            const float max_reprojection_error = 5.f; // in pixels
            m3 Fcurrent_candidate;
            auto reprojection_error = estimate_fundamental(points_candidate, points_current, Fcurrent_candidate, rng, 20, max_reprojection_error);

            if(reprojection_error < max_reprojection_error) {
                typedef std::pair<size_t, float> index_distance;
                constexpr size_t matches_per_candidate = 3;

                auto match = [&](const std::vector<size_t>& candidate_keypoint_indexes) {

                    auto keypoint_comp = [](const index_distance& p1, const index_distance& p2) {
                        return p1.second < p2.second;
                    };

                    for (auto candidate_point_idx : candidate_keypoint_indexes) {
                        auto& candidate_keypoint = *candidate_frame->keypoints[candidate_point_idx];
                        std::vector<index_distance> current_points_matched;
                        current_points_matched.reserve(std::min(current_frame->keypoints.size(), matches_per_candidate + 1));
                        for (size_t current_point_idx = 0; current_point_idx < current_frame->keypoints.size(); ++current_point_idx) {
                            auto& current_keypoint = *current_frame->keypoints[current_point_idx];
                            float dist = current_keypoint.descriptor.distance_reloc(candidate_keypoint.descriptor,
                                                                                    current_keypoint.descriptor);
                            current_points_matched.emplace_back(current_point_idx, dist);
                            push_heap(current_points_matched.begin(), current_points_matched.end(), keypoint_comp);
                            if(current_points_matched.size() > matches_per_candidate) {
                                pop_heap(current_points_matched.begin(), current_points_matched.end(), keypoint_comp);
                                current_points_matched.pop_back();
                            }
                        }
                        float best_distance = std::numeric_limits<float>::infinity();
                        v2 p_candidate = candidate_intrinsics->unnormalize_feature(
                                    candidate_intrinsics->undistort_feature(candidate_intrinsics->normalize_feature(candidate_frame->keypoints_xy[candidate_point_idx])));
                        v3 line = Fcurrent_candidate * v3{p_candidate[0], p_candidate[1], 1.f};
                        line = line/sqrtf(line[0]*line[0] + line[1]*line[1]);
                        int current_point_idx;
                        for(auto& current_match : current_points_matched) {
                            v2 p_current = current_intrinsics->unnormalize_feature(
                                current_intrinsics->undistort_feature(current_intrinsics->normalize_feature(current_frame->keypoints_xy[current_match.first])));
                            float distance = std::abs(line.dot(v3{p_current[0], p_current[1], 1.f}));
                            if(distance < 3.f && distance < best_distance) {
                                best_distance = distance;
                                current_point_idx = current_match.first;
                            }
                        }
                        if(best_distance < std::numeric_limits<float>::infinity())
                            current_to_candidate_matches.emplace_back(current_point_idx, candidate_point_idx);
                    }
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

                match(indexes_with_3d(candidate_frame->keypoints));
            }
        }
    }
    return current_to_candidate_matches;
}

bool mapper::estimate_pose(const aligned_vector<v3>& points_3d, const aligned_vector<v2>& points_2d, const rc_Sensor camera_id, transformation& G_candidateB_currentframeB, std::set<size_t>& inliers_set) const {
    state_extrinsics* const extrinsics = camera_extrinsics[camera_id];
    transformation G_BC = transformation(extrinsics->Q.v, extrinsics->T.v);
    state_vision_intrinsics* const intrinsics = camera_intrinsics[camera_id];
    const f_t focal_px = intrinsics->focal_length.v * intrinsics->image_height;
    const f_t max_reprojection_error = 2*sigma_px/focal_px;
    std::minstd_rand rng(-1);
    transformation G_currentframeC_candidateB;
    auto reprojection_error = estimate_transformation(points_3d, points_2d, G_currentframeC_candidateB, rng, max_iter, max_reprojection_error, confidence, min_num_inliers, &inliers_set);
    if(reprojection_error < max_reprojection_error) {
        G_candidateB_currentframeB = invert(G_BC*G_currentframeC_candidateB);
        return true;
    } else {
        return false;
    }
}

bool mapper::get_stage(const std::string &name, stage &stage, nodeid current_id, const transformation &G_world_current, stage::output &current_stage) {
    if (stage.current_id != current_id) {
        bool ok = nodes.critical_section([&]() {
            if(find_relative_pose(current_id, stage.closest_id, stage.G_current_closest)) {
                stage.current_id = current_id;
                return true;
            } else {
                return false;
            }
        });
        if (!ok)
            return false;
    }
    current_stage = { name.c_str(), G_world_current * stage.G_current_closest * stage.Gr_closest_stage };
    return true;
}

map_relocalization_result mapper::relocalize(const camera_frame_t& camera_frame) const {

#if defined(RELOCALIZATION_DEBUG)
    visual_debug::batch batch;
#endif

    // note: mapper::relocalize can run in parallel to other mapper functions
    const std::shared_ptr<frame_t>& current_frame = camera_frame.frame;
    map_relocalization_result reloc_result;
    reloc_result.info.frame_timestamp = current_frame->timestamp;
    if (!current_frame->keypoints.size()) return reloc_result;

    START_EVENT(SF_FIND_CANDIDATES, 0);
    std::vector<std::pair<nodeid, float>> candidate_nodes = find_loop_closing_candidates(current_frame);
    END_EVENT(SF_FIND_CANDIDATES, candidate_nodes.size());
    reloc_result.info.rstatus = relocalization_status::find_candidates;

    const auto &keypoint_xy_current = current_frame->keypoints_xy;
    state_vision_intrinsics* const intrinsics = camera_intrinsics[camera_frame.camera_id];
    size_t best_num_inliers = 0;
    for (size_t idx = 0; idx < candidate_nodes.size(); ++idx) {
        const auto& nid = candidate_nodes[idx];

#if defined(RELOCALIZATION_DEBUG)
        bool is_relocalized_in_candidate = false;
#endif
        std::shared_ptr<frame_t> candidate_node_frame;
        transformation candidate_node_global_transformation;
        state_vision_intrinsics* node_intrinsics;

        bool ok = nodes.critical_section([&]() {
            auto it = nodes->find(nid.first);
            if (it != nodes->end()) {
                candidate_node_frame = it->second.frame;
                node_intrinsics = camera_intrinsics[it->second.camera_id];
                candidate_node_global_transformation = it->second.global_transformation;
                return true;
            }
            return false;
        });
        if (!ok) continue;

        START_EVENT(SF_MATCH_DESCRIPTORS, 0);
        matches matches_node_candidate = match_2d_descriptors(candidate_node_frame, node_intrinsics, current_frame, intrinsics);
        END_EVENT(SF_MATCH_DESCRIPTORS, matches_node_candidate.size());
        reloc_result.info.rstatus = relocalization_status::match_descriptors;

        // Just keep candidates with more than a min number of mathces
        std::set<size_t> inliers_set;
        if(matches_node_candidate.size() >= min_num_inliers) {
            // Estimate pose from 3d-2d matches
            std::unordered_map<nodeid, transformation> G_candidate_neighbors;
            ok = nodes.critical_section([&](){
                auto it_node = nodes->find(nid.first);
                if (it_node != nodes->end()) {
                    auto covisible_neighbors = it_node->second.covisibility_edges; // some of trhe points observed from this candidate node should be found in the covisible nodes
                    // distance # edges traversed
                    auto distance = [](const map_edge& edge) { return edge.G.T.norm(); };
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

            aligned_vector<v3> candidate_3d_points;
            aligned_vector<v2> current_2d_points;
            critical_section(nodes, features_dbow, [&]() {
                for (const auto& m : matches_node_candidate) {
                    auto &candidate = *candidate_node_frame->keypoints[m.second];
                    featureid keypoint_id = candidate.id;
                    auto it = features_dbow->find(keypoint_id);
                    if (it != features_dbow->end()) {
                        featureidx current_feature_index = m.first;
                        nodeid nodeid_keypoint = it->second;
                        auto it_node = nodes->find(nodeid_keypoint);
                        if (it_node != nodes->end()) {
                            // NOTE: We use 3d features observed from candidate, this does not mean
                            // these features belong to the candidate node (group)
                            auto it_G = G_candidate_neighbors.find(nodeid_keypoint);
                            if(it_G == G_candidate_neighbors.end()) {// TODO: all features observed from the candidate node should be represented wrt one of the covisible nodes but it does not happen ....
                                continue;
                            }
                            candidate_3d_points.emplace_back(it_G->second * get_feature3D(nodeid_keypoint, keypoint_id)); // feat is in body frame
                            current_2d_points.emplace_back(intrinsics->undistort_feature(intrinsics->normalize_feature(keypoint_xy_current[current_feature_index]))); //undistort keypoints at current frame
                        }
                    }
                }
            });
            transformation G_candidate_currentframe;
            inliers_set.clear();
            START_EVENT(SF_ESTIMATE_POSE,0);
            bool pose_found = estimate_pose(candidate_3d_points, current_2d_points, camera_frame.camera_id, G_candidate_currentframe, inliers_set);
            END_EVENT(SF_ESTIMATE_POSE,inliers_set.size());
            reloc_result.info.rstatus = relocalization_status::estimate_EPnP;
            if(pose_found && inliers_set.size() >= min_num_inliers) {
                transformation G_candidate_closestnode = G_candidate_currentframe*invert(camera_frame.G_closestnode_frame);
                reloc_result.edges.emplace_back(nid.first, camera_frame.closest_node, G_candidate_closestnode, edge_type::relocalization);
                reloc_result.info.candidates.emplace_back(nid.first, G_candidate_currentframe, candidate_node_global_transformation, candidate_node_frame->timestamp);
                if (inliers_set.size() > best_num_inliers) {
                    // keep the best relocalization the first of the list
                    best_num_inliers = inliers_set.size();
                    std::swap(reloc_result.info.candidates[0], reloc_result.info.candidates.back());
                    std::swap(reloc_result.edges[0], reloc_result.edges.back());
                }
                reloc_result.info.is_relocalized = true;
#if defined(RELOCALIZATION_DEBUG)
                is_relocalized_in_candidate = true;
#endif
            }
        }
        if (!inliers_set.size())
            log->debug("{}/{}) candidate nid: {:3} score: {:.5f}, matches: {:2}",
                       idx, candidate_nodes.size(), nid.first, nid.second, matches_node_candidate.size());
        else
            log->debug(" {}/{}) candidate nid: {:3} score: {:.5f}, matches: {:2}, EPnP inliers: {}",
                      idx, candidate_nodes.size(), nid.first, nid.second, matches_node_candidate.size(), inliers_set.size());

#if defined(RELOCALIZATION_DEBUG)
        const auto &keypoint_xy_candidates = candidate_node_frame->keypoints_xy;
        const auto &keypoint_candidates = candidate_node_frame->keypoints;
        assert(keypoint_candidates.size() == keypoint_xy_candidates.size());
        cv::Mat color_candidate_image, color_current_image;
        cv::cvtColor(candidate_node_frame->image, color_candidate_image, CV_GRAY2BGRA);
        cv::cvtColor(current_frame->image, color_current_image, CV_GRAY2BGRA);

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

        if(is_relocalized_in_candidate) {
            cv::putText(color_current_image,"RELOCALIZED",
                        cv::Point(30,30),cv::FONT_HERSHEY_COMPLEX,0.8,cv::Scalar(0,255,0,255));
        } else {
            cv::putText(color_current_image,"RELOCALIZATION FAILED",
                        cv::Point(30,30),cv::FONT_HERSHEY_COMPLEX,0.8,cv::Scalar(255,0,0,255));
        }

        int rows = color_current_image.rows;
        int cols = color_current_image.cols;
        int image_type = color_current_image.type();

        cv::Mat compound_matches(rows, 2*cols, image_type);
        color_current_image.copyTo(compound_matches(cv::Rect(0, 0, cols, rows)));
        flipped_color_candidate_image.copyTo(compound_matches(cv::Rect(cols, 0, cols, rows)));
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
            cv::circle(compound_matches,cv::Point(p1[0],p1[1]),3,color,2);
            cv::circle(compound_matches,cv::Point(compound_matches.cols-p2[0],p2[1]),3,color,2);
            cv::line(compound_matches,cv::Point(p1[0],p1[1]),cv::Point(compound_matches.cols-p2[0],p2[1]),color_line,2);
            matchidx++;
        }

        cv::Mat compound_keypoints(rows, 2*cols, image_type);
        for(auto &pc : keypoint_xy_current) {
            cv::circle(color_current_image, cv::Point(pc[0],pc[1]),3,cv::Scalar{0,255,0,255},2);
        }

        for(size_t i=0; i<keypoint_xy_candidates.size(); ++i) {
            cv::Scalar color{0,255,0,255};
            if (features_dbow->find(keypoint_candidates[i]->id) == features_dbow->end())
                color = cv::Scalar{255,0,0,255};
            auto &pc = keypoint_xy_candidates[i];
            cv::circle(flipped_color_candidate_image, cv::Point(flipped_color_candidate_image.cols-pc[0],pc[1]),3,color,2);
        }
        color_current_image.copyTo(compound_keypoints(cv::Rect(0, 0, cols, rows)));
        flipped_color_candidate_image.copyTo(compound_keypoints(cv::Rect(cols, 0, cols, rows)));

        std::string batch_name;
        if (!is_relocalized_in_candidate) {
            if(matches_node_candidate.size() < min_num_inliers) {
                batch_name = "BoWs";
            } else {
                batch_name = "Match2d";
            }
        } else {
            batch_name = "Relocalized";
        }

        batch.add(compound_matches, batch_name);
        batch.add(compound_keypoints, batch_name);

#endif
    }

#if defined(RELOCALIZATION_DEBUG)
    visual_debug::send(batch, reloc_result.info.is_relocalized);
#endif


    return reloc_result;
}

bool mapper::link_map(const map_relocalization_edge& edge) {
    const auto source_id = edge.id2;
    return nodes.critical_section([&, source_id]() {
        auto it = nodes->find(source_id);
        if (it != nodes->end()) {
            auto distance = [](const map_edge& edge) { return edge.G.T.norm(); };
            auto is_node_searched = [this](const mapper::node_path& path) { return path.id < this->get_node_id_offset(); };
            auto finish_search = [](const mapper::node_path&) { return false; };
            auto loaded_map_nodes = dijkstra_shortest_path(mapper::node_path{source_id, it->second.global_transformation, 0},
                                                           distance, is_node_searched, finish_search);
            for(auto& loaded_map_node : loaded_map_nodes) {
                nodes->at(loaded_map_node.id).global_transformation = loaded_map_node.G;
            }
            unlinked = false;
            return true;
        }
        return false;
    });
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

void mapper::predict_map_features(const uint64_t camera_id_now, const mapper::nodes_path& neighbors, const size_t min_group_map_add) {
    map_feature_tracks.clear();

    const state_extrinsics* const extrinsics_now = camera_extrinsics[camera_id_now];
    const state_vision_intrinsics* const intrinsics_now = camera_intrinsics[camera_id_now];
    transformation G_CB = invert(transformation(extrinsics_now->Q.v, extrinsics_now->T.v));

    int groups_added = 0;
    for(const auto& neighbor : neighbors) {
        map_node& node_neighbor = nodes->at(neighbor.id);
        std::vector<map_feature_track> tracks;
        const transformation& G_Bnow_Bneighbor = neighbor.G;
        transformation G_Cnow_Bneighbor = G_CB*G_Bnow_Bneighbor;
        size_t potential = node_neighbor.features.size();
        for(const auto& f : node_neighbor.features) {
            // predict feature in current camera pose
            v3 p3dC = G_Cnow_Bneighbor * get_feature3D(neighbor.id, f.second.feature->id);
            if(p3dC.z() < 0)
                continue;
            feature_t kpd = intrinsics_now->project_feature(p3dC);
            if(kpd.x() < 0 || kpd.x() > intrinsics_now->image_width ||
               kpd.y() < 0 || kpd.y() > intrinsics_now->image_height){
                potential--;
                //early exit if not enough features
                if(potential < min_group_map_add) {
                    break;
                }
                continue;
            }
            // create feature track
            tracker::feature_track track(f.second.feature, INFINITY, INFINITY, 0.0f);
            track.pred_x = kpd.x();
            track.pred_y = kpd.y();
            tracks.emplace_back(std::move(track), f.second.v, f.second.v_var);
        }
        if(tracks.size() >= min_group_map_add){
            map_feature_tracks.emplace_back(neighbor.id, invert(G_Bnow_Bneighbor), std::move(tracks));
            groups_added++;
        }
        //more than 4 will create a long tracking time
        if(groups_added >= 4) break;
    }
}

/// --------------------- S T R E A M I N G   O F   M A P   S T R U C T U R E S ------------------//
#define MAPPER_SERIALIZED_VERSION 2

#include "map_loader.h"
#include "bstream.h"

static bstream_writer & operator << (bstream_writer& content, const v2 &vec) {
    return content << vec[0] << vec[1];
}

static bstream_writer & operator << (bstream_writer & cur_stream, const transformation &transform) {
    cur_stream << transform.T[0] << transform.T[1] << transform.T[2];
    cur_stream << transform.Q.w() << transform.Q.x() << transform.Q.y() << transform.Q.z();
    return cur_stream;
}

static bstream_writer &operator << (bstream_writer &content, const map_edge &edge) {
    return content << static_cast<uint8_t>(edge.type) << edge.G;
}

static bstream_writer &operator << (bstream_writer &content, const mapper::stage &stage) {
    return content << stage.closest_id << stage.Gr_closest_stage;
}

static bstream_writer & operator << (bstream_writer& content, const std::shared_ptr<fast_tracker::fast_feature<patch_orb_descriptor>> &feat) {
    content << feat->id << (std::array<uint8_t, patch_descriptor::L> &)feat->descriptor.patch.descriptor << feat->descriptor.orb_computed;
    if (feat->descriptor.orb_computed)
        content << (std::array<uint8_t, orb_descriptor::L> &)feat->descriptor.orb.descriptor << feat->descriptor.orb.cos_ << feat->descriptor.orb.sin_;
    return content;
}

static bstream_writer & operator << (bstream_writer &content, const map_feature &feat) {
    content << feat.v->initial[0] << feat.v->initial[1] << feat.v->v << feat.feature; //NOTE: redundant saving of same feature
    return content;
}

static bstream_writer & operator << (bstream_writer &content, const std::shared_ptr<frame_t> &frame) {
    content << frame->keypoints << frame->keypoints_xy;
    uint64_t frame_ts = sensor_clock::tp_to_micros(frame->timestamp);
#ifdef RELOCALIZATION_DEBUG
    if (!frame->image.empty() && frame->image.isContinuous()) {
        content << (uint8_t)1U; //has image data
        content << (uint32_t)frame->image.cols << (uint32_t)frame->image.rows << (uint32_t)frame->image.step[0];
        size_t img_size = (size_t)(frame->image.rows * frame->image.step[0]);
        content.write((const char *)frame->image.data, img_size);
    }
    else
#endif
    content << (uint8_t)0U;

    return content << frame_ts;
}

static bstream_writer & operator << (bstream_writer &content, const map_node &node) {
    content << node.id << node.camera_id << node.frames_active << node.edges;
    content << (uint8_t)(node.frame != nullptr);
    if (node.frame) content << node.frame;
    return content << node.covisibility_edges << node.features;
}

static const char magic_file_format_num[4] = { 'R', 'C', 'M', '\0' }; // RC Map

bool mapper::serialize(rc_SaveCallback func, void *handle) const {
    bstream_writer cur_stream(func, handle);
    cur_stream.write(magic_file_format_num, sizeof(magic_file_format_num));
    cur_stream << (uint8_t)MAPPER_SERIALIZED_VERSION;
    auto t = critical_section(nodes, features_dbow, stages, [&]() {
        return std::make_tuple(*nodes, *features_dbow, *stages);
    });
    cur_stream << std::get<0>(t) << std::get<1>(t) << std::get<2>(t);
    cur_stream.end_stream();
    if (!cur_stream.good()) log->error("map was not saved successfully.");
    return cur_stream.good();
}

bool mapper::deserialize(rc_LoadCallback func, void *handle, mapper &cur_map) {
    map_stream_reader cur_stream(func, handle);

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

    nodeid max_node_id = 0;
    for (auto &ele : *cur_map.nodes) {
        if (ele.second.frame) {
            ele.second.frame->calculate_dbow(cur_map.orb_voc.get()); // populate map_frame's dbow_histogram and dbow_direct_file
        }
        ele.second.status = node_status::finished;
        if (max_node_id < ele.second.id) max_node_id = ele.second.id;
    }
    cur_map.unlinked = true;
    cur_map.node_id_offset = max_node_id + 1;
    cur_map.feature_id_offset = cur_stream.max_loaded_featid + 1;

    if(!cur_map.nodes->empty()) {
        auto distance = [](const map_edge& edge) { return edge.G.T.norm(); };
        auto is_node_searched = [](const mapper::node_path& path) { return true; };
        auto finish_search = [](const mapper::node_path& path) { return false; };
        auto loaded_map_nodes = cur_map.dijkstra_shortest_path(mapper::node_path{0, transformation(quaternion::Identity(), v3(-10, 0, 0)), 0},
                                                               distance, is_node_searched, finish_search);
        for(auto& loaded_map_node : loaded_map_nodes) {
            cur_map.set_node_transformation(loaded_map_node.id, loaded_map_node.G);
        }
    }

    cur_map.log->info("Loaded map with {} nodes, {} feature offset and {} stages", cur_map.node_id_offset, cur_map.feature_id_offset, cur_map.stages->size());
    return true;
}

