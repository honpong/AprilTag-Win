#include <cmath>
#include <iomanip>
#include <fstream>
#include <type_traits>
#include <utility>
#include "gt_generator.h"
#include "image_reader.h"
#include "symmetric_matrix.h"
#include "calibration.h"
#include "rc_compat.h"
#include "DBoW2/endianness.h"

#ifdef HAVE_TBB
#include <tbb/parallel_for_each.h>
#endif

gt_generator::gt_generator() {}

gt_generator::gt_generator(const std::string& capture_file,
                           struct camera camera, bool verbose) {
    generate(capture_file, camera, verbose);
}

gt_generator::gt_generator(const std::string& capture_file, loop_gt& gt,
                           struct camera camera, bool verbose) {
    generate(capture_file, gt, camera, verbose);
}

bool gt_generator::generate(const std::string& capture_file,
                            struct camera camera, bool verbose) {
    camera_ = camera;
    verbose_ = verbose;
    if (!load_data(capture_file)) return false;
    run();
    return true;
}

bool gt_generator::generate(const std::string& capture_file, loop_gt& gt,
                            struct camera camera, bool verbose) {
    camera_ = camera;
    verbose_ = verbose;
    if (!load_data(capture_file)) return false;
    run();
    gt = get_loop_gt();
    return true;
}

bool gt_generator::load_data(const std::string &capture_file) {
    if (!load_calibration(capture_file)) {
        if (verbose_) std::cout << "No calibration file found for " << capture_file << std::endl;
        return false;
    }
    if (!load_reference_gt(capture_file)) {
        if (verbose_) std::cout << "No reference groundtruth file found for " << capture_file << std::endl;
        return false;
    }
    if (!load_image_timestamps(capture_file)) {
        if (verbose_) std::cout << "Error reading the capture file " << capture_file << std::endl;
        return false;
    }
    return true;
}

bool gt_generator::load_calibration(const std::string& capture_file) {
    calibration cal;
    auto load = [&cal](const std::string& filename) {
        std::ifstream file_handle(filename);
        if (file_handle.fail())
            return false;

        std::string json((std::istreambuf_iterator<char>(file_handle)),
                         std::istreambuf_iterator<char>());
        return calibration_deserialize(json, cal);
    };

    bool success = load(capture_file + ".json");
    if (!success) {
        auto found = capture_file.find_last_of("/\\");
        std::string path = capture_file.substr(0, found+1);
        success = load(path + "calibration.json");
    }
    if (success) {
        if (!cal.cameras.empty()) {
            G_body_camera0_ = rc_Extrinsics_to_sensor_extrinsics(cal.cameras[0].extrinsics).mean;
        } else {
            success = false;
        }
    }
    return success;
}

bool gt_generator::load_reference_gt(const std::string& capture_file) {
    auto load = [](const std::string& filename) {
        tpose_sequence tum_poses;
        return (tum_poses.load_from_file(filename) ? tum_poses : tpose_sequence());
    };
    tum_gt_poses_ = load(capture_file + ".tum");
    if (!tum_gt_poses_.size()) tum_gt_poses_ = load(capture_file + ".vicon");
    if (!tum_gt_poses_.size()) tum_gt_poses_ = load(capture_file + ".pose");
    return tum_gt_poses_.size() != 0;
}

bool gt_generator::load_image_timestamps(const std::string& capture_file) {
    image_reader reader;
    if (reader.open(capture_file) && reader.create_index()) {
        const auto& image_index = reader.get_raw_index();
        rc_image_timestamps_.reserve(image_index.size());
        for (auto& it : image_index) rc_image_timestamps_.emplace_back(it.first);
        std::sort(rc_image_timestamps_.begin(), rc_image_timestamps_.end());
        return true;
    }
    return false;
}

void gt_generator::run() {
    interpolated_poses_ = interpolate_poses();
    if (interpolated_poses_.empty()) {
        associations_.clear();
        return;
    }

    associations_.create(interpolated_poses_.size(), false);

    aligned_vector<transformation> G_world_cameras = [this](){
        aligned_vector<transformation> G_world_cameras;
        G_world_cameras.reserve(interpolated_poses_.size());
        for (const auto& pose : interpolated_poses_) {
            G_world_cameras.emplace_back(pose.G * G_body_camera0_);
        }
        return G_world_cameras;
    }();

    aligned_vector<frustum> frustums = [this]() {
        aligned_vector<frustum> frustums;
        frustums.reserve(interpolated_poses_.size());
        for (const auto& pose : interpolated_poses_) {
            transformation G_world_camera = pose.G * G_body_camera0_;
            frustums.emplace_back(G_world_camera);
        }
        return frustums;
    }();

    auto check_loop = [this, &frustums, &G_world_cameras]
            (const std::pair<size_t, size_t>& pair) {
        size_t cur_idx = pair.first;
        size_t prev_idx = pair.second;
        const frustum& cur_frustum = frustums[cur_idx];
        const frustum& prev_frustum = frustums[prev_idx];
        const transformation& cur_G_world_camera = G_world_cameras[cur_idx];
        const transformation& prev_G_world_camera = G_world_cameras[prev_idx];

        bool is_loop;
        covisibility covisible = covisible_by_proximity(cur_G_world_camera,
                                                        prev_G_world_camera);
        switch (covisible) {
        case is_covisible:
            is_loop = true;
            break;
        case no_covisible:
            is_loop = false;
            break;
        case maybe_covisible:
            is_loop = covisible_by_frustum_overlap(cur_frustum, prev_frustum);
            break;
        }
        if (is_loop) {
            associations_.set(prev_idx, cur_idx, true);
        }
    };

    std::vector<std::pair<size_t, size_t>> pairs;
    pairs.reserve(interpolated_poses_.size() * (interpolated_poses_.size() - 1) / 2);
    for (size_t cur_idx = 1; cur_idx < interpolated_poses_.size(); ++cur_idx) {
        for (size_t prev_idx = 0; prev_idx < cur_idx; ++prev_idx) {
            pairs.emplace_back(cur_idx, prev_idx);
        }
    }
#ifdef HAVE_TBB
    tbb::parallel_for_each(pairs.begin(), pairs.end(), check_loop);
#else
    std::for_each(pairs.begin(), pairs.end(), check_loop);
#endif

    get_connected_components();
}

std::vector<tpose> gt_generator::interpolate_poses() const {
    std::vector<tpose> poses;
    poses.reserve(rc_image_timestamps_.size());
    for (const auto& image_timestamp : rc_image_timestamps_) {
        tpose gt_pose(sensor_clock::micros_to_tp(image_timestamp));
        if (!poses.empty() && poses.back().t == gt_pose.t) {
            // avoid duplicated stereo timestamps
            continue;
        }
        if (tum_gt_poses_.get_pose(gt_pose.t, gt_pose)) {
            poses.emplace_back(std::move(gt_pose));
        } else if (verbose_) {
            std::cout << "Could not find a gt reference pose for timestamp "
                      << image_timestamp << std::endl;
        }
    }
    return poses;
}

gt_generator::covisibility gt_generator::covisible_by_proximity(
        const transformation &G_world_camera_A,
        const transformation &G_world_camera_B) const {
    constexpr double in_distance = 0.5;  // m
    constexpr double in_cos_angle = 0.866025403784439;  // cos(30 deg)
    constexpr double out_cos_angle = 0;  // cos(90 deg)
    const double out_distance = camera_.far_z_m * 2;  // m

    double distance = (G_world_camera_A.T - G_world_camera_B.T).norm();
    if (distance > out_distance) {
        return no_covisible;
    } else {
        v3 optical_axis_A = G_world_camera_A * (v3() << 0, 0, 1).finished();
        v3 optical_axis_B = G_world_camera_B * (v3() << 0, 0, 1).finished();
        double cos_angle = (optical_axis_A.dot(optical_axis_B) /
                            optical_axis_A.norm() / optical_axis_B.norm());

        if (distance <= in_distance && cos_angle >= in_cos_angle) {
            return is_covisible;
        } else if (cos_angle < out_cos_angle) {
            return no_covisible;
        } else {
            return maybe_covisible;
        }
    }
}

bool gt_generator::covisible_by_frustum_overlap(const frustum& lhs,
                                                const frustum& rhs) const {
    static auto point_to_segment_projection = [](const v3& p, const segment& s) {
        f_t t = (p-s.p).dot(s.v)/s.v.squaredNorm();
        if(t > 0.f && t < 1.f) {
            v3 p_projection = s.p + t*s.v;
            return std::pair<f_t, v3>{(p-p_projection).norm(), std::move(p_projection)};
        }
        return std::pair<f_t, v3>{std::numeric_limits<float>::max(), v3{}};
    };

    const double tan_half_fov = std::tan(camera_.fov_rad / 2);
    // plane is defined by frustum optical axis and point p
    auto plane_frustum_intersection = [this, tan_half_fov](const v3& p, const frustum& f) {
        f_t l = camera_.near_z_m * tan_half_fov;
        f_t L = camera_.far_z_m * tan_half_fov;
        f_t h = camera_.far_z_m - camera_.near_z_m;
        const v3& c = f.center + camera_.near_z_m * f.optical_axis;
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

    const double min_cos_angle = std::cos(camera_.fov_rad / 2);
    auto point_inside_frustum = [this, min_cos_angle](const v3& p, const frustum& f) {
        v3 point_axis = p - f.center;
        double distance = point_axis.norm();
        double cos_point_angle = point_axis.dot(f.optical_axis) / distance;
        if (cos_point_angle >= min_cos_angle) {
            double min_distance = camera_.near_z_m / cos_point_angle;
            double max_distance = camera_.far_z_m / cos_point_angle;
            return (min_distance <= distance && distance <= max_distance);
        }
        return false;
    };

    auto numerical_frustums_intersection = [this, &point_to_frustum_projection, &point_inside_frustum](
            const frustum& lhs, const frustum& rhs) {
      f_t distance = std::numeric_limits<f_t>::max();
      std::vector<const frustum*> frustums = {&lhs, &rhs};

      v3 p = rhs.center + camera_.near_z_m * rhs.optical_axis;
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

void gt_generator::get_connected_components() {
    std::unordered_map<size_t, size_t> lookup;
    labels_.create(associations_.rows(), no_label);

    struct {
        size_t operator()() { return next_label_++; }
     private:
        size_t next_label_ = 0;
    } new_label;

    // (two pass method for 4-connectivity)
    {
        const int row = 0;
        int col = 0;
        size_t label_left = new_label();
        labels_.set(row, col, label_left);  // associations(i, i) is always a loop
        ++col;
        for (auto it = std::next(associations_.begin(row)); it != associations_.end(row);
             ++it, ++col) {
            if (*it) {
                if (label_left == no_label) {
                    label_left = new_label();
                }
                labels_.set(row, col, label_left);
            } else {
                label_left = no_label;
            }
        }
    }
    for (int row = 1; row < labels_.rows(); ++row) {
        int col = row;
        size_t label_left = labels_.get(row - 1, col);
        if (label_left == no_label) label_left = new_label();
        labels_.set(row, col, label_left);
        ++col;
        for (auto it = std::next(associations_.begin(row)); it != associations_.end(row);
             ++it, ++col) {
            if (*it) {
                size_t label_above = labels_.get(row - 1, col);
                if (label_left == no_label && label_above == no_label) {
                    label_left = new_label();
                } else if (label_left == no_label && label_above != no_label) {
                    label_left = label_above;
                } else if (label_left != no_label && label_above != no_label &&
                           label_left != label_above) {
                    auto pair = std::minmax(label_left, label_above); // 1 <= 2
                    lookup[pair.second] = pair.first;
                    label_left = pair.first;
                }
                labels_.set(row, col, label_left);
            } else {
                label_left = no_label;
            }
        }
    }

    // reduce look up table
    for (auto it = lookup.begin(); it != lookup.end(); ++it) {
        std::vector<decltype(it)> edges(1, it);
        for (auto edge_it = lookup.find(edges.back()->second);
             edge_it != lookup.end();
             edge_it = lookup.find(edges.back()->second)) {
            edges.emplace_back(edge_it);
        }
        for (auto& it : edges) {
            it->second = edges.back()->second;
        }
    }

    // convert labels into continuous [0..n] range
    std::unordered_map<size_t, size_t> final_lookup;
    {
        std::vector<size_t> labels;
        const size_t end_label = new_label();
        for (size_t label = 0; label < end_label; ++label) {
            auto it = lookup.find(label);
            if (it == lookup.end()) {
                labels.emplace_back(label);
            }
        }
        for (size_t idx = 0; idx < labels.size(); ++idx) {
            if (idx != labels[idx]) {
                final_lookup[labels[idx]] = idx;
            }
        }
        for (auto& edge : lookup) {
            auto it = final_lookup.find(edge.second);
            if (it == final_lookup.end())
                final_lookup.emplace(edge);
            else
                final_lookup.emplace(edge.first, it->second);
        }
    }

    // second pass
    for (int row = 0; row < labels_.rows(); ++row) {
        for (int col = row; col < labels_.cols(); ++col) {
            size_t label = labels_.get(row, col);
            if (label != no_label) {
                auto it = final_lookup.find(label);
                if (it != final_lookup.end()) {
                    labels_.set(row, col, it->second);
                }
            }
        }
    }
}

gt_generator::loop_gt gt_generator::get_loop_gt() const {
    loop_gt gt;
    for (int col = 1; col < associations_.cols(); ++col) {
        const auto& cur = interpolated_poses_[col];
        std::unordered_set<rc_Timestamp>& matches = gt[sensor_clock::tp_to_micros(cur.t)];
        for (int row = 0; row < col; ++row) {
            if (associations_.get(row, col)) {
                const auto& prev = interpolated_poses_[row];
                // cur.t > prev.t
                matches.emplace(sensor_clock::tp_to_micros(prev.t));
            }
        }
    }
    return gt;
}

bool gt_generator::save_loop_file(const std::string& capture_file, bool binary_format) const {
    using save_function = std::function<void(std::ofstream&, rc_Timestamp, rc_Timestamp, size_t)>;
    save_function save_ascii = [](std::ofstream& file, rc_Timestamp cur_t, rc_Timestamp prev_t, size_t label) {
        // format: newer_timestamp older_timestamp group_id
        file << cur_t << " " << prev_t << " " << label << "\n";
    };
    save_function save_binary = [](std::ofstream& file, rc_Timestamp cur_t, rc_Timestamp prev_t, size_t label) {
        // format (little endian): 64bits 64bits 32bits
        uint64_t ts[2] = { htole64(static_cast<uint64_t>(cur_t)),
                           htole64(static_cast<uint64_t>(prev_t)) };
        uint32_t label32 = htole32(static_cast<uint32_t>(label));
        file.write(reinterpret_cast<char *>(ts), sizeof(uint64_t) * 2);
        file.write(reinterpret_cast<char *>(&label32), sizeof(uint32_t));
    };
    auto save_item = (binary_format ? save_binary : save_ascii);

    auto mode = (binary_format ? std::ios::out | std::ios::binary : std::ios::out);
    std::ofstream file(capture_file + ".loop", mode);
    if (!file) {
        std::cerr << "error: unable to write to " << (capture_file + ".loop") << std::endl;
        return false;
    }
    for (int col = 1; col < labels_.cols(); ++col) {
        const auto& cur = interpolated_poses_[col];
        for (int row = 0; row < col; ++row) {
            size_t label = labels_.get(row, col);
            if (label != no_label) {
                const auto& prev = interpolated_poses_[row];
                // cur.t > prev.t
                save_item(file, sensor_clock::tp_to_micros(cur.t),
                          sensor_clock::tp_to_micros(prev.t), label);
            }
        }
    }
    return true;
}

bool gt_generator::save_mat_file(const std::string& capture_file) const {
    std::ofstream file(capture_file + ".mat");
    if (!file) {
        std::cerr << "error: unable to write to " << (capture_file + ".mat") << std::endl;
        return false;
    }
    for (int row = 0; row < labels_.rows(); ++row) {
        for (int col = 0; col < labels_.cols(); ++col) {
            auto label = labels_.get(row, col);
            label = (label == no_label ? 0 : label + 1);
            file << label << " ";
        }
        file << "\n";
    }
    return true;
}
