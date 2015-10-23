//
//  rc_intel_interface.cpp
//
//  Created by Eagle Jones on 1/29/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#define RCTRACKER_API_EXPORTS
#include "rc_intel_interface.h"
#include "sensor_fusion.h"
#include "rs_calibration_json.h"
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
    std::basic_string<rc_char_t> jsonString;
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
    tracker->reset(sensor_clock::micros_to_tp(initialTime_us), rc_Pose_to_transformation(initialPose_m), false);
}

void rc_printDeviceConfig(rc_Tracker * tracker)
{
    device_parameters device = tracker->device;
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
}

void rc_configureCamera(rc_Tracker * tracker, rc_Camera camera, const rc_Pose pose_m, int width_px, int height_px, float center_x_px, float center_y_px, float focal_length_x_px, float focal_length_y_px, float skew, bool fisheye, float fisheye_fov_radians)
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

    tracker->device.fisheye = fisheye;
    if(fisheye)
    {
        tracker->device.K[0] = fisheye_fov_radians;
    }
    
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

rc_DeviceParameters rcCalFromRSCal(const rcCalibration &cal)
{
    rc_DeviceParameters out;

    out.version = cal.calibrationVersion;
    out.Fx = cal.Fx;
    out.Fy = cal.Fy;
    out.Cx = cal.Cx;
    out.Cy = cal.Cy;
    out.px = cal.px;
    out.py = cal.py;
    out.K[0] = cal.K0;
    out.K[1] = cal.K1;
    out.K[2] = cal.K2;
    if (cal.distortionModel == 1)
        out.K[0] = cal.Kw;
    out.a_bias[0] = cal.abias0;
    out.a_bias[1] = cal.abias1;
    out.a_bias[2] = cal.abias2;
    out.w_bias[0] = cal.wbias0;
    out.w_bias[1] = cal.wbias1;
    out.w_bias[2] = cal.wbias2;
    out.Tc[0] = cal.Tc0;
    out.Tc[1] = cal.Tc1;
    out.Tc[2] = cal.Tc2;
    out.Wc[0] = cal.Wc0;
    out.Wc[1] = cal.Wc1;
    out.Wc[2] = cal.Wc2;
    out.a_bias_var[0] = cal.abiasvar0;
    out.a_bias_var[1] = cal.abiasvar1;
    out.a_bias_var[2] = cal.abiasvar2;
    out.w_bias_var[0] = cal.wbiasvar0;
    out.w_bias_var[1] = cal.wbiasvar1;
    out.w_bias_var[2] = cal.wbiasvar2;
    out.Tc_var[0] = cal.TcVar0;
    out.Tc_var[1] = cal.TcVar1;
    out.Tc_var[2] = cal.TcVar2;
    out.Wc_var[0] = cal.WcVar0;
    out.Wc_var[1] = cal.WcVar1;
    out.Wc_var[2] = cal.WcVar2;
    out.w_meas_var = cal.wMeasVar;
    out.a_meas_var = cal.aMeasVar;
    out.image_width = cal.imageWidth;
    out.image_height = cal.imageHeight;
    out.fisheye = cal.distortionModel;

    return out;
}

rcCalibration rsCalFromRCCal(const rc_DeviceParameters &cal)
{
    rcCalibration out;

    out.calibrationVersion = cal.version;
    out.Fx = cal.Fx;
    out.Fy = cal.Fy;
    out.Cx = cal.Cx;
    out.Cy = cal.Cy;
    out.px = cal.px;
    out.py = cal.py;
    out.K0 = cal.K[0];
    out.K1 = cal.K[1];
    out.K2 = cal.K[2];
    if (cal.fisheye)
        out.Kw = cal.K[0];
    out.abias0 = cal.a_bias[0];
    out.abias1 = cal.a_bias[1];
    out.abias2 = cal.a_bias[2];
    out.wbias0 = cal.w_bias[0];
    out.wbias1 = cal.w_bias[1];
    out.wbias2 = cal.w_bias[2];
    out.Tc0 = cal.Tc[0];
    out.Tc1 = cal.Tc[1];
    out.Tc2 = cal.Tc[2];
    out.Wc0 = cal.Wc[0];
    out.Wc1 = cal.Wc[1];
    out.Wc2 = cal.Wc[2];
    out.abiasvar0 = cal.a_bias_var[0];
    out.abiasvar1 = cal.a_bias_var[1];
    out.abiasvar2 = cal.a_bias_var[2];
    out.wbiasvar0 = cal.w_bias_var[0];
    out.wbiasvar1 = cal.w_bias_var[1];
    out.wbiasvar2 = cal.w_bias_var[2];
    out.TcVar0 = cal.Tc_var[0];
    out.TcVar1 = cal.Tc_var[1];
    out.TcVar2 = cal.Tc_var[2];
    out.WcVar0 = cal.Wc_var[0];
    out.WcVar1 = cal.Wc_var[1];
    out.WcVar2 = cal.Wc_var[2];
    out.wMeasVar = cal.w_meas_var;
    out.aMeasVar = cal.a_meas_var;
    out.imageWidth = cal.image_width;
    out.imageHeight = cal.image_height;
    out.distortionModel = cal.fisheye;

    return out;
}

rcCalibration rc_getCalibrationStruct(rc_Tracker *tracker)
{
    device_parameters cal = filter_get_device_parameters(&tracker->sfm);
    return rsCalFromRCCal(cal);
}

bool rc_setCalibrationStruct(rc_Tracker *tracker, const rcCalibration &cal)
{
    if (tracker == nullptr) return false;
    tracker->set_device(rcCalFromRSCal(cal));
    return true;
}

size_t rc_getCalibration(rc_Tracker *tracker, const rc_char_t **buffer)
{
    device_parameters cal = filter_get_device_parameters(&tracker->sfm);
    rcCalibration rsCal = rsCalFromRCCal(cal);

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

bool rc_setCalibration(rc_Tracker *tracker, const rc_char_t *buffer)
{
    rcCalibration cal;
#ifdef _WIN32
    std::wstring_convert<std::codecvt_utf8<rc_char_t>, rc_char_t> converter;
    bool result = calibration_deserialize(converter.to_bytes(buffer), cal);
#else
    bool result = calibration_deserialize(buffer, cal);
#endif
    if (result)
    {
        device_parameters rcCal = rcCalFromRSCal(cal);
        tracker->set_device(rcCal);
    }
    return result;
}