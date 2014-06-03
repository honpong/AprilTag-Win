#ifndef __FILTER_H
#define __FILTER_H

#include "state_vision.h"
#include "observation.h"
#include "device_parameters.h"
#include "feature_info.h"
#include "tracker.h"
#include "scaled_mask.h"

struct filter {
filter(bool estimate_calibration): s(estimate_calibration, cov)
    {
        //make sure all pointers are null
        output = 0;
        control = 0;
        visbuf = 0;
        track.sink = 0;
        recognition_buffer = 0;
        scaled_mask = 0;
    }
    ~filter() {
        if(scaled_mask) delete scaled_mask;
    }
    int min_group_add;
    int max_group_add;
    
    int maxfeats;

    struct mapbuffer * output;
    struct mapbuffer * control;
    struct mapbuffer * visbuf;

    uint64_t last_time;
    uint64_t last_packet_time;
    int last_packet_type;
#ifdef SWIG
    %readonly
#endif
    state s;
#ifdef SWIG
    %readwrite
#endif
    
    covariance cov;

#ifndef SWIG
#endif
    f_t w_variance;
    f_t a_variance;

    bool gravity_init;

    enum { ST_STOP, ST_INERTIAL, ST_STATIC, ST_STEADY, ST_WANTVIDEO, ST_VIDEO, ST_ANY } status;
    uint64_t want_start;
    
    bool got_accelerometer, got_gyroscope, got_image;
    int image_height, image_width;
    uint64_t shutter_delay;
    uint64_t shutter_period;
    mapbuffer *recognition_buffer;
    bool detector_failed, tracker_failed, tracker_warned;
    bool speed_failed, speed_warning;
    bool numeric_failed;
    uint64_t speed_warning_time;
    bool ignore_lateness;
    tracker track;
    struct corvis_device_parameters device;
    stdev_vector gyro_stability, accel_stability;
    uint64_t stable_start;
    bool calibration_bad;
    scaled_mask *scaled_mask;
    
    bool valid_time;
    uint64_t first_time;
    
    int64_t mindelta;
    bool valid_delta;
    uint64_t last_arrival;
    
    uint64_t active_time;
    
    bool estimating_Tc;
    
    observation_queue observations;
};

bool filter_image_measurement(struct filter *f, unsigned char *data, int width, int height, int stride, uint64_t time);
void filter_accelerometer_measurement(struct filter *f, float data[3], uint64_t time);
void filter_gyroscope_measurement(struct filter *f, float data[3], uint64_t time);
void filter_set_reference(struct filter *f);
void filter_compute_gravity(struct filter *f, double latitude, double altitude);
void filter_start_static_calibration(struct filter *f);
void filter_stop_static_calibration(struct filter *f);
void filter_start_hold_steady(struct filter *f);
void filter_start_processing_video(struct filter *f);
void filter_stop_processing_video(struct filter *f);

#ifdef SWIG
%callback("%s_cb");
#endif
extern "C" void filter_image_packet(void *f, packet_t *p);
extern "C" void filter_imu_packet(void *f, packet_t *p);
extern "C" void filter_accelerometer_packet(void *f, packet_t *p);
extern "C" void filter_gyroscope_packet(void *f, packet_t *p);
extern "C" void filter_features_added_packet(void *f, packet_t *p);
extern "C" void filter_control_packet(void *_f, packet_t *p);
#ifdef SWIG
%nocallback;
#endif

extern "C" void filter_initialize_once(struct filter *f, corvis_device_parameters device);
extern "C" void filter_reset_full(struct filter *f);
extern "C" void filter_reset_for_inertial(struct filter *f);
extern "C" void filter_reset_position(struct filter *f);
float filter_converged(struct filter *f);
bool filter_is_steady(struct filter *f);
int filter_get_features(struct filter *f, struct corvis_feature_info *features, int max);
void filter_get_camera_parameters(struct filter *f, float matrix[16], float focal_center_radial[5]);
void filter_select_feature(struct filter *f, float x, float y);

#endif
