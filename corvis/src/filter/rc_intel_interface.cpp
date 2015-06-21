//
//  rc_intel_interface.cpp
//
//  Created by Eagle Jones on 1/29/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#define RCTRACKER_API_EXPORTS
#include "rc_intel_interface.h"
#include "sensor_fusion.h"
#include "calibration_json_store.h"
#include <codecvt>

static void transformation_to_rc_Pose(const transformation &g, rc_Pose p)
{
    m4 R = to_rotation_matrix(g.Q);
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            p[i * 4 + j] = (float)R(i, j);
        }
        p[i * 4 + 3] = (float)g.T[i];
    }
}

static transformation rc_Pose_to_transformation(const rc_Pose p)
{
    transformation g;
    m4 R = m4::Zero();
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            R(i,j) = p[i * 4 + j];
        }
        g.T[i] = p[i * 4 + 3];
    }
    g.Q = to_quaternion(R);
    return g;
}

static rc_TrackerState tracker_state_from_run_state(RCSensorFusionRunState run_state)
{
    switch(run_state)
    {
        case RCSensorFusionRunStateDynamicInitialization:
            return rc_E_DYNAMIC_INITIALIZATION;
        case RCSensorFusionRunStateInactive:
            return rc_E_INACTIVE;
        case RCSensorFusionRunStateLandscapeCalibration:
            return rc_E_LANDSCAPE_CALIBRATION;
        case RCSensorFusionRunStatePortraitCalibration:
            return rc_E_PORTRAIT_CALIBRATION;
        case RCSensorFusionRunStateRunning:
            return rc_E_RUNNING;
        case RCSensorFusionRunStateStaticCalibration:
            return rc_E_STATIC_CALIBRATION;
        case RCSensorFusionRunStateSteadyInitialization:
            return rc_E_STEADY_INITIALIZATION;
        default: // This case should never be reached
            return rc_E_INACTIVE;
    }
}

static rc_TrackerError tracker_error_from_error(RCSensorFusionErrorCode error)
{
    switch(error)
    {
        case RCSensorFusionErrorCodeNone:
            return rc_E_ERROR_NONE;
        case RCSensorFusionErrorCodeOther:
            return rc_E_ERROR_OTHER;
        case RCSensorFusionErrorCodeVision:
            return rc_E_ERROR_VISION;
        case RCSensorFusionErrorCodeTooFast:
            return rc_E_ERROR_SPEED;
        default:
            return rc_E_ERROR_OTHER;
    }
}

static rc_TrackerConfidence tracker_confidence_from_confidence(RCSensorFusionConfidence confidence)
{
    switch(confidence)
    {
        case RCSensorFusionConfidenceHigh:
            return rc_E_CONFIDENCE_HIGH;
        case RCSensorFusionConfidenceMedium:
            return rc_E_CONFIDENCE_MEDIUM;
        case RCSensorFusionConfidenceLow:
            return rc_E_CONFIDENCE_LOW;
        case RCSensorFusionConfidenceNone:
        default:
            return RC_E_CONFIDENCE_NONE;
    }
}

struct rc_Tracker: public sensor_fusion
{
    rc_Tracker(bool immediate_dispatch): sensor_fusion(immediate_dispatch ? fusion_queue::latency_strategy::ELIMINATE_LATENCY : fusion_queue::latency_strategy::IMAGE_TRIGGER) {}
    std::wstring jsonString;
    std::vector<rc_Feature> features;
    std::string timingStats;
};

std::vector<rc_Feature> copy_features_from_sensor_fusion(const std::vector<sensor_fusion::feature_point> &in_feats)
{
    std::vector<rc_Feature> features;
    features.reserve(in_feats.size());
    for (auto fp : in_feats)
    {
        rc_Feature feat;
        feat.image_x = fp.x;
        feat.image_y = fp.y;
        feat.world.x = fp.worldx;
        feat.world.y = fp.worldy;
        feat.world.z = fp.worldz;
        feat.id = fp.id;
        features.push_back(feat);
    }
    return features;
}

extern "C" rc_Tracker * rc_create()
{
    rc_Tracker * tracker = new rc_Tracker(false); //don't dispatch immediately - intel doesn't really make any data interleaving guarantees
    tracker->sfm.ignore_lateness = true; //and don't drop frames to keep up

    return tracker;
}

extern "C" void rc_destroy(rc_Tracker * tracker)
{
    delete tracker;
}

extern "C" void rc_reset(rc_Tracker * tracker, rc_Timestamp initialTime_us, const rc_Pose initialPose_m)
{
    tracker->reset(sensor_clock::micros_to_tp(initialTime_us), rc_Pose_to_transformation(initialPose_m));
}

void rc_printDeviceConfig(rc_Tracker * tracker)
{
    corvis_device_parameters device = tracker->device;
    fprintf(stderr, "Fx, Fy; %f %f\n", device.Fx, device.Fy);
    fprintf(stderr, "Cx, Cy; %f %f\n", device.Cx, device.Cy);
    fprintf(stderr, "px, py; %f %f\n", device.px, device.py);
    fprintf(stderr, "K[3]; %f %f %f\n", device.K[0], device.K[1], device.K[2]);
    fprintf(stderr, "a_bias[3]; %f %f %f\n", device.a_bias[0], device.a_bias[1], device.a_bias[2]);
    fprintf(stderr, "a_bias_var[3]; %f %f %f\n", device.a_bias_var[0], device.a_bias_var[1], device.a_bias_var[2]);
    fprintf(stderr, "w_bias[3]; %f %f %f\n", device.w_bias[0], device.w_bias[1], device.w_bias[2]);
    fprintf(stderr, "w_bias_var[3]; %f %f %f\n", device.w_bias_var[0], device.w_bias_var[1], device.w_bias_var[2]);
    fprintf(stderr, "w_meas_var; %f\n", device.w_meas_var);
    fprintf(stderr, "a_meas_var; %f\n", device.a_meas_var);
    fprintf(stderr, "Tc[3]; %f %f %f\n", device.Tc[0], device.Tc[1], device.Tc[2]);
    fprintf(stderr, "Tc_var[3]; %f %f %f\n", device.Tc_var[0], device.Tc_var[1], device.Tc_var[2]);
    fprintf(stderr, "Wc[3]; %f %f %f\n", device.Wc[0], device.Wc[1], device.Wc[2]);
    fprintf(stderr, "Wc_var[3]; %f %f %f\n", device.Wc_var[0], device.Wc_var[1], device.Wc_var[2]);
    fprintf(stderr, "int image_width, image_height; %d %d\n", device.image_width, device.image_height);
    //fprintf(stderr, "sensor_clock::duration shutter_delay, shutter_period;
}

void rc_configureCamera(rc_Tracker * tracker, rc_Camera camera, const rc_Pose pose_m, int width_px, int height_px, float center_x_px, float center_y_px, float focal_length_x_px, float focal_length_y_px, float skew)
{
    tracker->device.Cx = center_x_px;
    tracker->device.Cy = center_y_px;
    tracker->device.Fx = focal_length_x_px;
    tracker->device.Fy = focal_length_y_px;
    tracker->device.image_width = width_px;
    tracker->device.image_height = height_px;
    tracker->device.K[0] = 0;
    tracker->device.K[1] = 0;
    tracker->device.K[2] = 0;

    transformation g = rc_Pose_to_transformation(pose_m);
    rotation_vector W = to_rotation_vector(g.Q);
    for(int i = 0; i < 3; ++i)
    {
        tracker->device.Tc[i] = (float)g.T[i];
        tracker->device.Wc[i] = (float)W.raw_vector()[i];
    }
}


void rc_configureAccelerometer(rc_Tracker * tracker, const rc_Pose pose_m, const rc_Vector bias_m__s2, float noiseVariance_m2__s4)
{
    //TODO: Support pose_m
    tracker->device.a_bias[0] = bias_m__s2.x;
    tracker->device.a_bias[1] = bias_m__s2.y;
    tracker->device.a_bias[2] = bias_m__s2.z;
    for(int i = 0; i < 3; i++)
        tracker->device.a_bias_var[i] = noiseVariance_m2__s4;
}

void rc_configureGyroscope(rc_Tracker * tracker, const rc_Pose pose_m, const rc_Vector bias_rad__s, float noiseVariance_rad2__s2)
{
    //TODO: Support pose_m
    tracker->device.w_bias[0] = bias_rad__s.x;
    tracker->device.w_bias[1] = bias_rad__s.y;
    tracker->device.w_bias[2] = bias_rad__s.z;
    for(int i = 0; i < 3; i++)
        tracker->device.w_bias_var[i] = noiseVariance_rad2__s2;
}

void rc_configureLocation(rc_Tracker * tracker, double latitude_deg, double longitude_deg, double altitude_m)
{
    tracker->set_location(latitude_deg, longitude_deg, altitude_m);
}

RCTRACKER_API void rc_setDataCallback(rc_Tracker *tracker, rc_DataCallback callback, void *handle)
{
    if(callback) tracker->camera_callback = [callback, handle](std::unique_ptr<sensor_fusion::data> d, camera_data &&cam) {
        uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(d->time.time_since_epoch()).count();

        rc_Pose p;
        transformation_to_rc_Pose(d->transform, p);

        std::vector<rc_Feature> features = copy_features_from_sensor_fusion(d->features);
        rc_Feature *f = nullptr;
        int count = features.size();
        if(count) f = &features[0];
        callback(handle, micros, p, f, count);
    };
    else tracker->camera_callback = nullptr;
}

RCTRACKER_API void rc_setStatusCallback(rc_Tracker *tracker, rc_StatusCallback callback, void *handle)
{
    if(callback) tracker->status_callback = [callback, handle](sensor_fusion::status s) {
        callback(handle, tracker_state_from_run_state(s.run_state), tracker_error_from_error(s.error), tracker_confidence_from_confidence(s.confidence), s.progress);
    };
}

void rc_startCalibration(rc_Tracker * tracker)
{
    tracker->start_calibration(false);
}

/*void rc_startInertialOnly(rc_Tracker * tracker)
{
    tracker->start_inertial_only();
}*/

void rc_startTracker(rc_Tracker * tracker)
{
    tracker->start_offline();
}

void rc_stopTracker(rc_Tracker * tracker)
{
    tracker->stop();
    if(tracker->output_enabled)
        tracker->output.stop();
    tracker->output_enabled = false;
}

//This is only for writing to logfiles - uses start time of image capture
void copy_camera_data(rc_Tracker * tracker, rc_Timestamp time_us, rc_Timestamp shutter_time_us, int stride, const void *image, camera_data & d)
{
    //TODO: don't malloc here
    int bytes = tracker->device.image_width * tracker->device.image_height;
    d.image_handle = std::unique_ptr<void, void(*)(void *)>(malloc(bytes), free);
    d.image = (uint8_t *)d.image_handle.get();
    for (int i = 0; i < tracker->device.image_height; i++) memcpy(d.image + i * tracker->device.image_width, ((unsigned char *)image) + i * stride, tracker->device.image_width);
    d.width = tracker->device.image_width;
    d.height = tracker->device.image_height;
    // TODO: Check that we support stride
    d.stride = d.width;
    //only for writing to logfiles
    d.timestamp = sensor_clock::micros_to_tp(time_us);
}

void rc_receiveImage(rc_Tracker *tracker, rc_Camera camera, rc_Timestamp time_us, rc_Timestamp shutter_time_us, const rc_Pose poseEstimate_m, bool force_recognition, int stride, const void *image, void(*completion_callback)(void *callback_handle), void *callback_handle)
{
    if(camera == rc_EGRAY8) {
        camera_data d;
        d.image_handle = std::unique_ptr<void, void(*)(void *)>(callback_handle, completion_callback);
        d.image = (uint8_t *)image;
        d.width = tracker->device.image_width;
        d.height = tracker->device.image_height;
        d.stride = stride;
        d.timestamp = sensor_clock::micros_to_tp(time_us + shutter_time_us / 2);
        tracker->receive_image(std::move(d));
        if(tracker->output_enabled) {
            camera_data d2;
            copy_camera_data(tracker, time_us, shutter_time_us, stride, image, d2);
            tracker->output.receive_camera(std::move(d2));
        }
    }
    tracker->trigger_log();
}

void rc_receiveAccelerometer(rc_Tracker * tracker, rc_Timestamp time_us, const rc_Vector acceleration_m__s2)
{
    accelerometer_data d;
    d.accel_m__s2[0] = acceleration_m__s2.x;
    d.accel_m__s2[1] = acceleration_m__s2.y;
    d.accel_m__s2[2] = acceleration_m__s2.z;
    d.timestamp = sensor_clock::micros_to_tp(time_us);
    if(tracker->output_enabled) {
        accelerometer_data d2;
        for(int i = 0; i < 3; ++i) d2.accel_m__s2[i] = d.accel_m__s2[i];
        d2.timestamp = d.timestamp;
        tracker->output.receive_accelerometer(std::move(d2));
    }
    tracker->receive_accelerometer(std::move(d));
}

void rc_receiveGyro(rc_Tracker * tracker, rc_Timestamp time_us, const rc_Vector angular_velocity_rad__s)
{
    gyro_data d;
    d.angvel_rad__s[0] = angular_velocity_rad__s.x;
    d.angvel_rad__s[1] = angular_velocity_rad__s.y;
    d.angvel_rad__s[2] = angular_velocity_rad__s.z;
    d.timestamp = sensor_clock::micros_to_tp(time_us);
    if(tracker->output_enabled) {
        gyro_data d2;
        for(int i = 0; i < 3; ++i) d2.angvel_rad__s[i] = d.angvel_rad__s[i];
        d2.timestamp = d.timestamp;
        tracker->output.receive_gyro(std::move(d2));
    }
    tracker->receive_gyro(std::move(d));
}

void rc_getPose(const rc_Tracker * tracker, rc_Pose pose_m)
{
    transformation_to_rc_Pose(tracker->get_transformation(), pose_m);
}

int rc_getFeatures(rc_Tracker * tracker, rc_Feature **features_px)
{
    tracker->features = copy_features_from_sensor_fusion(tracker->get_features());
    *features_px = nullptr;
    int count = tracker->features.size();
    if(count) *features_px = &tracker->features[0];
    return count;
}

rc_TrackerState rc_getState(const rc_Tracker *tracker)
{
    return tracker_state_from_run_state(tracker->sfm.run_state);
}

rc_TrackerConfidence rc_getConfidence(const rc_Tracker *tracker)
{
    rc_TrackerConfidence confidence = RC_E_CONFIDENCE_NONE;
    if(tracker->sfm.run_state == rc_E_RUNNING)
    {
        if(tracker->sfm.detector_failed) confidence = rc_E_CONFIDENCE_LOW;
        else if(tracker->sfm.has_converged) confidence = rc_E_CONFIDENCE_HIGH;
        else confidence = rc_E_CONFIDENCE_MEDIUM;
    }
    return confidence;
}

rc_TrackerError rc_getError(const rc_Tracker *tracker)
{
    rc_TrackerError error = rc_E_ERROR_NONE;
    if(tracker->sfm.numeric_failed) error = rc_E_ERROR_OTHER;
    else if(tracker->sfm.speed_failed) error = rc_E_ERROR_SPEED;
    else if(tracker->sfm.detector_failed) error = rc_E_ERROR_VISION;
    return error;
}

float rc_getProgress(const rc_Tracker *tracker)
{
    return filter_converged(&tracker->sfm);
}

void rc_setLog(rc_Tracker * tracker, void (*log)(void *handle, const char *buffer_utf8, size_t length), bool stream, rc_Timestamp period_us, void *handle)
{
    tracker->set_log_function(log, stream, std::chrono::duration_cast<sensor_clock::duration>(std::chrono::microseconds(period_us)), handle);
}

void rc_triggerLog(const rc_Tracker * tracker)
{
    tracker->trigger_log();
}

void rc_setOutputLog(rc_Tracker * tracker, const wchar_t * wfilename)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    std::string filename = converter.to_bytes(wfilename);
    tracker->set_output_log(filename.c_str());
}

corvis_device_parameters rc_getCalibration(rc_Tracker *tracker)
{
    corvis_device_parameters calibration;
    calibration.Fx = (float)tracker->sfm.s.focal_length.v;
    calibration.Fy = (float)tracker->sfm.s.focal_length.v;
    calibration.Cx = (float)tracker->sfm.s.center_x.v;
    calibration.Cy = (float)tracker->sfm.s.center_y.v;
    calibration.w_meas_var = (float)tracker->sfm.w_variance;
    calibration.a_meas_var = (float)tracker->sfm.a_variance;
    calibration.K[0] = (float)tracker->sfm.s.k1.v;
    calibration.K[1] = (float)tracker->sfm.s.k2.v;
    calibration.K[2] = (float)tracker->sfm.s.k3.v;
    calibration.Wc[0] = (float)tracker->sfm.s.Wc.v.x();
    calibration.Wc[1] = (float)tracker->sfm.s.Wc.v.y();
    calibration.Wc[2] = (float)tracker->sfm.s.Wc.v.z();
    for(int i = 0; i < 3; i++) {
        calibration.a_bias[i] = (float)tracker->sfm.s.a_bias.v[i];
        calibration.a_bias_var[i] = (float)tracker->sfm.s.a_bias.variance()[i];
        calibration.w_bias[i] = (float)tracker->sfm.s.w_bias.v[i];
        calibration.w_bias_var[i] = (float)tracker->sfm.s.w_bias.variance()[i];
        calibration.Tc[i] = (float)tracker->sfm.s.Tc.v[i];
        calibration.Tc_var[i] = (float)tracker->sfm.s.Tc.variance()[i];
        calibration.Wc_var[i] = (float)tracker->sfm.s.Wc.variance()[i];
    }
    calibration.image_width = tracker->sfm.image_width;
    calibration.image_height = tracker->sfm.image_height;
    return calibration;
}

const char *rc_getTimingStats(rc_Tracker *tracker)
{
    tracker->timingStats = tracker->get_timing_stats();
    return tracker->timingStats.c_str();
}

size_t rc_getCalibration(rc_Tracker *tracker, const wchar_t** buffer)
{
    corvis_device_parameters cal = rc_getCalibration(tracker);
    std::string json;
    bool result = calibration_serialize(cal, json);
    if (result)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        tracker->jsonString = converter.from_bytes(json);
        *buffer = tracker->jsonString.c_str();
        return tracker->jsonString.length();
    }
    else
    {
        return 0;
    }
}

bool rc_setCalibration(rc_Tracker *tracker, const wchar_t *wbuffer)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    std::string buffer = converter.to_bytes(wbuffer);
    corvis_device_parameters cal;
    bool result = calibration_deserialize(buffer, cal);
    if (result) tracker->set_device(cal);
    return result;
}
