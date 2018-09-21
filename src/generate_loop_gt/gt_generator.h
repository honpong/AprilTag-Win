#ifndef GT_GENERATOR_H
#define GT_GENERATOR_H

#include <functional>
#include <tuple>
#include <set>
#include <unordered_set>
#include "loop_tester.h"
#include "rc_tracker.h"
#include "rc_internal.h"
#include "tpose.h"

/** Generates loop groundtruth online.
 */
class gt_generator {
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    void set_camera(const rc_Extrinsics& extrinsics, const loop_tester::fov& fov = {});
    void add_reference_poses(const tpose_sequence& poses_G_world_body, rc_SessionId sid);
    void update_map(rc_MapNode* map_nodes, int num_map_nodes);
    void clear();
    std::set<rc_SessionTimestamp> get_reference_edges(rc_Timestamp query_frame_us) const;
    const std::vector<tpose_sequence>& get_reference_poses() const { return ref_poses_; }

 private:
    transformation G_body_camera0_;
    loop_tester::fov fov_;
    std::vector<tpose_sequence> ref_poses_;  // G_world_body indexed by session id
    std::vector<std::map<rc_Timestamp, loop_tester::frustum>> map_;
};

#endif // GT_GENERATOR_H
