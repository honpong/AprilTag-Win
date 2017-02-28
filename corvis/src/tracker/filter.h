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
#include "rc_compat.h"
#include "mapper.h"

#include <mutex>

struct filter {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    filter() { }
    RCSensorFusionRunState run_state;
    int min_group_add;
    int max_group_add;
    
    sensor_clock::time_point last_time;

    observation_queue observations;
    covariance cov;
    state s{cov};

    struct {
        observation_queue observations;
        covariance cov;
        state_motion state {cov};
    } _mini[2], *mini = &_mini[0], *catchup = &_mini[1];
    std::recursive_mutex mini_mutex;

    std::unique_ptr<spdlog::logger> &log = s.log;

    sensor_clock::time_point want_start;
    bool detector_failed, tracker_failed, tracker_warned;
    bool speed_failed, speed_warning;
    bool numeric_failed;
    sensor_clock::time_point speed_warning_time;
    sensor_clock::time_point stable_start;
    bool calibration_bad;
    
    float max_velocity;
    float median_depth_variance;
    bool has_converged;

    std::unique_ptr<mapper> map;

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

    bool got_any_gyroscopes()     const { for (const auto &gyro  :     gyroscopes) if (gyro->got)  return true; return false;}
    bool got_any_accelerometers() const { for (const auto &accel : accelerometers) if (accel->got) return true; return false; }
};

bool filter_depth_measurement(struct filter *f, const sensor_data & data);
bool filter_image_measurement(struct filter *f, const sensor_data & data);
const vector<tracker::point> &filter_detect(struct filter *f, const sensor_data &data);
bool filter_accelerometer_measurement(struct filter *f, const sensor_data & data);
bool filter_gyroscope_measurement(struct filter *f, const sensor_data & data);
bool filter_mini_accelerometer_measurement(struct filter * f, observation_queue &queue, state_motion &state, const sensor_data &data);
bool filter_mini_gyroscope_measurement(struct filter * f, observation_queue &queue, state_motion &state, const sensor_data &data);
void filter_compute_gravity(struct filter *f, double latitude, double altitude);
void filter_start_static_calibration(struct filter *f);
void filter_start_hold_steady(struct filter *f);
void filter_start_dynamic(struct filter *f);
void filter_start_inertial_only(struct filter *f);
#ifdef ENABLE_QR
void filter_start_qr_detection(struct filter *f, const std::string& data, float dimension, bool use_gravity);
void filter_stop_qr_detection(struct filter *f);
void filter_start_qr_benchmark(struct filter *f, float dimension);
#endif
void filter_set_origin(struct filter *f, const transformation &origin, bool gravity_aligned);

void filter_initialize(struct filter *f);
void filter_deinitialize(struct filter *f);
float filter_converged(const struct filter *f);
bool filter_is_steady(const struct filter *f);
std::string filter_get_stats(const struct filter *f);

#endif