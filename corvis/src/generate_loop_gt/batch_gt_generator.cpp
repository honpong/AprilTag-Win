#include <cmath>
#include <iomanip>
#include <fstream>
#include <type_traits>
#include <utility>
#include "batch_gt_generator.h"
#include "loop_tester.h"
#include "image_reader.h"
#include "symmetric_matrix.h"
#include "calibration.h"
#include "rc_compat.h"
#include "DBoW2/endianness.h"

#ifdef HAVE_TBB
#include <tbb/parallel_for_each.h>
#endif

batch_gt_generator::batch_gt_generator() {}

batch_gt_generator::batch_gt_generator(const std::string& capture_file,
                                       camera cam, bool verbose) {
    generate(capture_file, cam, verbose);
}

batch_gt_generator::batch_gt_generator(const std::string& capture_file, loop_gt& gt,
                                       camera cam, bool verbose) {
    generate(capture_file, gt, cam, verbose);
}

bool batch_gt_generator::generate(const std::string& capture_file,
                                  camera cam, bool verbose) {
    camera_ = cam;
    verbose_ = verbose;
    if (!load_data(capture_file)) return false;
    run();
    return true;
}

bool batch_gt_generator::generate(const std::string& capture_file, loop_gt& gt,
                                  camera cam, bool verbose) {
    camera_ = cam;
    verbose_ = verbose;
    if (!load_data(capture_file)) return false;
    run();
    gt = get_loop_gt();
    return true;
}

bool batch_gt_generator::load_data(const std::string &capture_file) {
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

bool batch_gt_generator::load_calibration(const std::string& capture_file) {
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

bool batch_gt_generator::load_reference_gt(const std::string& capture_file) {
    auto load = [](const std::string& filename) {
        tpose_sequence tum_poses;
        return (tum_poses.load_from_file(filename) ? tum_poses : tpose_sequence());
    };
    tum_gt_poses_ = load(capture_file + ".tum");
    if (!tum_gt_poses_.size()) tum_gt_poses_ = load(capture_file + ".vicon");
    if (!tum_gt_poses_.size()) tum_gt_poses_ = load(capture_file + ".pose");
    return tum_gt_poses_.size() != 0;
}

bool batch_gt_generator::load_image_timestamps(const std::string& capture_file) {
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

void batch_gt_generator::run() {
    interpolated_poses_ = interpolate_poses();
    if (interpolated_poses_.empty()) {
        associations_.clear();
        return;
    }

    associations_.create(interpolated_poses_.size(), false);

    aligned_vector<loop_tester::frustum> frustums = [this]() {
        aligned_vector<loop_tester::frustum> frustums;
        frustums.reserve(interpolated_poses_.size());
        for (const auto& pose : interpolated_poses_) {
            transformation G_world_camera = pose.G * G_body_camera0_;
            frustums.emplace_back(G_world_camera);
        }
        return frustums;
    }();

    auto check_loop = [this, &frustums](const std::pair<size_t, size_t>& pair) {
        size_t cur_idx = pair.first;
        size_t prev_idx = pair.second;
        if (loop_tester::is_loop(frustums[cur_idx], frustums[prev_idx], camera_)) {
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

std::vector<tpose> batch_gt_generator::interpolate_poses() const {
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

void batch_gt_generator::get_connected_components() {
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

batch_gt_generator::loop_gt batch_gt_generator::get_loop_gt() const {
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

bool batch_gt_generator::save_loop_file(const std::string& capture_file, bool binary_format) const {
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

bool batch_gt_generator::save_mat_file(const std::string& capture_file) const {
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
