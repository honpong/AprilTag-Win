#ifndef __GT_GENERATOR_H
#define __GT_GENERATOR_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "loop_tester.h"
#include "symmetric_matrix.h"
#include "rc_tracker.h"
#include "vec4.h"
#include "transformation.h"
#include "tpose.h"

/** Generates loop groundtruth for a single sequence.
 */
class batch_gt_generator {
 public:
    using camera = loop_tester::fov;
    // newer timestamp --> older timestamp matches
    using loop_gt = std::unordered_map<rc_Timestamp, std::unordered_set<rc_Timestamp>>;

 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    batch_gt_generator();
    batch_gt_generator(const std::string& capture_file,
                       camera cam = {}, bool verbose = false);
    batch_gt_generator(const std::string& capture_file, loop_gt& gt,
                       camera cam = {}, bool verbose = false);

    bool generate(const std::string& capture_file,
                  camera cam = {}, bool verbose = false);
    bool generate(const std::string& capture_file, loop_gt& gt,
                  camera cam = {}, bool verbose = false);

    loop_gt get_loop_gt() const;

    bool save_loop_file(const std::string& capture_file, bool binary_format = true) const;

    bool save_mat_file(const std::string& capture_file) const;

 private:
    bool load_data(const std::string &capture_file);
    bool load_calibration(const std::string& capture_file);
    bool load_reference_gt(const std::string& capture_file);
    bool load_image_timestamps(const std::string& capture_file);

    void run();
    std::vector<tpose> interpolate_poses() const;
    void get_connected_components();

    bool verbose_;
    camera camera_;
    transformation G_body_camera0_;
    tpose_sequence tum_gt_poses_;  // all 6dog groundtruth
    std::vector<rc_Timestamp> rc_image_timestamps_;  // timestamps from rc file (N)
    std::vector<tpose> interpolated_poses_;  // poses at rc timestamps (M <= N)
    SymmetricMatrix<char> associations_;  // MxM items  (chars used as bools)
    SymmetricMatrix<size_t> labels_;  // MxM items
    static constexpr size_t no_label = std::numeric_limits<size_t>::max();
};

#endif  // __GT_GENERATOR_H
