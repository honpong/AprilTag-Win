#ifndef __FILTER_H
#define __FILTER_H

#include "state_vision.h"
#include "observation.h"
#include "transformation.h"
#ifdef ENABLE_QR
#include "qr.h"
#endif
#include "RCSensorFusionInternals.h"
#include "platform/sensor_clock.h"
#include "sensor_data.h"
#include "spdlog/spdlog.h"
#include "mapper.h"
#include "storage.h"
#include "state_size.h"
#include "tpose.h"
#include "future_every_n.h"

struct filter {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    filter() { }
    RCSensorFusionRunState run_state;
    size_t min_group_add;
    size_t max_group_add;
    size_t min_group_map_add;

    sensor_clock::time_point last_time;

    kalman_storage<MAXSTATESIZE, MAXOBSERVATIONSIZE, FAKESTATESIZE> store;
    observation_queue observations{store.x, store.y, store.R, store.HP, store.S};
    covariance cov{store.maxstatesize, store.P, store.Q, store.iP, store.iQ};
    state s{cov, store.FP};

    struct {
        // maxobservationsize is 3, but due to a bug in BLIS we need
        // matrices to be resizable to boundaries divisible by 4
        kalman_storage<MAX_FAST_PATH_STATESIZE, 4, FAKESTATESIZE> store;
        observation_queue observations{store.x, store.y, store.R, store.HP, store.S};
        covariance cov{store.maxstatesize, store.P, store.Q, store.iP, store.iQ};
        state_motion state{cov, store.FP};
        sensor_clock::time_point slow_path_timestamp;
        bool valid{false};
    } _mini[2], *mini = &_mini[0], *catchup = &_mini[1];

    std::unique_ptr<spdlog::logger> &log = s.log;

    sensor_clock::time_point want_start;
    bool detector_failed, tracker_failed, tracker_warned;
    bool speed_failed, speed_warning;
    bool numeric_failed;
    sensor_clock::time_point detector_failed_time;
    sensor_clock::time_point speed_warning_time;

    float max_velocity;
    float median_depth_variance;
    bool has_converged;
    bool stereo_enabled;
    bool relocalize;
    bool save_map;
    map_relocalization_info relocalization_info;

    std::unique_ptr<mapper> map;
    future_every_n<map_relocalization_info, 8> relocalization_future;

#ifdef ENABLE_QR
    qr_detector qr;
    sensor_clock::time_point last_qr_time;
    qr_benchmark qr_bench;
    bool qr_origin_gravity_aligned;
#endif
    
    transformation origin;
    bool origin_set;

    std::unique_ptr<sensor_data> recent_depth; //TODOMSM - per depth
    bool has_depth; //TODOMSM - per depth

    std::vector<std::unique_ptr<sensor_grey>> cameras;
    std::vector<std::unique_ptr<sensor_depth>> depths;
    std::vector<std::unique_ptr<sensor_accelerometer>> accelerometers;
    std::vector<std::unique_ptr<sensor_gyroscope>> gyroscopes;
    std::vector<std::unique_ptr<sensor_thermometer>> thermometers;
    std::vector<std::unique_ptr<sensor_velocimeter>> velocimeters;

    bool got_any_gyroscopes()     const { for (const auto &gyro  :     gyroscopes) if (gyro->got)  return true; return false;}
    bool got_any_accelerometers() const { for (const auto &accel : accelerometers) if (accel->got) return true; return false; }
    bool got_any_velocimeters() const { for (const auto &velo : velocimeters) if (velo->got) return true; return false; }
};

bool filter_depth_measurement(struct filter *f, const sensor_data & data);
bool filter_image_measurement(struct filter *f, const sensor_data & data);
bool filter_stereo_initialize(struct filter *f, rc_Sensor camera1_id, rc_Sensor camera2_id, const sensor_data & data);
std::unique_ptr<camera_frame_t> filter_create_camera_frame(const struct filter *f, const sensor_data& data);
size_t filter_detect(struct filter *f, const sensor_data &data, const std::unique_ptr<camera_frame_t>& camera_frame);
bool filter_compute_orb(struct filter *f, const sensor_data &data, camera_frame_t& camera_frame);
void filter_compute_dbow(struct filter *f, camera_frame_t& camera_frame);
bool filter_node_requires_frame(struct filter *f, const camera_frame_t& camera_frame);
void filter_update_map_index(struct filter *f);
map_relocalization_info filter_relocalize(struct filter *f, const camera_frame_t& camera_frame);
bool filter_accelerometer_measurement(struct filter *f, const sensor_data & data);
bool filter_gyroscope_measurement(struct filter *f, const sensor_data & data);
bool filter_velocimeter_measurement(struct filter *f, const sensor_data & data);
bool filter_mini_accelerometer_measurement(struct filter * f, observation_queue &queue, state_motion &state, const sensor_data &data);
bool filter_mini_gyroscope_measurement(struct filter * f, observation_queue &queue, state_motion &state, const sensor_data &data);
void filter_compute_gravity(struct filter *f, double latitude, double altitude);
void filter_start(struct filter *f);
void filter_start_inertial_only(struct filter *f);
void filter_update_triangulated_tracks(const filter *f, const rc_Sensor camera_id);
void filter_bring_groups_back(struct filter *f, const rc_Sensor camera_id);
#ifdef ENABLE_QR
void filter_start_qr_detection(struct filter *f, const std::string& data, float dimension, bool use_gravity);
void filter_stop_qr_detection(struct filter *f);
void filter_start_qr_benchmark(struct filter *f, float dimension);
#endif
void filter_set_origin(struct filter *f, const transformation &origin, bool gravity_aligned);

void filter_initialize(struct filter *f);
void filter_deinitialize(struct filter *f);
std::string filter_get_stats(const struct filter *f);

#endif
