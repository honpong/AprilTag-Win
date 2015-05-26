//
//  rc_intel_interface.cpp
//
//  Created by Eagle Jones on 1/29/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#include "rc_intel_interface.h"
#include "sensor_fusion.h"
#include "device_parameters.h"

static const unsigned T0 = 3;
static const unsigned T1 = 7;
static const unsigned T2 = 11;

static sensor_clock::time_point time_100_ns_to_tp(rc_Timestamp time_100_ns)
{
    return sensor_clock::time_point(std::chrono::duration_cast<sensor_clock::duration>(std::chrono::duration<long long, std::ratio<1, 10000000>>(time_100_ns)));
}

struct rc_Tracker: public sensor_fusion
{
    rc_Tracker(bool immediate_dispatch): sensor_fusion(immediate_dispatch) {}
};

extern "C" rc_Tracker * rc_create()
{
    rc_Tracker * tracker = new rc_Tracker(false); //TODO: when we switch to intel logs, request immediate dispatch
    tracker->sfm.ignore_lateness = true; //and don't drop frames to keep up
    corvis_device_parameters d;
    get_parameters_for_device(DEVICE_TYPE_UNKNOWN, &d);
    tracker->set_device(d);

    return tracker;
}

extern "C" void rc_destroy(rc_Tracker * tracker)
{
    delete tracker;
}

extern "C" void rc_reset(rc_Tracker * tracker, rc_Timestamp initialTime_100_ns, const rc_Pose initialPose_m)
{
    m4 R = m4::Identity();
    v4 T = v4::Zero();
    for(int i = 0; i < 3; ++i) {
        T[i] = initialPose_m[i * 4 + 3];
        for(int j = 0; j < 3; ++ j) {
            R(i, j) = initialPose_m[i * 4 + j];
        }
    }
    tracker->reset(time_100_ns_to_tp(initialTime_100_ns), transformation(R, T));
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

void rc_configureCamera(rc_Tracker * tracker, rc_Camera camera, const rc_Pose pose_m, int width_px, int height_px, float center_x_px, float center_y_px, float focal_length_px, float focal_length_xy_px, float focal_length_y_px)
{
    tracker->device.Cx = center_x_px;
    tracker->device.Cy = center_y_px;
    tracker->device.Fx = tracker->device.Fy = focal_length_px;
    tracker->device.image_width = width_px;
    tracker->device.image_height = height_px;
    // TODO: remove or set these to zero
    tracker->device.K[0] = .17f;
    tracker->device.K[1] = -.38f;

    tracker->device.Tc[0] = pose_m[T0];
    tracker->device.Tc[1] = pose_m[T1];
    tracker->device.Tc[2] = pose_m[T2];
    m4 R = m4::Identity();
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            R(i, j) = pose_m[i * 4 + j];
        }
    }
    rotation_vector r = to_rotation_vector(R);
    tracker->device.Wc[0] = r.x();
    tracker->device.Wc[1] = r.y();
    tracker->device.Wc[2] = r.z();
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
}

void rc_receiveImage(rc_Tracker * tracker, rc_Camera camera, rc_Timestamp time_100_ns, rc_Timestamp shutter_time, const rc_Pose poseEstimate_m, bool force_recognition, int stride, const void *image)
{
    if(camera == rc_EGRAY8) {
        camera_data d;
        int bytes = stride*tracker->device.image_height;
        d.image_handle = std::unique_ptr<void, void(*)(void *)>(malloc(bytes), free);
        d.image = (uint8_t *)d.image_handle.get();
        memcpy(d.image, image, bytes);
        d.width = tracker->device.image_width;
        d.height = tracker->device.image_height;
        // TODO: Check that we support stride
        d.stride = stride;
        d.timestamp = time_100_ns_to_tp(time_100_ns + shutter_time / 2);
        tracker->receive_image(std::move(d));
    }
}

void rc_receiveAccelerometer(rc_Tracker * tracker, rc_Timestamp time_100_ns, const rc_Vector acceleration_m__s2)
{
    accelerometer_data d;
    d.accel_m__s2[0] = acceleration_m__s2.x;
    d.accel_m__s2[1] = acceleration_m__s2.y;
    d.accel_m__s2[2] = acceleration_m__s2.z;
    d.timestamp = time_100_ns_to_tp(time_100_ns);
    tracker->receive_accelerometer(std::move(d));
}

void rc_receiveGyro(rc_Tracker * tracker, rc_Timestamp time_100_ns, const rc_Vector angular_velocity_rad__s)
{
    gyro_data d;
    d.angvel_rad__s[0] = angular_velocity_rad__s.x;
    d.angvel_rad__s[1] = angular_velocity_rad__s.y;
    d.angvel_rad__s[2] = angular_velocity_rad__s.z;
    d.timestamp = time_100_ns_to_tp(time_100_ns);
    tracker->receive_gyro(std::move(d));
}

void rc_getPose(const rc_Tracker * tracker, rc_Pose pose_m)
{
    pose_m[T0] = tracker->sfm.s.T.v[0];
    pose_m[T1] = tracker->sfm.s.T.v[1];
    pose_m[T2] = tracker->sfm.s.T.v[2];

    m4 R = to_rotation_matrix(tracker->sfm.s.W.v);
    pose_m[0]  = R(0, 0); pose_m[1]  = R(0, 1); pose_m[2]  = R(0, 2);
    pose_m[4]  = R(1, 0); pose_m[5]  = R(1, 1); pose_m[6]  = R(1, 2);
    pose_m[8]  = R(2, 0); pose_m[9]  = R(2, 1); pose_m[10] = R(2, 2);
}

int rc_getFeatures(const rc_Tracker * tracker, rc_Feature **features_px)
{
    int num_features = (int)tracker->sfm.s.features.size();
    *features_px = (rc_Feature *)malloc(num_features*sizeof(rc_Feature));
    int i = 0;
    for(auto f : tracker->sfm.s.features) {
        (*features_px)[i].id = f->id;
        (*features_px)[i].world.x = f->world[0];
        (*features_px)[i].world.y = f->world[1];
        (*features_px)[i].world.z = f->world[2];
        (*features_px)[i].image_x = f->current[0];
        (*features_px)[i].image_y = f->current[1];
        i++;
    }
    return num_features;
}

rc_TrackerState rc_getState(const rc_Tracker *tracker)
{
    switch(tracker->sfm.run_state)
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
    }
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

void rc_setLog(rc_Tracker * tracker, void (*log)(void *handle, const char *buffer_utf8, size_t length), bool stream, rc_Timestamp period_100_ns, void *handle)
{
    tracker->set_log_function(log, stream, std::chrono::duration_cast<sensor_clock::duration>(std::chrono::duration<long long, std::ratio<1, 10000000>>(period_100_ns)), handle);
}

void rc_triggerLog(const rc_Tracker * tracker)
{
    tracker->trigger_log();
}

