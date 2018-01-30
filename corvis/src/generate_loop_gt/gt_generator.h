#ifndef __GT_GENERATOR_H
#define __GT_GENERATOR_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "symmetric_matrix.h"
#include "rc_tracker.h"
#include "vec4.h"
#include "transformation.h"
#include "tpose.h"

/** Generates loop groundtruth for a single sequence.
 */
class gt_generator {
 public:
    struct camera {
        double fov_rad;  // field of view (radians)
        double near_z_m;  // frustum near plane (meters)
        double far_z_m;  // frustum far plane (meters)
        camera(double fov_rad = 80. * M_PI / 180.,
               double near_z_m = 1., double far_z_m = 5.) :
            fov_rad(fov_rad), near_z_m(near_z_m), far_z_m(far_z_m) {}
    };

    // newer timestamp --> older timestamp matches
    using loop_gt = std::unordered_map<rc_Timestamp, std::unordered_set<rc_Timestamp>>;

 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    gt_generator();
    gt_generator(const std::string& capture_file,
                 struct camera camera = {}, bool verbose = false);
    gt_generator(const std::string& capture_file, loop_gt& gt,
                 struct camera camera = {}, bool verbose = false);

    bool generate(const std::string& capture_file,
                  struct camera camera = {}, bool verbose = false);
    bool generate(const std::string& capture_file, loop_gt& gt,
                  struct camera camera = {}, bool verbose = false);

    loop_gt get_loop_gt() const;

    bool save_loop_file(const std::string& capture_file, bool binary_format = true) const;

    bool save_mat_file(const std::string& capture_file) const;

 private:
    struct frustum {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        v3 center;
        v3 optical_axis;
        frustum() {}
        frustum(const transformation& G_world_camera) :
            center(G_world_camera.T),
            optical_axis((G_world_camera.Q * (v3() << 0, 0, 1).finished()).normalized()) {}
    };

    struct segment {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        segment(const v3& p1, const v3& p2) : p{p1}, v{p2-p1} {}
        v3 p, v;
    };

    enum covisibility {
        is_covisible, maybe_covisible, no_covisible
    };

    bool load_data(const std::string &capture_file);
    bool load_calibration(const std::string& capture_file);
    bool load_reference_gt(const std::string& capture_file);
    bool load_image_timestamps(const std::string& capture_file);

    void run();
    std::vector<tpose> interpolate_poses() const;
    covisibility covisible_by_proximity(const transformation &G_world_camera_A,
                                        const transformation &G_world_camera_B) const;
    bool covisible_by_frustum_overlap(const frustum& lhs, const frustum& rhs) const;

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
