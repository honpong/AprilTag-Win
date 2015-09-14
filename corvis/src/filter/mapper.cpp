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
        float idf = log(1.f * nodes.size() / document_frequency[label]);
        score += count1 * count2 * idf;
    }
    return score;
}

//unused
float mapper::one_to_one_idf_score(const list<map_feature *> &hist1, const list<map_feature *> &hist2)
{
    float score = 0.;
    list<map_feature *>::const_iterator first = hist1.begin(), second = hist2.begin();
    while(first != hist1.end()) {
        while(second != hist2.end() && (*second)->label < (*first)->label) ++second;
        if(second == hist2.end()) break;
        if((*first)->label == (*second)->label) {
            assert(document_frequency[(*first)->label]);
            float idf = log(nodes.size() / document_frequency[(*first)->label]);
            score += idf;
            ++second;
        }
        ++first;
    }
    return score;
}

mapper::mapper(): feature_count(0), no_search(false), feature_dictionary(corvis_dimension, corvis_num_centers, corvis_centers)
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
    local_features.clear();
    origins.clear();
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
    if(nodes.size() <= id1) nodes.resize(id1+1);
    if(nodes.size() <= id2) nodes.resize(id2+1);
    nodes[id1].get_add_neighbor(id2);
    nodes[id2].get_add_neighbor(id1);
}

int mapper::num_features(uint64_t id)
{
    return (int)nodes[id].features.size();
}

void mapper::add_node(uint64_t id, const quaternion & gravity)
{
    if(nodes.size() <= id) nodes.resize(id + 1);
    nodes[id].id = id;
    nodes[id].parent = -1;
    set_node_orientation(id, gravity);
}


map_feature::map_feature(const uint64_t _id, const v4 &p, const float v, const uint32_t l, const descriptor & desc): id(_id), position(p), variance(v), label(l), d(desc)
{
}

bool map_node::add_feature(const uint64_t id, const v4 &pos, const float variance, const uint32_t label, const descriptor & d)
{
    // position is rotated by gravity
    map_feature *feat = new map_feature(id, global_orientation*pos, variance, label, d);
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
            features.push_back(f->d);
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
    for(auto f : nodes[groupid].features) {
        if(f->id == id) {
            // position is rotated by gravity
            f->position = nodes[groupid].global_orientation * pos;
            f->variance = variance;
        }
    }
}

void mapper::add_feature(uint64_t groupid, uint64_t id, const v4 &pos, float variance, const descriptor & d)
{
    uint32_t label = project_feature(d);
    ++feature_count;
    if(nodes[groupid].add_feature(id, pos, variance, label, d)) {
        ++document_frequency[label];
    }
}

void mapper::internal_set_geometry(uint64_t id1, uint64_t id2, const transformation_variance &transform)
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
    }
}

void mapper::set_geometry(uint64_t id1, uint64_t id2, const transformation_variance &transform)
{
    if(nodes.size() <= id1) nodes.resize(id1+1);
    if(nodes.size() <= id2) nodes.resize(id2+1);
    internal_set_geometry(id1, id2, transform);
}

transformation mapper::get_relative_transformation(uint64_t from_id, uint64_t to_id)
{
    nodes[to_id].transform = transformation_variance();
    breadth_first(to_id, 0, NULL);

    return nodes[from_id].transform.transform;
}

void mapper::set_relative_transformation(const transformation &T) {
    relative_transformation = T;
}

/*
vector<map_match> *mapper::new_query(const list<map_feature *> &histogram, size_t K)
{
    vector<map_match> *result = new vector<map_match>();
    result->reserve(K+1);
    int bound = INT_MAX;
    for(uint64_t node = 0; node < nodes.size(); ++node) {
        int dist_2 = map_histogram_dist_2(histogram, nodes[node].histogram, bound);
        if(dist_2 >= bound) continue;
        result->push_back(map_match{node, dist_2});
        push_heap(result->begin(), result->end(), map_match_compare);
        if(result->size() > K) {
            pop_heap(result->begin(), result->end(), map_match_compare);
            result->pop_back();
            bound = (*result)[0].score;
        }
    }
    sort_heap(result->begin(), result->end(), map_match_compare);
    reverse(result->begin(), result->end());
    return result;
}

void mapper::delete_query(vector<map_match> *query)
{
    delete query;
}

void mapper::add_matches(vector<int> &matches, const vector<int> &histogram)
{
    size_t nnodes = nodes.size();
    if(matches.size() < nnodes) matches.resize(nnodes, 0);
    for(size_t i = 0; i < nnodes; ++i) {
        matches[i] += map_histogram_score(histogram, nodes[i].histogram);
    }
}*/

void mapper::tf_idf_match(vector<float> &matches, const list<map_feature *> &histogram)
{
    size_t nnodes = nodes.size();
    if(matches.size() < nnodes) matches.resize(nnodes);
    for(size_t i = 0; i < nnodes; ++i) {
        matches[i] = tf_idf_score(histogram, nodes[i].features);
    }
}

void mapper::diffuse_matches(uint64_t node_id, vector<float> &matches, vector<map_match> &result, int max, int unrecent)
{
    //mark the nodes that are too close to this one
    for(int i = 0; i < matches.size(); ++i) {
        if(nodes[i].id == node_id) continue; // Can't match ourselves
        if(!nodes[i].finished) continue; // Can't match unfinished nodes
        // depth == 0 and nodes[i].id != node_id means we didn't reach
        // it in a breadth first traversal from the current node
        if(nodes[i].depth != 0 && nodes[i].depth <= unrecent) continue;
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

void localize_features(map_node &node, list<local_feature> &features);
void assign_matches(list<local_feature> &f1, list<local_feature> &f2, list<match_pair> &matches);

int mapper::brute_force_rotation(uint64_t id1, uint64_t id2, transformation_variance &trans, int threshhold, float min, float max)
{
    transformation R;
    int best = 0;
    int worst = 100000000;
    f_t first_best, last_best;
    bool running = false;
    assert(max > min);
    //get all features for each group and its neighbors into the local frames
    list<local_feature> f1, f2;
    nodes[id1].transform = transformation_variance();
    nodes[id2].transform = transformation_variance();
    breadth_first(id1, 1, NULL);
    breadth_first(id2, 1, NULL);
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    //ONLY look for matches in THIS group
    list<match_pair> matches;
    assign_matches(f1, f2, matches);
    if(matches.size() == 0) return 0;
    //BUT, check support on ENTIRE neighborhood.
    f1.clear();
    f2.clear();
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    localize_neighbor_features(id1, f1);
    localize_neighbor_features(id2, f2);
    list<match_pair> neighbor_matches;
    assign_matches(f1, f2, neighbor_matches);
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
        return best-worst;
    }
    return 0;
}

bool mapper::get_matches(uint64_t id, vector<map_match> &matches, int max, int suppression)
{
    bool found = false;
    //rebuild the map relative to the current node
    nodes[id].transform = transformation_variance();
    for(auto n : nodes)
        n.depth = 0;
#if 1
    breadth_first(id, 0, NULL);
#else
    // TODO: if we match, need to rotate the resulting transform back
    // into the frame of id? As simple as:
    // matches.g.T = conjugage(nodes[id].global_transformation.transform.Q)*matches.g.T;
    // ?
    nodes[id].transform.transform = transformation(nodes[id].global_transformation.transform.Q, v4(0,0,0,0));
    breadth_first(id, 0, NULL);
    // Breadth first from every disconnected component
    for(auto n : nodes) {
        if(n.depth == 0 && n.id != id) {

            nodes[n.id].transform.transform = transformation(nodes[n.id].global_transformation.transform.Q, v4(0,0,0,0));
            breadth_first(n.id, 0, NULL);
        }
    }
#endif

    list<map_feature *> histogram;
    vector<float> scores;
    joint_histogram(id, histogram);
    tf_idf_match(scores, histogram);
    diffuse_matches(id, scores, matches, max, suppression);
    int best = 0;
    transformation_variance bestg;
    int bestid;
    int threshhold = 8;
    float besttheta = 0.;
    for(int i = 0; i < matches.size(); ++i) {
        transformation_variance g;
        int score = 0;
        float theta = 0.;
        score = check_for_matches(id, matches[i].to, g, threshhold);
        matches[i].score = score;
        matches[i].g = g.transform;
        if(score > best) {
            best = score;
            bestg = g;
            bestid = matches[i].to;
            besttheta = theta;
        }
    }
    sort(matches.begin(), matches.end(), map_match_compare);
    if(best >= threshhold) {
        found = true;
        transformation_variance newT = bestg;
        /*
        if(brute_force_rotation(id, bestid, newT, threshhold, besttheta-M_PI/6., besttheta+M_PI/6.) >= threshhold) {
            fprintf(stderr, "****************** %llu - %d ********************\n", id, bestid);
            found = true;
            matches[0].g = newT.transform;
        }
        */
        //internal_set_geometry(id, bestid, newT);
    }
    return found;
}

bool mapper::find_closure(vector<map_match> &matches, int max, int suppression)
{
    for(int i = 0; i < nodes.size(); i++) {
        if(nodes[i].finished && !nodes[i].match_attempted && i + 10 < nodes.size()) {
            //fprintf(stderr, "searching for loop closure for %llu\n", nodes[i].id);
            nodes[i].match_attempted = true;
            matches.clear();
            if(get_matches(nodes[i].id, matches, max, suppression)) {
                return true;
            }
        }
    }
    return false;
}

static bool local_feature_compare(const local_feature &first, const local_feature &second) {
    return map_feature_compare(first.feature, second.feature);
}

void localize_features(map_node &node, list<local_feature> &features)
{
    for(list<map_feature *>::iterator feat = node.features.begin(); feat != node.features.end(); ++feat) {
        features.push_back(local_feature{transformation_apply(node.transform.transform, (*feat)->position), *feat});
    }
}

void mapper::localize_neighbor_features(uint64_t id, list<local_feature> &features)
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
    float sum = 0;
    for(int i = 0; i < descriptor_size; ++i) {
        sum += first.d.d[i]*second.d.d[i];
    }
    sum = -2 * (sum - 1);
    return sum;
}

void assign_matches(list<local_feature> &f1, list<local_feature> &f2, list<match_pair> &matches) {
    const float max_sift_distance_2 = 0.7*0.7;
    for(list<local_feature>::iterator fi1 = f1.begin(); fi1 != f1.end(); ++fi1) {
        float best_score = FLT_MAX;
        list<local_feature>::iterator best = f2.end();
        for(list<local_feature>::iterator candidate = f2.begin(); candidate != f2.end(); ++candidate) {
            if(fi1->feature->id == candidate->feature->id) continue;
            float score = sift_distance(*fi1->feature, *candidate->feature);
            if(score < best_score) {
                best_score = score;
                best = candidate;
            }
        }
        if(best != f2.end() && best_score < max_sift_distance_2) {
            matches.push_back(match_pair{ *fi1, *best, best_score });
            f2.erase(best);
        }
    }
}

float refine_transformation(const transformation_variance &base, transformation_variance &dR, transformation_variance &dT, const list<match_pair> &neighbor_matches)
{
    //determine inliers and translation
    v4 total_dT(0., 0., 0., 0.);
    int inliers = 0;
    transformation_variance total = dT * base * dR;
    float resid = 1.e10;
    float meanstd = 0;
    for(list<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
        meanstd += sqrt(neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
    }
    meanstd /= neighbor_matches.size();
    //fprintf(stderr, "meanvar: %f\n", meanstd*meanstd);

    for(list<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
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
    for(list<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
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
            //fprintf(stderr, "%f\n", theta);
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

int mapper::new_check_for_matches(uint64_t id1, uint64_t id2, transformation_variance &relpos, int min_inliers)
{
    //get all features for each group and its neighbors into the local frames
    list<local_feature> f1, f2;
    //Compute the relative transformation to neighbors
    nodes[id1].transform = transformation_variance();
    nodes[id2].transform = transformation_variance();
    breadth_first(id1, 1, NULL);
    breadth_first(id2, 1, NULL);
    //Get all features into a local frame
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    //ONLY look for matches in THIS group
    list<match_pair> matches;
    assign_matches(f1, f2, matches);
    //BUT, check support on ENTIRE neighborhood.
    f1.clear();
    f2.clear();
    //Get all node and neighbor features into a local frame
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    localize_neighbor_features(id1, f1);
    localize_neighbor_features(id2, f2);
    list<match_pair> neighbor_matches;
    assign_matches(f1, f2, neighbor_matches);
    int best_score = 0;
    // Now use node matches to generate transformations, then count
    // inliers in the neighbor matches
    // TODO: Why shouldn't this include neighbors to generate matches?
    for(list<match_pair>::iterator match1 = matches.begin(); match1 != matches.end(); ++match1) {
        list<match_pair>::iterator match2 = match1;
        ++match2;
        for(; match2 != matches.end(); ++match2) {
            transformation_variance trn;
            if(generate_transformation(*match1, *match2, trn)) {
                int inliers = 0;
                for(list<match_pair>::iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
                    v4 error = neighbor_match->first.position - (trn.transform * neighbor_match->second.position);
                    float resid = error.norm()*error.norm();
                    //3 sigma
                    float threshhold = 3.*3. * (neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
                    //float baseline_threshhold = norm(neighbor_match->first.position-match->first.position);
                    if(resid < threshhold /*&& resid <= baseline_threshhold*/) {
                        //neighbor_match->first.position.print();
                        //(neighbor_match->second.position + dT).print();
                        //fprintf(stderr, " resid %f, thresh %f, baseth %f\n", resid, threshhold, baseline_threshhold);
                        ++inliers;
                    }
                }
                if(inliers > best_score) {
                    best_score = inliers;
                    relpos = trn;
                }
            }
        }
    }
    return best_score;
}

int mapper::estimate_translation(uint64_t id1, uint64_t id2, v4 &result, int min_inliers, const transformation &pre_transform, const list <match_pair> &matches, const list<match_pair> &neighbor_matches)
{
    int best_score = 0;
    v4 bestdT;
    for(list<match_pair>::const_iterator match = matches.begin(); match != matches.end(); ++match) {
        v4 dT = match->first.position - pre_transform * match->second.position;
        int inliers = 0;
        v4 total_dT = v4(0,0,0,0);
        for(list<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
            v4 thisdT = neighbor_match->first.position - pre_transform * neighbor_match->second.position;
            v4 error = dT - thisdT;
            float resid = error.norm()*error.norm();
            //3 sigma
            float threshhold = 3.*3. * (neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
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
    list<local_feature> f1, f2;
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    //ONLY look for matches in THIS group
    list<match_pair> matches;
    assign_matches(f1, f2, matches);
    //BUT, check support on ENTIRE neighborhood.
    f1.clear();
    f2.clear();
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    localize_neighbor_features(id1, f1);
    localize_neighbor_features(id2, f2);
    list<match_pair> neighbor_matches;
    assign_matches(f1, f2, neighbor_matches);

    int best_score = 0;
    v4 bestdT;
    for(list<match_pair>::iterator match = matches.begin(); match != matches.end(); ++match) {
        v4 dT = match->first.position - match->second.position;
        int inliers = 0;
        v4 var = nodes[id2].transform.transform.T;
        for(list<match_pair>::iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
            v4 error = neighbor_match->first.position - (neighbor_match->second.position + dT);
            float resid = error.norm()*error.norm();
            //3 sigma
            float threshhold = 3.*3. * (neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
            //float baseline_threshhold = norm(neighbor_match->first.position-match->first.position);
            if(resid < threshhold /*&& resid <= baseline_threshhold*/) {
                //neighbor_match->first.position.print();
                //(neighbor_match->second.position + dT).print();
                //fprintf(stderr, " resid %f, thresh %f, baseth %f\n", resid, threshhold, baseline_threshhold);
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
    fprintf(stderr, "nodes: %lu\n", nodes.size());
    fprintf(stderr, "features: %llu\n", feature_count);
}

void mapper::set_node_transformation(uint64_t id, const transformation & G)
{
    nodes[id].global_transformation = transformation_variance();
    nodes[id].global_transformation.transform = G;
}

void mapper::set_node_orientation(uint64_t id, const quaternion & q)
{
    nodes[id].global_orientation = q;
}

void mapper::node_finished(uint64_t id, const transformation & G)
{
    set_node_transformation(id, G);
    nodes[id].finished = true;
    for(list<map_edge>::iterator edge = nodes[id].edges.begin(); edge != nodes[id].edges.end(); ++edge) {
        uint64_t nid = edge->neighbor;
        if(nodes[nid].finished && !edge->geometry) {
            //fprintf(stderr, "setting an edge for %llu to %llu\n", id, nid);
            transformation_variance tv;
            tv.transform = invert(nodes[id].global_transformation.transform)*nodes[nid].global_transformation.transform;
            set_geometry(id, nid, tv);
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
