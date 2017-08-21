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

map_edge &map_node::get_add_neighbor(uint64_t neighbor)
{
    for(list<map_edge>::iterator edge = edges.begin(); edge != edges.end(); ++edge) {
        if(edge->neighbor == neighbor) return *edge;
    }
    neighbors.insert(neighbor);
    edges.push_back(map_edge{neighbor, 0});
    return edges.back();
}

void mapper::add_edge(uint64_t id1, uint64_t id2)
{
    id1 += node_id_offset;
    id2 += node_id_offset;
    if(nodes.size() <= id1) nodes.resize(id1+1);
    if(nodes.size() <= id2) nodes.resize(id2+1);
    nodes[id1].get_add_neighbor(id2);
    nodes[id2].get_add_neighbor(id1);
}

void mapper::add_node(uint64_t id)
{
    id += node_id_offset;
    if(nodes.size() <= id) nodes.resize(id + 1);
    nodes[id].id = id;
    nodes[id].frame = std::move(current_frame);
}

void mapper::process_keypoints(const std::vector<tracker::feature_track*> &keypoints, const rc_Sensor camera_id, const tracker::image& image)
{
    // fill in relocalization variables
    current_frame.camera_id = camera_id;

    current_frame.keypoints.clear();
    for (auto &p : keypoints)
        if (std::is_same<DESCRIPTOR, orb_descriptor>::value)
            current_frame.keypoints.emplace_back(std::static_pointer_cast<fast_tracker::fast_feature<orb_descriptor>>(p->feature));
        else if (fast_tracker::is_trackable<orb_descriptor::border_size>((int)p->x, (int)p->y, image.width_px, image.height_px))
            current_frame.keypoints.emplace_back(make_shared<fast_tracker::fast_feature<orb_descriptor>>(p->feature->id, p->x, p->y, image));

    // copy pyramid descriptors to a vector of descriptors
    std::vector<orb_descriptor::raw> v_descriptor;
    v_descriptor.reserve(current_frame.keypoints.size());
    for ( auto& p : current_frame.keypoints )
        v_descriptor.push_back(p->descriptor.descriptor);
    int num_words_missing = orb_voc->transform(v_descriptor, current_frame.dbow_histogram,
                                               current_frame.dbow_direct_file, 6);
}

void map_node::set_feature(const uint64_t id, const v3 &pos, const float variance)
{
    features[id].position = pos;
    features[id].variance = variance;
    features[id].id = id;
}

void mapper::set_feature(uint64_t groupid, uint64_t id, const v3 &pos, float variance, bool is_new) {
    groupid += node_id_offset;
    id += feature_id_offset;
    nodes[groupid].set_feature(id, pos, variance);
    if (is_new) features_dbow[id] = groupid;
}

void mapper::set_node_transformation(uint64_t id, const transformation & G)
{
    id += node_id_offset;
    nodes[id].global_transformation = G;
    nodes[id].status = node_status::normal;
}

void mapper::node_finished(uint64_t id)
{
    id += node_id_offset;
    if (nodes[id].status == node_status::normal) {
        nodes[id].status = node_status::finished;
        for (auto &word : nodes[id].frame.dbow_histogram)
            dbow_inverted_index[word.first].push_back(id); // Add this node to inverted index
    } else {
        // delete node when its status is not normal
        nodes[id] = std::move(nodes.back());
        nodes.pop_back();
    }
}
    
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#define MAPPER_SERIALIZED_VERSION 1
#define KEY_VERSION "version"
#define KEY_NODES "nodes"
#define KEY_NODE_ID "id"
#define KEY_NODE_NEIGHBORS "neighbors"
#define KEY_NODE_TRANSLATION "T"
#define KEY_NODE_QUATERNION "Q"
#define KEY_FEATURES "features"
#define KEY_FEATURE_ID "id"
#define KEY_FEATURE_VARIANCE "variance"
#define KEY_FEATURE_POSITION "position"
#define KEY_FEATURE_DESCRIPTOR "d"

using namespace rapidjson;

bool mapper::serialize(std::string &json)
{
    // Build DOM
    Document map_json;
    map_json.SetObject();
    Document::AllocatorType & allocator = map_json.GetAllocator();

    Value version(MAPPER_SERIALIZED_VERSION);
    map_json.AddMember(KEY_VERSION, version, allocator);

    vector<uint64_t> id_map; id_map.resize(nodes.size());
    uint64_t to = 0;
    for(uint64_t from = 0; from < nodes.size(); from++) {
        if(nodes[from].status != node_status::finished) {
            id_map[from] = nodes.size() + 1;
            continue;
        }
        id_map[from] = to;
        to++;
    }

    Value nodes_json(kArrayType);
    for(int i = 0; i < nodes.size(); i++) {
        // Only write finished nodes, because geometry is not valid
        // otherwise
        if(nodes[i].status != node_status::finished) continue;
        Value node_json(kObjectType);
        node_json.AddMember(KEY_NODE_ID, id_map[nodes[i].id], allocator);

        Value node_neighbors_json(kArrayType);
        for(list<map_edge>::iterator edge = nodes[i].edges.begin(); edge != nodes[i].edges.end(); ++edge) {
            uint64_t nid = edge->neighbor;
            if(nodes[nid].status != node_status::finished) continue;
            Value neighbor_id(id_map[nid]);
            node_neighbors_json.PushBack(neighbor_id, allocator);
        }
        node_json.AddMember(KEY_NODE_NEIGHBORS, node_neighbors_json, allocator);

        Value node_features_json(kArrayType);
        for(auto &f : nodes[i].features) {
            Value node_feature_json(kObjectType);

            Value fid(f.second.id);
            node_feature_json.AddMember(KEY_FEATURE_ID, fid, allocator);

            Value fvariance(f.second.variance);
            node_feature_json.AddMember(KEY_FEATURE_VARIANCE, fvariance, allocator);

            Value fposition(kArrayType);
            fposition.PushBack(f.second.position[0], allocator);
            fposition.PushBack(f.second.position[1], allocator);
            fposition.PushBack(f.second.position[2], allocator);
            node_feature_json.AddMember(KEY_FEATURE_POSITION, fposition, allocator);

            // Save descriptor, feature_dbow and track_id
//            Value fdescriptor(kArrayType);
//            for(int j = 0; j < descriptor_size; j++)
//                fdescriptor.PushBack(f->dvec(j), allocator);
//            node_feature_json.AddMember(KEY_FEATURE_DESCRIPTOR, fdescriptor, allocator);

            node_features_json.PushBack(node_feature_json, allocator);
        }
        node_json.AddMember(KEY_FEATURES, node_features_json, allocator);

        Value translation_json(kArrayType);
        translation_json.PushBack(nodes[i].global_transformation.T[0], allocator);
        translation_json.PushBack(nodes[i].global_transformation.T[1], allocator);
        translation_json.PushBack(nodes[i].global_transformation.T[2], allocator);
        node_json.AddMember(KEY_NODE_TRANSLATION, translation_json, allocator);

        Value rotation_json(kArrayType);
        rotation_json.PushBack(nodes[i].global_transformation.Q.w(), allocator);
        rotation_json.PushBack(nodes[i].global_transformation.Q.x(), allocator);
        rotation_json.PushBack(nodes[i].global_transformation.Q.y(), allocator);
        rotation_json.PushBack(nodes[i].global_transformation.Q.z(), allocator);
        node_json.AddMember(KEY_NODE_QUATERNION, rotation_json, allocator);

        nodes_json.PushBack(node_json, allocator);
    }
    map_json.AddMember(KEY_NODES, nodes_json, allocator);

    // Write json to string buffer
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    map_json.Accept(writer);
    json = buffer.GetString();

    return json.length() > 0;
}

bool mapper::deserialize(const std::string &json, mapper & map)
{
    Document map_json; map_json.Parse(json.c_str());
    if(map_json.HasParseError())
        return false;

    int version = map_json[KEY_VERSION].GetInt();
    if(version != MAPPER_SERIALIZED_VERSION)
        return false;

    map.reset();

    const Value & nodes = map_json[KEY_NODES];
    if(!nodes.IsArray()) return false;

    uint64_t max_feature_id = 0;
    uint64_t max_node_id = 0;
    for(SizeType i = 0; i < nodes.Size(); i++) {
        uint64_t node_id = nodes[i][KEY_NODE_ID].GetUint64();
        if(node_id > max_node_id)
            max_node_id = node_id;
        map.add_node(node_id);

        const Value & neighbors = nodes[i][KEY_NODE_NEIGHBORS];
        for(SizeType j = 0; j < neighbors.Size(); j++) {
            uint64_t neighbor_id = neighbors[j].GetUint64();
            map.add_edge(node_id, neighbor_id);
        }

        const Value & features = nodes[i][KEY_FEATURES];
        for(SizeType j = 0; j < features.Size(); j++) {
            uint64_t feature_id = features[j][KEY_FEATURE_ID].GetUint64();
            if(feature_id > max_feature_id)
                max_feature_id = feature_id;

            float variance = (float)features[j][KEY_FEATURE_VARIANCE].GetDouble();

            v3 position = v3::Zero();
            const Value & feature_position = features[j][KEY_FEATURE_POSITION];
            for(SizeType k = 0; k < position.size() && k < feature_position.Size(); k++)
                position[k] = (float)feature_position[k].GetDouble();

            // Load descriptor, feature_dbow and track_id
//            const Value & desc = features[j][KEY_FEATURE_DESCRIPTOR];
//            descriptor d;
//            for(SizeType k = 0; k < desc.Size(); k++) {
//                d.d[k] = (float)desc[k].GetDouble();
//            }

//            map.add_feature(node_id, feature_id, position);
        }

        transformation G;
        const Value & translation = nodes[i][KEY_NODE_TRANSLATION];
        for(SizeType j = 0; j < G.T.size() && j < translation.Size(); j++) {
            G.T[j] = (float)translation[j].GetDouble();
        }

        const Value & rotation = nodes[i][KEY_NODE_QUATERNION];
        G.Q.w() = (float)rotation[0].GetDouble();
        G.Q.x() = (float)rotation[1].GetDouble();
        G.Q.y() = (float)rotation[2].GetDouble();
        G.Q.z() = (float)rotation[3].GetDouble();

        map.set_node_transformation(node_id, G);
        map.node_finished(node_id);
    }
    map.node_id_offset = max_node_id + 1;
    map.feature_id_offset = max_feature_id + 1;
    map.unlinked = true;
    map.log->info("Loaded map with {} nodes and {} features", map.node_id_offset, map.feature_id_offset);
    return true;
}

std::vector<std::pair<mapper::nodeid,float>> find_loop_closing_candidates(
    const map_frame &current_frame,
    const aligned_vector<map_node> &nodes,
    const std::map<unsigned int, std::vector<mapper::nodeid>> &dbow_inverted_index,
    const orb_vocabulary* orb_voc
) {
    std::vector<std::pair<mapper::nodeid, float>> loop_closing_candidates;

    // find nodes sharing words with current frame
    std::map<mapper::nodeid,uint32_t> common_words_per_node;
    uint32_t max_num_shared_words = 0;
    for (auto word : current_frame.dbow_histogram) {
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
    for (auto node_candidate : common_words_per_node) {
        if (node_candidate.second > min_num_shared_words) {
            const map_node& node = nodes[node_candidate.first];
            float dbow_score = static_cast<float>(orb_voc->score(node.frame.dbow_histogram,
                                                                 current_frame.dbow_histogram));
            dbow_scores.push_back(std::make_pair(node_candidate.first, dbow_score));
            best_score = std::max(dbow_score, best_score);
        }
    }

    // sort candidates by dbow_score and age
    auto compare_dbow_scores = [](std::pair<mapper::nodeid, float> p1, std::pair<mapper::nodeid, float> p2) {
        return (p1.second > p2.second) || (p1.second == p2.second && p1.first < p2.first);
    };
    std::sort(dbow_scores.begin(), dbow_scores.end(), compare_dbow_scores);

    // keep good candidates according to a minimum score
    float min_score = std::max(best_score * 0.75f, 0.0f); // assuming L1 norm
    for (auto dbow_score : dbow_scores) {
        if (dbow_score.second > min_score) {
            loop_closing_candidates.push_back(dbow_score);
        }
    }
    return loop_closing_candidates;
}

static size_t calculate_orientation_bin(const orb_descriptor &a, const orb_descriptor &b, const size_t num_orientation_bins) {
    return static_cast<size_t>(((a - b) * (float)M_1_PI + 1) / 2 * num_orientation_bins + 0.5f) % num_orientation_bins;
}

static mapper::matches match_2d_descriptors(const map_frame& candidate_frame, const map_frame& current_frame,
                                            std::map<uint64_t, mapper::nodeid> &features_dbow) {
    //matches per orientationn increment between current frame and node candidate
    const int num_orientation_bins = 30;
    std::vector<mapper::matches> increment_orientation_histogram(num_orientation_bins);

    mapper::matches current_to_candidate_matches;

    if (candidate_frame.keypoints.size() > 0 && current_frame.keypoints.size() > 0) {
        auto it_candidate = candidate_frame.dbow_direct_file.begin();
        auto it_current = current_frame.dbow_direct_file.begin();

        while (it_candidate != candidate_frame.dbow_direct_file.end() &&
               it_current != current_frame.dbow_direct_file.end()) {
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
                    auto& current_keypoint = *current_frame.keypoints[current_point_idx];
                    for (int candidate_point_idx : candidate_keypoint_indexes) {
                        auto& candidate_keypoint = *candidate_frame.keypoints[candidate_point_idx];
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
                        auto& best_candidate_keypoint = *candidate_frame.keypoints[best_candidate_point_idx];
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

static void estimate_pose(const aligned_vector<v3>& points_3d, const aligned_vector<v2>& points_2d, transformation& G_WC, std::set<size_t>& inliers_set) {
    const f_t max_reprojection_error = 4.0f/38; //threshold = 2*sigma (pixels) / f_px? FIXME!
    const int max_iter = 10; // 10
    const float confidence = 0.9; //0.9
    std::default_random_engine rng(-1);
    transformation G_CW;
    estimate_transformation(points_3d, points_2d, G_CW, rng, max_iter, max_reprojection_error, confidence, 5, &inliers_set);
    G_WC = invert(G_CW);
}

bool mapper::relocalize(std::vector<transformation>& vG_WC, const transformation& G_BC) {
    if (!current_frame.keypoints.size())
        return false;

    bool is_relocalized = false;
    const int min_num_inliers = 12;
    int best_num_inliers = 0;
    int i = 0;

    std::vector<std::pair<nodeid, float>> candidate_nodes =
        find_loop_closing_candidates(current_frame, nodes, dbow_inverted_index, orb_voc);
    const auto &keypoint_current = current_frame.keypoints;
    state_vision_intrinsics* const intrinsics = camera_intrinsics[current_frame.camera_id];
    for (auto nid : candidate_nodes) {
        matches matches_node_candidate = match_2d_descriptors(nodes[nid.first].frame, current_frame, features_dbow);
        // Just keep candidates with more than a min number of mathces
        std::set<size_t> inliers_set;
        aligned_vector<v3> candidate_3d_points;
        aligned_vector<v2> current_2d_points;
        transformation G_WC;
        if(matches_node_candidate.size() >= min_num_inliers) {
            // Estimate pose from 3d-2d matches
            const auto &keypoint_candidates = nodes[nid.first].frame.keypoints;
            for (auto m : matches_node_candidate) {
                auto &candidate = *keypoint_candidates[m.second];
                uint64_t keypoint_id = candidate.id;
                nodeid nodeid_keypoint = features_dbow[keypoint_id];
                // NOTE: We use 3d features observed from candidate, this does not mean
                // these features belong to the candidate node (group)
                map_feature &mfeat = nodes[nodeid_keypoint].features[keypoint_id]; // feat is in body frame
                v3 p_w = nodes[nodeid_keypoint].global_transformation * mfeat.position;
                candidate_3d_points.push_back(p_w);
                //undistort keypoints at current frame
                auto& current = *keypoint_current[m.first];
                feature_t kp = {current.x, current.y};
                feature_t ukp = intrinsics->undistort_feature(intrinsics->normalize_feature(kp));
                current_2d_points.push_back(ukp);
            }
            estimate_pose(candidate_3d_points, current_2d_points, G_WC, inliers_set);
            if(inliers_set.size() >= min_num_inliers) {
                is_relocalized = true;
//                vG_WC.push_back(G_WC);

                if(inliers_set.size() >  best_num_inliers) {
                    best_num_inliers = inliers_set.size();
                    vG_WC.clear();
                    vG_WC.push_back(G_WC);
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

    current_frame.keypoints.clear();

    return is_relocalized;
}
