#pragma once
#include <string>
#include <vector>
#include <utility>

#include "rc_tracker.h"

#define CALIBRATION_VERSION 10

template<class T> class sensor_calibration {
    public:
        std::string name;
        rc_Extrinsics extrinsics;
        T intrinsics;
        sensor_calibration(rc_Extrinsics _extrinsics, T _intrinsics) : extrinsics(_extrinsics), intrinsics(_intrinsics) {};
        sensor_calibration() : extrinsics({rc_POSE_IDENTITY}), intrinsics({}) {};
};

typedef sensor_calibration<rc_CameraIntrinsics> sensor_calibration_camera;
typedef sensor_calibration<rc_CameraIntrinsics> sensor_calibration_depth;

struct imu_intrinsics {
    rc_AccelerometerIntrinsics accelerometer;
    rc_GyroscopeIntrinsics gyroscope;
    rc_ThermometerIntrinsics thermometer;
};

typedef sensor_calibration<imu_intrinsics> sensor_calibration_imu;
typedef sensor_calibration<rc_AccelerometerIntrinsics> sensor_calibration_accelerometer;
typedef sensor_calibration<rc_GyroscopeIntrinsics> sensor_calibration_gyroscope;
typedef sensor_calibration<rc_ThermometerIntrinsics> sensor_calibration_thermometer;
typedef sensor_calibration<rc_VelocimeterIntrinsics> sensor_calibration_velocimeter;

struct calibration_coordinates {
    rc_Vector world_up, world_initial_forward, body_forward;
};

struct calibration {
    int version = CALIBRATION_VERSION;
    std::string device_id;
    std::string device_type;
    std::pair<bool, calibration_coordinates> coordinates = {false,{}}; // FIXME: use std::optional
    std::vector<sensor_calibration_imu> imus;
    std::vector<sensor_calibration_velocimeter> velocimeters;
    std::vector<sensor_calibration_camera> cameras;
    std::vector<sensor_calibration_depth> depths;
};

bool calibration_serialize(const calibration &cal, std::string &jsonString);
bool calibration_deserialize(const std::string &jsonString, calibration &cal);
bool calibration_append_deserialize(const std::string &jsonString, calibration &cal);
