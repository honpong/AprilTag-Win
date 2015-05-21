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
    tracker->reset(sensor_clock::time_point(sensor_clock::duration(initialTime_100_ns)),     transformation(R, T));
}

void rc_configureCamera(rc_Tracker * tracker, rc_Camera camera, const rc_Pose pose_m, int width_px, int height_px, float center_x_px, float center_y_px, float focal_length_px)
{
    tracker->device.Cx = center_x_px;
    tracker->device.Cy = center_y_px;
    tracker->device.Fx = tracker->device.Fy = focal_length_px;
    tracker->device.image_width = width_px;
    tracker->device.image_height = height_px;
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

void rc_startCalibration(rc_Tracker * tracker);

void rc_startInertialOnly(rc_Tracker * tracker)
{
    tracker->start_inertial_only();
}

void rc_startTracker(rc_Tracker * tracker);

void rc_stopTracker(rc_Tracker * tracker);

void rc_receiveImage(rc_Tracker * tracker, rc_Camera camera, rc_Timestamp time_100_ns, rc_Timestamp shutter_time, const rc_Pose poseEstimate_m, bool force_recognition, int stride, const uint8_t *image)
{
    if(camera == rgb) {
        camera_data d;
        int bytes = stride*tracker->device.image_height;
        d.image_handle = std::unique_ptr<void, void(*)(void *)>(malloc(bytes), free);
        d.image = (uint8_t *)d.image_handle.get();
        memcpy(d.image, image, bytes);
        d.width = tracker->device.image_width;
        d.height = tracker->device.image_height;
        // TODO: Check that we support stride
        d.stride = stride;
        d.timestamp = sensor_clock::time_point(sensor_clock::duration(time_100_ns));
        tracker->receive_image(std::move(d));
    }
}

void rc_receiveAccelerometer(rc_Tracker * tracker, rc_Timestamp time_100_ns, const rc_Vector acceleration_m__s2)
{
    accelerometer_data d;
    d.accel_m__s2[0] = acceleration_m__s2.x;
    d.accel_m__s2[1] = acceleration_m__s2.y;
    d.accel_m__s2[2] = acceleration_m__s2.z;
    d.timestamp = sensor_clock::time_point(sensor_clock::duration(time_100_ns));
    tracker->receive_accelerometer(std::move(d));
}

void rc_receiveGyro(rc_Tracker * tracker, rc_Timestamp time_100_ns, const rc_Vector angular_velocity_rad__s)
{
    gyro_data d;
    d.angvel_rad__s[0] = angular_velocity_rad__s.x;
    d.angvel_rad__s[1] = angular_velocity_rad__s.y;
    d.angvel_rad__s[2] = angular_velocity_rad__s.z;
    d.timestamp = sensor_clock::time_point(sensor_clock::duration(time_100_ns));
    tracker->receive_gyro(std::move(d));
}

void rc_getPose(rc_Tracker * tracker, rc_Pose pose_m)
{
    pose_m[3] = tracker->sfm.s.T.v[0];
    pose_m[7] = tracker->sfm.s.T.v[1];
    pose_m[11] = tracker->sfm.s.T.v[2];

    m4 R = to_rotation_matrix(tracker->sfm.s.W.v);
    pose_m[0]  = R(0, 0); pose_m[1]  = R(0, 1); pose_m[2]  = R(0, 2);
    pose_m[4]  = R(1, 0); pose_m[5]  = R(1, 1); pose_m[6]  = R(1, 2);
    pose_m[8]  = R(2, 0); pose_m[9]  = R(2, 1); pose_m[10] = R(2, 2);
}

int rc_getFeatures(rc_Tracker * tracker, rc_Feature **features_px)
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

void rc_setLog(rc_Tracker * tracker, void (*log)(void *handle, char *buffer_utf8, size_t length), bool stream, rc_Timestamp period_100_ns, void *handle);

void rc_triggerLog(rc_Tracker * tracker);

