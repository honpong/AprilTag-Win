#ifndef __FILTER_H
#define __FILTER_H

#include "state_vision.h"
#include "observation.h"
#include "../numerics/transformation.h"
#ifdef ENABLE_QR
#include "qr.h"
#endif
#include "RCSensorFusionInternals.h"
#include "../cor/platform/sensor_clock.h"
#include "../cor/sensor_data.h"
#include "spdlog/spdlog.h"
#include "rc_compat.h"

struct filter {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    filter(): s(cov) {}
    RCSensorFusionRunState run_state;
    int min_group_add;
    int max_group_add;
    
    sensor_clock::time_point last_time;
    sensor_clock::time_point last_packet_time;
    int last_packet_type;
    state s;
    
    covariance cov;
    std::unique_ptr<spdlog::logger> &log = s.log;

    //TODOMSM
    f_t w_variance;
    f_t a_variance;

    sensor_clock::time_point want_start;
    v3 last_gyro_meas, last_accel_meas; //TODOMSM - per-sensor
    bool detector_failed, tracker_failed, tracker_warned;
    bool speed_failed, speed_warning;
    bool numeric_failed;
    sensor_clock::time_point speed_warning_time;
    bool ignore_lateness = true;
    stdev<3> gyro_stability, accel_stability; //TODOMSM - either just first sensor or per-sensor
    sensor_clock::time_point stable_start;
    bool calibration_bad;
    
    float max_velocity;
    float median_depth_variance;
    bool has_converged;
    state_vision_group *detecting_group;

    sensor_clock::duration mindelta;
    bool valid_delta;
    sensor_clock::time_point last_arrival;
    
    sensor_clock::time_point active_time;
    
#ifdef ENABLE_QR
    qr_detector qr;
    sensor_clock::time_point last_qr_time;
    qr_benchmark qr_bench;
    bool qr_origin_gravity_aligned;
#endif
    
    transformation origin;
    bool origin_set;

    //TODOMSM - per sensor
    v3 a_bias_start, w_bias_start; //for tracking calibration progress
    
    observation_queue observations;
    
    std::unique_ptr<sensor_data> recent_depth; //TODOMSM - per depth
    bool has_depth; //TODOMSM - per depth

    std::vector<std::unique_ptr<sensor_grey>> cameras;
    std::vector<std::unique_ptr<sensor_depth>> depths;
    std::vector<std::unique_ptr<sensor_accelerometer>> accelerometers;
    std::vector<std::unique_ptr<sensor_gyroscope>> gyroscopes;

    bool got_any_gyroscopes()     const { for (const auto &gyro  :     gyroscopes) if (gyro->got)  return true; return false;}
    bool got_any_accelerometers() const { for (const auto &accel : accelerometers) if (accel->got) return true; return false; }

    //TODOMSM - per sensor
    std::chrono::duration<float, milli> accel_timer, gyro_timer, track_timer, detect_timer;
};

bool filter_depth_measurement(struct filter *f, const sensor_data & data);
bool filter_image_measurement(struct filter *f, const sensor_data & data);
void filter_detect_features(struct filter *f, state_vision_group *g, sensor_data &&image);
bool filter_accelerometer_measurement(struct filter *f, const sensor_data & data);
bool filter_gyroscope_measurement(struct filter *f, const sensor_data & data);
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

extern "C" void filter_initialize(struct filter *f);
float filter_converged(const struct filter *f);
bool filter_is_steady(const struct filter *f);

std::unique_ptr<sensor_data> filter_aligned_depth_to_intrinsics(const struct filter *f, const sensor_data & depth);
//std::unique_ptr<image_depth16> filter_aligned_depth_overlay(const struct filter *f, const image_depth16 &depth, const image_gray8 & image);

#endif
