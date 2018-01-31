#include <functional>
#include <unordered_set>
#include "gt_generator.h"
#include "rc_compat.h"

void gt_generator::set_camera(const rc_Extrinsics& extrinsics, const loop_tester::fov& fov) {
    G_body_camera0_ = to_transformation(extrinsics.pose_m);
    fov_ = fov;
}

void gt_generator::add_reference_poses(const tpose_sequence& poses) {
    if (ref_poses_.tposes.empty()) {
        ref_poses_ = poses;
    } else if (!poses.tposes.empty()) {
        if (poses.tposes.front().t > ref_poses_.tposes.back().t) {
            // insert at the end
            ref_poses_.tposes.insert(ref_poses_.tposes.end(),
                                     poses.tposes.begin(), poses.tposes.end());
        } else if (poses.tposes.back().t < ref_poses_.tposes.front().t) {
            // insert at the beginning
            ref_poses_.tposes.insert(ref_poses_.tposes.begin(),
                                     poses.tposes.begin(), poses.tposes.end());
        } else {
            assert(false && "gt_generator::add_reference_poses: Pose sequences overlap");
        }
    }
}

void gt_generator::update_map(rc_MapNode* map_nodes, int num_map_nodes) {
    if (map_nodes && num_map_nodes > 0) {
        auto add_node = [this](decltype(map_)::iterator hint, rc_Timestamp time_us) {
            tpose G_world_body(sensor_clock::micros_to_tp(time_us));
            if (ref_poses_.get_pose(G_world_body.t, G_world_body)) {
                map_.emplace_hint(hint, time_us, G_world_body.G * G_body_camera0_);
            }
        };
        const rc_MapNode* const map_end = map_nodes + num_map_nodes;
        const rc_MapNode* map_node = map_nodes;
        auto local_it = map_.begin();
        for (; local_it != map_.end() && map_node != map_end; ) {
            if (local_it->first < map_node->time_us) {
                local_it = map_.erase(local_it);
            } else if (local_it->first > map_node->time_us) {
                add_node(local_it, map_node->time_us);
                ++map_node;
            } else {
                ++local_it;
                ++map_node;
            }
        }
        map_.erase(local_it, map_.end());
        for (; map_node != map_end; ++map_node) {
            add_node(map_.end(), map_node->time_us);
        }
    } else {
        map_.clear();
    }
}

void gt_generator::clear() {
    ref_poses_.tposes.clear();
    map_.clear();
}

std::unordered_set<rc_Timestamp> gt_generator::get_reference_edges(rc_Timestamp query_frame_us) const {
    std::unordered_set<rc_Timestamp> gt_loops;
    tpose query_G_world_body(sensor_clock::micros_to_tp(query_frame_us));
    if (ref_poses_.get_pose(query_G_world_body.t, query_G_world_body)) {
        loop_tester::frustum query_frame(query_G_world_body.G * G_body_camera0_);
        for (auto& it : map_) {
            if (it.first >= query_frame_us) break;  // do not match against the future
            if (loop_tester::is_loop(query_frame, it.second, fov_))
                gt_loops.insert(it.first);
        }
    }
    return gt_loops;
}
