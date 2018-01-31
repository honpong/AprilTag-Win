#ifndef GT_GENERATOR_H
#define GT_GENERATOR_H

#include <functional>
#include <tuple>
#include <unordered_set>
#include "loop_tester.h"
#include "rc_tracker.h"
#include "tpose.h"

/** Generates loop groundtruth online.
 */
class gt_generator {
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    void set_camera(const rc_Extrinsics& extrinsics, const loop_tester::fov& fov = {});
    void add_reference_poses(const tpose_sequence& poses_G_world_body);
    void update_map(rc_MapNode* map_nodes, int num_map_nodes);
    void clear();
    std::unordered_set<rc_Timestamp> get_reference_edges(rc_Timestamp query_frame_us) const;
    const tpose_sequence& get_reference_poses() const { return ref_poses_; }

 private:
    transformation G_body_camera0_;
    loop_tester::fov fov_;
    tpose_sequence ref_poses_;  // G_world_body
    std::map<rc_Timestamp, loop_tester::frustum> map_;
};

#endif // GT_GENERATOR_H
