#ifndef __FILTER_H
#define __FILTER_H

#include "state_vision.h"
#include "observation.h"
#include "device_parameters.h"
#include "feature_info.h"
#include "tracker.h"
#include "scaled_mask.h"
#include "../numerics/transformation.h"
#ifdef ENABLE_QR
#include "qr.h"
#endif
#include "../../../shared_corvis_3dk/RCSensorFusionInternals.h"
#include "../../../shared_corvis_3dk/camera_control_interface.h"
#include "../cor/platform/sensor_clock.h"
#include "../cor/sensor_data.h"

struct filter {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
filter(): s(cov)
    {
        //make sure all pointers are null
        mask = 0;
        
        //these need to be initialized to defaults - everything else is handled in filter_initialize that is called every time
        gravity_magnitude = 9.80665;
        ignore_lateness = false;
    }
    ~filter() {
        if(mask) delete mask;
    }
    RCSensorFusionRunState run_state;
    int min_group_add;
    int max_group_add;
    
    sensor_clock::time_point last_time;
    sensor_clock::time_point last_packet_time;
    int last_packet_type;
    state s;
    
    covariance cov;

    f_t w_variance;
    f_t a_variance;

    bool gravity_init;
    f_t gravity_magnitude;

    sensor_clock::time_point want_start;
    bool got_accelerometer, got_gyroscope, got_image;
    v4 last_gyro_meas, last_accel_meas;
    sensor_clock::duration shutter_delay;
    sensor_clock::duration shutter_period;
    bool detector_failed, tracker_failed, tracker_warned;
    bool speed_failed, speed_warning;
    bool numeric_failed;
    sensor_clock::time_point speed_warning_time;
    bool ignore_lateness;
    tracker track;
    stdev_vector gyro_stability, accel_stability;
    sensor_clock::time_point stable_start;
    bool calibration_bad;
    
    float max_velocity;
    float median_depth_variance;
    bool has_converged;

    scaled_mask *mask;
    
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

    v4 a_bias_start, w_bias_start; //for tracking calibration progress
    
    observation_queue observations;
    
    camera_control_interface camera_control;
};

bool filter_image_measurement(struct filter *f, const camera_data & camera);
void filter_accelerometer_measurement(struct filter *f, const float data[3], sensor_clock::time_point time);
void filter_gyroscope_measurement(struct filter *f, const float data[3], sensor_clock::time_point time);
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
corvis_device_parameters filter_get_device_parameters(const struct filter *f);

extern "C" void filter_initialize(struct filter *f, corvis_device_parameters device);
float filter_converged(const struct filter *f);
bool filter_is_steady(const struct filter *f);
int filter_get_features(const struct filter *f, struct corvis_feature_info *features, int max);

#endif
