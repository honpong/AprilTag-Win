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

typedef enum
{
    DEVICE_TYPE_UNKNOWN,
    DEVICE_TYPE_IPHONE4S,
    DEVICE_TYPE_IPHONE5,
    DEVICE_TYPE_IPHONE5C,
    DEVICE_TYPE_IPHONE5S,
    DEVICE_TYPE_IPHONE6,
    DEVICE_TYPE_IPHONE6PLUS,
    DEVICE_TYPE_IPHONE6S,
    DEVICE_TYPE_IPHONE6SPLUS,
    DEVICE_TYPE_IPHONESE,

    DEVICE_TYPE_IPOD5,
    
    DEVICE_TYPE_IPAD2,
    DEVICE_TYPE_IPAD3,
    DEVICE_TYPE_IPAD4,
    DEVICE_TYPE_IPADAIR,
    DEVICE_TYPE_IPADAIR2,

    DEVICE_TYPE_IPADMINI,
    DEVICE_TYPE_IPADMINIRETINA,
    DEVICE_TYPE_IPADMINIRETINA2,

    DEVICE_TYPE_GIGABYTES11
} corvis_device_type;

bool get_parameters_for_device(corvis_device_type type, calibration_json *dc);
void device_set_resolution(calibration_json *dc, int image_width, int image_height);
