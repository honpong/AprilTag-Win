#include "loop_tester.h"

bool loop_tester::is_loop(const frustum& G_world_camera_A,
                          const frustum& G_world_camera_B,
                          const fov& camera) {
    covisibility covisible = covisible_by_proximity(G_world_camera_A,
                                                    G_world_camera_B,
                                                    camera);
    switch (covisible) {
    case is_covisible:
        return true;
    case no_covisible:
        return false;
    case maybe_covisible:
        return covisible_by_frustum_overlap(G_world_camera_A, G_world_camera_B,
                                            camera);
    }
}

loop_tester::covisibility loop_tester::covisible_by_proximity(
        const frustum &G_world_camera_A,
        const frustum &G_world_camera_B,
        const fov& camera) {
    constexpr double in_distance = 0.5;  // m
    constexpr double in_cos_angle = 0.866025403784439;  // cos(30 deg)
    constexpr double out_cos_angle = 0;  // cos(90 deg)
    const double out_distance = camera.far_z_m * 2;  // m

    double distance = (G_world_camera_A.center - G_world_camera_B.center).norm();
    if (distance > out_distance) {
        return no_covisible;
    } else {
        double cos_angle = (G_world_camera_A.optical_axis.dot(G_world_camera_B.optical_axis));

        if (distance <= in_distance && cos_angle >= in_cos_angle) {
            return is_covisible;
        } else if (cos_angle < out_cos_angle) {
            return no_covisible;
        } else {
            return maybe_covisible;
        }
    }
}

bool loop_tester::covisible_by_frustum_overlap(const frustum& lhs,
                                               const frustum& rhs,
                                               const fov& camera) {
    static auto point_to_segment_projection = [](const v3& p, const segment& s) {
        f_t t = (p-s.p).dot(s.v)/s.v.squaredNorm();
        if(t > 0.f && t < 1.f) {
            v3 p_projection = s.p + t*s.v;
            return std::pair<f_t, v3>{(p-p_projection).norm(), std::move(p_projection)};
        }
        return std::pair<f_t, v3>{std::numeric_limits<float>::max(), v3{}};
    };

    const double tan_half_fov = std::tan(camera.fov_rad / 2);
    // plane is defined by frustum optical axis and point p
    auto plane_frustum_intersection = [tan_half_fov, &camera](const v3& p, const frustum& f) {
        f_t l = camera.near_z_m * tan_half_fov;
        f_t L = camera.far_z_m * tan_half_fov;
        f_t h = camera.far_z_m - camera.near_z_m;
        const v3& c = f.center + camera.near_z_m * f.optical_axis;
        const v3& y = f.optical_axis;
        v3 x = ((p-c) - ((p-c).dot(y)) * y).normalized();
        if(x.sum() == 0) // if point p on optical axis select yz plane
            x = v3{1.f, 0.f, 0.f};
        // plane intersects frustum (truncated cone) in 4 corners
        std::vector<v3> intersection = {c-l*x, c+l*x, c+h*y+L*x, c+h*y-L*x};
        return intersection;
    };

    auto point_to_frustum_projection = [&plane_frustum_intersection](const v3& p, const frustum& f) {
        f_t min_distance = std::numeric_limits<float>::max();
        v3 p_projected;

        // distance to plane-cone intersection corners
        std::vector<v3> points = plane_frustum_intersection(p, f);
        for(int i=0; i<4; ++i) {
            f_t distance = (p-points[i]).norm();
            if(distance < min_distance) {
                min_distance = distance;
                p_projected = points[i];
            }
        }

        // distance to plane-cone intersection segments
        for(int i=0; i<4; ++i) {
            segment s(points[i], points[(i+1)%4]);
            std::pair<f_t, v3> distance_point = point_to_segment_projection(p, s);
            if(distance_point.first < min_distance) {
                min_distance = distance_point.first;
                p_projected = distance_point.second;
            }
        }

        return p_projected;
    };

    const double min_cos_angle = std::cos(camera.fov_rad / 2);
    auto point_inside_frustum = [min_cos_angle, &camera](const v3& p, const frustum& f) {
        v3 point_axis = p - f.center;
        double distance = point_axis.norm();
        double cos_point_angle = point_axis.dot(f.optical_axis) / distance;
        if (cos_point_angle >= min_cos_angle) {
            double min_distance = camera.near_z_m / cos_point_angle;
            double max_distance = camera.far_z_m / cos_point_angle;
            return (min_distance <= distance && distance <= max_distance);
        }
        return false;
    };

    auto numerical_frustums_intersection = [&point_to_frustum_projection, &point_inside_frustum, &camera](
            const frustum& lhs, const frustum& rhs) {
      f_t distance = std::numeric_limits<f_t>::max();
      std::vector<const frustum*> frustums = {&lhs, &rhs};

      v3 p = rhs.center + camera.near_z_m * rhs.optical_axis;
      bool point_covisible = point_inside_frustum(p, lhs);
      bool stop = point_covisible;
      int iteration = 0;
      while (!stop && iteration < 10) {
          v3 p_new = point_to_frustum_projection(p, *frustums[iteration%2]);
          point_covisible = point_inside_frustum(p, *frustums[1-(iteration%2)]);
          f_t distance_new = (p_new-p).norm();
          if (point_covisible || std::abs(distance-distance_new) < 1e-3 || distance_new < 1e-3) // in theory distance >= distance_new always
              stop = true;

          distance = distance_new;
          p = p_new;
          iteration++;
      }

      if (point_covisible || distance < 1e-3)
          return true;

      return false;
    };

    return numerical_frustums_intersection(lhs, rhs);
}
