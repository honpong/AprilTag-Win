//
//  rc_intel_interface.h
//
//  Created by Eagle Jones on 1/29/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#include "rc_intel_interface.h"
#include "sensor_fusion.h"

struct rc_Tracker: public sensor_fusion
{
};

extern "C" rc_Tracker * rc_create()
{
    return new rc_Tracker;
}

extern "C" void rc_destroy(rc_Tracker * tracker)
{
    delete tracker;
}

extern "C" void rc_reset(rc_Tracker * tracker, rc_Timestamp initialTime_ns, const rc_Pose initialPose_m)
{
    m4 R = m4::Identity();
    v4 T = v4::Zero();
    for(int i = 0; i < 3; ++i) {
        T[i] = initialPose_m[i * 4 + 3];
        for(int j = 0; j < 3; ++ j) {
            R(i, j) = initialPose_m[i * 4 + j];
        }
    }
    tracker->reset(sensor_clock::time_point(std::chrono::nanoseconds(initialTime_ns)),     transformation(R, T));
}

void rc_configureCamera(rc_Tracker * tracker, rc_Camera camera, const rc_Pose pose_m, int width_px, int height_px, float center_x_px, float center_y_px, float focal_length_px)
{
    tracker->device.Cx = center_x_px;
    tracker->device.Cy = center_y_px;
    tracker->device.Fx = tracker->device.Fy = focal_length_px;
    tracker->device.image_width = width_px;
    tracker->device.image_height = height_px;
}


void rc_configureAccelerometer(rc_Tracker * tracker, const rc_Pose pose_m, const rc_Vector bias_m__s2, float noiseVariance_m2__s4);
void rc_configureGyroscope(rc_Tracker * tracker, const rc_Pose pose_m, const rc_Vector bias_rad__s, float noiseVariance_rad2__s2);
void rc_configureLocation(rc_Tracker * tracker, double latitude_deg, double longitude_deg, double altitude_m);

void rc_startCalibration(rc_Tracker * tracker);

void rc_startInertialOnly(rc_Tracker * tracker);

void rc_startTracker(rc_Tracker * tracker);

void rc_stopTracker(rc_Tracker * tracker);

void rc_receiveImage(rc_Tracker * tracker, rc_Camera camera, rc_Timestamp time_ns, rc_Timestamp shutter_time, const rc_Pose poseEstimate_m, bool force_recognition, int stride, const uint8_t *image);
void rc_receiveAccelerometer(rc_Tracker * tracker, rc_Timestamp time_ns, const rc_Vector acceleration_m__s2);
void rc_receiveGyro(rc_Tracker * tracker, rc_Timestamp time_ns, const rc_Vector angular_velocity_rad__s);

void rc_getPose(rc_Tracker * tracker, rc_Pose pose_m);
int rc_getFeatures(rc_Tracker * tracker, struct rc_Feature **features_px);

void rc_setLog(rc_Tracker * tracker, void (*log)(void *handle, char *buffer_utf8, size_t length), bool stream, rc_Timestamp period_ns, void *handle);

void rc_triggerLog(rc_Tracker * tracker);

