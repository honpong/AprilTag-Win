#ifndef __FILTER_H
#define __FILTER_H

#include "model.h"
#include "observation.h"
#include "device_parameters.h"

struct tracker {
    int width;
    int height;
    int maxfeats;
    int groupsize;
    int maxgroupsize;
    uint8_t *im1, *im2;
    struct mapbuffer *sink;
};

struct filter {
filter(bool estimate_calibration): min_feats_per_group(0), output(0), control(0), visbuf(0), recognition_buffer(0), last_time(0), last_packet_time(0), s(estimate_calibration), gravity_init(0), frame(0), active(0), got_accelerometer(0), got_gyroscope(0), accelerometer_max(0.), gyroscope_max(0.), got_image(0), need_reference(true), latitude(37.7750), longitude(-122.4183), altitude(0.), location_valid(false), measurement_running(false), detector_failed(false), tracker_failed(false), tracker_warned(false), speed_failed(false), speed_warning(false), speed_warning_time(0), numeric_failed(false) {}

    int min_feats_per_group;
    int max_features;
    int min_group_add;
    int max_group_add;

    struct mapbuffer * output;
    struct mapbuffer * control;
    struct mapbuffer * visbuf;

    uint64_t last_time;
    uint64_t last_packet_time;
    state s;

#ifndef SWIG
#endif
    f_t vis_cov;
    f_t init_vis_cov;
    f_t max_add_vis_cov;
    f_t min_add_vis_cov;
    f_t vis_ref_noise;
    f_t vis_noise;
    f_t w_variance;
    f_t a_variance;

    bool gravity_init;
    int frame;
    bool active;
    bool got_accelerometer, got_gyroscope, got_image;
    bool need_reference;
    int skip;
    int max_state_size;
    f_t min_group_health;
    f_t max_feature_std_percent;
    f_t outlier_thresh;
    f_t outlier_reject;
    int image_height;
    f_t accelerometer_max, gyroscope_max;
    uint64_t shutter_delay;
    uint64_t shutter_period;
    double latitude;
    double longitude;
    double altitude;
    bool location_valid;
    int dictionary_size;
    mapbuffer *recognition_buffer;
    void (*measurement_callback)(void *);
    void *measurement_callback_object;
    bool measurement_running;
    bool detector_failed, tracker_failed, tracker_warned;
    bool speed_failed, speed_warning;
    bool numeric_failed;
    uint64_t speed_warning_time;
    tracker track;
    struct corvis_device_parameters device;

    f_t confusion[500][500];
    observation_queue observations;
};

#ifdef SWIG
%callback("%s_cb");
#endif
extern "C" void sfm_image_measurement(void *f, packet_t *p);
extern "C" void sfm_imu_measurement(void *f, packet_t *p);
extern "C" void sfm_accelerometer_measurement(void *f, packet_t *p);
extern "C" void sfm_gyroscope_measurement(void *f, packet_t *p);
//extern "C" void sfm_vis_measurement(void *f, packet_t *p);
extern "C" void sfm_features_added(void *f, packet_t *p);
extern "C" void sfm_control(void *_f, packet_t *p);
#ifdef SWIG
%nocallback;
#endif

extern "C" void filter_init(struct filter *f, corvis_device_parameters device);
extern "C" void filter_reset_full(struct filter *f);
extern "C" void filter_reset_position(struct filter *f);
float filter_converged(struct filter *f);
bool filter_is_steady(struct filter *f);
bool filter_is_aligned(struct filter *f);
#endif
