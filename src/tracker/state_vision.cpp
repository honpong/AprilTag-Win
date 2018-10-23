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
f_t state_vision_feature::good_variance;
f_t state_vision_feature::max_variance;

state_vision_feature::state_vision_feature(const tracker::feature_track &track_, state_vision_group &group_):
    state_leaf("feature", constant), v(std::make_shared<log_depth>()), feature(track_.feature), group(group_)
{
    v->initial = {track_.x, track_.y};
    reset();
}

void state_vision_feature::drop()
{
    status = feature_empty;
}

void state_vision_feature::make_lost()
{
    status = feature_lost;
    save_initial_variance();
    index = -1;
}

void state_vision_feature::make_outlier()
{
    status = feature_outlier;
}

bool state_vision_feature::should_drop() const
{
    return status == feature_outlier || status == feature_empty || variance() > max_variance;
}

bool state_vision_feature::is_valid() const
{
    return (status == feature_initializing || status == feature_normal);
}

bool state_vision_feature::is_good() const
{
    return is_valid() && variance() < good_variance;
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
            best_group = g.get();
        }
    }
    if(best_group && need_reference) {
        best_group->make_reference();
        reference_group = best_group;
        if(map) {
            if(map->reference_node) {
                // remove filter edges between active groups and dropped reference group
                // we will connect them to the new reference group below
                for(auto &g : groups.children) {
                    if(g->id != map->reference_node->id && g->id != reference_group->id) {
                        edge_type type;
                        if(map->edge_in_map(map->reference_node->id, g->id, type)) {
                            if(type == edge_type::filter) {
                                map->remove_edge(map->reference_node->id, g->id);
                            }
                        }
                    }
                }
            }
            map->reference_node = &map->get_node(reference_group->id);
        }
    }

    //Then: remove tracks based on feature and group status
    for(auto &camera : cameras.children)
        camera->tracks.remove_if([](const state_vision_track &t) {
            return t.feature.should_drop() || t.feature.group.status == group_empty;
        });

    // store info about reference group in case all groups are removed
    if(map && reference_group) {
        map->reference_node->global_transformation = *reference_group->Gr;
    }

    groups.children.remove_if([&](const std::unique_ptr<state_vision_group> &g) {
        if(map && g->id != map->reference_node->id) {
            // update map edges
            edge_type type = g->status == group_empty ? edge_type::map : edge_type::filter;
            map->add_edge(map->reference_node->id, g->id, invert(map->reference_node->global_transformation)*(*g->Gr), type);
        }
        //Finally: remove features and groups
        if(g->status == group_empty) {
            if (map) {
                map->finish_node(g->id);
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

std::vector<triangulated_track> state_camera::process_tracks()
{
    for(auto &t : tracks) {
        if(!t.feature.tracks_found) {
            // Drop tracking failures
            if(t.feature.is_good() && t.outlier < t.outlier_lost_reject)
                t.feature.make_lost();
            else
                t.feature.drop();
        } else if(t.outlier > t.outlier_reject)
            // Drop outliers
            t.feature.make_outlier();
    }

    std::vector<triangulated_track> lost_triangulated_tracks;
    standby_tracks.remove_if([&lost_triangulated_tracks](triangulated_track &t) {
        if(!t.found()) {
            if (t.good() && t.outlier < state_vision_track::outlier_lost_reject && !t.state_shared()) {
                lost_triangulated_tracks.emplace_back(std::move(t));
            }
        }
        return !t.found();
    });
    return lost_triangulated_tracks;
}

void state_vision::update_map(mapper *map, const std::vector<triangulated_track> &lost_triangulated_tracks)
{
    if (!map) return;
    for (auto &g : groups.children) {
        map->set_node_transformation(g->id, *g->Gr);
        map->get_node(g->id).frames_active++;
        for (auto &f : g->features.children) {
            bool good = f->variance() < .05f*.05f; // f->variance() is equivalent to (stdev_meters/depth)^2
            if (good) {
                nodeid current_node;
                if (map->feature_in_map(f->feature->id, &current_node)) {
                    assert(g->id == current_node);
                    map->set_feature_type(current_node, f->feature->id, f->variance(), feature_type::tracked);
                } else {
                    auto feature = std::static_pointer_cast<fast_tracker::fast_feature<DESCRIPTOR>>(f->feature);
                    map->add_feature(g->id, feature, f->v, f->variance());
                }
            }
        }
    }
    for (auto &t : lost_triangulated_tracks)
        map->add_feature(t.reference_node(), std::static_pointer_cast<fast_tracker::fast_feature<DESCRIPTOR>>(t.feature),
                         t.v(), t.v_var(), feature_type::triangulated);
}

void state_vision::project_new_group_covariance(const state_vision_group &g)
{
    cov.cov.map()(g.Tr.index + 0, g.Tr.index + 0) *= 1.01;
    cov.cov.map()(g.Tr.index + 1, g.Tr.index + 1) *= 1.01;
    cov.cov.map()(g.Tr.index + 2, g.Tr.index + 2) *= 1.01;

    cov.cov.map()(g.Qr.index + 0, g.Qr.index + 0) *= 1.01;
    cov.cov.map()(g.Qr.index + 1, g.Qr.index + 1) *= 1.01;
    cov.cov.map()(g.Qr.index + 2, g.Qr.index + 2) *= 1.01;
}

state_vision_group * state_vision::add_group(const rc_Sensor camera_id, mapper *map)
{
    state_camera& camera = *cameras.children[camera_id];
    transformation G(Q.v, T.v);
    auto g = std::make_unique<state_vision_group>(G, camera, group_counter++);
    g->Tr.index = T.index;
    g->Qr.index = Q.index;
    g->Tr.set_initial_variance(T.initial_covariance.diagonal()); // Used when both T and Tr are added at the same time wo/remap() between
    g->Qr.set_initial_variance(Q.initial_covariance.diagonal());

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

    // if the total number of finished nodes is bigger than 150 (for example: current session + unlinked loaded map) or the
    // local number of finished nodes is bigger than 40 (just current session) remove 10 nodes with the lowest number of active frames
    constexpr size_t global_max_nodes = 150;
    constexpr size_t local_max_nodes = 40;
    constexpr size_t num_nodes_removed = 10;
    static_assert(global_max_nodes > local_max_nodes, "Global max number of nodes should be bigger than Local max nodes");
    static_assert(local_max_nodes > num_nodes_removed, "Local max number of nodes should be bigger than Num nodes removed");
    if(map && map->get_nodes().size() >= local_max_nodes) {
        std::vector<std::pair<nodeid, uint64_t>> removable_nodes;
        removable_nodes.reserve(num_nodes_removed + 1);
        auto frames_active_comp = [](std::pair<nodeid, uint64_t>& n1, std::pair<nodeid, uint64_t>& n2) {
                              return n1.second < n2.second;};
        for(auto& node : map->get_nodes()) {
            if(map->get_nodes().size() > global_max_nodes || !map->is_unlinked(node.second.id)) {
                if(node.second.status == node_status::finished && !map->is_root(node.second.id)) {
                    removable_nodes.emplace_back(node.second.id, node.second.frames_active);
                    push_heap(removable_nodes.begin(), removable_nodes.end(), frames_active_comp);
                    if(removable_nodes.size() > num_nodes_removed) {
                        pop_heap(removable_nodes.begin(), removable_nodes.end(), frames_active_comp);
                        removable_nodes.pop_back();
                    }
                }
            }
        }
        for(auto& remove_node : removable_nodes) {
            map->remove_node(remove_node.first);
        }
    }
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

feature_t state_vision_intrinsics::project_feature(const v3 &ray) const
{
    return unnormalize_feature(distort_feature(ray.head<2>() / ray.z()));
}

v3 state_vision_intrinsics::unproject_feature(const feature_t &pix) const
{
    return undistort_feature(normalize_feature(pix)).homogeneous();
}

m<2,3> state_vision_intrinsics::dproject_dX(const v3 &X) const
{
    // xu = X/X[2]
    // xd = kd_u * xu
    // project = (Xd * F + C) * h + {w-1,h-1}/2
    v2 xu = X.head<2>() / X[2];
    m<1,2> dkd_u_dxu;
    f_t kd_u = get_distortion_factor(xu, &dkd_u_dxu, nullptr);
    f_t invZ = 1/X[2];
    m<2,3> dxu_dX = {{ invZ,    0, -xu[0] * invZ },
                     {    0, invZ, -xu[1] * invZ }};
    m<1,3> dkd_u_dX = dkd_u_dxu * dxu_dX;
    return image_height * focal_length.v * (xu * dkd_u_dX + dxu_dX * kd_u);
}

void state_vision_intrinsics::dproject_dintrinsics(const v3 &X, m<2,2> &dx_dc, m<2,1> &dx_dF, m<2,4> &dx_dk) const
{
    m<1,4> dkd_u_dk;
    v2 Xu = X.head<2>() / X[2];
    f_t kd_u = get_distortion_factor(Xu, nullptr, &dkd_u_dk);
    dx_dF = image_height * Xu * kd_u;
    dx_dk = image_height * focal_length.v * Xu * dkd_u_dk;
    dx_dc = image_height * m<2,2>::Identity();
}

void state_vision_intrinsics::dunproject_dintrinsics(const feature_t &feat, m<3,2> &dX_dc, m<3,1> &dX_dF, m<3,4> &dX_dk) const
{
    // xd = ((project - {w-1,h-1}/2) / h - C) / F
    // xu = ku_d * xd
    // X = xu.homogeneous()
    m<1,2> dku_d_dxd;
    f_t ku_d; m<1,4> dku_d_dk;
    auto xd = normalize_feature(feat);
    ku_d = get_undistortion_factor(xd, &dku_d_dxd, &dku_d_dk);
    dX_dc = (v3(xd.x(), xd.y(), 0) *  dku_d_dxd +      ku_d * m<3,2>::Identity()) / -focal_length.v;
    dX_dF =  v3(xd.x(), xd.y(), 0) * (dku_d_dxd * xd + ku_d * m<1,1>::Identity()) / -focal_length.v;
    dX_dk =  v3(xd.x(), xd.y(), 0) *  dku_d_dk;
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

void state_camera::update_map_tracks(const sensor_data &data, mapper *map, const mapper::nodes_path& neighbors,
                                     const size_t min_group_map_add) {
    START_EVENT(SF_TRACK, 0);
    feature_tracker->tracks.clear();
    START_EVENT(SF_PREDICT_MAP, 0);
    // create tracks of features visible in inactive map nodes
    map->predict_map_features(data.id, neighbors, min_group_map_add);
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

void state_camera::add_detected_features(std::vector<tracker::feature_track> &detected) {
    // insert (newest w/highest score first)
    standby_tracks.insert(standby_tracks.begin(), std::make_move_iterator(detected.begin()), std::make_move_iterator(detected.end()));
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

bool triangulated_track::measure(const transformation &G_now_ref, const v2 &X_un_ref, const v2 &X_un_now, f_t sigma2) {
    // calculate point prediction on current camera frame
    v3 X_now = G_now_ref.Q * X_un_ref.homogeneous() + G_now_ref.T * state->v->invdepth();
    // features are in front of the camera
    if (X_now.z() < 0)
        return false;
    v2 h = X_now.head<2>() * (1/X_now.z());
    v2 inn = X_un_now-h;

    // compute jacobians
    m<2,3> dh_dX_now = {{1/X_now.z(), 0,           -h.x() * (1/X_now.z())},
                        {0,           1/X_now.z(), -h.y() * (1/X_now.z())}};
    v3 dX_now_dv = G_now_ref.T * state->v->invdepth_jacobian();
    v2 H = dh_dX_now * dX_now_dv;

    if (inn.squaredNorm() > sigma2) {
        state->track_count = 0;
        outlier += inn.squaredNorm() / sigma2;
        sigma2 = inn.squaredNorm();
    } else
        outlier = 0;

    auto R = v2{sigma2,sigma2}.asDiagonal();
    m<2,1> HP = H*state->P;
    m<2,2> S = HP*H.transpose(); S += R;
    Eigen::LLT<m<2,2>> Sllt = S.llt();
    m<1,2> K =  Sllt.solve(HP).transpose();

    state->v->v += K*inn;
    state->P -= K * HP;
    state->track_count++;
    state->cos_parallax = std::min(state->cos_parallax, X_un_ref.homogeneous().normalized().dot(X_un_now.homogeneous().normalized()));

    return inn.dot(Sllt.solve(inn)) < 6; // check mahalanobis distance to remove outliers
}

void triangulated_track::merge(const triangulated_track& rhs) {
    feature = rhs.feature;
    state = rhs.state;
}
