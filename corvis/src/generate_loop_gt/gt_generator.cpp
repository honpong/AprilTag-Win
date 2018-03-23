#include <functional>
#include <unordered_set>
#include "gt_generator.h"
#include "rc_compat.h"

void gt_generator::set_camera(const rc_Extrinsics& extrinsics, const loop_tester::fov& fov) {
    G_body_camera0_ = to_transformation(extrinsics.pose_m);
    fov_ = fov;
}

void gt_generator::add_reference_poses(const tpose_sequence& poses, rc_SessionId sid) {
    if (ref_poses_.size() <= sid) {
        ref_poses_.resize(sid + 1);
        map_.resize(sid + 1);
    }
    ref_poses_[sid] = poses;
}

void gt_generator::update_map(rc_MapNode* map_nodes, int num_map_nodes) {
    if (map_nodes && num_map_nodes > 0) {
        auto add_node = [this](rc_SessionId sid, std::map<rc_Timestamp, loop_tester::frustum>::iterator hint, rc_Timestamp time_us) {
            tpose G_world_body(sensor_clock::micros_to_tp(time_us));
            if (ref_poses_[sid].get_pose(G_world_body.t, G_world_body)) {
                map_[sid].emplace_hint(hint, time_us, G_world_body.G * G_body_camera0_);
            }
        };
        // map_nodes is sorted by session id and then timestamp
        const rc_MapNode* const map_end = map_nodes + num_map_nodes;
        const rc_MapNode* map_node = map_nodes;
        while (map_node != map_end) {
            rc_SessionId sid = map_node->time.session_id;
            if (ref_poses_.size() <= sid) {
                ref_poses_.resize(sid + 1);
                map_.resize(sid + 1);
            }
            auto& local_map = map_[sid];
            auto local_it = local_map.begin();
            for (; local_it != local_map.end() && map_node != map_end && map_node->time.session_id == sid; ) {
                if (local_it->first < map_node->time.time_us) {
                    local_it = local_map.erase(local_it);
                } else if (local_it->first > map_node->time.time_us) {
                    add_node(sid, local_it, map_node->time.time_us);
                    ++map_node;
                } else {
                    ++local_it;
                    ++map_node;
                }
            }
            local_map.erase(local_it, local_map.end());
            for (; map_node != map_end && map_node->time.session_id == sid; ++map_node) {
                add_node(sid, local_map.end(), map_node->time.time_us);
            }
        }
    } else {
        map_.clear();
    }
}

void gt_generator::clear() {
    ref_poses_.clear();
    map_.clear();
}

std::set<rc_SessionTimestamp> gt_generator::get_reference_edges(rc_Timestamp query_frame_us) const {
    std::set<rc_SessionTimestamp> gt_loops;
    if (rc_SESSION_CURRENT_SESSION < ref_poses_.size()) {
        tpose query_G_world_body(sensor_clock::micros_to_tp(query_frame_us));
        if (ref_poses_[rc_SESSION_CURRENT_SESSION].get_pose(query_G_world_body.t, query_G_world_body)) {
            loop_tester::frustum query_frame(query_G_world_body.G * G_body_camera0_);
            for (auto& it : map_[rc_SESSION_CURRENT_SESSION]) {
                if (it.first >= query_frame_us) break;  // do not match against the future
                if (loop_tester::is_loop(query_frame, it.second, fov_))
                    gt_loops.insert({it.first, rc_SESSION_CURRENT_SESSION});
            }
            if (rc_SESSION_PREVIOUS_SESSION < map_.size()) {
                for (auto& it : map_[rc_SESSION_PREVIOUS_SESSION]) {
                    if (loop_tester::is_loop(query_frame, it.second, fov_))
                        gt_loops.insert({it.first, rc_SESSION_PREVIOUS_SESSION});
                }
            }
        }
    }
    return gt_loops;
}
