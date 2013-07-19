#ifndef __FILTER_H
#define __FILTER_H

#include "model.h"
#include "observation.h"
#include "device_parameters.h"
#include "feature_info.h"
#include "tracker.h"

struct filter {
filter(bool estimate_calibration): min_feats_per_group(0), output(0), control(0), visbuf(0), last_time(0), last_packet_time(0), last_packet_type(0), s(estimate_calibration), gravity_init(0), frame(0), active(0), got_accelerometer(0), got_gyroscope(0), got_image(0), need_reference(true), accelerometer_max(0.), gyroscope_max(0.), latitude(37.7750), longitude(-122.4183), altitude(0.), location_valid(false), recognition_buffer(0), reference_set(false), detector_failed(false), tracker_failed(false), tracker_warned(false), speed_failed(false), speed_warning(false), numeric_failed(false), speed_warning_time(0), ignore_lateness(false), run_static_calibration(false), calibration_bad(false)
    {
        track.sink = 0;
        s.mapperbuf = 0;
    }

    int min_feats_per_group;
    int min_group_add;
    int max_group_add;

    struct mapbuffer * output;
    struct mapbuffer * control;
    struct mapbuffer * visbuf;

    uint64_t last_time;
    uint64_t last_packet_time;
    int last_packet_type;
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
    bool reference_set;
    bool detector_failed, tracker_failed, tracker_warned;
    bool speed_failed, speed_warning;
    bool numeric_failed;
    uint64_t speed_warning_time;
    bool ignore_lateness;
    tracker track;
    void (*detect) (const uint8_t * im, const uint8_t * mask, int width, int height, vector<feature_t> & keypoints, int number_wanted);
    struct corvis_device_parameters device;
    bool run_static_calibration;
    stdev_vector gyro_stability, accel_stability;
    uint64_t stable_start;
    bool calibration_bad;

    f_t confusion[500][500];
    observation_queue observations;
};

bool filter_image_measurement(struct filter *f, unsigned char *data, int width, int height, uint64_t time);
void filter_accelerometer_measurement(struct filter *f, float data[3], uint64_t time);
void filter_gyroscope_measurement(struct filter *f, float data[3], uint64_t time);
void filter_set_reference(struct filter *f);
void filter_set_initial_conditions(struct filter *f, v4 a, v4 gravity, v4 w, v4 w_bias, uint64_t time);
void filter_gravity_init(struct filter *f, v4 gravity, uint64_t time);

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

extern "C" void filter_init(struct filter *f, corvis_device_parameters device);
extern "C" void filter_reset_full(struct filter *f);
extern "C" void filter_reset_position(struct filter *f);
float filter_converged(struct filter *f);
bool filter_is_steady(struct filter *f);
bool filter_is_aligned(struct filter *f);
int filter_get_features(struct filter *f, struct corvis_feature_info *features, int max);
void filter_get_camera_parameters(struct filter *f, float matrix[16], float focal_center_radial[5]);
#endif
