// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include "state_vision.h"

f_t state_vision_feature::initial_depth_meters;
f_t state_vision_feature::initial_var;
f_t state_vision_feature::initial_process_noise;
f_t state_vision_feature::measurement_var;
f_t state_vision_feature::outlier_thresh;
f_t state_vision_feature::outlier_reject;
f_t state_vision_feature::max_variance;
uint64_t state_vision_group::counter = 0;
uint64_t state_vision_feature::counter = 0;

state_vision_feature::state_vision_feature(f_t initialx, f_t initialy): state_leaf("feature"), outlier(0.), initial(initialx, initialy, 1., 0.), current(initial), status(feature_initializing)
{
    id = counter++;
    set_initial_variance(initial_var);
    innovation_variance_x = 0.;
    innovation_variance_y = 0.;
    innovation_variance_xy = 0.;
    v.set_depth_meters(initial_depth_meters);
    set_process_noise(initial_process_noise);
    dt = sensor_clock::duration(0);
    last_dt = sensor_clock::duration(0);
    image_velocity.x = 0;
    image_velocity.y = 0;
    world = v4(0, 0, 0, 0);
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
    return status == feature_empty || status == feature_gooddrop;
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
        v.set_depth_meters(initial_depth_meters);
        (*cov)(index, index) = initial_var;
        status = feature_normal;
        return true;
    } else if(status == feature_ready) {
        status = feature_normal;
        return true;
    }
    return false;
}

f_t state_vision_group::ref_noise;
f_t state_vision_group::min_feats;

state_vision_group::state_vision_group(const state_vision_group &other): Tr(other.Tr), Wr(other.Wr), features(other.features), health(other.health), status(other.status)
{
    assert(status == group_normal); //only intended for use at initialization
    children.push_back(&Tr);
    children.push_back(&Wr);
}

state_vision_group::state_vision_group(): Tr("Tr"), Wr("Wr"), health(0), status(group_initializing)
{
    id = counter++;
    Tr.dynamic = true;
    Wr.dynamic = true;
    children.push_back(&Tr);
    children.push_back(&Wr);
    Tr.v = v4(0., 0., 0., 0.);
    Wr.v = rotation_vector();
    Tr.set_initial_variance(0., 0., 0.);
    Wr.set_initial_variance(0., 0., 0.);
    Tr.set_process_noise(ref_noise);
    Wr.set_process_noise(ref_noise);
}

void state_vision_group::make_empty()
{
    for(state_vision_feature *f : features.children)
        f->dropping_group();
    features.children.clear();
    status = group_empty;
}

int state_vision_group::process_features()
{
    features.children.remove_if([&](state_vision_feature *f) {
        return f->should_drop();
    });

    health = features.children.size();
    if(health < min_feats)
        health = 0;

    return health;
}

int state_vision_group::make_reference()
{
    if(status == group_initializing) make_normal();
    assert(status == group_normal);
    status = group_reference;
    int normals = 0;
    for(state_vision_feature *f : features.children) {
        if(f->is_initialized()) ++normals;
    }
    if(normals < 3) {
        for(state_vision_feature *f : features.children) {
            if(!f->is_initialized()) {
                if (f->force_initialize()) ++normals;
                if(normals >= 3) break;
            }
        }
    }
    //remove_child(&Tr);
    //Wr.saturate();
    return 0;
}

int state_vision_group::make_normal()
{
    assert(status == group_initializing);
    children.push_back(&features);
    status = group_normal;
    return 0;
}

state_vision::state_vision(covariance &c): state_motion(c), Tc("Tc"), Wc("Wc"), focal_length("focal_length"), center_x("center_x"), center_y("center_y"), k1("k1"), k2("k2"), k3("k3"), fisheye(false), total_distance(0.), last_position(v4::Zero()), reference(nullptr)
{
    reference = NULL;
    if(estimate_camera_intrinsics)
    {
        children.push_back(&focal_length);
        children.push_back(&center_x);
        children.push_back(&center_y);
        children.push_back(&k1);
        children.push_back(&k2);
    }
    if(estimate_camera_extrinsics) {
        children.push_back(&Tc);
        children.push_back(&Wc);
    }
    children.push_back(&groups);
}

void state_vision::clear_features_and_groups()
{
    for(state_vision_group *g : groups.children)
        delete g;
    groups.children.clear();
    for(state_vision_feature *i : features)
        delete i;
    features.clear();
}

state_vision::~state_vision()
{
    clear_features_and_groups();
}

void state_vision::reset()
{
    clear_features_and_groups();
    reference = NULL;
    total_distance = 0.;
    state_motion::reset();
}

void state_vision::reset_position()
{
    T.v = v4::Zero();
    total_distance = 0.;
    last_position = v4::Zero();
}

int state_vision::process_features(sensor_clock::time_point time)
{
    int useful_drops = 0;
    int total_feats = 0;
    int outliers = 0;
    int track_fail = 0;
    int toobig = statesize - maxstatesize;
    //TODO: revisit this - should check after dropping other features, make this more intelligent
    if(toobig > 0) {
        int dropped = 0;
        vector<f_t> vars;
        for(state_vision_feature *i : features) {
            vars.push_back(i->variance());
        }
        std::sort(vars.begin(), vars.end());
        if((int)vars.size() > toobig) {
            f_t min = vars[vars.size() - toobig];
            for(state_vision_feature *i : features) {
                if(i->variance() >= min) {
                    i->status = feature_empty;
                    ++dropped;
                    if(dropped >= toobig) break;
                }
            }
            if (log_enabled) fprintf(stderr, "state is %d too big, dropped %d features, min variance %f\n",toobig, dropped, min);
        }
    }
    for(state_vision_feature *i : features) {
        if(i->current[0] == INFINITY) {
            ++track_fail;
            if(i->is_good()) ++useful_drops;
            i->drop();
        } else {
            if(i->status == feature_normal) ++total_feats;
            if(i->outlier > i->outlier_reject) {
                i->status = feature_empty;
                ++outliers;
            }
        }
    }
    if(track_fail && !total_feats && log_enabled) fprintf(stderr, "Tracker failed! %d features dropped.\n", track_fail);
    //    if (log_enabled) fprintf(stderr, "outliers: %d/%d (%f%%)\n", outliers, total_feats, outliers * 100. / total_feats);

    int total_health = 0;
    bool need_reference = true;
    state_vision_group *best_group = 0;
    int best_health = -1;
    int normal_groups = 0;

    for(state_vision_group *g : groups.children) {
        // Delete the features we marked to drop, return the health of
        // the group (the number of features)
        int health = g->process_features();

        if(g->status && g->status != group_initializing)
            total_health += health;

        // Notify features that this group is about to disappear
        // This sets group_empty (even if group_reference)
        if(!health)
            g->make_empty();

        // Found our reference group
        if(g->status == group_reference)
            need_reference = false;

        // If we have enough features to initialize the group, do it
        if(g->status == group_initializing && health >= g->min_feats)
            g->make_normal();

        if(g->status == group_normal) {
            ++normal_groups;
            if(health > best_health) {
                best_group = g;
                best_health = g->health;
            }
        }
    }

    if(best_group && need_reference) {
        total_health += best_group->make_reference();
        reference = best_group;
    }

    //clean up dropped features and groups
    features.remove_if([](state_vision_feature *i) {
        if(i->status == feature_gooddrop) i->status = feature_empty;
        if(i->status == feature_empty) {
            delete i;
            return true;
        } else
            return false;
    });

    groups.children.remove_if([](state_vision_group *g) {
        if(g->status == group_empty) {
            delete g;
            return true;
        } else {
            return false;
        }
    });

    remap();

    return total_health;
}

state_vision_feature * state_vision::add_feature(f_t initialx, f_t initialy)
{
    state_vision_feature *f = new state_vision_feature(initialx, initialy);
    features.push_back(f);
    //allfeatures.push_back(f);
    return f;
}

void state_vision::project_new_group_covariance(const state_vision_group &g)
{
    //Note: this only works to fill in the covariance for Tr, Wr because it fills in cov(T,Tr) etc first (then copies that to cov(Tr,Tr).
    for(int i = 0; i < cov.cov.rows(); ++i)
    {
        v4 cov_W = W.copy_cov_from_row(cov.cov, i);
        g.Wr.copy_cov_to_col(cov.cov, i, cov_W);
        g.Wr.copy_cov_to_row(cov.cov, i, cov_W);
    }
}

state_vision_group * state_vision::add_group(sensor_clock::time_point time)
{
    state_vision_group *g = new state_vision_group();
    for(state_vision_group *neighbor : groups.children) {
        g->old_neighbors.push_back(neighbor->id);
        neighbor->neighbors.push_back(g->id);
    }
    groups.children.push_back(g);
    remap();
#ifdef TEST_POSDEF
    if(!test_posdef(cov.cov)) fprintf(stderr, "not pos def after propagating group\n");
#endif
    return g;
}

feature_t state_vision::project_feature(const feature_t &feat) const
{
    feature_t feat_p;
    feat_p.x = (float)(((feat.x - image_width / 2. + .5) / image_height - center_x.v) / focal_length.v);
    feat_p.y = (float)(((feat.y - image_height / 2. + .5) / image_height - center_y.v) / focal_length.v);
    return feat_p;
}

feature_t state_vision::unproject_feature(const feature_t &feat_n) const
{
    feature_t feat;
    feat.x = (float)(feat_n.x * focal_length.v + center_x.v) * image_height + image_width / 2. - .5;
    feat.y = (float)(feat_n.y * focal_length.v + center_y.v) * image_height + image_height / 2. - .5;
    return feat;
}

feature_t state_vision::uncalibrate_feature(const feature_t &feat_n) const
{
    f_t r2, kr;
    fill_calibration(feat_n, r2, kr);

    feature_t feat_u;
    feat_u.x = (float)(feat_n.x / kr);
    feat_u.y = (float)(feat_n.y / kr);

    return unproject_feature(feat_u);
}

feature_t state_vision::calibrate_feature(const feature_t &feat_u) const
{
    feature_t feat_n = project_feature(feat_u);

    f_t r2, kr;
    fill_calibration(feat_n, r2, kr);

    feature_t feat_c;
    feat_c.x = (float)(feat_n.x * kr);
    feat_c.y = (float)(feat_n.y * kr);

    return feat_c;
}

float state_vision::median_depth_variance()
{
    float median_variance = 1.f;

    vector<state_vision_feature *> useful_feats;
    for(auto i: features) {
        if(i->is_initialized()) useful_feats.push_back(i);
    }

    if(useful_feats.size()) {
        sort(useful_feats.begin(), useful_feats.end(), [](state_vision_feature *a, state_vision_feature *b) { return a->variance() < b->variance(); });
        median_variance = (float)useful_feats[useful_feats.size() / 2]->variance();
    }

    return median_variance;
}

void state_vision::remove_non_orientation_states()
{
    if(estimate_camera_extrinsics) {
        remove_child(&Tc);
        remove_child(&Wc);
    }
    if(estimate_camera_intrinsics)
    {
        remove_child(&focal_length);
        remove_child(&center_x);
        remove_child(&center_y);
        remove_child(&k1);
        remove_child(&k2);
    }
    remove_child(&groups);
    state_motion::remove_non_orientation_states();
}

void state_vision::add_non_orientation_states()
{
    state_motion::add_non_orientation_states();

    if(estimate_camera_extrinsics) {
        children.push_back(&Tc);
        children.push_back(&Wc);
    }
    if(estimate_camera_intrinsics)
    {
        children.push_back(&focal_length);
        children.push_back(&center_x);
        children.push_back(&center_y);
        children.push_back(&k1);
        children.push_back(&k2);
    }
    children.push_back(&groups);
}

void state_vision::evolve_state(f_t dt)
{
    for(state_vision_group *g : groups.children) {
        m4 Rr = to_rotation_matrix(g->Wr.v);
        g->Tr.v = g->Tr.v + Rr * Rt * dT;
        g->Wr.v = integrate_angular_velocity(g->Wr.v, dW);
    }
    state_motion::evolve_state(dt);
}

void state_vision::cache_jacobians(f_t dt)
{
    state_motion::cache_jacobians(dt);

    for(state_vision_group *g : groups.children) {
        integrate_angular_velocity_jacobian(g->Wr.v, dW, g->dWrp_dWr, g->dWrp_ddW);
        m4 Rrt_dRr_dWr = to_body_jacobian(g->Wr.v);
        g->Rr = to_rotation_matrix(g->Wr.v);
        g->dTrp_ddT = g->Rr * Rt;
        m4 RrRtdT = g->Rr * skew3(Rt * dT);
        g->dTrp_dWr = RrRtdT * Rrt_dRr_dWr;
        g->dTrp_dW  = RrRtdT * Rt_dR_dW;
    }
}

void state_vision::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    for(state_vision_group *g : groups.children) {
        for(int i = 0; i < src.rows(); ++i) {
            v4 cov_Tr = g->Tr.copy_cov_from_row(src, i);
            v4 cov_Wr = g->Wr.copy_cov_from_row(src, i);
            v4 cov_W = W.copy_cov_from_row(src, i);
            v4 cov_V = V.copy_cov_from_row(src, i);
            v4 cov_a = a.copy_cov_from_row(src, i);
            v4 cov_w = w.copy_cov_from_row(src, i);
            v4 cov_dw = dw.copy_cov_from_row(src, i);
            v4 cov_dT = dt * (cov_V + (dt / 2) * cov_a);
            v4 cov_Tp = cov_Tr +
            g->dTrp_ddT * cov_dT +
            g->dTrp_dW * cov_W - g->dTrp_dWr * cov_Wr;
            g->Tr.copy_cov_to_col(dst, i, cov_Tp);
            v4 cov_dW = dt * (cov_w + dt/2. * cov_dw);
            g->Wr.copy_cov_to_col(dst, i, g->dWrp_dWr * cov_Wr + g->dWrp_ddW * cov_dW);
        }
    }
    state_motion::project_motion_covariance(dst, src, dt);
}
