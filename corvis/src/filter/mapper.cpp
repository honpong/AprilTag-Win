// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include <algorithm>
#include <assert.h>
#include <limits.h>
#include <queue>
#include "mapper.h"
#include "corvis_dictionary.h"
#include <iostream>

transformation_variance invert(const transformation_variance & T)
{
    transformation_variance result = T;
    result.transform = invert(T.transform);
    // TODO: deal with variance
    return result;
}

transformation_variance operator *(const transformation_variance & T1, const transformation_variance & T2)
{
    transformation_variance result;
    result.transform = compose(T1.transform, T2.transform);
    // TODO: deal with variance
    return result;
}

size_t map_node::histogram_size;

//NOTE: this is reversed so that we can pop the smallest item from the heap
bool map_match_compare(const map_match &x, const map_match &y)
{
    return (x.score > y.score);
}

int map_histogram_dist_2(const vector<int> &hist1, const vector<int> &hist2, int bound = INT_MAX)
{
    int dist = 0;
    assert(hist1.size() == hist2.size());
    for(size_t i = 0; i < hist1.size(); ++i) {
        int delta = hist1[i] - hist2[i];
        dist += delta * delta;
        if(dist >= bound) return dist;
    }
    return dist;
}

int map_histogram_score(const vector<int> &hist1, const vector<int> &hist2)
{
    int score = 0;
    assert(hist1.size() == hist2.size());
    for(size_t i = 0; i < hist1.size(); ++i) {
        if(hist1[i] && hist2[i]) ++score;
    }
    return score;
}

//scoring functions: no lock needed: only accesses document_frequency, which is static, and don't mind getting old values
float mapper::tf_idf_score(const list<map_feature *> &hist1, const list<map_feature *> &hist2)
{
    float score = 0.;
    list<map_feature *>::const_iterator first = hist1.begin(), second = hist2.begin();
    while(first != hist1.end()) {
        int label = (*first)->label;
        int count1 = 0, count2 = 0;
        while(first != hist1.end() && (*first)->label == label) { ++first; ++count1; }
        while(second != hist2.end() && (*second)->label < label) ++second;
        while(second != hist2.end() && (*second)->label == label) { ++second; ++count2; }
        float idf = std::log(1.f * nodes.size() / document_frequency[label]);
        score += count1 * count2 * idf;
    }
    return score;
}

mapper::mapper(): feature_count(0), feature_dictionary(corvis_dimension, corvis_num_centers, corvis_centers)
{
    int dict_size = feature_dictionary.get_num_centers();
    map_node::histogram_size = dict_size;
    document_frequency.resize(dict_size);
    unlinked = false;
}

mapper::~mapper()
{
    reset();
}

void mapper::reset()
{
    for(int i = 0; i < (int)nodes.size(); i++) {
        for(map_feature * f : nodes[i].features)
            delete f;
        nodes[i].features.clear();
    }
    nodes.clear();
    geometry.clear();
    document_frequency.clear();
    document_frequency.resize(map_node::histogram_size);
    feature_count = 0;
    feature_id_offset = 0;
    node_id_offset = 0;
    unlinked = false;
}

map_edge &map_node::get_add_neighbor(uint64_t neighbor)
{
    for(list<map_edge>::iterator edge = edges.begin(); edge != edges.end(); ++edge) {
        if(edge->neighbor == neighbor) return *edge;
    }
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
    nodes[id].parent = -1;
}


map_feature::map_feature(const uint64_t _id, const v4 &p, const float v, const uint32_t l, const descriptor & desc): id(_id), position(p), variance(v), label(l)
{
    Eigen::Map<const Eigen::VectorXf> eigen_d(desc.d, descriptor_size, 1);
    dvec = Eigen::VectorXf(eigen_d);
}

bool map_node::add_feature(const uint64_t id, const v4 &pos, const float variance, const uint32_t label, const descriptor & d)
{
    map_feature *feat = new map_feature(id, pos, variance, label, d);
    list<map_feature *>::iterator feature;
    for(feature = features.begin(); feature != features.end(); ++feature) {
        if((*feature)->label >= label) break;
    }
    features.insert(feature, feat);
    ++terms;
    return (feature == features.end() || (*feature)->label != label); //true if this was a new label for this group
}

void mapper::train_dictionary() const
{
    vector<descriptor> features;
    for(int n = 0; n < nodes.size(); n++) {
        for(auto f : nodes[n].features) {
            descriptor d;
            for(int i = 0; i < descriptor_size; i++)
                d.d[i] = f->dvec(i);
            features.push_back(d);
        }
    }
    class dictionary dict(features, 30);
    dict.write("log.dictionary");
}

uint32_t mapper::project_feature(const descriptor & d)
{
    return feature_dictionary.quantize(d);
}

void mapper::update_feature_position(uint64_t groupid, uint64_t id, const v4 &pos, float variance)
{
    groupid += node_id_offset;
    id += feature_id_offset;
    for(auto f : nodes[groupid].features) {
        if(f->id == id) {
            f->position = pos;
            f->variance = variance;
        }
    }
}

void mapper::add_feature(uint64_t groupid, uint64_t id, const v4 &pos, float variance, const descriptor & d)
{
    groupid += node_id_offset;
    id += feature_id_offset;
    uint32_t label = project_feature(d);
    ++feature_count;
    if(nodes[groupid].add_feature(id, pos, variance, label, d)) {
        ++document_frequency[label];
    }
}

void mapper::internal_set_geometry(uint64_t id1, uint64_t id2, const transformation_variance &transform, bool loop_closure)
{
    map_edge &edge1 = nodes[id1].get_add_neighbor(id2);
    int64_t id = edge1.geometry;
    if(id > 0) {
        geometry[id-1] = transform;
    } else {
        if(id) {
            id = -id;
            geometry[id-1] = transform;
        } else {
            geometry.push_back(transform);
            id = geometry.size();
        }
        map_edge &edge2 = nodes[id2].get_add_neighbor(id1);
        edge1.geometry = id;
        edge2.geometry = -id;
        edge1.loop_closure = loop_closure;
        edge2.loop_closure = loop_closure;
    }
}

void mapper::set_geometry(uint64_t id1, uint64_t id2, const transformation_variance &transform)
{
    id1 += node_id_offset;
    id2 += node_id_offset;
    if(nodes.size() <= id1) nodes.resize(id1+1);
    if(nodes.size() <= id2) nodes.resize(id2+1);
    internal_set_geometry(id1, id2, transform, false);
}

transformation mapper::get_relative_transformation(uint64_t from_id, uint64_t to_id)
{
    nodes[to_id].transform = transformation_variance();
    breadth_first(to_id, 0, NULL);

    return nodes[from_id].transform.transform;
}

void mapper::tf_idf_match(vector<float> &matches, const list<map_feature *> &histogram)
{
    size_t nnodes = nodes.size();
    if(matches.size() < nnodes) matches.resize(nnodes);
    for(size_t i = 0; i < nnodes; ++i) {
        matches[i] = tf_idf_score(histogram, nodes[i].features);
    }
}

void mapper::diffuse_matches(uint64_t node_id, vector<float> &matches, aligned_vector<map_match> &result, int max, int unrecent)
{
    //mark the nodes that are too close to this one
    for(int i = 0; i < matches.size(); ++i) {
        if(unlinked && i < node_id_offset) {
            if(!nodes[i].finished) continue; // Can't match unfinished nodes
        }
        else {
            if(nodes[i].id == node_id) continue; // Can't match ourselves
            if(!nodes[i].finished) continue; // Can't match unfinished nodes
            if(nodes[i].depth <= unrecent) continue; // Can't match nodes that are too close
        }
        float num = matches[i];
        int denom = nodes[i].terms;
        for(list<map_edge>::iterator edge = nodes[i].edges.begin(); edge != nodes[i].edges.end(); ++edge) {
            num += matches[edge->neighbor];
            denom += nodes[edge->neighbor].terms;
        }
        result.push_back(map_match {(uint64_t)node_id, (uint64_t)i, num / denom});
        push_heap(result.begin(), result.end(), map_match_compare);
        if(result.size() > max) {
            pop_heap(result.begin(), result.end(), map_match_compare);
            result.pop_back();
        }
    }
    sort_heap(result.begin(), result.end(), map_match_compare);
}

static bool map_feature_compare(const map_feature *first, const map_feature *second)
{
    return first->label < second->label;
}

void mapper::joint_histogram(int node, list<map_feature *> &histogram)
{
    list<map_feature *> copy(nodes[node].features);
    histogram.merge(copy, map_feature_compare);
    for(list<map_edge>::iterator edge = nodes[node].edges.begin(); edge != nodes[node].edges.end(); ++edge) {
        list<map_feature *> neighbor_copy(nodes[edge->neighbor].features);
        histogram.merge(neighbor_copy, map_feature_compare);
    }
}

void localize_features(map_node &node, aligned_list<local_feature> &features);
void assign_matches(const aligned_list<local_feature> &f1, const aligned_list<local_feature> &f2, aligned_list<match_pair> &matches, bool slow);

int mapper::brute_force_rotation(uint64_t id1, uint64_t id2, transformation_variance &trans, int threshhold, float min, float max)
{
    transformation R;
    int best = 0;
    int worst = 100000000;
    f_t first_best, last_best;
    bool running = false;
    assert(max > min);
    //get all features for each group and its neighbors into the local frames
    aligned_list<local_feature> f1, f2;
    transformation Gw1 = nodes[id1].global_transformation.transform;
    transformation Gw2 = nodes[id2].global_transformation.transform;
    nodes[id1].transform = transformation_variance();
    nodes[id1].transform.transform = Gw1;
    nodes[id2].transform = transformation_variance();
    nodes[id2].transform.transform = Gw2;
    breadth_first(id1, 1, NULL);
    breadth_first(id2, 1, NULL);
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    //ONLY look for matches in THIS group
    aligned_list<match_pair> matches;
    assign_matches(f1, f2, matches, false);
    if(matches.size() == 0) return 0;
    //BUT, check support on ENTIRE neighborhood.
    f1.clear();
    f2.clear();
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    localize_neighbor_features(id1, f1);
    localize_neighbor_features(id2, f2);
    aligned_list<match_pair> neighbor_matches;
    assign_matches(f1, f2, neighbor_matches, false);
    if(neighbor_matches.size() < threshhold) return 0;
    for(float theta = min; theta < max; theta += (max-min) / 200.) {
        transformation R(rotation_vector(0., 0., theta), v4(0,0,0,0));
        v4 dT;
        int in = estimate_translation(id1, id2, dT, threshhold, R, matches, neighbor_matches);
        if(in < worst) worst = in;
        if(in > best) {
            first_best = theta;
            last_best = theta;
            best = in;
            running = true;
        }
        if(in == best && running) {
            last_best = theta;
        }
        if(in < best) running = false;
    }
    if(best-worst >= threshhold) {
        trans.transform = transformation(rotation_vector(0., 0., (first_best + last_best) / 2.), v4(0,0,0,0));
        v4 dT;
        estimate_translation(id1, id2, dT, threshhold, trans.transform, matches, neighbor_matches);
        trans.transform.T = dT;
        trans.transform = invert(Gw1)*trans.transform*Gw2;
        return best-worst;
    }
    return 0;
}

bool mapper::get_matches(uint64_t id, aligned_vector<map_match> &matches, int max, int suppression)
{
    bool found = false;
    //rebuild the map relative to the current node
    for(auto n : nodes)
        n.depth = 0;
    nodes[id].transform = transformation_variance();
    breadth_first(id, 0, NULL);
    if(unlinked) {
        nodes[0].transform = transformation_variance();
        breadth_first(0, 0, NULL);
    }

    list<map_feature *> histogram;
    vector<float> scores;
    joint_histogram(id, histogram);
    tf_idf_match(scores, histogram);
    diffuse_matches(id, scores, matches, max, suppression);
    int threshhold = 8;
    for(int i = 0; i < matches.size(); ++i) {
        transformation_variance g;
        if(unlinked && matches[i].to < node_id_offset)
            matches[i].score = brute_force_rotation(matches[i].from, matches[i].to, g, threshhold, -M_PI, M_PI);
        else
            matches[i].score = check_for_matches(id, matches[i].to, g, threshhold);
        matches[i].g = g.transform;
    }
    sort(matches.begin(), matches.end(), map_match_compare);
    if(matches.size() > 0 && matches[0].score >= threshhold) {
        for(int i = 0; i < matches.size(); i++) {
            if(matches[i].score != matches[0].score)
                break;

            transformation_variance newT;
            newT.transform = matches[i].g;
            int brute_score = brute_force_rotation(matches[0].from, matches[0].to, newT, threshhold, -M_PI/6., M_PI/6.);
            if(brute_score >= threshhold) {
                found = true;
                matches[i].g = newT.transform;
                matches[i].score = brute_score;
            }
        }

        if(found) {
            // This sort makes sure the best fitting final
            // transformation is selected
            sort(matches.begin(), matches.end(), map_match_compare);
            //internal_set_geometry(matches[0].from, matches[0].to, tv, true);
            if(unlinked && matches[0].to < node_id_offset) {
                unlinked = false;
                transformation_variance tv;
                tv.transform = matches[0].g;
                internal_set_geometry(matches[0].from, matches[0].to, tv, true);
            }
        }
    }
    return found;
}

bool mapper::find_closure(int max, int suppression, transformation & offset)
{
    for(int i = 0; i < nodes.size(); i++) {
        if(nodes[i].finished && !nodes[i].match_attempted && i + 10 < nodes.size()) {
            //log->info("searching for loop closure for {}", nodes[i].id);
            nodes[i].match_attempted = true;
            aligned_vector<map_match> matches;
            if(get_matches(nodes[i].id, matches, max, suppression)) {
                map_match m = matches[0];
                //log->info("Loop closure: match {} - {} {}", m.from, m.to, m.score);
                //log->info() << m.g.T;
                //transformation new_t = initial * map.get_relative_transformation(m.from, 0);
                transformation new_t = nodes[m.to].global_transformation.transform * invert(m.g);
                transformation old_t = nodes[m.from].global_transformation.transform;
                offset = new_t * invert(old_t);
                return true;
            }
        }
    }
    return false;
}

static bool local_feature_compare(const local_feature &first, const local_feature &second) {
    return map_feature_compare(first.feature, second.feature);
}

void localize_features(map_node &node, aligned_list<local_feature> &features)
{
    for(list<map_feature *>::iterator feat = node.features.begin(); feat != node.features.end(); ++feat) {
        features.push_back(local_feature{transformation_apply(node.transform.transform, (*feat)->position), *feat});
    }
}

void mapper::localize_neighbor_features(uint64_t id, aligned_list<local_feature> &features)
{
    for(list<map_edge>::iterator edge = nodes[id].edges.begin(); edge != nodes[id].edges.end(); ++edge) {   
        localize_features(nodes[edge->neighbor], features);
    }
}

static inline float sift_distance(const map_feature &first, const map_feature &second) {
    // ||d1 - d2||   = sqrt(||d1||^2 + ||d2||^2 - 2*d1*d2)
    // since each descriptor has a norm of one:
    // ||d1 - d2||   = sqrt(2 - 2*d1*d2)
    // ||d1 - d2||^2 = 2 - 2*d1*d2
    // ||d1 - d2||^2 = -2 (d2*d1 - 1)
    return 2.f*(1.f - first.dvec.dot(second.dvec));
}

void assign_matches_slow(const aligned_list<local_feature> &f1, const aligned_list<local_feature> &f2, aligned_list<match_pair> &matches) {
    const float max_sift_distance_2 = 0.7*0.7;
    for(aligned_list<local_feature>::const_iterator fi1 = f1.begin(); fi1 != f1.end(); ++fi1) {
        float best_score = max_sift_distance_2;
        aligned_list<local_feature>::const_iterator best = f2.end();
        for(aligned_list<local_feature>::const_iterator candidate = f2.begin(); candidate != f2.end(); ++candidate) {
            if(fi1->feature->id == candidate->feature->id) continue;
            float score = sift_distance(*fi1->feature, *candidate->feature);
            if(score < best_score) {
                best_score = score;
                best = candidate;
            }
        }
        if(best != f2.end() && best_score < max_sift_distance_2) {
            matches.push_back(match_pair{ *fi1, *best, best_score });
        }
    }
}

// Uses the feature id to to matching, and then takes only the best match
void assign_matches_fast(const aligned_list<local_feature> &list1, const aligned_list<local_feature> &list2, aligned_list<match_pair> &matches) {
    const float max_sift_distance_2 = 0.7*0.7;
    for(const local_feature & f1 : list1) {
        float best_score = max_sift_distance_2;
        local_feature best_match;
        for(const local_feature & f2 : list2) {
            if(f1.feature->label == f2.feature->label) {
                float score = sift_distance(*f1.feature, *f2.feature);
                if(score < best_score) {
                    best_score = score;
                    best_match = f2;
                }
            }
        }
        if(best_score < max_sift_distance_2)
            matches.push_back(match_pair{ f1, best_match, best_score });
    }
}

void assign_matches(const aligned_list<local_feature> &f1, const aligned_list<local_feature> &f2, aligned_list<match_pair> &matches, bool fast)
{
    if(fast)
        assign_matches_fast(f1, f2, matches);
    else
        assign_matches_slow(f1, f2, matches);
}

float refine_transformation(const transformation_variance &base, transformation_variance &dR, transformation_variance &dT, const aligned_list<match_pair> &neighbor_matches)
{
    //determine inliers and translation
    v4 total_dT(0., 0., 0., 0.);
    int inliers = 0;
    transformation_variance total = dT * base * dR;
    float resid = 1.e10;
    float meanstd = 0;
    for(aligned_list<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
        meanstd += sqrt(neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
    }
    meanstd /= neighbor_matches.size();
    //log->info("meanvar: {}", meanstd*meanstd);

    for(aligned_list<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
        v4 local = transformation_apply(compose(total.transform, invert(base).transform), neighbor_match->second.position);
        v4 error = neighbor_match->first.position - local;
        resid = error.norm()*error.norm();
        //float threshhold = 3. * 3. * (neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
        float threshhold = 3. * 3. * (2*meanstd*2*meanstd);
        if(resid < threshhold) {
            total_dT = total_dT + error;
            ++inliers;
        }
    }
    assert(inliers);
    dT.transform.T = dT.transform.T + total_dT / inliers;
    return 0;

    //TODO: Fix this
    //double total_theta;
    total = invert(dT * base * dR);
    v4 total_rot;
    inliers = 0;
    for(aligned_list<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
        v4 local = total.transform * neighbor_match->first.position;
        v4 other = invert(base).transform * neighbor_match->second.position;
        v4 error = local - other;
        resid = error.norm()*error.norm();
        float threshhold = 3. * 3. * (neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
        if(resid < threshhold) {
            //now both are in the rotated frame of the other group. find relative rotation
            v4 a = v4(other);
            a[2] = 0.;
            v4 b =v4 (local);
            b[2] = 0.;
            rotation_vector dW_rot = to_rotation_vector(rotation_between_two_vectors(a, b));
            v4 dW = v4(dW_rot.x(), dW_rot.y(), dW_rot.z(), 0);
            //double norm_prod = sum(first * second) / (first.norm() * second.norm());
            //double theta = acos(norm_prod);
            //log->info("{}", theta);
            total_rot = total_rot + dW;
            ++inliers;
        }
    }
    assert(inliers);
    v4 dW = total_rot / inliers;
    //    m4 dR = rodrigues(v4(0., 0., dtheta, 0.), NULL);
    //dR.transform.set_rotation(dR.transform.get_rotation() * rodrigues(dW, NULL));
    return resid;
}

//this just checks matches -- external bits need to make sure not to send it something too close
bool generate_transformation(const match_pair &match1, const match_pair &match2, transformation_variance &trn)
{
    transformation center1, center2;
    center1.T = -match1.first.position;
    center2.T = -match1.second.position;
    v4 v1 = center1*match2.first.position;
    v4 v2 = center2*match2.second.position;
    float d1 = v1.norm(),
          d2 = v2.norm();
    float thresh = 3. * sqrt(match1.first.feature->variance + match1.second.feature->variance + match2.first.feature->variance + match2.second.feature->variance);
    if(fabs(d1-d2) > thresh) return false;

    quaternion q = rotation_between_two_vectors(v2, v1);
    transformation rotation(q, v4(0,0,0,0));
    transformation aggregate = compose(invert(center1), compose(rotation, center2));
    trn.transform = aggregate;
    return true;
}

int mapper::estimate_translation(uint64_t id1, uint64_t id2, v4 &result, int min_inliers, const transformation &pre_transform, const aligned_list<match_pair> &matches, const aligned_list<match_pair> &neighbor_matches)
{
    float meanstd = 0;
    for(aligned_list<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
        meanstd += sqrt(neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
    }
    meanstd /= neighbor_matches.size();
    //log->info("meanvar: {}", meanstd*meanstd);

    int best_score = 0;
    v4 bestdT;
    for(aligned_list<match_pair>::const_iterator match = neighbor_matches.begin(); match != neighbor_matches.end(); ++match) {
        v4 dT = match->first.position - pre_transform * match->second.position;
        int inliers = 0;
        v4 total_dT = v4(0,0,0,0);
        for(aligned_list<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
            v4 thisdT = neighbor_match->first.position - pre_transform * neighbor_match->second.position;
            v4 error = dT - thisdT;
            float resid = error.norm()*error.norm();
            //3 sigma
            //float threshhold = 3.*3. * (neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
            float threshhold = 3. * 3. * (2*meanstd*2*meanstd);

            if(resid < threshhold) {
                ++inliers;
                total_dT = total_dT + thisdT;
            }
        }
        if(inliers > best_score) {
            best_score = inliers;
            bestdT = total_dT / inliers;
        }
    }
    result = bestdT;
    return best_score;
}

int mapper::check_for_matches(uint64_t id1, uint64_t id2, transformation_variance &relpos, int min_inliers)
{
    //get all features for each group and its neighbors into the local frames
    aligned_list<local_feature> f1, f2;
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    //ONLY look for matches in THIS group
    aligned_list<match_pair> matches;
    assign_matches(f1, f2, matches, true);
    //BUT, check support on ENTIRE neighborhood.
    f1.clear();
    f2.clear();
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    localize_neighbor_features(id1, f1);
    localize_neighbor_features(id2, f2);
    aligned_list<match_pair> neighbor_matches;
    assign_matches(f1, f2, neighbor_matches, true);

    int best_score = 0;
    v4 bestdT;
    for(aligned_list<match_pair>::iterator match = neighbor_matches.begin(); match != neighbor_matches.end(); ++match) {
        v4 dT = match->first.position - match->second.position;
        int inliers = 0;
        v4 var = nodes[id2].transform.transform.T;
        for(aligned_list<match_pair>::iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
            v4 error = neighbor_match->first.position - (neighbor_match->second.position + dT);
            float resid = error.norm()*error.norm();
            //3 sigma
            float threshhold = 3.*3. * (neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
            //float baseline_threshhold = norm(neighbor_match->first.position-match->first.position);
            if(resid < threshhold /*&& resid <= baseline_threshhold*/) {
                //neighbor_match->first.position.print();
                //(neighbor_match->second.position + dT).print();
                //log->info(" resid {}, thresh {}, baseth {}", resid, threshhold, baseline_threshhold);
                ++inliers;
            }
        }
        if(inliers > best_score) {
            best_score = inliers;
            bestdT = dT;
        }
    }
    transformation_variance dR; //identity
    transformation_variance dT;
    dT.transform.T = bestdT;
    relpos.transform = nodes[id2].transform.transform * invert(nodes[id1].transform.transform);
    if(best_score >= min_inliers) {
        float residual = refine_transformation(relpos, dR, dT, neighbor_matches);
        relpos = dT * relpos * dR;
    }
    return best_score;

}

void mapper::dump_map(const char *filename)
{
    FILE *group_graph = fopen(filename, "wt");
    fprintf(group_graph, "graph G {\nedge[len=2];\n");
    for(unsigned int node = 0; node < nodes.size(); ++node) {
        for(list<map_edge>::iterator edge = nodes[node].edges.begin(); edge != nodes[node].edges.end(); ++edge) {
            unsigned int neighbor = edge->neighbor;
            if(edge->geometry > 0) {
                fprintf(group_graph, "%d -- %d [dir=forward]\n;", node, neighbor);
            } else if(edge->geometry == 0 && node < neighbor) {
                fprintf(group_graph, "%d -- %d;\n", node, neighbor);
            }
        }
    }
    fprintf(group_graph, "}\n");
    fclose(group_graph);
}

void mapper::breadth_first(int start, int maxdepth, void(mapper::*callback)(map_node &)) {
    queue<int> next;
    list<int> done;
    next.push(start);
    nodes[start].parent = -2;
    nodes[start].depth = 0;
    while (!next.empty()) {
        int u = next.front();
        next.pop();
        if(callback) (this->*callback)(nodes[u]);
        if(!maxdepth || nodes[u].depth <= maxdepth) {
            for(list<map_edge>::iterator edge = nodes[u].edges.begin(); edge != nodes[u].edges.end(); ++edge) {
                int v = edge->neighbor;
                transformation_variance Gr = nodes[u].transform;
                if(edge->geometry && nodes[v].parent == -1) {
                    transformation_variance dG = geometry[abs(edge->geometry)-1];
                    nodes[v].transform = (edge->geometry > 0) ? Gr * dG : Gr * invert(dG);
                    nodes[v].depth = nodes[u].depth + 1;
                    nodes[v].parent = u;
                    next.push(v);
                }
            }
        }
        done.push_back(u);
    }
    for(list<int>::iterator reset = done.begin(); reset != done.end(); ++reset) {
        nodes[*reset].parent = -1;
    }
}

void mapper::print_stats()
{
    log->info("Map nodes: {}", nodes.size());
    log->info("features: {}", feature_count);
    log->info("document frequency:");
    for(uint64_t frequency : document_frequency) {
        log->info("{} ", frequency);
    }
}

void mapper::set_node_transformation(uint64_t id, const transformation & G)
{
    nodes[id].global_transformation = transformation_variance();
    nodes[id].global_transformation.transform = G;
}

void mapper::node_finished(uint64_t id, const transformation & G)
{
    id += node_id_offset;
    set_node_transformation(id, G);
    nodes[id].finished = true;
    for(list<map_edge>::iterator edge = nodes[id].edges.begin(); edge != nodes[id].edges.end(); ++edge) {
        uint64_t nid = edge->neighbor;
        if(nodes[nid].finished && !edge->geometry) {
            //log->info("setting an edge for {} to {}", id, nid);
            transformation_variance tv;
            tv.transform = invert(nodes[id].global_transformation.transform)*nodes[nid].global_transformation.transform;
            internal_set_geometry(id, nid, tv, false);
        }
    }
}
    
transformation_variance rodrigues_variance(const v4 &W, const v4 &W_var, const v4 &T, const v4 &T_var)
{
    /*
    m4v4 dR_dW;
    m4 R = rodrigues(W, &dR_dW);
    m4 R_var;
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            R_var[i][j] = sum(dR_dW[i][j] * dR_dW[i][j] * W_var);
        }
    }
    return transformation_variance(R, R_var, T, T_var);
    //TODO: fix this
    */
    transformation_variance Tv;
    return Tv;
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

    Value nodes_json(kArrayType);
    for(int i = 0; i < nodes.size(); i++) {
        // Only write finished nodes, because geometry is not valid
        // otherwise
        if(!nodes[i].finished) continue;
        Value node_json(kObjectType);
        node_json.AddMember(KEY_NODE_ID, nodes[i].id, allocator);

        Value node_neighbors_json(kArrayType);
        for(list<map_edge>::iterator edge = nodes[i].edges.begin(); edge != nodes[i].edges.end(); ++edge) {
            uint64_t nid = edge->neighbor;
            if(!nodes[nid].finished) continue;
            Value neighbor_id(nid);
            node_neighbors_json.PushBack(neighbor_id, allocator);
        }
        node_json.AddMember(KEY_NODE_NEIGHBORS, node_neighbors_json, allocator);

        Value node_features_json(kArrayType);
        for(auto f : nodes[i].features) {
            Value node_feature_json(kObjectType);

            Value fid(f->id);
            node_feature_json.AddMember(KEY_FEATURE_ID, fid, allocator);

            Value fvariance(f->variance);
            node_feature_json.AddMember(KEY_FEATURE_VARIANCE, fvariance, allocator);

            Value fposition(kArrayType);
            fposition.PushBack(Value(f->position[0]), allocator);
            fposition.PushBack(Value(f->position[1]), allocator);
            fposition.PushBack(Value(f->position[2]), allocator);
            fposition.PushBack(Value(f->position[3]), allocator);
            node_feature_json.AddMember(KEY_FEATURE_POSITION, fposition, allocator);

            Value fdescriptor(kArrayType);
            for(int j = 0; j < descriptor_size; j++)
                fdescriptor.PushBack(Value((double)f->dvec(j)), allocator);
            node_feature_json.AddMember(KEY_FEATURE_DESCRIPTOR, fdescriptor, allocator);

            node_features_json.PushBack(node_feature_json, allocator);
        }
        node_json.AddMember(KEY_FEATURES, node_features_json, allocator);

        Value translation_json(kArrayType);
        translation_json.PushBack(nodes[i].global_transformation.transform.T[0], allocator);
        translation_json.PushBack(nodes[i].global_transformation.transform.T[1], allocator);
        translation_json.PushBack(nodes[i].global_transformation.transform.T[2], allocator);
        translation_json.PushBack(nodes[i].global_transformation.transform.T[3], allocator);
        node_json.AddMember(KEY_NODE_TRANSLATION, translation_json, allocator);

        Value rotation_json(kArrayType);
        rotation_json.PushBack(nodes[i].global_transformation.transform.Q.w(), allocator);
        rotation_json.PushBack(nodes[i].global_transformation.transform.Q.x(), allocator);
        rotation_json.PushBack(nodes[i].global_transformation.transform.Q.y(), allocator);
        rotation_json.PushBack(nodes[i].global_transformation.transform.Q.z(), allocator);
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

            v4 position;
            const Value & feature_position = features[j][KEY_FEATURE_POSITION];
            for(SizeType k = 0; k < feature_position.Size(); k++)
                position[k] = (float)feature_position[k].GetDouble();

            const Value & desc = features[j][KEY_FEATURE_DESCRIPTOR];
            descriptor d;
            for(SizeType k = 0; k < desc.Size(); k++) {
                d.d[k] = (float)desc[k].GetDouble();
            }

            map.add_feature(node_id, feature_id, position, variance, d);
        }

        transformation G;
        const Value & translation = nodes[i][KEY_NODE_TRANSLATION];
        for(SizeType j = 0; j < translation.Size(); j++) {
            G.T[j] = (float)translation[j].GetDouble();
        }

        const Value & rotation = nodes[i][KEY_NODE_QUATERNION];
        G.Q.w() = (float)rotation[0].GetDouble();
        G.Q.x() = (float)rotation[1].GetDouble();
        G.Q.y() = (float)rotation[2].GetDouble();
        G.Q.z() = (float)rotation[3].GetDouble();

        map.node_finished(node_id, G);
        map.nodes[node_id].match_attempted = true;
    }
    map.node_id_offset = max_node_id + 1;
    map.feature_id_offset = max_feature_id + 1;
    map.unlinked = true;
    return true;
}
