// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include "state_vision.h"
#include "fast_tracker.h"
#include "transformation.h"
#include <spdlog/fmt/ostr.h> // must be included to use our operator<<
#include "Trace.h"

#ifdef MYRIAD2
    //#define SHAVE_PROJECT_MOTION_COVARIANCE
    #include <OsCommon.h>
    #include "project_covariance_definitions.h"
    #include "covariance_projector.h"
#endif

f_t state_vision_feature::initial_depth_meters;
f_t state_vision_feature::initial_var;
f_t state_vision_feature::initial_process_noise;
f_t state_vision_feature::outlier_thresh;
f_t state_vision_feature::outlier_reject;
f_t state_vision_feature::outlier_lost_reject;
f_t state_vision_feature::max_variance;

state_vision_feature::state_vision_feature(const tracker::feature_track &track_, state_vision_group &group_):
    state_leaf("feature", constant), track(track_), initial(track_.x, track_.y), group(group_)
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

void state_vision_feature::make_lost()
{
    status = feature_lost;
    save_initial_variance();
    index = -1;
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

state_vision_group::state_vision_group(state_camera &camera_, uint64_t group_id): camera(camera_), health(0), status(group_initializing)
{
    id = group_id;
    children.push_back(&Qr);
    children.push_back(&Tr);
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
    for(state_vision_feature *f : lost_features) {
        f->dropping_group();
        delete f;
    }
    features.children.clear();
    lost_features.clear();
    status = group_empty;
}

int state_vision_group::process_features()
{
    features.children.remove_if([&](state_vision_feature *f) {
        if(f->should_drop()) {
            delete f;
            return true;
        } else if(f->status == feature_lost) {
            lost_features.push_back(f);
            return true;
        } else
            return false;
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

void state_camera::clear_features_and_groups()
{
    for(state_vision_group *g : groups.children) {
        g->make_empty();
        delete g;
    }
    groups.children.clear();
    standby_features.clear();
}

state_vision::~state_vision()
{
    for (auto &camera : cameras.children)
        camera->clear_features_and_groups();
}

void state_vision::reset()
{
    for (auto &camera : cameras.children) {
        camera->clear_features_and_groups();
        camera->detecting_space = 0;
    }
    state_motion::reset();
}

int state_vision::feature_count() const
{
    int res = 0;
    for (auto &camera : cameras.children) res += camera->feature_count();
    return res;
}

void state_vision::clear_features_and_groups()
{
    for (auto &camera : cameras.children) camera->clear_features_and_groups();
}

void state_vision::enable_orientation_only(bool _remap)
{
    clear_features_and_groups();
    state_motion::enable_orientation_only(_remap);
}

int state_camera::feature_count() const
{
    int count = 0;
    for(auto *g : groups.children)
        count += g->features.children.size();

    return count + standby_features.size();
}

transformation state_vision::get_transformation() const
{
    return loop_offset*transformation(Q.v, T.v);
}

int state_camera::process_features(mapper *map, spdlog::logger &log)
{
    int useful_drops = 0;
    int total_feats = 0;
    int outliers = 0;
    int track_fail = 0;
    for(auto *g : groups.children) {
        for(state_vision_feature *i : g->features.children) {
            if(i->track.found() == false) {
                // Drop tracking failures
                ++track_fail;
                if(i->is_good()) ++useful_drops;
                if(i->is_good() && i->outlier < i->outlier_lost_reject)
                    i->make_lost();
                else
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
    }
    standby_features.remove_if([](tracker::feature_track &t) {return !t.found();});

    if(track_fail && !total_feats) log.warn("Tracker failed! {} features dropped.", track_fail);
    //    log.warn("outliers: {}/{} ({}%)", outliers, total_feats, outliers * 100. / total_feats);

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

    if(best_group && need_reference)
        total_health += best_group->make_reference();

    for(auto i = groups.children.begin(); i != groups.children.end();)
    {
        auto g = *i;
        ++i;
        if(g->status == group_empty) remove_group(g, map);
    }

    return total_health;
}

void state_vision::update_map(const rc_ImageData &image, mapper *map, spdlog::logger &log)
{
    if (!map) return;

    for (auto &camera : cameras.children) {
        for (auto &g : camera->groups.children) {
            if (g->status == group_normal || g->status == group_reference)
                map->set_node_transformation(g->id, get_transformation()*invert(transformation(g->Qr.v, g->Tr.v)));

            for (state_vision_feature *f : g->features.children) {
                float stdev = (float)f->v.stdev_meters(sqrt(f->variance()));
                float variance_meters = stdev*stdev;
                const float measurement_var = 1.e-3f*1.e-3f;
                if (variance_meters < measurement_var)
                    variance_meters = measurement_var;

                bool good = stdev / f->v.depth() < .05f;
                if (good && f->descriptor_valid)
                    map->update_feature_position(g->id, f->track.feature->id, f->node_body, variance_meters);
                if (good && !f->descriptor_valid) {
                    float scale = static_cast<float>(f->v.depth());
                    float radius = 32.f/scale * (image.width / 320.f);
                    if(radius < 4.f) {
                        radius = 4.f;
                    }
                    //log.info("feature {} good radius {}", f->id, radius);
                    if (descriptor_compute((uint8_t*)image.image, image.width, image.height, image.stride,
                                           static_cast<float>(f->track.x), static_cast<float>(f->track.y), radius,
                                           f->descriptor)) {
                        f->descriptor_valid = true;
                        map->add_feature(g->id, f->track.feature->id, f->node_body, variance_meters, f->descriptor);
                    }
                }
            }
        }
    }

    transformation offset;
    int max = 20;
    int suppression = 10;
    if (map->find_closure(max, suppression, offset)) {
        loop_offset = offset*loop_offset;
        log.info("loop closed, offset: {}", std::cref(loop_offset));
    }
}

state_vision_feature * state_vision::add_feature(const tracker::feature_track &track_, state_vision_group &group)
{
    return new state_vision_feature(track_, group);
}

state_vision_group * state_vision::add_group(state_camera &camera, mapper *map)
{
    state_vision_group *g = new state_vision_group(camera, group_counter++);
    if(map) {
        map->add_node(g->id);
        if(camera.groups.children.empty() && g->id != 0) // FIXME: what if the other camera has groups?
        {
            map->add_edge(g->id, g->id-1);
            g->old_neighbors.push_back(g->id-1);
        }
        for(auto &neighbor : camera.groups.children) {
            map->add_edge(g->id, neighbor->id);

            g->old_neighbors.push_back(neighbor->id);
            neighbor->neighbors.push_back(g->id);
        }
    }
    camera.groups.children.push_back(g);
    return g;
}

void state_camera::remove_group(state_vision_group *g, mapper *map)
{
    if (map)
        map->node_finished(g->id);
    groups.remove_child(g);
    delete g;
}

feature_t state_vision_intrinsics::normalize_feature(const feature_t &feat) const
{
    return ((feat - f_t(.5) * image_size() + feature_t{.5,.5}) / image_height - center.v) / focal_length.v;
}

feature_t state_vision_intrinsics::unnormalize_feature(const feature_t &feat_n) const
{
    return (feat_n * focal_length.v + center.v) * image_height + f_t(.5) * image_size() - feature_t{.5,.5};
}

feature_t state_vision_intrinsics::distort_feature(const feature_t &feat_u) const
{
    return feat_u * get_distortion_factor(feat_u);
}

feature_t state_vision_intrinsics::undistort_feature(const feature_t &feat_d) const
{
    return feat_d * get_undistortion_factor(feat_d);
}

f_t state_vision_intrinsics::get_distortion_factor(const feature_t &feat_u, m<1,2> *dkd_u_dfeat_u, m<1,4> *dkd_u_dk) const
{
    f_t kd_u, ru2, ru = std::sqrt(ru2 = feat_u.squaredNorm());
    switch (type) {
    default:
    case rc_CALIBRATION_TYPE_UNKNOWN:
    case rc_CALIBRATION_TYPE_UNDISTORTED:
        kd_u = 1;
        if (dkd_u_dfeat_u)
            *dkd_u_dfeat_u = m<1,2>::Zero();
        if (dkd_u_dk)
            *dkd_u_dk = m<1,4>::Zero();
        break;
    case rc_CALIBRATION_TYPE_FISHEYE: {
        f_t w = k.v[0];
        if(ru < F_T_EPS) ru = F_T_EPS;
        kd_u = std::atan(2 * std::tan(w/2) * ru) / (ru * w);  // FIXME: add higher order terms (but not the linear one)
        if (dkd_u_dfeat_u)
            *dkd_u_dfeat_u = (2 * std::tan(w/2) / (w + 4 * ru * ru * w * std::tan(w/2) * std::tan(w/2)) - kd_u) / ru * feat_u.transpose() / ru;
        if (dkd_u_dk)
            *dkd_u_dk = m<1,4> {{ (2 / (1 + std::cos(w) + 4 * ru * ru * (1 - std::cos(w))) - kd_u) / w, 0, 0, 0 }};
    }   break;
    case rc_CALIBRATION_TYPE_POLYNOMIAL3: {
        kd_u = f_t(1) + ru2 * (k.v[0] + ru2 * (k.v[1] + ru2 * k.v[2]));
        if (dkd_u_dfeat_u)
            *dkd_u_dfeat_u = (k.v[0] + ru2 * (2 * k.v[1] + 3 * k.v[2] * ru2)) * 2 * feat_u.transpose();
        if (dkd_u_dk)
            *dkd_u_dk = m<1,4> {{
                ru2,
                ru2 * ru2,
                ru2 * ru2 * ru2,
                0,
            }};
    }   break;
    case rc_CALIBRATION_TYPE_KANNALA_BRANDT4: {
        if (ru < F_T_EPS) ru = F_T_EPS;
        f_t theta = std::atan(ru);
        f_t theta2 = theta*theta;
        f_t series = 1 + theta2*(k.v[0] + theta2*(k.v[1] + theta2*(k.v[2] + theta2*k.v[3])));
        f_t theta_ru = theta / ru;
        kd_u = theta_ru*series;
        if (dkd_u_dfeat_u)
            *dkd_u_dfeat_u = ((theta2*(3 * k.v[0] + theta2*(5 * k.v[1] + theta2*(9 * k.v[3] *theta2 + 7 * k.v[2]))) + 1) / ((ru2 + 1)*ru) - (series*theta) / ru2) * feat_u.transpose() / ru;
        if (dkd_u_dk)
            *dkd_u_dk = m<1,4> {{
                theta_ru * theta2,
                theta_ru * theta2 * theta2,
                theta_ru * theta2 * theta2 * theta2,
                theta_ru * theta2 * theta2 * theta2 * theta2,
            }};
        break;
    }
    }
    return kd_u;
}

f_t state_vision_intrinsics::get_undistortion_factor(const feature_t &feat_d, m<1,2> *dku_d_dfeat_d, m<1,4> *dku_d_dk) const
{
    f_t ku_d, rd2, rd = sqrt(rd2 = feat_d.squaredNorm());
    switch (type) {
    default:
    case rc_CALIBRATION_TYPE_UNKNOWN:
    case rc_CALIBRATION_TYPE_UNDISTORTED:
        ku_d = 1;
        if (dku_d_dfeat_d)
            *dku_d_dfeat_d = m<1,2>::Zero();
        if (dku_d_dk)
            *dku_d_dk = m<1,4>::Zero();
        break;
    case rc_CALIBRATION_TYPE_FISHEYE: {
        f_t w = k.v[0];
        if(rd < F_T_EPS) rd = F_T_EPS;
        ku_d = std::tan(w * rd) / (2 * std::tan(w/2) * rd);
        if (dku_d_dfeat_d)
            *dku_d_dfeat_d = (rd * w / (std::cos(rd * w) * std::cos(rd * w) * (2 * rd * std::tan(w/2))) - ku_d) / rd * feat_d.transpose() / rd;
        if (dku_d_dk)
            *dku_d_dk = m<1,4> {{
                (2 * rd * std::sin(w) - std::sin(2 * rd * w)) / (8 * rd * (std::cos(rd * w) * std::cos(rd * w)) * (std::sin(w/2) * std::sin(w/2))),
                0,
                0,
                0
            }};
    }   break;
    case rc_CALIBRATION_TYPE_POLYNOMIAL3: {
        f_t kd_u, ru2 = rd2, dkd_u_dru2;
        for (int i=0; i<4; i++) {
           kd_u =  1 + ru2 * (k.v[0] + ru2 * (k.v[1] + ru2 * k.v[2]));
           dkd_u_dru2 = k.v[0] + 2 * ru2 * (k.v[1] + 3 * ru2 * k.v[2]);
           // f(ru2) == ru2 * kd_u * kd_u - rd2 == 0;
           // ru2 -= f(ru2) / f'(ru2)
           ru2 -= (ru2 * kd_u * kd_u - rd2) / (kd_u * (kd_u + 2 * ru2 * dkd_u_dru2));
        }
        ku_d = 1 / kd_u;
        f_t ru = std::sqrt(ru2), dkd_u_dru = 2 * ru * dkd_u_dru2;
        // dku_d_drd = d/rd (1/kd_u) = d/ru (1/kd_u) dru/drd = d/ru (1/kd_u) / (drd/dru) = d/ru (1/kd_u) / (d/ru (ru kd_u)) = -dkd_u_dru/kd_u/kd_u / (kd_u + ru dkd_u_dru)
        if (dku_d_dfeat_d)
            *dku_d_dfeat_d = -dkd_u_dru/(kd_u * kd_u * (kd_u + ru * dkd_u_dru)) * feat_d.transpose() / rd;
        if (dku_d_dk)
            *dku_d_dk = m<1,4> {{
                -(ru2            )/(kd_u*kd_u),
                -(ru2 * ru2      )/(kd_u*kd_u),
                -(ru2 * ru2 * ru2)/(kd_u*kd_u),
                0,
            }};
    }   break;
    case rc_CALIBRATION_TYPE_KANNALA_BRANDT4: {
        if (rd < F_T_EPS) rd = F_T_EPS;
        f_t theta = rd;
        f_t theta2 = theta*theta;
        for (int i = 0; i < 4; i++) {
            f_t f = theta*(1 + theta2*(k.v[0] + theta2*(k.v[1] + theta2*(k.v[2] + theta2*k.v[3])))) - rd;
            if (f == 0)
                break;
            f_t df = 1 + theta2*(3 * k.v[0] + theta2*(5 * k.v[1] + theta2*(7 * k.v[2] + 9 * theta2*k.v[3])));
            // f(theta) == theta*(1 + theta2*(k0 + theta2*(k1 + theta2*(k2 + theta2*k3)))) - rd == 0;
            // theta -= f(theta) / f'(theta)
            theta -= f / df;
            theta2 = theta*theta;
        }
        f_t ru = std::tan(theta);
        f_t ru_rd2 = ru / rd2;
        ku_d = ru / rd;
        if (dku_d_dfeat_d)
            *dku_d_dfeat_d = -ru_rd2 * feat_d.transpose() / rd;
        // ku_d = ru / theta*(1 + theta2*(k.v[0] + theta2*(k.v[1] + theta2*(k.v[2] + theta2*k.v[3]))));
        if (dku_d_dk)
            *dku_d_dk = m<1,4> {{
                -ru_rd2 * theta * theta2,
                -ru_rd2 * theta * theta2 * theta2,
                -ru_rd2 * theta * theta2 * theta2 * theta2,
                -ru_rd2 * theta * theta2 * theta2 * theta2 * theta2,
            }};
        break;
    }
    }
    return ku_d;
}

void state_camera::update_feature_tracks(const rc_ImageData &image)
{
    START_EVENT(SF_TRACK, 0);
    tracker::image current_image;
    current_image.image = (uint8_t *)image.image;
    current_image.width_px = image.width;
    current_image.height_px = image.height;
    current_image.stride_px = image.stride;

    feature_tracker->tracks.clear();
    feature_tracker->tracks.reserve(feature_count());
    for(state_vision_group *g : groups.children) {
        if(!g->status || g->status == group_initializing) continue;
        for(state_vision_feature *feature : g->features.children)
            feature_tracker->tracks.emplace_back(&feature->track);
        for(state_vision_feature *feature : g->lost_features)
            feature_tracker->tracks.emplace_back(&feature->track);

    }
    for(auto &t:standby_features) feature_tracker->tracks.emplace_back(&t);

    if (feature_tracker->tracks.size())
        feature_tracker->track(current_image, feature_tracker->tracks);
    END_EVENT(SF_TRACK, feature_tracker->tracks.size())
}

float state_vision::median_depth_variance()
{
    float median_variance = 1;

    std::vector<state_vision_feature *> useful_feats;
    for (auto &c : cameras.children)
        for(auto &g: c->groups.children)
            for(auto &i: g->features.children)
                if(i->is_initialized())
                    useful_feats.push_back(i);

    if(useful_feats.size()) {
        sort(useful_feats.begin(), useful_feats.end(), [](state_vision_feature *a, state_vision_feature *b) { return a->variance() < b->variance(); });
        median_variance = (float)useful_feats[useful_feats.size() / 2]->variance();
    }

    return median_variance;
}

void state_vision::evolve_state(f_t dt)
{
    for (auto &c : cameras.children)
        for(auto &g : c->groups.children) {
            g->Tr.v += g->dTrp_ddT * dT;
            rotation_vector dWr(dW[0], dW[1], dW[2]);
            g->Qr.v *= to_quaternion(dWr); // FIXME: cache this?
        }
    state_motion::evolve_state(dt);
}

void state_vision::cache_jacobians(f_t dt)
{
    state_motion::cache_jacobians(dt);

    for (auto &c : cameras.children)
        for(auto &g : c->groups.children) {
            m3 Rr = g->Qr.v.toRotationMatrix();
            g->dTrp_ddT = (g->Qr.v * Q.v.conjugate()).toRotationMatrix();
            m3 xRrRtdT = skew(g->dTrp_ddT * dT);
            g->dTrp_dQ_s   = xRrRtdT;
            g->dQrp_s_dW = Rr * JdW_s;
        }

}

#ifdef SHAVE_PROJECT_MOTION_COVARIANCE
__attribute__((section(".cmx_direct.data"))) project_motion_covariance_data data;
void state_vision::project_motion_covariance_shave(matrix &dst, const matrix &src, f_t dt) const
{
    data.src_rows = src.rows();
    data.src_cols = src.cols();
    data.src_stride = src.get_stride();
    data.dst_rows = dst.rows();
    data.dst_cols = dst.cols();
    data.dst_stride = dst.get_stride();

    data.w.index = w.index;
    data.dw.index = dw.index;
    data.ddw.index = ddw.index;
    data.V.index = V.index;
    data.a.index = a.index;
    data.T.index = T.index;
    data.da.index = da.index;
    data.Q.index = Q.index;

    data.w.initial_covariance = w.get_initial_covariance();
    data.dw.initial_covariance = dw.get_initial_covariance();
    data.ddw.initial_covariance = ddw.get_initial_covariance();
    data.V.initial_covariance = V.get_initial_covariance();
    data.a.initial_covariance = a.get_initial_covariance();
    data.T.initial_covariance = T.get_initial_covariance();
    data.da.initial_covariance = da.get_initial_covariance();
    data.Q.initial_covariance = Q.get_initial_covariance();

    data.w.use_single_index = w.single_index();
    data.dw.use_single_index = dw.single_index();
    data.ddw.use_single_index = ddw.single_index();
    data.V.use_single_index = V.single_index();
    data.a.use_single_index = a.single_index();
    data.T.use_single_index = T.single_index();
    data.da.use_single_index = da.single_index();
    data.Q.use_single_index = Q.single_index();

    data.dQp_s_dW = dQp_s_dW.data();
    data.dt = dt;
    int camera_count = 0;
    for (auto &c : cameras.children){
        for(auto &g : c->groups.children) {
            data.tr[camera_count].index = g->Tr.index;
            data.qr[camera_count].index = g->Qr.index;
            data.tr[camera_count].initial_covariance = g->Tr.get_initial_covariance();
            data.qr[camera_count].initial_covariance = g->Qr.get_initial_covariance();
            data.tr[camera_count].use_single_index = g->Tr.single_index();
            data.qr[camera_count].use_single_index = g->Qr.single_index();
            data.dTrp_dQ_s_matrix[camera_count] = g->dTrp_dQ_s.data();
            data.dQrp_s_dW_matrix[camera_count] = g->dQrp_s_dW.data();
            data.dTrp_ddT_matrix[camera_count] = g->dTrp_ddT.data();
            camera_count++;
        }
    }
    data.camera_count = camera_count;

    static covariance_projector projector;
    projector.project_motion_covariance(dst.Data(), src.Data(), data);

    rtems_cache_invalidate_data_range(dst.Data(), dst.rows() * dst.get_stride() * sizeof(float) );
}
#endif

template<int N>
int state_vision::project_motion_covariance(matrix &dst, const matrix &src, f_t dt, int i) const
{
    //NOTE: Any changes here must also be reflected in state_motion:project_motion_covariance
    for(; i < (N > 1 ? std::min(src.cols(),dst.cols())/N*N : dst.cols()); i+=N) {
        const m<3,N> cov_w = w.from_row<N>(src, i);
        const m<3,N> cov_dw = dw.from_row<N>(src, i);
        const m<3,N> cov_ddw = ddw.from_row<N>(src, i);
        const m<3,N> cov_dW = dt * (cov_w + dt/2 * (cov_dw + dt/3 * cov_ddw));
        const m<3,N> scov_Q = Q.from_row<N>(src, i);
        dw.to_col<N>(dst, i) = cov_dw + dt * cov_ddw;
        w.to_col<N>(dst, i) = cov_w + dt * (cov_dw + dt/2 * cov_ddw);
        Q.to_col<N>(dst, i) = scov_Q + dQp_s_dW * cov_dW;
        const m<3,N> cov_V = V.from_row<N>(src, i);
        const m<3,N> cov_a = a.from_row<N>(src, i);
        const m<3,N> cov_T = T.from_row<N>(src, i);
        const m<3,N> cov_da = da.from_row<N>(src, i);
        const m<3,N> cov_dT = dt * (cov_V + dt/2 * (cov_a + dt/3 * cov_da));
        a.to_col<N>(dst, i) = cov_a + dt * cov_da;
        V.to_col<N>(dst, i) = cov_V + dt * (cov_a + dt/2 * cov_da);
        T.to_col<N>(dst, i) = cov_T + cov_dT;
        for (auto &c : cameras.children)
            for(auto &g : c->groups.children) {
                const auto cov_Tr = g->Tr.from_row<N>(src, i);
                const auto scov_Qr = g->Qr.from_row<N>(src, i);
                g->Qr.to_col<N>(dst, i) = scov_Qr + g->dQrp_s_dW * cov_dW;
                g->Tr.to_col<N>(dst, i) = cov_Tr + g->dTrp_dQ_s * (scov_Q - scov_Qr) + g->dTrp_ddT * cov_dT;
            }
    }
    return i;
}

void state_vision::project_motion_covariance(matrix &dst, const matrix &src, f_t dt) const
{
    START_EVENT(SF_PROJECT_MOTION_COVARIANCE, 0);
#ifdef SHAVE_PROJECT_MOTION_COVARIANCE
//#define SHAVE_PROJECT_COVARIANCE_TEST
#ifdef SHAVE_PROJECT_COVARIANCE_TEST
    matrix dstShave(dst.rows(), dst.cols());
    project_motion_covariance_shave(dstShave, src, dt);
#else
    project_motion_covariance_shave(dst, src, dt);
    if(0)
#endif
    {
#endif

    int i = 0;
    i = project_motion_covariance<4>(dst, src, dt, i);
    i = project_motion_covariance<1>(dst, src, dt, i);

#ifdef SHAVE_PROJECT_MOTION_COVARIANCE
    }
#ifdef SHAVE_PROJECT_COVARIANCE_TEST
    if(dst.identical(dstShave, 0.001)){
        printf("identical\n");
    }
#endif
#endif
    END_EVENT(SF_PROJECT_MOTION_COVARIANCE, 0);
}
