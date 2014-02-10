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

state_vision_feature::state_vision_feature(f_t initialx, f_t initialy): outlier(0.), initial(initialx, initialy, 1., 0.), current(initial), status(feature_initializing), user(false)
{
    id = counter++;
    set_initial_variance(initial_var);
    v = initial_rho;
    set_process_noise(initial_process_noise);
    image_velocity.x = 0;
    image_velocity.y = 0;
}

void state_vision_feature::dropping_group()
{
    //TODO: keep features after group is gone
    if(status != feature_empty) drop();
}

void state_vision_feature::drop()
{
    if(is_good()) status = feature_gooddrop;
    else status = feature_empty;
}

bool state_vision_feature::should_drop() const
{
    return status == feature_empty || status == feature_reject || status == feature_gooddrop;
}

bool state_vision_feature::is_valid() const
{
    return (status == feature_initializing || status == feature_ready || status == feature_normal);
}

bool state_vision_feature::is_good() const
{
    return is_valid() && variance() < max_variance;
}

bool state_vision_feature::force_initialize()
{
    if(status == feature_initializing) {
        //not ready yet, so reset
        v = initial_rho;
        (*cov)(index, index) = initial_var;
        status = feature_normal;
        return true;
    } else if(status == feature_ready) {
        status = feature_normal;
        return true;
    }
    return false;
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

state_vision_group::state_vision_group(const state_vector &T, const state_rotation_vector &W): health(0.), status(group_initializing)
{
    id = counter++;
    children.push_back(&Tr);
    children.push_back(&Wr);
    Tr.v = T.v;
    Wr.v = W.v;
    Tr.set_initial_variance(T.variance()[0], T.variance()[1], T.variance()[2]);
    Wr.set_initial_variance(W.variance()[0], W.variance()[1], W.variance()[2]);
    Tr.set_process_noise(ref_noise);
    Wr.set_process_noise(ref_noise);
}

void state_vision_group::make_empty()
{
    for(list <state_vision_feature *>::iterator fiter = features.children.begin(); fiter != features.children.end(); fiter = features.children.erase(fiter)) {
        state_vision_feature *f = *fiter;
        f->dropping_group();
    }
    status = group_empty;
}

int state_vision_group::process_features()
{
    int ingroup = 0;
    int good_in_group = 0;
    list<state_vision_feature *>::iterator fiter = features.children.begin(); 
    while(fiter != features.children.end()) {
        state_vision_feature *f = *fiter;
        if(f->should_drop()) {
            fiter = features.children.erase(fiter);
        } else {
            if(f->is_good()) ++good_in_group;
            ++ingroup;
            ++fiter;
        }
    }
    if(ingroup < min_feats) {
        return 0;
    }
    health = good_in_group;
    return ingroup;
}

int state_vision_group::make_reference()
{
    if(status == group_initializing) make_normal();
    assert(status == group_normal);
    status = group_reference;
    int normals = 0;
    for(list <state_vision_feature *>::iterator fiter = features.children.begin(); fiter != features.children.end(); fiter++) {
        if((*fiter)->is_initialized()) ++normals;
    }
    if(normals < 3) {
        for(list<state_vision_feature *>::iterator fiter = features.children.begin(); fiter != features.children.end(); fiter++) {
            if(!(*fiter)->is_initialized()) {
                if ((*fiter)->force_initialize()) ++normals;
                if(normals >= 3) break;
            }
        }
    }
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

state_vision::state_vision(bool _estimate_calibration, covariance &c): state_motion(c)
{
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

int state_vision::process_features(uint64_t time)
{
    int feats_used = 0;
    bool need_reference = true;
    state_vision_group *best_group = 0;
    int best_health = -1;
    int normal_groups = 0;
    for(list<state_vision_group *>::iterator giter = groups.children.begin(); giter != groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        int feats = g->process_features();
        if(g->status && g->status != group_initializing) feats_used += feats;
        if(!feats) {
            if(g->status == group_reference) {
                last_reference = g->id;
                last_Tr = g->Tr.v;
                last_Wr = g->Wr.v;
                reference = 0;
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
        feats_used += best_group->make_reference();
        reference = best_group;
    } else if(!normal_groups && best_group) {
        best_group->make_normal();
    }
    return feats_used;
}

state_vision_feature * state_vision::add_feature(f_t initialx, f_t initialy)
{
    state_vision_feature *f = new state_vision_feature(initialx, initialy);
    f->Tr = T.v;
    f->Wr = W.v;
    features.push_back(f);
    //allfeatures.push_back(f);
    return f;
}

void state_vision::project_new_group_covariance(matrix &dst, const matrix &src, const state_vision_group &g)
{
    for(int i = 0; i < src.rows; ++i)
    {
        v4 cov_T = T.copy_cov_from_row(src, i);
        g.Tr.copy_cov_to_col(dst, i, cov_T);
        v4 cov_W = W.copy_cov_from_row(src, i);
        g.Wr.copy_cov_to_col(dst, i, cov_W);
    }
}

void state_vision::propagate_new_group(const state_vision_group &g)
{
    project_new_group_covariance(cov.cov, cov.cov, g);
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < cov.size(); ++j) {
            cov.cov(j, g.Tr.index + i) = cov.cov(g.Tr.index + i, j);
        }
    }
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < cov.size(); ++j) {
            cov.cov(j, g.Wr.index + i) = cov.cov(g.Wr.index + i, j);
        }
    }
    project_new_group_covariance(cov.cov, cov.cov, g);
}

state_vision_group * state_vision::add_group(uint64_t time)
{
    state_vision_group *g = new state_vision_group(T, W);
    for(list<state_vision_group *>::iterator giter = groups.children.begin(); giter != groups.children.end(); ++giter) {
        state_vision_group *neighbor = *giter;
        g->old_neighbors.push_back(neighbor->id);
        neighbor->neighbors.push_back(g->id);
    }
    groups.children.push_back(g);
    remap();
#ifdef TEST_POSDEF
    if(!test_posdef(cov.cov)) fprintf(stderr, "not pos def before propagating group\n");
#endif
    propagate_new_group(*g);
    for(int i = 0; i < 3; ++i) {
        //perturb to make positive definite.
        //TODO: investigate how to fix the model so that this dependency goes away
        //TODO: this is clearly wrong. lower is worse, higher is not necessarily worse. weird
#warning I think I can get rid of this now! (no, actually fails positive definiteness test without this)
        cov.cov(g->Wr.index + i, g->Wr.index + i) *= 1.1;
        cov.cov(g->Tr.index + i, g->Tr.index + i) *= 1.1;
    }
#ifdef TEST_POSDEF
    if(!test_posdef(cov.cov)) fprintf(stderr, "not pos def after propagating group\n");
#endif
    return g;
}

void state_vision::fill_calibration(feature_t &initial, f_t &r2, f_t &r4, f_t &r6, f_t &kr) const
{
    r2 = initial.x * initial.x + initial.y * initial.y;
    r4 = r2 * r2;
    r6 = r4 * r2;
    kr = 1. + r2 * k1.v + r4 * k2.v + r6 * k3.v;
}

feature_t state_vision::calibrate_feature(const feature_t &initial)
{
    feature_t norm, calib;
    f_t r2, r4, r6, kr;
    norm.x = (initial.x - center_x.v) / focal_length.v;
    norm.y = (initial.y - center_y.v) / focal_length.v;
    //forward calculation - guess calibrated from initial
    fill_calibration(norm, r2, r4, r6, kr);
    calib.x = norm.x / kr;
    calib.y = norm.y / kr;
    //backward calbulation - use calibrated guess to get new parameters and recompute
    fill_calibration(calib, r2, r4, r6, kr);
    calib.x = norm.x / kr;
    calib.y = norm.y / kr;
    return calib;
}
