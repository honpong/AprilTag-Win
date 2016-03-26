#pragma once

#include "rc_tracker.h"
#include "transformation.h"

struct calibration {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    std::string device_id;
    struct camera {
        std::string name;
        transformation extrinsics_wrt_imu_m;
        struct transformation_var { v3 W, T; } extrinsics_var_wrt_imu_m;
        rc_CameraIntrinsics intrinsics;
    } fisheye, color, depth, ir;
    struct imu {
        m3     w_alignment = m3::Identity(), a_alignment = m3::Identity();
        v3     w_bias_rad__s = v3::Zero(),   a_bias_m__s2 = v3::Zero();
        v3     w_bias_var_rad2__s2,          a_bias_var_m2__s4;
        double w_noise_var_rad2__s2,         a_noise_var_m2__s4;
    } imu;
    transformation device_wrt_imu_m;
    struct frame {
        transformation wrt_device_m;
    } display, global_local_level, opengl, unity;
    struct geo_location {
        double latitude_deg, longitude_deg, altitude_m;
    } geo_location;
};

bool calibration_serialize_xml(const calibration &cal, std::string &xml);
bool calibration_deserialize_xml(const std::string &xml, calibration &cal);
