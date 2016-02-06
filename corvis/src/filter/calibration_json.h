#pragma once
#include <string>
#include "rc_tracker.h"
#include "calibration_xml.h"

#define CALIBRATION_VERSION 8

struct calibration_json {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    int version;
    char device_id[256];
    struct calibration::imu imu;
    struct calibration::camera color, depth;
};

bool calibration_serialize(const calibration_json &cal, std::string &jsonString);
bool calibration_deserialize(const std::string &jsonString, calibration_json &cal);
