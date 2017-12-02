// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include "state_vision.h"
#include "fast_tracker.h"
#include "transformation.h"
#include <spdlog/fmt/ostr.h> // must be included to use our operator<<
#include "Trace.h"
#include <limits>

#ifdef MYRIAD2
    #include "platform_defines.h"
    #include "covariance_projector.h"
#endif

f_t state_vision_feature::initial_depth_meters;
f_t state_vision_feature::initial_var;
f_t state_vision_feature::initial_process_noise;
f_t state_vision_track::outlier_thresh;
f_t state_vision_track::outlier_reject;
f_t state_vision_track::outlier_lost_reject;
f_t state_vision_feature::max_variance;

state_vision_feature::state_vision_feature(const tracker::feature_track &track_, state_vision_group &group_):
    state_leaf("feature", constant), v(std::make_shared<log_depth>()), feature(track_.feature), group(group_)
{
    v->initial = {track_.x, track_.y};
    reset();
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

state_vision_group::state_vision_group(state_camera &camera_, uint64_t group_id): Gr (std::make_shared<transformation>()), Tr(Gr->T, "Tr", dynamic), Qr(Gr->Q, "Qr", dynamic),  camera(camera_)
{
    id = group_id;
    children.push_back(&Qr);
    children.push_back(&Tr);
    children.push_back(&features);
    Tr.v = v3(0, 0, 0);
    Qr.v = quaternion::Identity();
    f_t near_zero = F_T_EPS * 100;
    Tr.set_initial_variance({near_zero, near_zero, near_zero});
    Qr.set_initial_variance({near_zero, near_zero, near_zero});
    Tr.set_process_noise(ref_noise);
    Qr.set_process_noise(ref_noise);
}

void state_vision_group::make_reference()
{
    assert(status == group_normal);
    status = group_reference;
    int normals = 0;
    for(auto &f : features.children) {
        if(f->is_initialized()) ++normals;
    }
    if(normals < 3) {
        for(auto &f : features.children) {
            if(!f->is_initialized()) {
                if (f->force_initialize()) ++normals;
                if(normals >= 3) break;
            }
        }
    }
    //remove_child(&Tr);
    //Qr.saturate();
}

state_vision::~state_vision()
{
    clear_features_and_groups();
}

void state_vision::reset()
{
    clear_features_and_groups();
    for (auto &camera : cameras.children) {
        camera->detecting_space = 0;
    }
    state_motion::reset();
}

size_t state_vision::track_count() const
{
    int res = 0;
    for (auto &camera : cameras.children) res += camera->track_count();
    return res;
}

void state_vision::clear_features_and_groups()
{
    for (auto &camera : cameras.children)
    {
        camera->tracks.clear();
        camera->standby_tracks.clear();
    }
    for(auto &g : groups.children) { //This shouldn't be necessary, but keep to remind us to clear if features move
        g->features.children.clear();
        g->lost_features.clear();
    }
    groups.children.clear();
}

int state_vision::process_features(mapper *map)
{
    int total_health = 0;
    bool need_reference = true;
    state_vision_group *best_group = 0;
    int best_health = -1;

    //First: process groups, mark additional features for deletion
    for(auto &g : groups.children) {
        int health = 0;
        for(auto &f : g->features.children)
            if(!f->should_drop() && f->status != feature_lost) ++health;
        
        if(health < state_vision_group::min_feats)
            g->status = group_empty;
        else
            total_health += health;
        
        // Found our reference group
        if(g->status == group_reference) need_reference = false;
        
        if(g->status == group_normal) {
            if(health > best_health) {
                best_group = g.get();
                best_health = g->health;
            }
        }
    }
    if(best_group && need_reference) best_group->make_reference();

    //Then: remove tracks based on feature and group status
    for(auto &camera : cameras.children)
        for(auto t = camera->tracks.begin(); t != camera->tracks.end();)
            if(t->feature.should_drop() || t->feature.group.status == group_empty) t = camera->tracks.erase(t);
            else ++t;

    //Finally: remove features and groups
    for(auto i = groups.children.begin(); i != groups.children.end();) {
        auto &g = *i;
        if(g->status == group_empty) {
            if (map) map->node_finished(g->id);
            g->unmap();
            g->features.children.clear();
            g->lost_features.clear();
            i = groups.children.erase(i);
        } else {
            // Delete the features we marked to drop
            for(auto f = g->features.children.begin(); f != g->features.children.end();) {
                if((*f)->should_drop()) {
                    f = g->features.children.erase(f);
                } else if((*f)->status == feature_lost) {
                    g->lost_features.push_back(std::move(*f));
                    f = g->features.children.erase(f);
                } else {
                    f++;
                }
            }
            ++i;
        }
    }
    return total_health;
}

void state_vision::enable_orientation_only(bool _remap)
{
    clear_features_and_groups();
    state_motion::enable_orientation_only(_remap);
}

size_t state_camera::track_count() const
{
    int count = 0;
    for(auto &t : tracks)
        if(t.track.found()) ++count;

    return count + standby_tracks.size();
}

transformation state_vision::get_transformation() const
{
    return loop_offset*transformation(Q.v, T.v);
}

bool state_vision::get_closest_group_transformation(const uint64_t group_id, transformation& G) const
{
    for (auto &g : groups.children) {
        if(g->id == group_id) {
            G = *g->Gr;
            return true;
        }
    }
    return false;
}

int state_camera::process_tracks(mapper *map, spdlog::logger &log)
{
    int useful_drops = 0;
    int total_feats = 0;
    int outliers = 0;
    int track_fail = 0;
    for(auto &t : tracks) {
        if(!t.feature.tracks_found)
        {
            // Drop tracking failures
            ++track_fail;
            if(t.feature.is_good()) ++useful_drops;
            if(t.feature.is_good() && t.outlier < t.outlier_lost_reject)
                t.feature.make_lost();
            else
                t.feature.drop();
        } else {
            // Drop outliers
            if(t.feature.status == feature_normal) ++total_feats;
            if(t.outlier > t.outlier_reject) {
                t.feature.status = feature_empty;
                ++outliers;
            }
        }
    }
    const f_t focal_px = intrinsics.focal_length.v * intrinsics.image_height;
    const f_t sigma = 5 / focal_px; // sigma_px = 5
    standby_tracks.remove_if([&map, &log, sigma](tracker::feature_track &t) {
        bool not_found = !t.found();
        if (map && not_found) {
            // Triangulate point not in filter neither being tracked
            if (t.group_tracks.size() > 1) {
                aligned_vector<v2> tracks_2d;
                std::vector<transformation> camera_poses;
                const uint64_t& ref_group_id = t.group_tracks[0].group_id;
                map->get_triangulation_geometry(ref_group_id, t, tracks_2d, camera_poses);
                f_t depth_m;
                float mean_error_point = estimate_3d_point(tracks_2d,camera_poses, depth_m);
                if ((depth_m > 0) && (mean_error_point <  2*sigma)) // a good 3d point has to be in front of the camera
                {
                    auto f = std::static_pointer_cast<fast_tracker::fast_feature<DESCRIPTOR>>(t.feature);
                    std::shared_ptr<log_depth> v = std::make_shared<log_depth>();
                    v->set_depth_meters(depth_m);
                    v->initial[0] = t.group_tracks[0].x;
                    v->initial[1] = t.group_tracks[0].y;
                    map->add_triangulated_feature_to_group(ref_group_id, f, v);
                }
                else
                    log.debug("{}/{}) Reprojection error too large for triangulated point with id: {}", t.feature->id);
            }
        }
        return not_found;
    });

    if(track_fail && !total_feats) log.warn("Tracker failed! {} features dropped.", track_fail);
    //    log.warn("outliers: {}/{} ({}%)", outliers, total_feats, outliers * 100. / total_feats);
    return total_feats;
}

void state_vision::update_map(mapper *map)
{
    if (!map) return;
    float distance_current_node = std::numeric_limits<float>::max();
    for (auto &g : groups.children) {
        map->set_node_transformation(g->id, get_transformation()*invert(*g->Gr));
        // Set current node as the closest active group to current pose
        if(g->Tr.v.norm() <= distance_current_node) {
            distance_current_node = g->Tr.v.norm();
            map->current_node_id = g->id;
        }

        // update map edges for all active groups
        for (auto &g2 : groups.children) {
            if(g2->id > g->id) {
                const transformation& Gg_now = *g->Gr;
                const transformation& Gnow_g2 = invert(*g2->Gr);
                map->add_edge(g->id, g2->id, Gg_now*Gnow_g2);
            }
        }
        for (auto &f : g->features.children) {
            float stdev = (float)f->v->stdev_meters(sqrt(f->variance()));
            float variance_meters = stdev*stdev;
            const float measurement_var = 1.e-3f*1.e-3f;
            if (variance_meters < measurement_var)
                variance_meters = measurement_var;
            
            bool good = stdev / f->v->depth() < .05f;
            if (good) {
                if(f->is_in_map) {
                    map->set_feature_type(g->id, f->feature->id, feature_type::tracked);
                } else {
                    auto feature = std::static_pointer_cast<fast_tracker::fast_feature<DESCRIPTOR>>(f->feature);
                    map->add_feature(g->id, feature, f->v);
                }
                f->is_in_map = true;
            }
        }
    }
}

state_vision_group * state_vision::add_group(const rc_Sensor camera_id, mapper *map)
{
    state_camera& camera = *cameras.children[camera_id];
    auto g = std::make_unique<state_vision_group>(camera, group_counter++);
    if(map) {
        map->add_node(g->id, camera_id);
        // add group id to standby_tracks to triangulate
        for (tracker::feature_track &f : camera.standby_tracks) {
            f.group_tracks.push_back({g->id,f.x,f.y});
        }
        // Connect graph again if it got disconnected
        if(groups.children.empty() && (map->current_node_id != std::numeric_limits<uint64_t>::max())) {
            mapper::nodes_path path = map->breadth_first_search(map->get_node_id_offset(), std::set<mapper::nodeid>{map->current_node_id});
            transformation& G_init_current = path[map->current_node_id];
            transformation G_current_now = invert(transformation{map->get_node(map->get_node_id_offset()).global_transformation.Q, v3::Zero()}*G_init_current)
                    *get_transformation();
            map->add_edge(map->current_node_id, g->id, G_current_now);
        }
    }
    auto *p = g.get();
    groups.children.push_back(std::move(g));
    return p;
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
    f_t kd_u = 1, ru2, ru = std::sqrt(ru2 = feat_u.squaredNorm());
    switch (type) {
    case rc_CALIBRATION_TYPE_UNKNOWN:
        assert(0);
        break;
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
    f_t ku_d = 1, rd2, rd = sqrt(rd2 = feat_d.squaredNorm());
    switch (type) {
    case rc_CALIBRATION_TYPE_UNKNOWN:
        assert(0);
        break;
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
        f_t kd_u = 1, ru2 = rd2, dkd_u_dru2 = 0;
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
            if (std::abs(f) < F_T_EPS)
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

void state_camera::update_feature_tracks(const sensor_data &data, mapper *map, const transformation& G_Bcurrent_Bnow)
{
    START_EVENT(SF_TRACK, 0);
    tracker::image current_image;
    current_image.image = (uint8_t *)data.image.image;
    current_image.width_px = data.image.width;
    current_image.height_px = data.image.height;
    current_image.stride_px = data.image.stride;

    feature_tracker->tracks.clear();
    feature_tracker->tracks.reserve(track_count());
    for(auto &feature : tracks) feature_tracker->tracks.emplace_back(&feature.track);
    for(auto &t:standby_tracks) feature_tracker->tracks.emplace_back(&t);

    // create tracks of features visible in inactive map nodes
    if(map) {
        map->predict_map_features(data.id, G_Bcurrent_Bnow);
        for(auto &nft : map->map_feature_tracks) {
            for(auto &mft : nft.tracks)
                feature_tracker->tracks.emplace_back(&mft.track);
        }
    }

    if (feature_tracker->tracks.size())
        feature_tracker->track(current_image, feature_tracker->tracks);

    // sort map tracks according to number of features found
    if(map) {
        for (auto &nft : map->map_feature_tracks)
            for (auto &mft : nft.tracks)
                nft.found += mft.track.found();
        std::sort(map->map_feature_tracks.begin(), map->map_feature_tracks.end(),
                  [](const mapper::node_feature_track &a, const mapper::node_feature_track &b) {
                      return a.found > b.found;
                  });
    }

    END_EVENT(SF_TRACK, feature_tracker->tracks.size())
}

float state_vision::median_depth_variance()
{
    float median_variance = 1;

    std::vector<state_vision_feature *> useful_feats;
    for(auto &g: groups.children)
        for(auto &i: g->features.children)
            if(i->is_initialized())
                useful_feats.push_back(i.get());

    if(useful_feats.size()) {
        sort(useful_feats.begin(), useful_feats.end(), [](state_vision_feature *a, state_vision_feature *b) { return a->variance() < b->variance(); });
        median_variance = (float)useful_feats[useful_feats.size() / 2]->variance();
    }

    return median_variance;
}

void state_vision::evolve_state(f_t dt)
{
    for(auto &g : groups.children) {
        g->Tr.v += g->dTrp_ddT * dT;
        rotation_vector dWr(dW[0], dW[1], dW[2]);
        g->Qr.v *= to_quaternion(dWr); // FIXME: cache this?
    }
    state_motion::evolve_state(dt);
}

void state_vision::cache_jacobians(f_t dt)
{
    state_motion::cache_jacobians(dt);

    for(auto &g : groups.children) {
        m3 Rr = g->Qr.v.toRotationMatrix();
        g->dTrp_ddT = (g->Qr.v * Q.v.conjugate()).toRotationMatrix();
        m3 xRrRtdT = skew(g->dTrp_ddT * dT);
        g->dTrp_dQ_s   = xRrRtdT;
        g->dQrp_s_dW = Rr * JdW_s;
    }

}

#ifdef ENABLE_SHAVE_PROJECT_MOTION_COVARIANCE
__attribute__((section(".cmx_direct.data"))) project_motion_covariance_data data;
void state_vision::project_motion_covariance(matrix &dst, const matrix &src, f_t dt) const
{
    START_EVENT(SF_PROJECT_MOTION_COVARIANCE, 0);

    data.src = src.Data();
    data.src_rows = src.rows();
    data.src_cols = src.cols();
    data.src_stride = src.get_stride();
    data.dst = dst.Data();
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
    for(const auto &g : groups.children) {
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
    data.camera_count = camera_count;

    static covariance_projector projector;
    projector.project_motion_covariance(data);

    END_EVENT(SF_PROJECT_MOTION_COVARIANCE, 0);
}

#else

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
        for(auto &g : groups.children) {
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
    int i = 0;
    i = project_motion_covariance<4>(dst, src, dt, i);
    i = project_motion_covariance<1>(dst, src, dt, i);
    END_EVENT(SF_PROJECT_MOTION_COVARIANCE, 0);
}
#endif
