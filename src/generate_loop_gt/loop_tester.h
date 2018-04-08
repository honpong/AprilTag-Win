#ifndef __LOOP_TESTER_H
#define __LOOP_TESTER_H

#include "vec4.h"
#include "transformation.h"

class loop_tester {
 public:
    loop_tester() = delete;

 public:
    struct fov {
        double fov_rad;  // field of view (radians)
        double near_z_m;  // frustum near plane (meters)
        double far_z_m;  // frustum far plane (meters)
        fov(double fov_rad = 80. * M_PI / 180.,
            double near_z_m = 1., double far_z_m = 5.) :
            fov_rad(fov_rad), near_z_m(near_z_m), far_z_m(far_z_m) {}
    };

    struct frustum {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        v3 center;
        v3 optical_axis;
        frustum() {}
        frustum(const transformation& G_world_camera) :
            center(G_world_camera.T),
            optical_axis(G_world_camera.Q * v3{0, 0, 1}) {}
    };

 public:
    static bool is_loop(const frustum& G_world_camera_A,
                        const frustum& G_world_camera_B,
                        const fov& camera);

 private:
    struct segment {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        segment(const v3& p1, const v3& p2) : p{p1}, v{p2-p1} {}
        v3 p, v;
    };

    enum covisibility {
        is_covisible, maybe_covisible, no_covisible
    };

   static covisibility covisible_by_proximity(const frustum& G_world_camera_A,
                                              const frustum& G_world_camera_B,
                                              const fov& camera);

   static bool covisible_by_frustum_overlap(const frustum& G_world_camera_A,
                                            const frustum& G_world_camera_B,
                                            const fov& camera);

};

#endif  // __LOOP_TESTER_H
