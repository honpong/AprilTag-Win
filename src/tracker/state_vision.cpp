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
    return (status == feature_initializing || status == feature_normal);
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
    }
    return false;
}

f_t state_vision_group::ref_noise;
f_t state_vision_group::min_feats;

state_vision_group::state_vision_group(const transformation &G, state_camera &camera_, groupid group_id): Gr (make_aligned_shared<transformation>(G)), Tr(Gr->T, "Tr", constant), Qr(Gr->Q, "Qr", constant),  camera(camera_)
{
    id = group_id;
    children.push_back(&Qr);
    children.push_back(&Tr);
    children.push_back(&features);
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
    for (auto &camera : cameras.children)
        camera->reset();
    state_motion::reset();
}

void state_vision::clear_features_and_groups()
{
    stereo_matches.clear();
    for (auto &camera : cameras.children)
        camera->clear();
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
    state_vision_group *best_group = nullptr;
    state_vision_group *reference_group = nullptr;
    int best_health = -1;

    //First: process groups, mark additional features for deletion
    for(auto &g : groups.children) {
        int health = 0;
        for(auto &f : g->features.children)
            if(f->is_valid())
                ++health;

        // store current reference group (even in case it is removed)
        if(g->status == group_reference) reference_group = g.get();
        
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
        g->frames_active++;
    }
    if(best_group && need_reference) {
        best_group->make_reference();
        if(map) {
            map->reference_node = &map->get_node(best_group->id);
            if(reference_group) {
                // remove filter edges between active groups and dropped reference group
                // we will connect them to the new reference group below
                for(auto &g : groups.children) {
                    if(g->id != reference_group->id) {
                        edge_type type;
                        if(map->edge_in_map(reference_group->id, g->id, type)) {
                            assert(type != edge_type::relocalization);
                            assert(type != edge_type::dead_reckoning);
                            if(type == edge_type::filter) {
                                map->remove_edge(reference_group->id, g->id);
                            }
                        }
                    }
                }
            }
        }
        reference_group = best_group;
    }

    //Then: remove tracks based on feature and group status
    for(auto &camera : cameras.children)
        camera->tracks.remove_if([](const state_vision_track &t) {
            return t.feature.should_drop() || t.feature.group.status == group_empty;
        });

    // store info about reference group in case all groups are removed
    transformation G_reference_W;
    groupid reference_id{0};
    if(reference_group) {
        reference_id = reference_group->id;
        G_reference_W = invert(*reference_group->Gr);
    }

    groups.children.remove_if([&](const std::unique_ptr<state_vision_group> &g) {
        if(map && g->id != reference_id) {
            // update map edges
            edge_type type = g->status == group_empty ? edge_type::map : edge_type::filter;
            map->add_edge(reference_id, g->id, G_reference_W*(*g->Gr), type);
        }
        //Finally: remove features and groups
        if(g->status == group_empty) {
            if (map) {
                map->finish_node(g->id, !g->reused);
            }
            g->unmap();
            g->features.children.clear();
            g->lost_features.clear();
            return true;
        } else {
            // Delete the features we marked to drop
            g->features.children.remove_if([&](std::unique_ptr<state_vision_feature> &f) {
                if(f->status == feature_lost) {
                    g->lost_features.push_back(std::move(f));
                    return true;
                }
                return f->should_drop();
            });
            return false;
        }
    });
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

bool state_vision::get_closest_group_transformation(groupid &group_id, transformation& G) const {
    float min_group_distance = std::numeric_limits<float>::max();
    transformation G_now = transformation(Q.v,T.v);
    for (auto &g : groups.children) {
        auto temp = invert(*g->Gr)*G_now;
        float distance = temp.T.norm();
        if(distance <= min_group_distance) {
            min_group_distance = distance;
            group_id = g->id;
            G = temp;
        }
    }

    return min_group_distance < std::numeric_limits<float>::max();
}

bool state_vision::get_group_transformation(const groupid group_id, transformation& G) const
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
    int total_feats = 0;
    int outliers = 0;
    int track_fail = 0;
    for(auto &t : tracks) {
        if(!t.feature.tracks_found)
        {
            // Drop tracking failures
            ++track_fail;
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

    standby_tracks.remove_if([&map, &log](const tracker::feature_track &t) {
        bool not_found = !t.found();
        if(map && not_found) map->finish_lost_tracks(t);
        return not_found;
    });

    if(track_fail && !total_feats) log.warn("Tracker failed! {} features dropped.", track_fail);
    //    log.warn("outliers: {}/{} ({}%)", outliers, total_feats, outliers * 100. / total_feats);
    return total_feats;
}

void state_vision::update_map(mapper *map)
{
    if (!map) return;
    for (auto &g : groups.children) {
        map->set_node_transformation(g->id, *g->Gr);
        for (auto &f : g->features.children) {
            bool good = f->variance() < .05f*.05f; // f->variance() is equivalent to (stdev_meters/depth)^2
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

template<int N>
int state_vision::project_new_group_covariance(const state_vision_group &g, int i)
{
    //Note: this only works to fill in the covariance for Tr, Wr because it fills in cov(T,Tr) etc first (then copies that to cov(Tr,Tr).
    for(; i < cov.cov.rows(); i += N)
    {
        const m<3, N> cov_T = T.from_row<N>(cov.cov, i);
        g.Tr.to_col<N>(cov.cov, i) = cov_T;
        g.Tr.to_row<N>(cov.cov, i) = cov_T;
        const m<3, N> cov_Q = Q.from_row<N>(cov.cov, i);
        g.Qr.to_col<N>(cov.cov, i) = cov_Q;
        g.Qr.to_row<N>(cov.cov, i) = cov_Q;
    }
    return i;
}

void state_vision::project_new_group_covariance(const state_vision_group &g)
{
    size_t i = 0;
    i = project_new_group_covariance<4>(g, i);
    i = project_new_group_covariance<1>(g, i);
}

state_vision_group * state_vision::add_group(const rc_Sensor camera_id, mapper *map)
{
    state_camera& camera = *cameras.children[camera_id];
    transformation G(Q.v, T.v);
    auto g = std::make_unique<state_vision_group>(G, camera, group_counter++);
    if(map) {
        map->add_node(g->id, camera_id);

        state_vision_group *reference_group = nullptr;
        for(auto &group : groups.children) {
            map->add_covisibility_edge(g->id, group->id);
            if(group->status == group_reference)
                reference_group = group.get();
        }

        if(reference_group)
            map->add_edge(reference_group->id, g->id, invert(*reference_group->Gr) * (*g->Gr), edge_type::filter);
        else if(map->reference_node) // connect graph again using dead reckoning
            map->add_edge(map->reference_node->id, g->id,
                          invert(map->reference_node->global_transformation) * (*g->Gr), edge_type::dead_reckoning);
    }
    auto *p = g.get();
    groups.children.push_back(std::move(g));
    remap();
    project_new_group_covariance(*p);
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

void state_camera::update_feature_tracks(const sensor_data &data)
{
    START_EVENT(SF_TRACK, 0);
    feature_tracker->tracks.clear();
    feature_tracker->tracks.reserve(track_count());
    for(auto &t:tracks) feature_tracker->tracks.emplace_back(&t.track);
    for(auto &t:standby_tracks) feature_tracker->tracks.emplace_back(&t);

    if (feature_tracker->tracks.size()) {
        auto start = std::chrono::steady_clock::now();
        feature_tracker->track(data.tracker_image(), feature_tracker->tracks);
        auto stop = std::chrono::steady_clock::now();
        track_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    }

    END_EVENT(SF_TRACK, feature_tracker->tracks.size())
}

void state_camera::update_map_tracks(const sensor_data &data, mapper *map,
                                     const size_t min_group_map_add, const groupid closest_group_id,
                                     const transformation &G_Bclosest_Bnow) {
    START_EVENT(SF_TRACK, 0);
    feature_tracker->tracks.clear();
    START_EVENT(SF_PREDICT_MAP, 0);
    // create tracks of features visible in inactive map nodes
    map->predict_map_features(data.id, min_group_map_add, closest_group_id, G_Bclosest_Bnow);
    for(auto &nft : map->map_feature_tracks) {
        for(auto &mft : nft.tracks)
            feature_tracker->tracks.emplace_back(&mft.track);
    }
    END_EVENT(SF_PREDICT_MAP, map->map_feature_tracks.size());

    if (feature_tracker->tracks.size()) {
        auto start = std::chrono::steady_clock::now();
        feature_tracker->track(data.tracker_image(), feature_tracker->tracks);
        auto stop = std::chrono::steady_clock::now();
        map_track_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
        // sort map tracks according to number of features found
        for (auto &nft : map->map_feature_tracks) {
            for (auto &mft : nft.tracks) {
                nft.found += mft.track.found();
            }
        }
        auto it = std::remove_if(map->map_feature_tracks.begin(), map->map_feature_tracks.end(),
                                 [&min_group_map_add](mapper::node_feature_track& nft) {
                                    return nft.found < min_group_map_add;
                                 });
        map->map_feature_tracks.erase(it, map->map_feature_tracks.end());

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

//NOTE: Any changes here must also be reflected in state_motion:project_motion_covariance
#ifdef ENABLE_SHAVE_PROJECT_MOTION_COVARIANCE
void state_vision::project_motion_covariance(matrix &dst, const matrix &src, f_t dt) const
{
    START_EVENT(SF_PROJECT_MOTION_COVARIANCE, std::min(src.cols(),dst.cols()));

    __attribute__((section(".cmx_direct.data")))
    static project_motion_covariance_data data;
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

    __attribute__((section(".cmx_direct.bss")))
    static covariance_projector projector;
    projector.project_motion_covariance(data);

    END_EVENT(SF_PROJECT_MOTION_COVARIANCE, std::min(src.cols(),dst.cols()));
}
#endif