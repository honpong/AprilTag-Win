#ifndef __FILTER_H
#define __FILTER_H

#include "state_vision.h"
#include "observation.h"
#include "device_parameters.h"
#include "calibration_xml.h"
#include "fast_tracker.h"
#include "../numerics/transformation.h"
#ifdef ENABLE_QR
#include "qr.h"
#endif
#include "RCSensorFusionInternals.h"
#include "camera_control_interface.h"
#include "../cor/platform/sensor_clock.h"
#include "../cor/sensor_data.h"
#include "spdlog/spdlog.h"

struct filter {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    filter(): s(cov)
    {
        //these need to be initialized to defaults - everything else is handled in filter_initialize that is called every time
        ignore_lateness = false;
    }
    RCSensorFusionRunState run_state;
    int min_group_add;
    int max_group_add;
    
    sensor_clock::time_point last_time;
    sensor_clock::time_point last_packet_time;
    int last_packet_type;
    state s;
    
    covariance cov;
    std::unique_ptr<spdlog::logger> &log = s.log;

    f_t w_variance;
    f_t a_variance;

    m3 w_alignment;
    m3 a_alignment;

    bool gravity_init;

    calibration::camera depth = {};

    sensor_clock::time_point want_start;
    bool got_accelerometer, got_gyroscope, got_image;
    v3 last_gyro_meas, last_accel_meas;
    bool detector_failed, tracker_failed, tracker_warned;
    bool speed_failed, speed_warning;
    bool numeric_failed;
    sensor_clock::time_point speed_warning_time;
    bool ignore_lateness;
    stdev<3> gyro_stability, accel_stability;
    sensor_clock::time_point stable_start;
    bool calibration_bad;
    
    float max_velocity;
    float median_depth_variance;
    bool has_converged;

    sensor_clock::duration mindelta;
    bool valid_delta;
    sensor_clock::time_point last_arrival;
    
    sensor_clock::time_point active_time;
    
#ifdef ENABLE_QR
    qr_detector qr;
    sensor_clock::time_point last_qr_time;
    qr_benchmark qr_bench;
#endif
    
    transformation origin;
    bool origin_gravity_aligned;

    v3 a_bias_start, w_bias_start; //for tracking calibration progress
    
    observation_queue observations;
    
    camera_control_interface camera_control;
    image_depth16 recent_depth;
    bool has_depth;
    
    std::chrono::duration<float, milli> accel_timer, gyro_timer, image_timer;
};

bool filter_depth_measurement(struct filter *f, const image_depth16 & depth);
bool filter_image_measurement(struct filter *f, const image_gray8 & image);
void filter_accelerometer_measurement(struct filter *f, const accelerometer_data &data);
void filter_gyroscope_measurement(struct filter *f, const gyro_data &data);
void filter_set_origin(struct filter *f, const transformation &origin, bool gravity_aligned);
void filter_set_reference(struct filter *f);
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
void filter_get_device_parameters(const struct filter *f, device_parameters *device);

extern "C" void filter_initialize(struct filter *f, device_parameters *device);
float filter_converged(const struct filter *f);
bool filter_is_steady(const struct filter *f);

std::unique_ptr<image_depth16> filter_aligned_depth_to_intrinsics(const struct filter *f, const image_depth16 &depth);
std::unique_ptr<image_depth16> filter_aligned_depth_overlay(const struct filter *f, const image_depth16 &depth, const image_gray8 & image);

#endif
