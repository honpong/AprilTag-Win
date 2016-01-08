//
//  rc_intel_interface.cpp
//
//  Created by Eagle Jones on 1/29/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#define RCTRACKER_API_EXPORTS
#include "rc_intel_interface.h"
#include "sensor_fusion.h"
#include "calibration_json.h"
#include <codecvt>
#include <fstream>

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
    std::basic_string<rc_char_t> jsonString;
    std::vector<rc_Feature> gottenFeatures;
    std::vector<rc_Feature> dataFeatures;
    std::string timingStats;
};

static void copy_features_from_sensor_fusion(std::vector<rc_Feature> &features, const std::vector<sensor_fusion::feature_point> &in_feats)
{
    features.clear();
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
    if (initialPose_m)
        tracker->reset(sensor_clock::micros_to_tp(initialTime_us), rc_Pose_to_transformation(initialPose_m), false);
    else
        tracker->reset(sensor_clock::micros_to_tp(initialTime_us), transformation(), true);
}

void rc_printDeviceConfig(rc_Tracker * tracker)
{
    rcCalibration device = tracker->device;
    fprintf(stderr, "Fx, Fy; %f %f\n", device.Fx, device.Fy);
    fprintf(stderr, "Cx, Cy; %f %f\n", device.Cx, device.Cy);
    fprintf(stderr, "px, py; %f %f\n", device.px, device.py);
    fprintf(stderr, "K[3]; %f %f %f\n", device.K0, device.K1, device.K2);
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
}

void rc_configureCamera(rc_Tracker * tracker, rc_Camera camera, const rc_Pose pose_m, int width_px, int height_px, float center_x_px, float center_y_px, float focal_length_x_px, float focal_length_y_px, float skew, bool fisheye, float fisheye_fov_radians)
{
    tracker->device.Cx = center_x_px;
    tracker->device.Cy = center_y_px;
    tracker->device.Fx = focal_length_x_px;
    tracker->device.Fy = focal_length_y_px;
    tracker->device.image_width = width_px;
    tracker->device.image_height = height_px;
    tracker->device.K0 = 0;
    tracker->device.K1 = 0;
    tracker->device.K2 = 0;
    tracker->device.Kw = fisheye_fov_radians;
    tracker->device.distortionModel = fisheye;

    transformation g = rc_Pose_to_transformation(pose_m);
    rotation_vector W = to_rotation_vector(g.Q);
    for(int i = 0; i < 3; ++i)
    {
        tracker->device.Tc[i] = (float)g.T[i];
        tracker->device.Wc[i] = (float)W.raw_vector()[i];
    }
}


void rc_configureAccelerometer(rc_Tracker * tracker, const rc_Vector bias_m__s2, float noiseVariance_m2__s4)
{
    tracker->device.a_bias[0] = bias_m__s2.x;
    tracker->device.a_bias[1] = bias_m__s2.y;
    tracker->device.a_bias[2] = bias_m__s2.z;
    for(int i = 0; i < 3; i++)
        tracker->device.a_bias_var[i] = noiseVariance_m2__s4;
}

void rc_configureGyroscope(rc_Tracker * tracker, const rc_Vector bias_rad__s, float noiseVariance_rad2__s2)
{
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
    if(callback) tracker->camera_callback = [callback, handle, tracker](std::unique_ptr<sensor_fusion::data> d, camera_data &&cam) {
        uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(d->time.time_since_epoch()).count();

        rc_Pose p;
        transformation_to_rc_Pose(d->transform, p);
        copy_features_from_sensor_fusion(tracker->dataFeatures, d->features);
        callback(handle, micros, p, tracker->dataFeatures.data(), tracker->dataFeatures.size());
    };
    else tracker->camera_callback = nullptr;
}

RCTRACKER_API void rc_setStatusCallback(rc_Tracker *tracker, rc_StatusCallback callback, void *handle)
{
    if(callback) tracker->status_callback = [callback, handle](sensor_fusion::status s) {
        callback(handle, tracker_state_from_run_state(s.run_state), tracker_error_from_error(s.error), tracker_confidence_from_confidence(s.confidence), s.progress);
    };
}

void rc_startCalibration(rc_Tracker * tracker, rc_TrackerRunFlags run_flags)
{
    tracker->start_calibration(run_flags == rc_E_ASYNCRONOUS);
}

void rc_pauseAndResetPosition(rc_Tracker * tracker)
{
    tracker->pause_and_reset_position();
}

void rc_unpause(rc_Tracker *tracker)
{
    tracker->unpause();
}

void rc_startBuffering(rc_Tracker * tracker)
{
    tracker->start_buffering();
}

void rc_startTracker(rc_Tracker * tracker, rc_TrackerRunFlags run_flags)
{
    if (run_flags == rc_E_ASYNCRONOUS)
        tracker->start_unstable(true);
    else
        tracker->start_offline();
}

void rc_stopTracker(rc_Tracker * tracker)
{
    tracker->stop();
    if(tracker->output_enabled)
        tracker->output.stop();
    tracker->output_enabled = false;
}

void rc_receiveImage(rc_Tracker *tracker, rc_Camera camera, rc_Timestamp time_us, rc_Timestamp shutter_time_us, const rc_Pose poseEstimate_m, bool force_recognition, int width, int height, int stride, const void *image, void(*completion_callback)(void *callback_handle), void *callback_handle)
{
	if (camera == rc_EGRAY8) {
		camera_data d;
		d.image_handle = std::unique_ptr<void, void(*)(void *)>(callback_handle, completion_callback);
		d.image = (uint8_t *)image;
		d.width = width;
		d.height = height;
		d.stride = stride;
        d.timestamp = sensor_clock::micros_to_tp(time_us);
        d.exposure_time = std::chrono::microseconds(shutter_time_us);
        if(tracker->output_enabled) {
            tracker->output.write_camera(d);
        }
        else
            tracker->receive_image(std::move(d));
	}
	tracker->trigger_log();
}

void rc_receiveImageWithDepth(rc_Tracker *tracker, rc_Camera camera, rc_Timestamp time_us, rc_Timestamp shutter_time_us, const rc_Pose poseEstimate_m, bool force_recognition, int width, int height, int stride, const void *image, void(*completion_callback)(void *callback_handle), void *callback_handle, int depthWidth, int depthHeight, int depthStride, const void *depthImage, void(*depth_completion_callback)(void *callback_handle), void *depth_callback_handle) 
{
    camera_data d;
    d.image_handle = std::unique_ptr<void, void(*)(void *)>(callback_handle, completion_callback);
    d.image = (uint8_t *)image;
    d.width = width;
    d.height = height;
    d.stride = stride;
    d.timestamp = sensor_clock::micros_to_tp(time_us);
    d.exposure_time = std::chrono::microseconds(shutter_time_us);
    d.depth = std::make_unique<image_depth16>();
    d.depth->image_handle = std::unique_ptr<void, void(*)(void *)>(depth_callback_handle, depth_completion_callback);
    d.depth->image = (uint16_t *)depthImage;
    d.depth->width = depthWidth;
    d.depth->height = depthHeight;
    d.depth->stride = depthStride;
    d.depth->timestamp = d.timestamp;
    d.depth->exposure_time = d.exposure_time;
    if(tracker->output_enabled) {
        tracker->output.write_camera(d);
    }
    else
        tracker->receive_image(std::move(d));
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
        tracker->output.write_accelerometer(d);
    }
    else
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
        tracker->output.write_gyro(d);
    }
    else
        tracker->receive_gyro(std::move(d));
}

void rc_getPose(const rc_Tracker * tracker, rc_Pose pose_m)
{
    transformation_to_rc_Pose(tracker->get_transformation(), pose_m);
}

void rc_setPose(rc_Tracker * tracker, const rc_Pose pose_m)
{
    tracker->set_transformation(rc_Pose_to_transformation(pose_m));
}

int rc_getFeatures(rc_Tracker * tracker, rc_Feature **features_px)
{
    copy_features_from_sensor_fusion(tracker->gottenFeatures, tracker->get_features());
    *features_px = tracker->gottenFeatures.data();
    return tracker->gottenFeatures.size();
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

void rc_setOutputLog(rc_Tracker * tracker, const rc_char_t *filename)
{
#ifdef _WIN32
    std::wstring_convert<std::codecvt_utf8<rc_char_t>, rc_char_t> converter;
    tracker->set_output_log(converter.to_bytes(filename).c_str());
#else
    tracker->set_output_log(filename);
#endif
}

const char *rc_getTimingStats(rc_Tracker *tracker)
{
    tracker->timingStats = tracker->get_timing_stats();
    return tracker->timingStats.c_str();
}

void rc_getCalibrationStruct(rc_Tracker *tracker, rcCalibration *calOut)
{
    filter_get_device_parameters(&tracker->sfm, calOut);

    // filter doesn't keep these internally, so copy them here
    snprintf(calOut->deviceName, sizeof(calOut->deviceName), "%s", tracker->device.deviceName);
    calOut->shutterDelay = tracker->device.shutterDelay;
    calOut->shutterPeriod = tracker->device.shutterPeriod;
    calOut->timeStampOffset = tracker->device.timeStampOffset;
    calOut->px = tracker->device.px;
    calOut->py = tracker->device.py;
    for (int i = 0; i < sizeof(calOut->accelerometerTransform) / sizeof(calOut->accelerometerTransform[0]); i++)
    {
        calOut->accelerometerTransform[i] = tracker->device.accelerometerTransform[i];
    }
    for (int i = 0; i < sizeof(calOut->gyroscopeTransform) / sizeof(calOut->gyroscopeTransform[0]); i++)
    {
        calOut->gyroscopeTransform[i] = tracker->device.gyroscopeTransform[i];
    }
}

bool rc_setCalibrationStruct(rc_Tracker *tracker, const rcCalibration *cal)
{
    if (tracker == nullptr) return false;
    tracker->set_device(*cal);

    // filter doesn't keep these internally, so copy them here
    snprintf(tracker->device.deviceName, sizeof(tracker->device.deviceName), "%s", cal->deviceName);
    tracker->device.shutterDelay = cal->shutterDelay;
    tracker->device.shutterPeriod = cal->shutterPeriod;
    tracker->device.timeStampOffset = cal->timeStampOffset;
    tracker->device.px = cal->px;
    tracker->device.py = cal->py;
    for (int i = 0; i < sizeof(cal->accelerometerTransform) / sizeof(cal->accelerometerTransform[0]); i++)
    {
        tracker->device.accelerometerTransform[i] = cal->accelerometerTransform[i];
    }
    for (int i = 0; i < sizeof(cal->gyroscopeTransform) / sizeof(cal->gyroscopeTransform[0]); i++)
    {
        tracker->device.gyroscopeTransform[i] = cal->gyroscopeTransform[i];
    }

    return true;
}

size_t rc_getCalibration(rc_Tracker *tracker, const rc_char_t **buffer)
{
    rcCalibration rsCal;
    rc_getCalibrationStruct(tracker, &rsCal);

    std::string json;
    if (!calibration_serialize(rsCal, json))
        return 0;
#ifdef _WIN32
    std::wstring_convert<std::codecvt_utf8<rc_char_t>, rc_char_t> converter;
    tracker->jsonString = converter.from_bytes(json);
#else
    tracker->jsonString = json;
#endif
    *buffer = tracker->jsonString.c_str();
    return tracker->jsonString.length();
}

bool rc_setCalibration(rc_Tracker *tracker, const rc_char_t *buffer, const rcCalibration *defaults)
{
    rcCalibration cal;
#ifdef _WIN32
    std::wstring_convert<std::codecvt_utf8<rc_char_t>, rc_char_t> converter;
    bool result = calibration_deserialize(converter.to_bytes(buffer), cal, defaults);
#else
    bool result = calibration_deserialize(buffer, cal, defaults);
#endif
    if (result)
    {
        rc_setCalibrationStruct(tracker, &cal);
    }
    return result;
}

bool rc_setCalibrationFromFile(rc_Tracker *tracker, const rc_char_t *filePath, const rcCalibration *defaults)
{
#ifdef _WIN32
    std::wifstream stream(filePath);
    std::wstringstream buffer;
#else
    std::ifstream stream(filePath);
    std::stringstream buffer;
#endif

    buffer << stream.rdbuf();
    bool result = rc_setCalibration(tracker, buffer.str().c_str(), defaults);
    return result;
}
