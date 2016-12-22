// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include "state_vision.h"
#include "fast_tracker.h"
#include "transformation.h"
#include <spdlog/fmt/ostr.h> // must be included to use our operator<<

f_t state_vision_feature::initial_depth_meters;
f_t state_vision_feature::initial_var;
f_t state_vision_feature::initial_process_noise;
f_t state_vision_feature::outlier_thresh;
f_t state_vision_feature::outlier_reject;
f_t state_vision_feature::max_variance;

state_vision_feature::state_vision_feature(state_vision_group &group_, uint64_t feature_id, const feature_t &initial_):
    state_leaf("feature", constant), group(group_), id(feature_id), initial(initial_), current(initial_)
{
    reset();
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
        reset();
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

state_vision_group::state_vision_group(uint64_t group_id): health(0), status(group_initializing)
{
    id = group_id;
    children.push_back(&Tr);
    children.push_back(&Qr);
    Tr.v = v3(0, 0, 0);
    Qr.v = quaternion::Identity();
    f_t near_zero = F_T_EPS * 100;
    Tr.set_initial_variance({near_zero, near_zero, near_zero});
    Qr.set_initial_variance({near_zero, near_zero, near_zero});
    Tr.set_process_noise(ref_noise);
    Qr.set_process_noise(ref_noise);
}

void state_vision_group::make_empty()
{
    for(state_vision_feature *f : features.children) {
        f->dropping_group();
        delete f;
    }
    features.children.clear();
    status = group_empty;
}

int state_vision_group::process_features(const rc_ImageData &image, mapper & map, bool map_enabled)
{
    features.children.remove_if([&](state_vision_feature *f) {
        if(f->should_drop()) {
            delete f;
            return true;
        } else
            return false;
    });

    if(map_enabled) {
        for(auto f : features.children) {
            float stdev = (float)f->v.stdev_meters(sqrt(f->variance()));
            float variance_meters = stdev*stdev;
            const float measurement_var = 1.e-3f*1.e-3f;
            if(variance_meters < measurement_var)
                variance_meters = measurement_var;

            bool good = stdev / f->v.depth() < .05f;
            if(good && f->descriptor_valid)
                map.update_feature_position(id, f->id, f->body, variance_meters);
            if(good && !f->descriptor_valid) {
                float scale = static_cast<float>(f->v.depth());
                float radius = 32.f/scale * (image.width / 320.f);
                if(radius < 4.f) {
                    radius = 4.f;
                }
                //log->info("feature {} good radius {}", f->id, radius);
                if(descriptor_compute((uint8_t*)image.image, image.width, image.height, image.stride,
                                      static_cast<float>(f->current[0]), static_cast<float>(f->current[1]), radius,
                                      f->descriptor)) {
                    f->descriptor_valid = true;
                    map.add_feature(id, f->id, f->body, variance_meters, f->descriptor);
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
    feature_counter(0), group_counter(0), reference(nullptr)
{
    reference = nullptr;
    children.push_back(&camera);
    children.push_back(&groups);
}

void state_vision::clear_features_and_groups()
{
    for(state_vision_group *g : groups.children) {
        g->make_empty();
        delete g;
    }
    groups.children.clear();
}

state_vision::~state_vision()
{
    clear_features_and_groups();
}

void state_vision::reset()
{
    clear_features_and_groups();
    reference = nullptr;
    map.reset();
    state_motion::reset();
}

int state_vision::feature_count() const
{
    int count = 0;
    for(auto *g : groups.children)
        count += g->features.children.size();

    return count;
}

transformation state_vision::get_transformation() const
{
    return loop_offset*transformation(Q.v, T.v);
}

int state_vision::process_features(const rc_ImageData &image)
{
    int useful_drops = 0;
    int total_feats = 0;
    int outliers = 0;
    int track_fail = 0;
    for(auto *g : groups.children) {
        for(state_vision_feature *i : g->features.children) {
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
            if(i->should_drop())
                camera.feature_tracker->drop_feature(i->tracker_id);
        }
    }

    if(track_fail && !total_feats) log->warn("Tracker failed! {} features dropped.", track_fail);
    //    log.warn("outliers: {}/{} ({}%)", outliers, total_feats, outliers * 100. / total_feats);

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
                    reference = nullptr;
                }
            }
            for(state_vision_feature *i : g->features.children)
                camera.feature_tracker->drop_feature(i->tracker_id);
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
            if(map_enabled) {
                transformation G = get_transformation()*invert(transformation(g->Qr.v, g->Tr.v));
                map.set_node_transformation(g->id, G);
            }
        }
    }

    if(map_enabled) {
        transformation offset;
        int max = 20;
        int suppression = 10;
        if(map.find_closure(max, suppression, offset)) {
            loop_offset = offset*loop_offset;
            log->info("loop closed, offset: {}", std::cref(loop_offset));
        }
    }

    if(best_group && need_reference) {
        total_health += best_group->make_reference();
        reference = best_group;
    }

    for(auto i = groups.children.begin(); i != groups.children.end();)
    {
        auto g = *i;
        ++i;
        if(g->status == group_empty) remove_group(g);
    }

    remap();

    return total_health;
}

state_vision_feature * state_vision::add_feature(state_vision_group &group, const feature_t &initial)
{
    return new state_vision_feature(group, feature_counter++, initial);
}

state_vision_group * state_vision::add_group()
{
    state_vision_group *g = new state_vision_group(group_counter++);
    if(map_enabled) {
        map.add_node(g->id);
        if(groups.children.empty() && g->id != 0)
        {
            map.add_edge(g->id, g->id-1);
            g->old_neighbors.push_back(g->id-1);
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

void state_vision::remove_group(state_vision_group *g)
{
    if(map_enabled) {
        transformation G = get_transformation()*invert(transformation(g->Qr.v, g->Tr.v));
        this->map.node_finished(g->id, G);
    }
    groups.remove_child(g);
    delete g;
}

feature_t state_vision_intrinsics::normalize_feature(const feature_t &feat) const
{
    return (((feat - f_t(.5) * image_size()) + feature_t{.5,.5}) / image_height - feature_t {center_x.v, center_y.v}) / focal_length.v;
}

feature_t state_vision_intrinsics::unnormalize_feature(const feature_t &feat_n) const
{
    return (feat_n * focal_length.v + feature_t {center_x.v, center_y.v}) * image_height + f_t(.5) * image_size() - feature_t{.5,.5};
}

feature_t state_vision_intrinsics::distort_feature(const feature_t &feat_u) const
{
    return feat_u * get_distortion_factor(feat_u);
}

feature_t state_vision_intrinsics::undistort_feature(const feature_t &feat_d) const
{
    return feat_d * get_undistortion_factor(feat_d);
}

f_t state_vision_intrinsics::get_distortion_factor(const feature_t &feat_u, feature_t *dkd_u_dfeat_u, f_t *dkd_u_dk1, f_t *dkd_u_dk2, f_t *dkd_u_dk3) const
{
    f_t kd_u, ru2, ru = std::sqrt(ru2 = feat_u.squaredNorm());
    switch (type) {
    default:
    case rc_CALIBRATION_TYPE_UNKNOWN:
    case rc_CALIBRATION_TYPE_UNDISTORTED:
        kd_u = 1;
        if (dkd_u_dfeat_u) *dkd_u_dfeat_u = 0 * feat_u;
        if (dkd_u_dk1) *dkd_u_dk1 = 0;
        if (dkd_u_dk2) *dkd_u_dk2 = 0;
        if (dkd_u_dk3) *dkd_u_dk3 = 0;
        break;
    case rc_CALIBRATION_TYPE_FISHEYE: {
        f_t w = k1.v;
        if(ru < F_T_EPS) ru = F_T_EPS;
        kd_u = std::atan(2 * std::tan(w/2) * ru) / (ru * w);  // FIXME: add higher order terms (but not the linear one)
        if (dkd_u_dfeat_u) *dkd_u_dfeat_u = (2 * std::tan(w/2) / (w + 4 * ru * ru * w * std::tan(w/2) * std::tan(w/2)) - kd_u) / ru * feat_u / ru;
        if (dkd_u_dk1) *dkd_u_dk1 = (2 / (1 + std::cos(w) + 4 * ru * ru * (1 - std::cos(w))) - kd_u) / w;
        if (dkd_u_dk2) *dkd_u_dk2 = 0;
        if (dkd_u_dk3) *dkd_u_dk3 = 0;
    }   break;
    case rc_CALIBRATION_TYPE_POLYNOMIAL3: {
        kd_u = f_t(1) + ru2 * (k1.v + ru2 * (k2.v + ru2 * k3.v));
        if (dkd_u_dfeat_u) *dkd_u_dfeat_u = (k1.v + ru2 * (2 * k2.v + 3 * k3.v * ru2)) * 2 * feat_u;
        if (dkd_u_dk1) *dkd_u_dk1 = ru2;
        if (dkd_u_dk2) *dkd_u_dk2 = ru2 * ru2;
        if (dkd_u_dk3) *dkd_u_dk3 = ru2 * ru2 * ru2;
    }   break;
    }
    return kd_u;
}

f_t state_vision_intrinsics::get_undistortion_factor(const feature_t &feat_d, feature_t *dku_d_dfeat_d, f_t *dku_d_dk1, f_t *dku_d_dk2, f_t *dku_d_dk3) const
{
    f_t ku_d, rd2, rd = sqrt(rd2 = feat_d.squaredNorm());
    switch (type) {
    default:
    case rc_CALIBRATION_TYPE_UNKNOWN:
    case rc_CALIBRATION_TYPE_UNDISTORTED:
        ku_d = 1;
        if (dku_d_dfeat_d) *dku_d_dfeat_d = 0 * feat_d;
        if (dku_d_dk1) *dku_d_dk1 = 0;
        if (dku_d_dk2) *dku_d_dk2 = 0;
        if (dku_d_dk3) *dku_d_dk3 = 0;
        break;
    case rc_CALIBRATION_TYPE_FISHEYE: {
        f_t w = k1.v;
        if(rd < F_T_EPS) rd = F_T_EPS;
        ku_d = std::tan(w * rd) / (2 * std::tan(w/2) * rd);
        if (dku_d_dfeat_d) *dku_d_dfeat_d = (rd * w / (std::cos(rd * w) * std::cos(rd * w) * (2 * rd * std::tan(w/2))) - ku_d) / rd * feat_d / rd;
        if (dku_d_dk1) *dku_d_dk1 = (2 * rd * std::sin(w) - std::sin(2 * rd * w)) / (8 * rd * (std::cos(rd * w) * std::cos(rd * w)) * (std::sin(w/2) * std::sin(w/2)));
        if (dku_d_dk2) *dku_d_dk2 = 0;
        if (dku_d_dk3) *dku_d_dk3 = 0;
    }   break;
    case rc_CALIBRATION_TYPE_POLYNOMIAL3: {
        f_t kd_u, ru2 = rd2, dkd_u_dru2;
        for (int i=0; i<4; i++) {
           kd_u =  1 + ru2 * (k1.v + ru2 * (k2.v + ru2 * k3.v));
           dkd_u_dru2 = k1.v + 2 * ru2 * (k2.v + 3 * ru2 * k3.v);
           // f(ru2) == ru2 * kd_u * kd_u - rd2 == 0;
           // ru2 -= f(ru2) / f'(ru2)
           ru2 -= (ru2 * kd_u * kd_u - rd2) / (kd_u * (kd_u + 2 * ru2 * dkd_u_dru2));
        }
        ku_d = 1 / kd_u;
        f_t ru = std::sqrt(ru2), dkd_u_dru = 2 * ru * dkd_u_dru2;
        // dku_d_drd = d/rd (1/kd_u) = d/ru (1/kd_u) dru/drd = d/ru (1/kd_u) / (drd/dru) = d/ru (1/kd_u) / (d/ru (ru kd_u)) = -dkd_u_dru/kd_u/kd_u / (kd_u + ru dkd_u_dru)
        if (dku_d_dfeat_d) *dku_d_dfeat_d = -dkd_u_dru/(kd_u * kd_u * (kd_u + ru * dkd_u_dru)) * feat_d / rd;
        if (dku_d_dk1) *dku_d_dk1 = -(ru2            )/(kd_u*kd_u);
        if (dku_d_dk2) *dku_d_dk2 = -(ru2 * ru2      )/(kd_u*kd_u);
        if (dku_d_dk3) *dku_d_dk3 = -(ru2 * ru2 * ru2)/(kd_u*kd_u);
    }   break;
    }
    return ku_d;
}

void state_vision::update_feature_tracks(const rc_ImageData &image)
{
    tracker::image current_image;
    current_image.image = (uint8_t *)image.image;
    current_image.width_px = image.width;
    current_image.height_px = image.height;
    current_image.stride_px = image.stride;

    std::map<uint64_t, state_vision_feature *> id_to_state;

    camera.feature_tracker->predictions.clear();
    camera.feature_tracker->predictions.reserve(feature_count());
    for(state_vision_group *g : groups.children) {
        if(!g->status || g->status == group_initializing) continue;
        for(state_vision_feature *feature : g->features.children) {
            id_to_state[feature->tracker_id] = feature;
            camera.feature_tracker->predictions.emplace_back(feature->tracker_id,
                                                            (float)feature->current.x(), (float)feature->current.y(),
                                                            (float)feature->prediction.x(), (float)feature->prediction.y());
        }
    }

    int i=0;
    if (camera.feature_tracker->predictions.size())
        for(const auto &p : camera.feature_tracker->track(current_image, camera.feature_tracker->predictions)) {
            state_vision_feature * feature = id_to_state[p.id];
            feature->current.x() = p.found ? p.x : INFINITY;
            feature->current.y() = p.found ? p.y : INFINITY;
        }
}

float state_vision::median_depth_variance()
{
    float median_variance = 1;

    vector<state_vision_feature *> useful_feats;
    for(auto g: groups.children) {
        for(auto i: g->features.children) {
            if(i->is_initialized()) useful_feats.push_back(i);
        }
    }

    if(useful_feats.size()) {
        sort(useful_feats.begin(), useful_feats.end(), [](state_vision_feature *a, state_vision_feature *b) { return a->variance() < b->variance(); });
        median_variance = (float)useful_feats[useful_feats.size() / 2]->variance();
    }

    return median_variance;
}

void state_vision::remove_non_orientation_states()
{
    remove_child(&camera);
    remove_child(&groups);
    state_motion::remove_non_orientation_states();
}

void state_vision::add_non_orientation_states()
{
    state_motion::add_non_orientation_states();
    children.push_back(&camera);
    children.push_back(&groups);
}

void state_vision::evolve_state(f_t dt)
{
    for(state_vision_group *g : groups.children) {
        g->Tr.v += g->dTrp_ddT * dT;
        rotation_vector dWr(dW[0], dW[1], dW[2]);
        g->Qr.v *= to_quaternion(dWr); // FIXME: cache this?
    }
    state_motion::evolve_state(dt);
}

void state_vision::cache_jacobians(f_t dt)
{
    state_motion::cache_jacobians(dt);

    for(state_vision_group *g : groups.children) {
        m3 Rr = g->Qr.v.toRotationMatrix();
        g->dTrp_ddT = (g->Qr.v * Q.v.conjugate()).toRotationMatrix();
        m3 xRrRtdT = skew(g->dTrp_ddT * dT);
        g->dTrp_dQ_s   = xRrRtdT;
        g->dQrp_s_dW = Rr * JdW_s;
    }
}

void state_vision::project_motion_covariance(matrix &dst, const matrix &src, f_t dt)
{
    //Previously we called state_motion::project_covariance here, but this is inlined into the above for faster performance
    for(int i = 0; i < dst.cols(); ++i) {
        // This should match state_motion_orientation::project_covariance
        const auto cov_w = w.from_row(src, i);
        const auto cov_dw = dw.from_row(src, i);
        const auto cov_ddw = ddw.from_row(src, i);
        const v3 cov_dW = dt * (cov_w + dt/2 * (cov_dw + dt/3 * cov_ddw));
        const auto scov_Q = Q.from_row(src, i);
        w.to_col(dst, i) = cov_w + dt * (cov_dw + dt/2 * cov_ddw);
        dw.to_col(dst, i) = cov_dw + dt * cov_ddw;
        Q.to_col(dst, i) = scov_Q + dQp_s_dW * cov_dW;
        // This should match state_motion::project_covariance
        const auto cov_V = V.from_row(src, i);
        const auto cov_a = a.from_row(src, i);
        const auto cov_T = T.from_row(src, i);
        const auto cov_da = da.from_row(src, i);
        const v3 cov_dT = dt * (cov_V + dt/2 * (cov_a + dt/3 * cov_da));
        T.to_col(dst, i) = cov_T + cov_dT;
        V.to_col(dst, i) = cov_V + dt * (cov_a + dt/2 * cov_da);
        a.to_col(dst, i) = cov_a + dt * cov_da;
        for(state_vision_group *g : groups.children) {
            const auto cov_Tr = g->Tr.from_row(src, i);
            const auto scov_Qr = g->Qr.from_row(src, i);
            g->Tr.to_col(dst, i) = cov_Tr + g->dTrp_dQ_s * (scov_Q - scov_Qr) + g->dTrp_ddT * cov_dT;
            g->Qr.to_col(dst, i) = scov_Qr + g->dQrp_s_dW * cov_dW;
        }
    }
}

bool state_vision::load_map(std::string map_json)
{
    if(map_enabled) {
        return mapper::deserialize(map_json, map);
    }
    return false;
}
