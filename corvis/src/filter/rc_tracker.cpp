//
//  rc_tracker.cpp
//
//  Created by Eagle Jones on 1/29/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#define RCTRACKER_API_EXPORTS
#include "rc_tracker.h"
#include "sensor_fusion.h"
#include "device_parameters.h"
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
    std::unique_ptr<image_depth16> last_depth;
    std::string jsonString;
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
        feat.initialized = fp.initialized;
        feat.stdev = fp.stdev;
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

void rc_configureCamera(rc_Tracker *tracker, rc_CameraId camera_id, const rc_Pose extrinsics_wrt_accel_m, const rc_CameraIntrinsics *intrinsics)
{
    // Make this given camera the current camera
    calibration::camera *cam =
        (camera_id == rc_CAMERA_ID_FISHEYE || camera_id == rc_CAMERA_ID_COLOR) ? &tracker->device.color :
        (camera_id == rc_CAMERA_ID_DEPTH)                                      ? &tracker->device.depth : nullptr;
    if (cam && extrinsics_wrt_accel_m)
        cam->extrinsics_wrt_imu_m = rc_Pose_to_transformation(extrinsics_wrt_accel_m);
    if (cam && intrinsics)
        cam->intrinsics = *intrinsics;

    // Also write through the calibration into the multi-camera calibration struct
    cam =
        camera_id == rc_CAMERA_ID_FISHEYE ? &tracker->calibration.fisheye :
        camera_id == rc_CAMERA_ID_COLOR   ? &tracker->calibration.color :
        camera_id == rc_CAMERA_ID_DEPTH   ? &tracker->calibration.depth :
        camera_id == rc_CAMERA_ID_IR      ? &tracker->calibration.ir : nullptr;
    if (cam && extrinsics_wrt_accel_m)
        cam->extrinsics_wrt_imu_m = rc_Pose_to_transformation(extrinsics_wrt_accel_m);
    if (cam && intrinsics)
        cam->intrinsics = *intrinsics;
}

bool rc_describeCamera(rc_Tracker *tracker,  rc_CameraId camera_id, rc_Pose extrinsics_wrt_accel_m, rc_CameraIntrinsics *intrinsics)
{
    // When you query a currently configure camera, you get the info from the current 'device' struct
    const calibration::camera *cam =
        (camera_id == rc_CAMERA_ID_DEPTH   && tracker->device.depth.intrinsics.type != rc_CALIBRATION_TYPE_UNKNOWN) ? &tracker->device.depth :
        (camera_id == rc_CAMERA_ID_COLOR   && tracker->device.color.intrinsics.type != rc_CALIBRATION_TYPE_FISHEYE) ||
        (camera_id == rc_CAMERA_ID_FISHEYE && tracker->device.color.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE) ? &tracker->device.color :
    // When you query a currently unused camera, you get the data from the milti-camera calibration struct
         camera_id == rc_CAMERA_ID_FISHEYE ? &tracker->calibration.fisheye :
         camera_id == rc_CAMERA_ID_COLOR   ? &tracker->calibration.color :
         camera_id == rc_CAMERA_ID_DEPTH   ? &tracker->calibration.depth :
         camera_id == rc_CAMERA_ID_IR      ? &tracker->calibration.ir : nullptr;
    if (cam && extrinsics_wrt_accel_m)
        transformation_to_rc_Pose(cam->extrinsics_wrt_imu_m, extrinsics_wrt_accel_m);
    if (cam && intrinsics)
        *intrinsics = cam->intrinsics;
    return true;
}

void rc_configureAccelerometer(rc_Tracker * tracker, const rc_Pose alignment_bias_m__s2, float noiseVariance_m2__s4)
{
    Eigen::Map<const Eigen::Matrix<float,3,4>>    a_alignment_bias_m__s2(alignment_bias_m__s2);
    tracker->device.imu.a_alignment        = tracker->calibration.imu.a_alignment        = a_alignment_bias_m__s2.block<3,3>(0,0).cast<f_t>();
    tracker->device.imu.a_bias_m__s2       = tracker->calibration.imu.a_bias_m__s2       = a_alignment_bias_m__s2.block<3,1>(0,3).cast<f_t>();
    tracker->device.imu.a_noise_var_m2__s4 = tracker->calibration.imu.a_noise_var_m2__s4 = noiseVariance_m2__s4;
}

void rc_configureGyroscope(rc_Tracker * tracker, const rc_Pose alignment_bias_rad__s, float noiseVariance_rad2__s2)
{
    Eigen::Map<const Eigen::Matrix<float,3,4>> w_alignment_bias_rad__s(alignment_bias_rad__s);
    tracker->device.imu.w_alignment          = tracker->calibration.imu.w_alignment          = w_alignment_bias_rad__s.block<3,3>(0,0).cast<f_t>();;
    tracker->device.imu.w_bias_rad__s        = tracker->calibration.imu.w_bias_rad__s        = w_alignment_bias_rad__s.block<3,1>(0,3).cast<f_t>();;
    tracker->device.imu.w_noise_var_rad2__s2 = tracker->calibration.imu.w_noise_var_rad2__s2 = noiseVariance_rad2__s2;
}

void rc_configureLocation(rc_Tracker * tracker, double latitude_deg, double longitude_deg, double altitude_m)
{
    tracker->set_location(latitude_deg, longitude_deg, altitude_m);
}

RCTRACKER_API void rc_setDataCallback(rc_Tracker *tracker, rc_DataCallback callback, void *handle)
{
    if(callback) tracker->camera_callback = [callback, handle, tracker](std::unique_ptr<sensor_fusion::data> d, image_gray8 &&cam) {
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

void rc_receiveImage(rc_Tracker *tracker, rc_Timestamp time_us, rc_Timestamp shutter_time_us, rc_ImageFormat format,
                     int width, int height, int stride, const void *image,
                     void(*completion_callback)(void *callback_handle), void *callback_handle)
{
    if (format == rc_FORMAT_DEPTH16) {
        image_depth16 d;
        d.image_handle = std::unique_ptr<void, void(*)(void *)>(callback_handle, completion_callback);
        d.image = (uint16_t *)image;
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
    } else if (format == rc_FORMAT_GRAY8) {
        image_gray8 d;
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

void rc_setOutputLog(rc_Tracker * tracker, const char *filename)
{
    tracker->set_output_log(filename);
}

const char *rc_getTimingStats(rc_Tracker *tracker)
{
    tracker->timingStats = tracker->get_timing_stats();
    return tracker->timingStats.c_str();
}

size_t rc_getCalibration(rc_Tracker *tracker, const char **buffer)
{
    device_parameters cal = {};
    filter_get_device_parameters(&tracker->sfm, &cal);

    std::string json;
    if (!calibration_serialize(cal, json))
        return 0;
    tracker->jsonString = json;
    *buffer = tracker->jsonString.c_str();
    return tracker->jsonString.length();
}

bool rc_setCalibration(rc_Tracker *tracker, const char *buffer)
{
    std::string str(buffer);
    if (str.find("<") != std::string::npos && (str.find("{") == std::string::npos || str.find("<") < str.find("{"))) {
        struct calibration multi_camera_calibration;
        if (!calibration_deserialize_xml(str, multi_camera_calibration))
            return false;
        // Store the multi-camera calibration for rc_describeCamera() and in case we want to write it back out
        tracker->calibration = multi_camera_calibration;
        // Pick the imu,depth,color combo from multi-camera calibration defaulting to fisheye if it's available
        device_parameters device;
        snprintf(device.device_id, sizeof(device.device_id), "%s", multi_camera_calibration.device_id);
        device.depth = multi_camera_calibration.depth;
        device.color = multi_camera_calibration.fisheye.intrinsics.type ?
            multi_camera_calibration.fisheye :
            multi_camera_calibration.color;
        device.imu   = multi_camera_calibration.imu;
        tracker->set_device(device);
    } else {
        device_parameters device;
        if (!calibration_deserialize(buffer, device))
            return false;
        tracker->set_device(device);
    }
    return true;
}
