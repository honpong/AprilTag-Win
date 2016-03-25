// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include "state_vision.h"
#include "../numerics/transformation.h"
#include "debug_log.h"

f_t state_vision_feature::initial_depth_meters;
f_t state_vision_feature::initial_var;
f_t state_vision_feature::initial_process_noise;
f_t state_vision_feature::measurement_var;
f_t state_vision_feature::outlier_thresh;
f_t state_vision_feature::outlier_reject;
f_t state_vision_feature::max_variance;

state_vision_feature::state_vision_feature(uint64_t feature_id, const feature_t & initial_): state_leaf("feature"), initial(initial_.x(), initial_.y(), 1, 0), current(initial)
{
    id = feature_id;
    set_initial_variance(initial_var);
    v.set_depth_meters(initial_depth_meters);
    set_process_noise(initial_process_noise);
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

state_vision_group::state_vision_group(const state_vision_group &other): Tr(other.Tr), Qr(other.Qr), features(other.features), health(other.health), status(other.status)
{
    assert(status == group_normal); //only intended for use at initialization
    children.push_back(&Tr);
    children.push_back(&Qr);
}

state_vision_group::state_vision_group(uint64_t group_id): Tr("Tr"), Qr("Qr"), health(0), status(group_initializing)
{
    id = group_id;
    Tr.dynamic = true;
    Qr.dynamic = true;
    children.push_back(&Tr);
    children.push_back(&Qr);
    Tr.v = v4(0., 0., 0., 0.);
    Qr.v = quaternion();
    Tr.set_initial_variance(0., 0., 0.);
    Qr.set_initial_variance(0., 0., 0.);
    Tr.set_process_noise(ref_noise);
    Qr.set_process_noise(ref_noise);
}

void state_vision_group::make_empty()
{
    for(state_vision_feature *f : features.children)
        f->dropping_group();
    features.children.clear();
    status = group_empty;
}

int state_vision_group::process_features(const image_gray8 &image, mapper & map, bool map_enabled)
{
    features.children.remove_if([&](state_vision_feature *f) {
        return f->should_drop();
    });

    if(map_enabled) {
        for(auto f : features.children) {
            float stdev = (float)f->v.stdev_meters(sqrt(f->variance()));
            float variance_meters = stdev*stdev;
            const float measurement_var = 1.e-3*1.e-3;
            if(variance_meters < measurement_var)
                variance_meters = measurement_var;

            bool good = stdev / f->v.depth() < .05;
            if(good && f->descriptor_valid)
                map.update_feature_position(id, f->id, f->Xcamera, variance_meters);
            if(good && !f->descriptor_valid) {
                float scale = f->v.depth();
                float radius = 32./scale * (image.width / 320.);
                if(radius < 4) {
                    radius = 4;
                }
                //debug_log->info("feature {} good radius {}", f->id, radius);
                if(descriptor_compute(image.image, image.width, image.height, image.stride,
                            f->current[0], f->current[1], radius, f->descriptor)) {
                    f->descriptor_valid = true;
                    map.add_feature(id, f->id, f->Xcamera, variance_meters, f->descriptor);
                }
            }
        }
    }

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
    //Qr.saturate();
    return 0;
}

int state_vision_group::make_normal()
{
    assert(status == group_initializing);
    children.push_back(&features);
    status = group_normal;
    return 0;
}

state_vision::state_vision(covariance &c):
    state_motion(c),
    Tc("Tc"), Qc("Qc"), focal_length("focal_length"), center_x("center_x"), center_y("center_y"), k1("k1"), k2("k2"), k3("k3"),
    fisheye(false), feature_counter(0), group_counter(0), reference(nullptr), lost_factor(.1)
{
    reference = NULL;
    if(estimate_camera_intrinsics)
    {
        children.push_back(&focal_length);
        children.push_back(&center_x);
        children.push_back(&center_y);
        children.push_back(&k1);
        children.push_back(&k2);
        children.push_back(&k3);
    }
    if(estimate_camera_extrinsics) {
        children.push_back(&Tc);
        children.push_back(&Qc);
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
    map.reset();
    state_motion::reset();
}

int state_vision::process_features(const image_gray8 &image, sensor_clock::time_point time)
{
    int useful_drops = 0;
    int total_feats = 0;
    int outliers = 0;
    int track_fail = 0;
    for(state_vision_feature *i : features) {
        if(i->current[0] == INFINITY) {
            // Drop tracking failures
            ++track_fail;
            if(i->is_good()) ++useful_drops;
            i->drop();
        } else {
            // Drop outliers
            if(i->status == feature_normal) ++total_feats;
            if(i->outlier > i->outlier_reject) {
                i->status = feature_empty;
                ++outliers;
            }
        }
    }
    if(track_fail && !total_feats) debug_log->warn("Tracker failed! {} features dropped.", track_fail);
    //    debug_log->warn("outliers: {}/{} ({}%)", outliers, total_feats, outliers * 100. / total_feats);

    int total_health = 0;
    bool need_reference = true;
    state_vision_group *best_group = 0;
    int best_health = -1;
    int normal_groups = 0;

    for(state_vision_group *g : groups.children) {
        // Delete the features we marked to drop, return the health of
        // the group (the number of features)
        int health = g->process_features(image, map, map_enabled);

        if(g->status && g->status != group_initializing)
            total_health += health;

        // Notify features that this group is about to disappear
        // This sets group_empty (even if group_reference)
        if(!health) {
            if(map_enabled) {
                if(g->status == group_reference) {
                    reference = 0;
                }
            }
            g->make_empty();
        }

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

    if(map_enabled) {
        transformation offset;
        int max = 20;
        int suppression = 10;
        if(map.find_closure(max, suppression, offset)) {
            loop_offset.T = loop_offset.T * (1. - lost_factor) + offset.T * lost_factor;
            if(lost_factor > .1) lost_factor -= .1;
            if(lost_factor < .1) lost_factor = .1;
            debug_log->info() << "loop closed, offset: " << loop_offset;
        }
    }

    if(best_group && need_reference) {
        total_health += best_group->make_reference();
        reference = best_group;
    }

    //clean up dropped features and groups
    features.remove_if([&](state_vision_feature *i) {
        if(i->should_drop()) {
            delete i;
            return true;
        } else
            return false;
    });

    groups.children.remove_if([this](state_vision_group *g) {
        if(g->status == group_empty) {
            if(map_enabled) {
                transformation G = transformation(Q.v, T.v)*invert(transformation(g->Qr.v, g->Tr.v));
                this->map.node_finished(g->id, G);
            }
            delete g;
            return true;
        } else {
            return false;
        }
    });

    remap();

    return total_health;
}

state_vision_feature * state_vision::add_feature(const feature_t & initial)
{
    state_vision_feature *f = new state_vision_feature(feature_counter++, initial);
    features.push_back(f);
    //allfeatures.push_back(f);
    return f;
}

state_vision_group * state_vision::add_group(sensor_clock::time_point time)
{
    state_vision_group *g = new state_vision_group(group_counter++);
    if(map_enabled) {
        map.add_node(g->id);
        if(groups.children.empty() && g->id != 0)
        {
            map.add_edge(g->id, g->id-1);
            g->old_neighbors.push_back(g->id-1);
            lost_factor = 1.;
        }
        for(state_vision_group *neighbor : groups.children) {
            map.add_edge(g->id, neighbor->id);

            g->old_neighbors.push_back(neighbor->id);
            neighbor->neighbors.push_back(g->id);
        }
    }
    groups.children.push_back(g);
    remap();
#ifdef TEST_POSDEF
    if(!test_posdef(cov.cov)) fprintf(stderr, "not pos def after propagating group\n");
#endif
    return g;
}

feature_t state_vision::normalize_feature(const feature_t &feat) const
{
    return (((feat - f_t(.5) * image_size()) + feature_t{.5,.5}) / image_height - feature_t {center_x.v, center_y.v}) / focal_length.v;
}

feature_t state_vision::unnormalize_feature(const feature_t &feat_n) const
{
    return (feat_n * focal_length.v + feature_t {center_x.v, center_y.v}) * image_height + f_t(.5) * image_size() - feature_t{.5,.5};
}

feature_t state_vision::distort_feature(const feature_t &feat_u, f_t *kd_u_, f_t *dkd_u_dru, f_t *dkd_u_dk1, f_t *dkd_u_dk2, f_t *dkd_u_dk3) const
{
    f_t kd_u, ru2, ru = std::sqrt(ru2 = feat_u.squaredNorm());
    if (fisheye) {
        f_t w = k1.v; if (!w) { w = .922; fprintf(stderr, "you really shouldn't have a zero-angle fisheye lens\n"); }
        if(ru < F_T_EPS) ru = F_T_EPS;
        kd_u = std::atan(f_t(2) * std::tan(w * f_t(.5)) * ru) / (ru * w);  // FIXME: add higher order terms (but not the linear one)
        if (dkd_u_dru) *dkd_u_dru = f_t(2) * std::tan(f_t(.5)*w) / (w + f_t(4) * ru * ru * w * std::tan(f_t(.5)*w) * std::tan(f_t(.5)*w));
        if (dkd_u_dk1) *dkd_u_dk1 = f_t(2) * ru / (w * (f_t(1) + std::cos(w) + f_t(4) * ru * ru * (f_t(1) - std::cos(w)))) - kd_u / w;
        if (dkd_u_dk2) *dkd_u_dk2 = 0;
        if (dkd_u_dk3) *dkd_u_dk3 = 0;
    } else {
        kd_u = f_t(1) + ru2 * (k1.v + ru2 * (k2.v + ru2 * k3.v));
        if (dkd_u_dru) *dkd_u_dru = 0 * (k1.v + ru2 * (f_t(2) * k2.v + f_t(3) * k3.v * ru2)) * f_t(2) * std::sqrt(ru2);
        if (dkd_u_dk1) *dkd_u_dk1 = ru2;
        if (dkd_u_dk2) *dkd_u_dk2 = ru2 * ru2;
        if (dkd_u_dk3) *dkd_u_dk3 = ru2 * ru2 * ru2;
    }
    if (kd_u_) *kd_u_ = kd_u;

    return feat_u * kd_u;
}

feature_t state_vision::undistort_feature(const feature_t &feat_d, f_t *ku_d_, f_t *dku_d_drd, f_t *dku_d_dk1, f_t *dku_d_dk2, f_t *dku_d_dk3) const
{
    f_t ku_d, rd2 = feat_d.squaredNorm();
    if (fisheye) {
        f_t rd = std::sqrt(rd2), w = k1.v; if (!w) { w = .922; fprintf(stderr, "you really shouldn't have a zero-angle fisheye lens\n"); }
        if(rd < F_T_EPS) rd = F_T_EPS;
        ku_d = std::tan(w * rd) / (f_t(2) * std::tan(f_t(.5)*w) * rd);
        if (dku_d_drd) *dku_d_drd = f_t(2) * (rd * w / (std::cos(rd * w) * std::cos(rd * w) * (f_t(2) * rd * std::tan(w*f_t(.5)))) - ku_d);
        if (dku_d_dk1) *dku_d_dk1 = (f_t(2) * rd * std::sin(w) - std::sin(f_t(2) * rd * w)) / (f_t(8) * rd * (std::cos(rd * w) * std::cos(rd * w)) * (std::sin(w*f_t(.5)) * std::sin(w*f_t(.5))));
        if (dku_d_dk2) *dku_d_dk2 = 0;
        if (dku_d_dk3) *dku_d_dk3 = 0;
    } else {
        f_t kd_u, ru2 = rd2, dkd_u_dru2;
        for (int i=0; i<4; i++) {
           kd_u =  f_t(1) + ru2 * (k1.v + ru2 * (k2.v + ru2 * k3.v));
           dkd_u_dru2 = k1.v + f_t(2) * ru2 * (k2.v + f_t(3) * ru2 * k3.v);
           // f(ru2) == ru2 * kd_u * kd_u - rd2 == 0;
           // ru2 -= f(ru2) / f'(ru2)
           ru2 -= (ru2 * kd_u * kd_u - rd2) / (kd_u * (kd_u + f_t(2) * ru2 * dkd_u_dru2));
        }
        ku_d = f_t(1) / kd_u;
        f_t ru = std::sqrt(ru2), dkd_u_dru = f_t(2) * ru * dkd_u_dru2;
        // dku_d_drd = d/rd (1/kd_u) = d/ru (1/kd_u) dru/drd = d/ru (1/kd_u) / (drd/dru) = d/ru (1/kd_u) / (d/ru (ru kd_u)) = -dkd_u_dru/kd_u/kd_u / (kd_u + ru dkd_u_dru)
        if (dku_d_drd) *dku_d_drd = 0 * -dkd_u_dru/(kd_u * kd_u * (kd_u + ru * dkd_u_dru));
        if (dku_d_dk1) *dku_d_dk1 = -(ru2            )/(kd_u*kd_u);
        if (dku_d_dk2) *dku_d_dk2 = -(ru2 * ru2      )/(kd_u*kd_u);
        if (dku_d_dk3) *dku_d_dk3 = -(ru2 * ru2 * ru2)/(kd_u*kd_u);
    }
    if (ku_d_) *ku_d_ = ku_d;

    return feat_d * ku_d;
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
        remove_child(&Qc);
    }
    if(estimate_camera_intrinsics)
    {
        remove_child(&focal_length);
        remove_child(&center_x);
        remove_child(&center_y);
        remove_child(&k1);
        remove_child(&k2);
        remove_child(&k3);
    }
    remove_child(&groups);
    state_motion::remove_non_orientation_states();
}

void state_vision::add_non_orientation_states()
{
    state_motion::add_non_orientation_states();

    if(estimate_camera_extrinsics) {
        children.push_back(&Tc);
        children.push_back(&Qc);
    }
    if(estimate_camera_intrinsics)
    {
        children.push_back(&focal_length);
        children.push_back(&center_x);
        children.push_back(&center_y);
        children.push_back(&k1);
        children.push_back(&k2);
        children.push_back(&k3);
    }
    children.push_back(&groups);
}

void state_vision::evolve_state(f_t dt)
{
    for(state_vision_group *g : groups.children) {
        g->Tr.v = g->Tr.v + g->dTrp_ddT * dT;
        rotation_vector dWr(dW[0], dW[1], dW[2]);
        g->Qr.v = g->Qr.v * to_quaternion(dWr); // FIXME: cache this?
    }
    state_motion::evolve_state(dt);
}

void state_vision::cache_jacobians(f_t dt)
{
    state_motion::cache_jacobians(dt);

    for(state_vision_group *g : groups.children) {
        m4 Rr = to_rotation_matrix(g->Qr.v);
        g->dTrp_ddT = to_rotation_matrix(g->Qr.v * conjugate(Q.v));
        m4 xRrRtdT = skew3(g->dTrp_ddT * dT);
        g->dTrp_dQ_s   = xRrRtdT;
        g->dQrp_s_dW = Rr * JdW_s;
    }
}

void state_vision::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    for(int i = 0; i < src.rows(); ++i) {
        const auto cov_w = w.from_row(src, i);
        const auto cov_dw = dw.from_row(src, i);
        const v3 cov_dW = dt * (cov_w + f_t(.5) * dt * cov_dw);
        const auto scov_Q = Q.from_row(src, i);
        const auto cov_V = V.from_row(src, i);
        const auto cov_a = a.from_row(src, i);
        const v3 cov_dT = dt * (cov_V + f_t(.5) * dt * cov_a);
        for(state_vision_group *g : groups.children) {
            const auto cov_Tr = g->Tr.from_row(src, i);
            const auto scov_Qr = g->Qr.from_row(src, i);
            g->Tr.to_col(dst, i) = cov_Tr + g->dTrp_dQ_s.block<3,3>(0,0) * (scov_Q - scov_Qr) + g->dTrp_ddT.block<3,3>(0,0) * cov_dT;
            g->Qr.to_col(dst, i) = scov_Qr + g->dQrp_s_dW.block<3,3>(0,0) * cov_dW;
        }
    }
    state_motion::project_motion_covariance(dst, src, dt);
}

bool state_vision::load_map(std::string map_json)
{
    if(map_enabled) {
        return mapper::deserialize(map_json, map);
    }
    return false;
}
