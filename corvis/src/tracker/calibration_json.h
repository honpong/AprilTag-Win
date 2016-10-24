#pragma once
#include <string>
#include "rc_tracker.h"
#include "calibration_xml.h"

#define CALIBRATION_VERSION_LEGACY 8

struct calibration_json {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    int version;
    std::string device_id;
    struct calibration_xml::imu imu;
    struct calibration_xml::camera color, depth;
};

bool calibration_serialize(const calibration_json &cal, std::string &jsonString);
bool calibration_deserialize(const std::string &jsonString, calibration_json &cal);
