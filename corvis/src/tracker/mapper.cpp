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
    log->debug("Map reset");
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


map_feature::map_feature(const uint64_t _id, const v3 &p, const float v, const uint32_t l, const descriptor & desc): id(_id), position(p), variance(v), label(l)
{
    Eigen::Map<const Eigen::VectorXf, Eigen::Unaligned> eigen_d(desc.d, descriptor_size, 1);
    dvec = Eigen::VectorXf(eigen_d);
}

bool map_node::add_feature(const uint64_t id, const v3 &pos, const float variance, const uint32_t label, const descriptor & d)
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

void mapper::update_feature_position(uint64_t groupid, uint64_t id, const v3 &pos, float variance)
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

void mapper::add_feature(uint64_t groupid, uint64_t id, const v3 &pos, float variance, const descriptor & d)
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
void assign_matches(const aligned_list<local_feature> &f1, const aligned_list<local_feature> &f2, aligned_vector<match_pair> &matches, bool slow);

int mapper::ransac_transformation(uint64_t id1, uint64_t id2, transformation_variance &trans)
{
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
    aligned_vector<match_pair> matches;
    assign_matches(f1, f2, matches, false);
    if(matches.size() == 0) return 0;
    //BUT, check support on ENTIRE neighborhood.
    f1.clear();
    f2.clear();
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    localize_neighbor_features(id1, f1);
    localize_neighbor_features(id2, f2);
    aligned_vector<match_pair> neighbor_matches;
    assign_matches(f1, f2, neighbor_matches, false);

    int min_inliers = 3;
    int best_inliers = 0;
    transformation_variance proposal;
    int inliers = pick_transformation_ransac(neighbor_matches, proposal);
    if(inliers > best_inliers) {
        trans = proposal;
        best_inliers = inliers;
    }
    //log->info("check for matches {}, {} best_score {}", id1, id2, best_inliers);
    if(best_inliers >= min_inliers) {
        //log->info("got something {}, {} best_score {}", id1, id2, best_inliers);
        best_inliers = estimate_transform_with_inliers(neighbor_matches, trans);
        trans.transform = invert(Gw1)*trans.transform*Gw2;
        return best_inliers-min_inliers;
    }
    return 0;
}

bool mapper::get_matches(uint64_t id, map_match & m, int max, int suppression)
{
    aligned_vector<map_match> matches;
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
            matches[i].score = ransac_transformation(matches[i].from, matches[i].to, g);
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
            int brute_score = ransac_transformation(matches[i].from, matches[i].to, newT);
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
            m = matches[0];
        }
    }
    return found;
}

void mapper::rebuild_map_from_node(int id)
{
    nodes[id].transform = transformation_variance();
    nodes[id].transform.transform = nodes[id].global_transformation.transform;
    breadth_first(id, 0, NULL);
    int start_id = node_id_offset;
    if(!unlinked)
        start_id = 0;
    for(int i = start_id; i < nodes.size(); i++) {
        if(!nodes[i].finished) continue;
        //log->info("setting {} from {} ",i, nodes[i].global_transformation.transform);
        //log->info("to {}", nodes[i].transform.transform);
        nodes[i].global_transformation = nodes[i].transform;
    }
}

bool mapper::find_closure(int max, int suppression, transformation & offset)
{
    for(int i = 0; i < nodes.size(); i++) {
        // the first node (node_id_offset) must be finished since we
        // rebuild the map relative to this node
        if(nodes[node_id_offset].finished && nodes[i].finished && !nodes[i].match_attempted && i + 10 < nodes.size()) {
            //log->info("searching for loop closure for {}", nodes[i].id);
            nodes[i].match_attempted = true;
            map_match m;
            if(get_matches(nodes[i].id, m, max, suppression)) {
                transformation old_t = nodes[m.from].global_transformation.transform;

                transformation_variance tv;
                tv.transform = m.g;
                internal_set_geometry(m.from, m.to, tv, true);
                if(unlinked && m.to < node_id_offset) {
                    unlinked = false;
                }
                rebuild_map_from_node(node_id_offset);

                transformation new_t = nodes[m.from].global_transformation.transform;
                offset = new_t * invert(old_t);
                //log->info("add this to offset: {}", offset);
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

void assign_matches_slow(const aligned_list<local_feature> &f1, const aligned_list<local_feature> &f2, aligned_vector<match_pair> &matches) {
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
void assign_matches_fast(const aligned_list<local_feature> &list1, const aligned_list<local_feature> &list2, aligned_vector<match_pair> &matches) {
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

void assign_matches(const aligned_list<local_feature> &f1, const aligned_list<local_feature> &f2, aligned_vector<match_pair> &matches, bool fast)
{
    if(fast)
        assign_matches_fast(f1, f2, matches);
    else
        assign_matches_slow(f1, f2, matches);
}

bool generate_transformation(const match_pair &match1, const match_pair &match2, transformation_variance &trn, float threshold)
{
    v3 to = match2.first.position - match1.first.position;
    v3 from = match2.second.position - match1.second.position;
    // project to z = 0
    from[2] = 0;
    to[2] = 0;

    if(from.norm() < F_T_EPS || to.norm() < F_T_EPS)
        return false;

    quaternion Q = rotation_between_two_vectors(from, to);
    // alternatively:
    // float theta = acos(v1.dot(v2) / v1.norm() / v2.norm());
    // quaternion Q = to_quaternion(rotation_vector(0,0,theta));

    // translate from the first position to zero, rotate, translate
    // back to the second position
    trn.transform = transformation(Q, match1.first.position - Q*match1.second.position);

    // Reject transformations that do not align the second point to
    // within the threshold
    float dist = (trn.transform*match2.second.position - match2.first.position).norm(); 
    //fprintf(stderr, "DIST %f\n", dist);
    //std::cerr << "trn " << trn.transform << "\n";
    if(dist*dist > threshold)
        return false;

    return true;
}


static vector<bool> get_inliers(const transformation_variance & proposal, const aligned_vector<match_pair> & matches, float threshold)
{
    int num_inliers = 0, z = 0;
    float mean_error = 0;
    vector<bool> inliers; inliers.resize(matches.size());
    for(auto m : matches) {
        float dist = (m.first.position - proposal.transform*m.second.position).norm();
        if(dist*dist < threshold) {
            mean_error += dist;
            num_inliers++;
            inliers[z] = true;
        }
        else
            inliers[z] = false;
        z++;
    }
    mean_error /= z;
    //fprintf(stderr, "mean error %f\n", mean_error);
    return inliers;
}

static size_t count_inliers(const transformation_variance & proposal, const aligned_vector<match_pair> & matches, float threshold)
{
    return get_inliers(proposal, matches, threshold).size();
}

#include <random>
int mapper::pick_transformation_ransac(const aligned_vector<match_pair> &neighbor_matches,  transformation_variance & G)
{
    if(neighbor_matches.size() < 2) return 0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> first(0, neighbor_matches.size()-1);
    std::uniform_int_distribution<> second(0, neighbor_matches.size()-2);

    float meanstd = 0;
    for(aligned_vector<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
        meanstd += sqrt(neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
    }
    meanstd /= neighbor_matches.size();
    float threshold = 3. * 3. * (2*meanstd*2*meanstd);
    //log->info("pick_ransac meanvar: {}", meanstd*meanstd);

    int best_inliers = 0;
    // log(1 - 0.99) / log(1 - p_outlier^2)
    // p_inlier = 0.5 (50%)
    // = 16
    // p_inlier = 0.25 (25%)
    // = 71
    for(int i = 0; i < std::max((int)neighbor_matches.size(), 71); i++) { /* TODO: adapt dynamically */
        int i1 = first(gen);
        int i2 = second(gen);
        if(i1 == i2)
            i2 = neighbor_matches.size()-2;

        transformation_variance proposal;
        //log->info("proposal is {} {}", i1, i2);
        if(generate_transformation(neighbor_matches[i1], neighbor_matches[i2], proposal, threshold)) {
            //log->info() << proposal.transform;
            size_t inliers = count_inliers(proposal, neighbor_matches, threshold);
            //log->info("inliers {}", inliers);
            if(inliers > best_inliers) {
                G = proposal;
                best_inliers = inliers;
            }
            if(best_inliers > 8 && best_inliers > neighbor_matches.size()*0.7)
                break;
        }

    }
    //log->info("best {}\n", best_inliers);
    return best_inliers;
}

int mapper::estimate_transform_with_inliers(const aligned_vector<match_pair> & matches, transformation_variance & tv)
{
    float meanstd = 0;
    for(auto m : matches) {
        meanstd += sqrt(m.first.feature->variance + m.second.feature->variance);
    }
    meanstd /= matches.size();
    //log->info("estimate transform meanvar: {}", meanstd*meanstd);
    float loose_threshold = 3. * 3. * (2*meanstd*2*meanstd);
    float tight_threshold = 3. * 3. * (2*meanstd*2*meanstd);
    vector<bool> inliers = get_inliers(tv, matches, loose_threshold);
    //log->info("num loose inliers {}", num_inliers);
    //log->info("old tv {}", tv.transform);

    aligned_vector<v3> from;
    aligned_vector<v3> to;
    for(int i = 0; i < matches.size(); i++) {
        if(inliers[i]) {
            from.push_back(matches[i].second.position);
            to.push_back(matches[i].first.position);
        }
    }

    transformation_variance estimate;
    if(estimate_transformation(from, to, estimate.transform)) {
        size_t new_inliers = count_inliers(estimate, matches, tight_threshold);
        //log->info("new tight inliers {}", new_inliers);
        //log->info("new tv {}", estimate.transform);
        //TODO: only do this when it is better, is it always better?
        tv = estimate;
        return new_inliers;
    }
    else {
        log->info("ESTIMATE TRANSFORM FAILED, {} and {} points", from.size(), to.size());
        //for(int i = 0; i < matches.size(); i++) {
        //    log->info("from: {} to: {}", from[i], to[i]);
        //}
    }
    return (int)inliers.size();
}

float mapper::refine_transformation(const transformation_variance &base, transformation_variance &dR, transformation_variance &dT, const aligned_vector<match_pair> &neighbor_matches)
{
    //determine inliers and translation
    v3 total_dT(0., 0., 0.);
    int inliers = 0;
    transformation_variance total = dT * base * dR;
    float resid = 1.e10;
    float meanstd = 0;
    for(aligned_vector<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
        meanstd += sqrt(neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
    }
    meanstd /= neighbor_matches.size();
    //log->info("meanvar: {}", meanstd*meanstd);

    for(aligned_vector<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
        v3 local = transformation_apply(compose(total.transform, invert(base).transform), neighbor_match->second.position);
        v3 error = neighbor_match->first.position - local;
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
    v3 total_rot;
    inliers = 0;
    for(aligned_vector<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
        v3 local = total.transform * neighbor_match->first.position;
        v3 other = invert(base).transform * neighbor_match->second.position;
        v3 error = local - other;
        resid = error.norm()*error.norm();
        float threshhold = 3. * 3. * (neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
        if(resid < threshhold) {
            //now both are in the rotated frame of the other group. find relative rotation
            v3 a = v3(other);
            a[2] = 0.;
            v3 b =v3 (local);
            b[2] = 0.;
            rotation_vector dW_rot = to_rotation_vector(rotation_between_two_vectors(a, b));
            v3 dW = v3(dW_rot.x(), dW_rot.y(), dW_rot.z());
            //double norm_prod = sum(first * second) / (first.norm() * second.norm());
            //double theta = acos(norm_prod);
            //log->info("{}", theta);
            total_rot = total_rot + dW;
            ++inliers;
        }
    }
    assert(inliers);
    //v3 dW = total_rot / inliers;
    //    m4 dR = rodrigues(v4(0., 0., dtheta, 0.), NULL);
    //dR.transform.set_rotation(dR.transform.get_rotation() * rodrigues(dW, NULL));
    return resid;
}

int mapper::estimate_translation(uint64_t id1, uint64_t id2, v3 &result, int min_inliers, const transformation &pre_transform, const aligned_vector<match_pair> &matches, const aligned_vector<match_pair> &neighbor_matches)
{
    float meanstd = 0;
    for(aligned_vector<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
        meanstd += sqrt(neighbor_match->first.feature->variance + neighbor_match->second.feature->variance);
    }
    meanstd /= neighbor_matches.size();
    //log->info("meanvar: {}", meanstd*meanstd);

    int best_score = 0;
    v3 bestdT;
    for(aligned_vector<match_pair>::const_iterator match = neighbor_matches.begin(); match != neighbor_matches.end(); ++match) {
        v3 dT = match->first.position - pre_transform * match->second.position;
        int inliers = 0;
        v3 total_dT = v3(0,0,0);
        for(aligned_vector<match_pair>::const_iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
            v3 thisdT = neighbor_match->first.position - pre_transform * neighbor_match->second.position;
            v3 error = dT - thisdT;
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
    aligned_vector<match_pair> matches;
    assign_matches(f1, f2, matches, true);
    //BUT, check support on ENTIRE neighborhood.
    f1.clear();
    f2.clear();
    localize_features(nodes[id1], f1);
    localize_features(nodes[id2], f2);
    localize_neighbor_features(id1, f1);
    localize_neighbor_features(id2, f2);
    aligned_vector<match_pair> neighbor_matches;
    assign_matches(f1, f2, neighbor_matches, true);

    int best_score = 0;
    v3 bestdT;
    for(aligned_vector<match_pair>::iterator match = matches.begin(); match != matches.end(); ++match) {
        v3 dT = match->first.position - match->second.position;
        int inliers = 0;
        v3 var = nodes[id2].transform.transform.T;
        for(aligned_vector<match_pair>::iterator neighbor_match = neighbor_matches.begin(); neighbor_match != neighbor_matches.end(); ++neighbor_match) {
            v3 error = neighbor_match->first.position - (neighbor_match->second.position + dT);
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

void mapper::dump_map(FILE *group_graph)
{
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
    id += node_id_offset;
    nodes[id].global_transformation = transformation_variance();
    nodes[id].global_transformation.transform = G;
}

void mapper::node_finished(uint64_t id)
{
    id += node_id_offset;
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
    
transformation_variance rodrigues_variance(const v3 &W, const v3 &W_var, const v3 &T, const v3 &T_var)
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

    vector<uint64_t> id_map; id_map.resize(nodes.size());
    uint64_t to = 0;
    for(uint64_t from = 0; from < nodes.size(); from++) {
        if(!nodes[from].finished) {
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
        if(!nodes[i].finished) continue;
        Value node_json(kObjectType);
        node_json.AddMember(KEY_NODE_ID, id_map[nodes[i].id], allocator);

        Value node_neighbors_json(kArrayType);
        for(list<map_edge>::iterator edge = nodes[i].edges.begin(); edge != nodes[i].edges.end(); ++edge) {
            uint64_t nid = edge->neighbor;
            if(!nodes[nid].finished) continue;
            Value neighbor_id(id_map[nid]);
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
            fposition.PushBack(f->position[0], allocator);
            fposition.PushBack(f->position[1], allocator);
            fposition.PushBack(f->position[2], allocator);
            node_feature_json.AddMember(KEY_FEATURE_POSITION, fposition, allocator);

            Value fdescriptor(kArrayType);
            for(int j = 0; j < descriptor_size; j++)
                fdescriptor.PushBack(f->dvec(j), allocator);
            node_feature_json.AddMember(KEY_FEATURE_DESCRIPTOR, fdescriptor, allocator);

            node_features_json.PushBack(node_feature_json, allocator);
        }
        node_json.AddMember(KEY_FEATURES, node_features_json, allocator);

        Value translation_json(kArrayType);
        translation_json.PushBack(nodes[i].global_transformation.transform.T[0], allocator);
        translation_json.PushBack(nodes[i].global_transformation.transform.T[1], allocator);
        translation_json.PushBack(nodes[i].global_transformation.transform.T[2], allocator);
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

            v3 position = v3::Zero();
            const Value & feature_position = features[j][KEY_FEATURE_POSITION];
            for(SizeType k = 0; k < position.size() && k < feature_position.Size(); k++)
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
        map.nodes[node_id].match_attempted = true;
    }
    map.node_id_offset = max_node_id + 1;
    map.feature_id_offset = max_feature_id + 1;
    map.unlinked = true;
    map.log->info("Loaded map with {} nodes and {} features", map.node_id_offset, map.feature_id_offset);
    return true;
}
