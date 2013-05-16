// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include "model.h"

f_t state_vision_feature::initial_rho;
f_t state_vision_feature::initial_var;
f_t state_vision_feature::initial_process_noise;
f_t state_vision_feature::measurement_var;
f_t state_vision_feature::outlier_thresh;
f_t state_vision_feature::outlier_reject;
f_t state_vision_feature::max_variance;
f_t state_vision_feature::min_add_vis_cov;
uint64_t state_vision_group::counter;
uint64_t state_vision_feature::counter;

state_vision_feature::state_vision_feature(f_t initialx, f_t initialy): outlier(0.), initial(initialx, initialy, 1., 0.), current(initial), status(feature_initializing)
{
    id = counter++;
    variance = initial_var;
    v = initial_rho;
    process_noise = initial_process_noise;
}

void state_vision_feature::make_reject()
{
    assert(status != feature_empty);
    status = feature_reject;
}


bool state_vision_feature::make_normal()
{
    assert(status == feature_normal);
    if(statesize < maxstatesize) {
        status = feature_normal;
        statesize++;
        return true;
    } else return false;
}

f_t state_vision_group::ref_noise;
f_t state_vision_group::min_feats;
f_t state_vision_group::min_health;

state_vision_group::state_vision_group(const state_vision_group &other): Tr(other.Tr), Wr(other.Wr), features(other.features), health(other.health), status(other.status)
{
    assert(status == group_normal); //only intended for use at initialization
    children.push_back(&Tr);
    children.push_back(&Wr);
}

state_vision_group::state_vision_group(v4 Tr_i, v4 Wr_i): health(0.), status(group_initializing)
{
    id = counter++;
    children.push_back(&Tr);
    children.push_back(&Wr);
    Tr = Tr_i;
    Wr = Wr_i;
    Tr.process_noise = ref_noise;
    Wr.process_noise = ref_noise;
}

void state_vision_group::make_empty()
{
    for(list <state_vision_feature *>::iterator fiter = features.children.begin(); fiter != features.children.end(); fiter = features.children.erase(fiter)) {
        state_vision_feature *f = *fiter;
        if(f->status == feature_normal) {
            f->make_reject();
            f->Tr = Tr;
            f->Wr = Wr;
        }
    }
    status = group_empty;
}

int state_vision_group::process_features()
{
    f_t cov_sum = 0.;
    int ingroup = 0;
    int good_in_group = 0;
    list<state_vision_feature *>::iterator fiter = features.children.begin(); 
    while(fiter != features.children.end()) {
        state_vision_feature *f = *fiter;
        switch(f->status) {
        case feature_empty:
        case feature_reject:
        case feature_gooddrop:
            fiter = features.children.erase(fiter);
            break;
        case feature_normal:
            if(f->variance < f->max_variance)
                ++good_in_group;
            cov_sum += f->variance;
            ++ingroup;
            ++fiter;
            break;
        default:
            assert(0);
        }
    }
    if(ingroup < min_feats) {
        return 0;
    }        
    //health = ingroup / cov_sum;
    health = good_in_group;
    return ingroup;
}

int state_vision_group::make_reference()
{
    if(status == group_initializing) make_normal();
    assert(status == group_normal);
    status = group_reference;
    //children.remove(&Tr);
    //children.remove(&Wr);
    return 0;
}

int state_vision_group::make_normal()
{
    assert(status == group_initializing);
    children.push_back(&features);
    status = group_normal;
    return 0;
}

state_vision::state_vision(bool _estimate_calibration)
{
    mapperbuf = NULL;
    reference = NULL;
    estimate_calibration = _estimate_calibration;
    children.push_back(&focal_length);
    children.push_back(&center_x);
    children.push_back(&center_y);
    children.push_back(&k1);
    children.push_back(&k2);
    //children.push_back(&k3);
    if(estimate_calibration) {
        children.push_back(&Tc);
        children.push_back(&Wc);
    }
    children.push_back(&groups);
}

state_vision::~state_vision()
{
    list<state_vision_group *>::iterator giter = groups.children.begin();
    while(giter != groups.children.end()) {
        delete *giter;
        giter = groups.children.erase(giter);
    }
    list<state_vision_feature *>::iterator fiter = features.begin();
    while(fiter != features.end()) {
        delete *fiter;
        fiter = features.erase(fiter);
    }
}

void state_vision::get_relative_transformation(const v4 &T, const v4 &W, v4 &rel_T, v4 &rel_W)
{
    v4 Tr, Wr;
    int refnode;
    if(reference) {
        Tr = reference->Tr;
        Wr = reference->Wr;
        refnode = reference->id;
    } else {
        Tr = last_Tr;
        Wr = last_Wr;
        refnode = last_reference;
    }
    m4 Rgr = rodrigues(W, NULL),
        Rwrt = transpose(rodrigues(Wr, NULL));
    rel_T = Rwrt * (T - Tr);
    rel_W = invrodrigues(Rwrt * Rgr, NULL);
}

void state_vision::set_geometry(state_vision_group *g, uint64_t time)
{
    if(g->id == 0 || mapperbuf == NULL) return;
    v4 rel_T, rel_W;
    get_relative_transformation(g->Tr, g->Wr, rel_T, rel_W);
    packet_map_edge_t *mp = (packet_map_edge_t *)mapbuffer_alloc(mapperbuf, packet_map_edge, sizeof(packet_map_edge_t));
    mp->first = reference?reference->id:last_reference;
    mp->second = g->id;
    for(int i = 0; i < 3; ++i) {
        mp->T[i] = rel_T[i];
        mp->W[i] = rel_W[i];
        mp->T_var[i] = g->Tr.variance[i];
        mp->W_var[i] = g->Wr.variance[i];
    }
    mp->header.user = 1;
    mapbuffer_enqueue(mapperbuf, (packet_t*)mp, time);
}

int state_vision::process_features(uint64_t time)
{
    int feats_used = 0;
    bool need_reference = true;
    state_vision_group *best_group = 0;
    int best_health = 0;
    int normal_groups = 0;
    for(list<state_vision_group *>::iterator giter = groups.children.begin(); giter != groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        int feats = g->process_features();
        if(g->status && g->status != group_initializing) feats_used += feats;
        if(!feats) {
            if(g->status == group_reference) {
                last_reference = g->id;
                last_Tr = g->Tr;
                last_Wr = g->Wr;
                reference = 0;
            } else {
                set_geometry(g, time);
            }
            g->make_empty();
        }
        if(g->status == group_reference) need_reference = false;
        if(g->status == group_initializing) {
            if(g->health >= g->min_feats) {
                g->make_normal();
            }
        }
        if(g->status == group_normal) {
            ++normal_groups;
            if(g->health > best_health) {
                best_group = g;
                best_health = g->health;
            }
        }
    }
    if(best_group && need_reference) {
        set_geometry(best_group, time);
        feats_used += best_group->make_reference();
        reference = best_group;
        need_reference = false;
    } else if(!normal_groups && best_group) {
        best_group->make_normal();
    }
    return feats_used;
}

state_vision_feature * state_vision::add_feature(f_t initialx, f_t initialy)
{
    state_vision_feature *f = new state_vision_feature(initialx, initialy);
    f->Tr = T;
    f->Wr = W;
    features.push_back(f);
    //allfeatures.push_back(f);
    return f;
}

state_vision_group * state_vision::add_group(uint64_t time)
{
    state_vision_group *g = new state_vision_group(T, W);
    for(list<state_vision_group *>::iterator giter = groups.children.begin(); giter != groups.children.end(); ++giter) {
        state_vision_group *neighbor = *giter;
        if(mapperbuf) {
            packet_map_edge_t *mp = (packet_map_edge_t *)mapbuffer_alloc(mapperbuf, packet_map_edge, sizeof(packet_map_edge_t));
            mp->first = g->id;
            mp->second = neighbor->id;
            mapbuffer_enqueue(mapperbuf, (packet_t*)mp, time);
        }
        g->old_neighbors.push_back(neighbor->id);
        neighbor->neighbors.push_back(g->id);
    }
    groups.children.push_back(g);
    //allgroups.push_back(g);
    int statesize = remap();
    //initialize ref cov and state - what should initial cov(T,Tr) be?
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < statesize; ++j) {
            cov(g->Tr.index + i, j) = cov(T.index + i, j);
            cov(j, g->Tr.index + i) = cov(j, T.index + i);
            cov(g->Wr.index + i, j) = cov(W.index + i, j);
            cov(j, g->Wr.index + i) = cov(j, W.index + i);
        }
    }
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            cov(g->Tr.index + i, T.index + j) = cov(T.index + i, T.index + j);
            cov(T.index + i, g->Tr.index + j) = cov(T.index + i, T.index + j);
            cov(g->Tr.index + i, W.index + j) = cov(T.index + i, W.index + j);
            cov(W.index + i, g->Tr.index + j) = cov(W.index + i, T.index + j);

            cov(g->Wr.index + i, W.index + j) = cov(W.index + i, W.index + j);
            cov(W.index + i, g->Wr.index + j) = cov(W.index + i, W.index + j);
            cov(g->Wr.index + i, T.index + j) = cov(W.index + i, T.index + j);
            cov(T.index + i, g->Wr.index + j) = cov(T.index + i, W.index + j);

            cov(g->Tr.index + i, g->Tr.index + j) = cov(T.index + i, T.index + j);
            cov(g->Tr.index + i, g->Wr.index + j) = cov(T.index + i, W.index + j);
            cov(g->Wr.index + i, g->Wr.index + j) = cov(W.index + i, W.index + j);
            cov(g->Wr.index + i, g->Tr.index + j) = cov(W.index + i, T.index + j);
        }
        //perturb to make positive definite.
        //TODO: investigate how to fix the model so that this dependency goes away
        cov(g->Wr.index + i, g->Wr.index + i) *= 1.1;
        cov(g->Tr.index + i, g->Tr.index + i) *= 1.1;
    }

  /*    for(int i = 0; i < 3; ++i) {
        cov(g->Tr.index + i, g->Tr.index + i) = cov(T.index + i, T.index + i);
        cov(g->Wr.index + i, g->Wr.index + i) = cov(W.index + i, W.index + i);
        }*/
    return g;
}
