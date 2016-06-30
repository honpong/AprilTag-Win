#include "calibration_json.h"
#include "calibration_defaults.h"
#include "json_keys.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <functional>
#include <numeric>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

using namespace rapidjson;
using namespace std;

static bool require_key(const Value &json, const char * KEY)
{
    if(!json.HasMember(KEY)) {
        fprintf(stderr, "Error: Required key %s not found\n", KEY);
        return false;
    }
    return true;
}

static void copy_json_to_camera(Value &json, calibration_xml::camera &cam, Document::AllocatorType& a);
static void copy_json_to_imu(Value &json, struct calibration_xml::imu &imu, Document::AllocatorType& a);
static void copy_json_to_calibration(Value &json, calibration_json &cal, Document::AllocatorType& a)
{
    cal = calibration_json{};

    if (json.HasMember(KEY_DEVICE_NAME)) cal.device_id = json[KEY_DEVICE_NAME].GetString();
    if (json.HasMember(KEY_CALIBRATION_VERSION)) cal.version = json[KEY_CALIBRATION_VERSION].GetInt();

    copy_json_to_imu(json, cal.imu, a);
    copy_json_to_camera(json, cal.color, a);
    if (json.HasMember(KEY_DEPTH) && json[KEY_DEPTH].IsObject())
        copy_json_to_camera(json[KEY_DEPTH], cal.depth, a);
}

static void copy_json_to_camera(Value &json, calibration_xml::camera &cam, Document::AllocatorType& a)
{
    if (json.HasMember(KEY_IMAGE_WIDTH))  cam.intrinsics.width_px  = json[KEY_IMAGE_WIDTH].GetInt();
    if (json.HasMember(KEY_IMAGE_HEIGHT)) cam.intrinsics.height_px = json[KEY_IMAGE_HEIGHT].GetInt();
    if (json.HasMember(KEY_FX))           cam.intrinsics.f_x_px    = json[KEY_FX].GetDouble();
    if (json.HasMember(KEY_FY))           cam.intrinsics.f_y_px    = json[KEY_FY].GetDouble();
    if (json.HasMember(KEY_CX))           cam.intrinsics.c_x_px    = json[KEY_CX].GetDouble();
    if (json.HasMember(KEY_CY))           cam.intrinsics.c_y_px    = json[KEY_CY].GetDouble();

    switch (json.HasMember(KEY_DISTORTION_MODEL) ? json[KEY_DISTORTION_MODEL].GetInt() : 0) {
        case 0:
            if (json.HasMember(KEY_K0) && json.HasMember(KEY_K1) && json.HasMember(KEY_K2)) {
                cam.intrinsics.k1 = json[KEY_K0].GetDouble();
                cam.intrinsics.k2 = json[KEY_K1].GetDouble();
                cam.intrinsics.k3 = json[KEY_K2].GetDouble();
                cam.intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
            } else
                cam.intrinsics.type = rc_CALIBRATION_TYPE_UNDISTORTED;
            break;
        case 1:
            if (require_key(json, KEY_KW)) {
                cam.intrinsics.w  = json[KEY_KW].GetDouble();
                cam.intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
            }
            break;
        default:
            cam.intrinsics.type = rc_CALIBRATION_TYPE_UNKNOWN;
            break;
    }

    if (require_key(json, KEY_TC0) && require_key(json, KEY_TC1) && require_key(json, KEY_TC2))
        cam.extrinsics_wrt_imu_m.T = v3(json[KEY_TC0].GetDouble(), json[KEY_TC1].GetDouble(), json[KEY_TC2].GetDouble());

    if (require_key(json, KEY_WC0) && require_key(json, KEY_WC1) && require_key(json, KEY_WC2))
        cam.extrinsics_wrt_imu_m.Q = to_quaternion(rotation_vector(json[KEY_WC0].GetDouble(), json[KEY_WC1].GetDouble(), json[KEY_WC2].GetDouble()));

    if (json.HasMember(KEY_TCVAR0) && json.HasMember(KEY_TCVAR1) && json.HasMember(KEY_TCVAR2))
        cam.extrinsics_var_wrt_imu_m.T = v3(json[KEY_TCVAR0].GetDouble(), json[KEY_TCVAR1].GetDouble(), json[KEY_TCVAR2].GetDouble());

    if (json.HasMember(KEY_WCVAR0) && json.HasMember(KEY_WCVAR1) && json.HasMember(KEY_WCVAR2))
        cam.extrinsics_var_wrt_imu_m.W = v3(json[KEY_WCVAR0].GetDouble(), json[KEY_WCVAR1].GetDouble(), json[KEY_WCVAR2].GetDouble());

}

static void copy_json_to_imu(Value &json, struct calibration_xml::imu &imu, Document::AllocatorType& a)
{
    if (json.HasMember(KEY_ABIAS0) && json.HasMember(KEY_ABIAS1) && json.HasMember(KEY_ABIAS2))
        imu.a_bias_m__s2 = v3(json[KEY_ABIAS0].GetDouble(),
                              json[KEY_ABIAS1].GetDouble(),
                              json[KEY_ABIAS2].GetDouble());

    if (json.HasMember(KEY_WBIAS0) && json.HasMember(KEY_WBIAS1) && json.HasMember(KEY_WBIAS2))
        imu.w_bias_rad__s = v3(json[KEY_WBIAS0].GetDouble(),
                               json[KEY_WBIAS1].GetDouble(),
                               json[KEY_WBIAS2].GetDouble());

    if (json.HasMember(KEY_ABIASVAR0) && json.HasMember(KEY_ABIASVAR1) && json.HasMember(KEY_ABIASVAR2))
        imu.a_bias_var_m2__s4 = v3(json[KEY_ABIASVAR0].GetDouble(),
                                   json[KEY_ABIASVAR1].GetDouble(),
                                   json[KEY_ABIASVAR2].GetDouble());

    if (json.HasMember(KEY_WBIASVAR0) && json.HasMember(KEY_WBIASVAR0) && json.HasMember(KEY_WBIASVAR0))
        imu.w_bias_var_rad2__s2 = v3(json[KEY_WBIASVAR0].GetDouble(),
                                     json[KEY_WBIASVAR1].GetDouble(),
                                     json[KEY_WBIASVAR2].GetDouble());

    if (json.HasMember(KEY_WMEASVAR))
        imu.w_noise_var_rad2__s2 = json[KEY_WMEASVAR].GetDouble();

    if (json.HasMember(KEY_AMEASVAR))
        imu.a_noise_var_m2__s4 = json[KEY_AMEASVAR].GetDouble();

    if (json.HasMember(KEY_ACCEL_TRANSFORM) && json[KEY_ACCEL_TRANSFORM].IsArray() && json[KEY_ACCEL_TRANSFORM].Size() == imu.a_alignment.size())
        for (int i=0; i<imu.a_alignment.size(); i++)
            imu.a_alignment(i) = json[KEY_ACCEL_TRANSFORM][i].GetDouble();

    if (json.HasMember(KEY_GYRO_TRANSFORM) && json[KEY_GYRO_TRANSFORM].IsArray() && json[KEY_GYRO_TRANSFORM].Size() == imu.w_alignment.size())
        for (int i=0; i<imu.a_alignment.size(); i++)
            imu.w_alignment(i) = json[KEY_GYRO_TRANSFORM][i].GetDouble();
}

static void copy_camera_to_json(const calibration_xml::camera &cam, Value &json, Document::AllocatorType& a);
static void copy_imu_to_json(const struct calibration_xml::imu &imu, Value &json, Document::AllocatorType& a);
static void copy_calibration_to_json(const calibration_json &cal, Value &json, Document::AllocatorType& a)
{
    Value name(kStringType);
    name.SetString(cal.device_id.c_str(), a);
    json.AddMember(KEY_DEVICE_NAME, name, a);
    json.AddMember(KEY_CALIBRATION_VERSION, CALIBRATION_VERSION_LEGACY, a);

    copy_imu_to_json(cal.imu, json, a);
    copy_camera_to_json(cal.color, json, a);
    if (cal.depth.intrinsics.type != rc_CALIBRATION_TYPE_UNKNOWN) {
        Value depth(kObjectType);
        copy_camera_to_json(cal.depth, depth, a);
        json.AddMember(KEY_DEPTH, depth, a);
    }
}

static void copy_camera_to_json(const calibration_xml::camera &cam, Value &json, Document::AllocatorType& a)
{
    json.AddMember(KEY_IMAGE_WIDTH, cam.intrinsics.width_px, a);
    json.AddMember(KEY_IMAGE_HEIGHT, cam.intrinsics.height_px, a);
    json.AddMember(KEY_FX, cam.intrinsics.f_x_px, a);
    json.AddMember(KEY_FY, cam.intrinsics.f_y_px, a);
    json.AddMember(KEY_CX, cam.intrinsics.c_x_px, a);
    json.AddMember(KEY_CY, cam.intrinsics.c_y_px, a);
    if (cam.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE) {
        json.AddMember(KEY_KW, cam.intrinsics.w, a);
        json.AddMember(KEY_DISTORTION_MODEL, 1, a);
    } else if (cam.intrinsics.type == rc_CALIBRATION_TYPE_UNDISTORTED) {
        json.AddMember(KEY_DISTORTION_MODEL, 0, a);
    } else {
        json.AddMember(KEY_K0, cam.intrinsics.k1, a);
        json.AddMember(KEY_K1, cam.intrinsics.k2, a);
        json.AddMember(KEY_K2, cam.intrinsics.k3, a);
        json.AddMember(KEY_DISTORTION_MODEL, 0, a);
    }

    json.AddMember(KEY_WC0, to_rotation_vector(cam.extrinsics_wrt_imu_m.Q).raw_vector()[0], a);
    json.AddMember(KEY_WC1, to_rotation_vector(cam.extrinsics_wrt_imu_m.Q).raw_vector()[1], a);
    json.AddMember(KEY_WC2, to_rotation_vector(cam.extrinsics_wrt_imu_m.Q).raw_vector()[2], a);
    json.AddMember(KEY_TC0, cam.extrinsics_wrt_imu_m.T[0], a);
    json.AddMember(KEY_TC1, cam.extrinsics_wrt_imu_m.T[1], a);
    json.AddMember(KEY_TC2, cam.extrinsics_wrt_imu_m.T[2], a);
    json.AddMember(KEY_WCVAR0, cam.extrinsics_var_wrt_imu_m.W[0], a);
    json.AddMember(KEY_WCVAR1, cam.extrinsics_var_wrt_imu_m.W[1], a);
    json.AddMember(KEY_WCVAR2, cam.extrinsics_var_wrt_imu_m.W[2], a);
    json.AddMember(KEY_TCVAR0, cam.extrinsics_var_wrt_imu_m.T[0], a);
    json.AddMember(KEY_TCVAR1, cam.extrinsics_var_wrt_imu_m.T[1], a);
    json.AddMember(KEY_TCVAR2, cam.extrinsics_var_wrt_imu_m.T[2], a);
}

static void copy_imu_to_json(const struct calibration_xml::imu &imu, Value &json, Document::AllocatorType& a)
{
    json.AddMember(KEY_ABIAS0, imu.a_bias_m__s2[0], a);
    json.AddMember(KEY_ABIAS1, imu.a_bias_m__s2[1], a);
    json.AddMember(KEY_ABIAS2, imu.a_bias_m__s2[2], a);
    json.AddMember(KEY_WBIAS0, imu.w_bias_rad__s[0], a);
    json.AddMember(KEY_WBIAS1, imu.w_bias_rad__s[1], a);
    json.AddMember(KEY_WBIAS2, imu.w_bias_rad__s[2], a);
    json.AddMember(KEY_ABIASVAR0, imu.a_bias_var_m2__s4[0],   a);
    json.AddMember(KEY_ABIASVAR1, imu.a_bias_var_m2__s4[1],   a);
    json.AddMember(KEY_ABIASVAR2, imu.a_bias_var_m2__s4[2],   a);
    json.AddMember(KEY_WBIASVAR0, imu.w_bias_var_rad2__s2[0], a);
    json.AddMember(KEY_WBIASVAR1, imu.w_bias_var_rad2__s2[1], a);
    json.AddMember(KEY_WBIASVAR2, imu.w_bias_var_rad2__s2[2], a);
    json.AddMember(KEY_WMEASVAR,  imu.w_noise_var_rad2__s2,   a);
    json.AddMember(KEY_AMEASVAR,  imu.a_noise_var_m2__s4,     a);

    Value accelArray(kArrayType);
    for (auto i=0; i<imu.a_alignment.size(); i++)
        accelArray.PushBack(imu.a_alignment(i), a);

    if (imu.a_alignment != m3::Zero() && imu.a_alignment != m3::Identity())
        json.AddMember(KEY_ACCEL_TRANSFORM, accelArray, a);

    Value gyroArray(kArrayType);
    for (auto i=0; i<imu.w_alignment.size(); i++)
        gyroArray.PushBack(imu.w_alignment(i), a);

    if (imu.w_alignment != m3::Zero() && imu.w_alignment != m3::Identity())
        json.AddMember(KEY_GYRO_TRANSFORM, gyroArray, a);
}

bool calibration_serialize(const calibration_json &cal, std::string &jsonString)
{
    Document json;
    json.SetObject();
    copy_calibration_to_json(cal, json, json.GetAllocator());

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    json.Accept(writer);
    jsonString = buffer.GetString();
    return jsonString.length() > 0;
}

bool calibration_deserialize(const std::string &jsonString, calibration_json &cal)
{
    Document json;
    if (json.Parse(jsonString.c_str()).HasParseError()) {
        std::cout << "JSON parse error (" << json.GetParseError() << ") at offset " << json.GetErrorOffset() << "\n";
        return false;
    }
    copy_json_to_calibration(json, cal, json.GetAllocator());
    return true;
}

void device_set_resolution(calibration_json *dc, int image_width, int image_height)
{
    auto &in = dc->color.intrinsics;
    int max_old_dim = in.width_px > in.height_px ? in.width_px : in.height_px;
    int max_new_dim = image_width > image_height ? image_width : image_height;

    in.width_px = image_width;
    in.height_px = image_height;
    in.c_x_px = (in.width_px - 1)/2.f;
    in.c_x_px = (in.height_px - 1)/2.f;
    // Scale the focal length depending on the resolution
    in.f_x_px = in.f_x_px * max_new_dim / max_old_dim;
    in.f_y_px = in.f_x_px;
}

// TODO: should this go away?
bool get_parameters_for_device(corvis_device_type type, calibration_json *dc)
{
    return calibration_load_defaults(type, *dc);
}
