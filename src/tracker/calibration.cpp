#include "calibration.h"
// For legacy (version < 10) support
#include "calibration_json.h"

#include "json_keys.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "calibration_keys.h"

#include "rc_compat.h"

#include <iostream>

using namespace rapidjson;
using rc::map;

static sensor_calibration_imu calibration_convert_imu(const struct calibration_xml::imu & legacy_imu)
{
    // In legacy files, extrinsics are always identity for imu
    rc_Extrinsics extrinsics = {rc_POSE_IDENTITY};
    imu_intrinsics intrinsics = {};

    map(intrinsics.accelerometer.scale_and_alignment.v)  = legacy_imu.a_alignment;
    map(intrinsics.accelerometer.bias_m__s2.v)           = legacy_imu.a_bias_m__s2;
    map(intrinsics.accelerometer.bias_variance_m2__s4.v) = legacy_imu.a_bias_var_m2__s4;
    intrinsics.accelerometer.measurement_variance_m2__s4 = legacy_imu.a_noise_var_m2__s4;

    map(intrinsics.gyroscope.scale_and_alignment.v)    = legacy_imu.w_alignment;
    map(intrinsics.gyroscope.bias_rad__s.v)            = legacy_imu.w_bias_rad__s;
    map(intrinsics.gyroscope.bias_variance_rad2__s2.v) = legacy_imu.w_bias_var_rad2__s2;
    intrinsics.gyroscope.measurement_variance_rad2__s2 = legacy_imu.w_noise_var_rad2__s2;

    return sensor_calibration_imu(extrinsics, intrinsics);
}

static sensor_calibration_camera calibration_convert_camera(const struct calibration_xml::camera & legacy_camera)
{
    rc_Extrinsics extrinsics = {};
    extrinsics.pose_m = to_rc_Pose(legacy_camera.extrinsics_wrt_imu_m);
    map(extrinsics.variance_m2.T.v) = legacy_camera.extrinsics_var_wrt_imu_m.T;
    map(extrinsics.variance_m2.W.v) = legacy_camera.extrinsics_var_wrt_imu_m.W;
    return sensor_calibration_camera(extrinsics, legacy_camera.intrinsics);
}

static bool calibration_convert(const calibration_json &cal, calibration &cal_output)
{
    cal_output.version = CALIBRATION_VERSION;
    cal_output.device_id = cal.device_id;
    cal_output.device_type = "legacy";

    cal_output.imus.push_back(calibration_convert_imu(cal.imu));

    if(cal.color.intrinsics.type != rc_CALIBRATION_TYPE_UNKNOWN)
        cal_output.cameras.push_back(calibration_convert_camera(cal.color));

    if(cal.depth.intrinsics.type != rc_CALIBRATION_TYPE_UNKNOWN)
        cal_output.depths.push_back(calibration_convert_camera(cal.depth));

    if (!cal_output.cameras.size()) return false;

    // Convert extrinsics from accelerometer to camera coordinates
    if (cal_output.depths.size())
        cal_output.depths[0].extrinsics.pose_m = to_rc_Pose(invert(to_transformation(cal_output.cameras[0].extrinsics.pose_m)) * to_transformation(cal_output.depths [0].extrinsics.pose_m));
    cal_output.imus      [0].extrinsics.pose_m = to_rc_Pose(invert(to_transformation(cal_output.cameras[0].extrinsics.pose_m)) * to_transformation(cal_output.imus   [0].extrinsics.pose_m));
    cal_output.cameras   [0].extrinsics.pose_m = to_rc_Pose(invert(to_transformation(cal_output.cameras[0].extrinsics.pose_m)) * to_transformation(cal_output.cameras[0].extrinsics.pose_m));

    return true;
}

static void rc_vector_to_json_array(const rc_Vector & v, const char * key, Value & json, Document::AllocatorType& a)
{
    Value json_value(kArrayType);
    for(int i = 0; i < 3; i++) json_value.PushBack(v.v[i], a);
    json.AddMember(StringRef(key), json_value, a); // assumes key will live long enough
}

static void copy_extrinsics_to_json(const rc_Extrinsics & extrinsics, Value & json, Document::AllocatorType& a)
{
    transformation pose = to_transformation(extrinsics.pose_m);
    rc_Vector W; map(W.v) = to_rotation_vector(pose.Q).raw_vector();
    rc_Vector T; map(T.v) = pose.T;
    rc_vector_to_json_array(T, KEY_EXTRINSICS_T, json, a);
    rc_vector_to_json_array(extrinsics.variance_m2.T, KEY_EXTRINSICS_T_VARIANCE, json, a);
    rc_vector_to_json_array(W, KEY_EXTRINSICS_W, json, a);
    rc_vector_to_json_array(extrinsics.variance_m2.W, KEY_EXTRINSICS_W_VARIANCE, json, a);
}

static void copy_camera_to_json(const sensor_calibration_camera & camera, Value & cameras, Document::AllocatorType& a)
{
    Value camera_object(kObjectType);

    Value image_size(kArrayType);
    image_size.PushBack((unsigned)camera.intrinsics.width_px, a);
    image_size.PushBack((unsigned)camera.intrinsics.height_px, a);
    camera_object.AddMember(KEY_CAMERA_IMAGE_SIZE, image_size, a);

    Value center(kArrayType);
    center.PushBack(camera.intrinsics.c_x_px, a);
    center.PushBack(camera.intrinsics.c_y_px, a);
    camera_object.AddMember(KEY_CAMERA_CENTER, center, a);

    Value focal_length(kArrayType);
    focal_length.PushBack(camera.intrinsics.f_x_px, a);
    focal_length.PushBack(camera.intrinsics.f_y_px, a);
    camera_object.AddMember(KEY_CAMERA_FOCAL_LENGTH, focal_length, a);

    Value distortion(kObjectType);
    {
        Value distortion_type(kStringType);
        if(camera.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE) {
            distortion_type = "fisheye";
            distortion.AddMember(KEY_CAMERA_DISTORTION_W, camera.intrinsics.w, a);
        }
        else if(camera.intrinsics.type == rc_CALIBRATION_TYPE_POLYNOMIAL3) {
            distortion_type = "polynomial";
            Value distortion_k(kArrayType);
            for(int i = 0; i < 3; i++) distortion_k.PushBack(camera.intrinsics.distortion[i], a);
            distortion.AddMember(KEY_CAMERA_DISTORTION_K, distortion_k, a);
        }
        else if (camera.intrinsics.type == rc_CALIBRATION_TYPE_KANNALA_BRANDT4) {
            distortion_type = "kannalabrandt4";
            Value distortion_k(kArrayType);
            for (int i = 0; i < 4; i++) distortion_k.PushBack(camera.intrinsics.distortion[i], a);
            distortion.AddMember(KEY_CAMERA_DISTORTION_K, distortion_k, a);
        }
        else if(camera.intrinsics.type == rc_CALIBRATION_TYPE_UNDISTORTED)
            distortion_type = "undistorted";
        else
            distortion_type = "unknown";
        distortion.AddMember(KEY_CAMERA_DISTORTION_TYPE, distortion_type, a);
    }

    camera_object.AddMember(KEY_CAMERA_DISTORTION, distortion, a);

    Value extrinsics(kObjectType);
    copy_extrinsics_to_json(camera.extrinsics, extrinsics, a);
    camera_object.AddMember(KEY_EXTRINSICS, extrinsics, a);

    cameras.PushBack(camera_object, a);
}

static void copy_imu_to_json(const sensor_calibration_imu & imu, Value & imus, Document::AllocatorType& a)
{
    Value imu_object(kObjectType);

    Value accelerometer(kObjectType);
    {
        Value scale_and_alignment(kArrayType);
        for(int r = 0; r < 3; r++)
            for(int c = 0; c < 3; c++)
                scale_and_alignment.PushBack(imu.intrinsics.accelerometer.scale_and_alignment.v[r][c], a);
        accelerometer.AddMember(KEY_SCALE_AND_ALIGNMENT, scale_and_alignment, a);

        Value bias(kArrayType);
        for(int i = 0; i < 3; i++) bias.PushBack(imu.intrinsics.accelerometer.bias_m__s2.v[i], a);
        accelerometer.AddMember(KEY_IMU_BIAS, bias, a);

        Value bias_variance(kArrayType);
        for(int i = 0; i < 3; i++) bias_variance.PushBack(imu.intrinsics.accelerometer.bias_variance_m2__s4.v[i], a);
        accelerometer.AddMember(KEY_IMU_BIAS_VARIANCE, bias_variance, a);

        accelerometer.AddMember(KEY_NOISE_VARIANCE, imu.intrinsics.accelerometer.measurement_variance_m2__s4, a);

        if (imu.intrinsics.accelerometer.decimate_by > 1)
            accelerometer.AddMember(KEY_DECIMATE_BY, (unsigned)imu.intrinsics.accelerometer.decimate_by, a);
    }
    imu_object.AddMember(KEY_IMU_ACCELEROMETER, accelerometer, a);

    Value gyroscope(kObjectType);
    {
        Value scale_and_alignment(kArrayType);
        for(int r = 0; r < 3; r++)
            for(int c = 0; c < 3; c++)
                scale_and_alignment.PushBack(imu.intrinsics.gyroscope.scale_and_alignment.v[r][c], a);
        gyroscope.AddMember(KEY_SCALE_AND_ALIGNMENT, scale_and_alignment, a);

        Value bias(kArrayType);
        for(int i = 0; i < 3; i++) bias.PushBack(imu.intrinsics.gyroscope.bias_rad__s.v[i], a);
        gyroscope.AddMember(KEY_IMU_BIAS, bias, a);

        Value bias_variance(kArrayType);
        for(int i = 0; i < 3; i++) bias_variance.PushBack(imu.intrinsics.gyroscope.bias_variance_rad2__s2.v[i], a);
        gyroscope.AddMember(KEY_IMU_BIAS_VARIANCE, bias_variance, a);

        gyroscope.AddMember(KEY_NOISE_VARIANCE, imu.intrinsics.gyroscope.measurement_variance_rad2__s2, a);

        if (imu.intrinsics.gyroscope.decimate_by > 1)
            gyroscope.AddMember(KEY_DECIMATE_BY, (unsigned)imu.intrinsics.gyroscope.decimate_by, a);
    }
    imu_object.AddMember(KEY_IMU_GYROSCOPE, gyroscope, a);

    if (imu.intrinsics.thermometer.measurement_variance_C2) {
        Value thermometer(kObjectType);

        auto &t = imu.intrinsics.thermometer;
        thermometer.AddMember(KEY_SCALE_AND_ALIGNMENT, t.scale_and_alignment, a);
        thermometer.AddMember(KEY_IMU_BIAS,                t.bias_C, a);
        thermometer.AddMember(KEY_IMU_BIAS_VARIANCE,       t.bias_variance_C2, a);
        thermometer.AddMember(KEY_NOISE_VARIANCE,      t.measurement_variance_C2, a);
        imu_object.AddMember(KEY_IMU_THERMOMETER, thermometer, a);
    }

    Value extrinsics(kObjectType);
    copy_extrinsics_to_json(imu.extrinsics, extrinsics, a);
    imu_object.AddMember(KEY_EXTRINSICS, extrinsics, a);

    imus.PushBack(imu_object, a);
}

static void copy_velocimeter_to_json(const sensor_calibration_velocimeter & velocimeter, Value & velocimeters, Document::AllocatorType& a)
{
    Value velocimeter_object(kObjectType);

    Value scale_and_alignment(kArrayType);
    for(int r = 0; r < 3; r++)
        for(int c = 0; c < 3; c++)
            scale_and_alignment.PushBack(velocimeter.intrinsics.scale_and_alignment.v[r][c], a);
    velocimeter_object.AddMember(KEY_SCALE_AND_ALIGNMENT, scale_and_alignment, a);

    velocimeter_object.AddMember(KEY_NOISE_VARIANCE, velocimeter.intrinsics.measurement_variance_m2__s2, a);

    if (velocimeter.intrinsics.decimate_by > 1)
        velocimeter_object.AddMember(KEY_DECIMATE_BY, (unsigned)velocimeter.intrinsics.decimate_by, a);

    Value extrinsics(kObjectType);
    copy_extrinsics_to_json(velocimeter.extrinsics, extrinsics, a);
    velocimeter_object.AddMember(KEY_EXTRINSICS, extrinsics, a);

    velocimeters.PushBack(velocimeter_object, a);
}

static void copy_coordiates_to_json(const calibration_coordinates &coords, Value &coordinates, Document::AllocatorType &a)
{
    rc_vector_to_json_array(coords.world_up,              KEY_WORLD_UP,              coordinates, a);
    rc_vector_to_json_array(coords.world_initial_forward, KEY_WORLD_INITIAL_FORWARD, coordinates, a);
    rc_vector_to_json_array(coords.body_forward,          KEY_BODY_FORWARD,          coordinates, a);
}

static void copy_calibration_to_json(const calibration &cal, Value & json, Document::AllocatorType& a)
{
    Value id(kStringType);
    id.SetString(cal.device_id.c_str(), a);
    json.AddMember(KEY_DEVICE_ID, id, a);

    Value type(kStringType);
    type.SetString(cal.device_type.c_str(), a);
    json.AddMember(KEY_DEVICE_TYPE, type, a);

    json.AddMember(KEY_VERSION, CALIBRATION_VERSION, a);

    Value cameras(kArrayType);
    for(const auto &camera : cal.cameras) copy_camera_to_json(camera, cameras, a);
    json.AddMember(KEY_CAMERAS, cameras, a);

    Value depths(kArrayType);
    for(const auto &depth : cal.depths) copy_camera_to_json(depth, depths, a);
    json.AddMember(KEY_DEPTHS, depths, a);

    Value imus(kArrayType);
    for(const auto &imu : cal.imus) copy_imu_to_json(imu, imus, a);
    json.AddMember(KEY_IMUS, imus, a);

    Value velocimeters(kArrayType);
    for(const auto &velocimeter : cal.velocimeters) copy_velocimeter_to_json(velocimeter, velocimeters, a);
    json.AddMember(KEY_VELOCIMETERS, velocimeters, a);

    if ((true)/*cal.coordinates.first*/) {
        Value coordinates(kObjectType);
        copy_coordiates_to_json(cal.coordinates.second, coordinates, a);
        json.AddMember(KEY_COORDINATES, coordinates, a);
    }
}

static bool require_key(const Value &json, const char * KEY)
{
    if(!json.HasMember(KEY)) {
        fprintf(stderr, "Error: Required key %s not found\n", KEY);
        return false;
    }
    return true;
}

static bool require_keys(const Value &json, std::vector<const char *> keys)
{
    for(const auto & key : keys)
        if(!require_key(json, key))
            return false;
    return true;
}

static bool copy_json_to_rc_matrix(Value & json, rc_Matrix & m)
{
    if(!json.IsArray() || json.Size() != 9) {
        fprintf(stderr, "Error: problem converting to an rc_Vector\n");
        return false;
    }
    for(int r = 0; r < 3; r++)
        for(int c = 0; c < 3; c++)
            m.v[r][c] = json[r*3+c].GetDouble();

    return true;
}

static bool copy_json_to_rc_vector(Value & json, rc_Vector & v)
{
    if(!json.IsArray() || json.Size() != 3) {
        fprintf(stderr, "Error: problem converting to an rc_Vector\n");
        return false;
    }
    for(int i = 0; i < 3; i++)
        v.v[i] = json[i].GetDouble();

    return true;
}

static bool copy_json_to_extrinsics(Value & json, rc_Extrinsics & extrinsics)
{
    if(!json.IsObject()) {
        fprintf(stderr, "Error: extrinsic is not an object\n");
        return false;
    }
    if(!require_keys(json, {KEY_EXTRINSICS_T, KEY_EXTRINSICS_W,
                KEY_EXTRINSICS_T_VARIANCE, KEY_EXTRINSICS_W_VARIANCE}))
        return false;

    rc_Vector W, T; // FIXME the JSON should use Q, T
    if(!copy_json_to_rc_vector(json[KEY_EXTRINSICS_T], T) ||
       !copy_json_to_rc_vector(json[KEY_EXTRINSICS_W], W) ||
       !copy_json_to_rc_vector(json[KEY_EXTRINSICS_T_VARIANCE], extrinsics.variance_m2.T) ||
       !copy_json_to_rc_vector(json[KEY_EXTRINSICS_W_VARIANCE], extrinsics.variance_m2.W))
        return false;

    extrinsics.pose_m = to_rc_Pose(transformation(to_quaternion(rotation_vector(W.x,W.y,W.z)), map(T.v)));

    return true;
}


static bool copy_json_to_velocimeter(Value & json, sensor_calibration_velocimeter & velocimeter)
{
    if(!json.IsObject()) {
        fprintf(stderr, "Error: velocimeter is not an object\n");
        return false;
    }
    if(!require_keys(json, {KEY_SCALE_AND_ALIGNMENT, KEY_NOISE_VARIANCE}))
        return false;

    if(!copy_json_to_rc_matrix(json[KEY_SCALE_AND_ALIGNMENT], velocimeter.intrinsics.scale_and_alignment))
        return false;

    if(!json[KEY_NOISE_VARIANCE].IsDouble()) {
        fprintf(stderr, "Error: measurement noise should be a double\n");
        return false;
    }
    velocimeter.intrinsics.measurement_variance_m2__s2 = json[KEY_NOISE_VARIANCE].GetDouble();

    copy_json_to_extrinsics(json[KEY_EXTRINSICS], velocimeter.extrinsics);

    return true;
}

static bool copy_json_to_velocimeters(Value & json, std::vector<sensor_calibration_velocimeter> & velocimeters)
{
    if(!json.IsArray()) {
        fprintf(stderr, "Error: velocimeters is not an array\n");
        return false;
    }

    for(size_t i = 0; i < json.Size(); i++) {
        sensor_calibration_velocimeter velocimeter;
        if(!copy_json_to_velocimeter(json[i], velocimeter))
            return false;

        velocimeters.push_back(velocimeter);
    }
    return true;
}

static bool copy_json_to_gyroscope(Value & json, rc_GyroscopeIntrinsics & gyroscope)
{
    if(!json.IsObject()) {
        fprintf(stderr, "Error: gyroscope is not an object\n");
        return false;
    }
    if(!require_keys(json, {KEY_SCALE_AND_ALIGNMENT, KEY_IMU_BIAS,
                KEY_IMU_BIAS_VARIANCE, KEY_NOISE_VARIANCE}))
        return false;

    if(!copy_json_to_rc_matrix(json[KEY_SCALE_AND_ALIGNMENT], gyroscope.scale_and_alignment) ||
       !copy_json_to_rc_vector(json[KEY_IMU_BIAS], gyroscope.bias_rad__s) ||
       !copy_json_to_rc_vector(json[KEY_IMU_BIAS_VARIANCE], gyroscope.bias_variance_rad2__s2))
        return false;

    if(!json[KEY_NOISE_VARIANCE].IsDouble()) {
        fprintf(stderr, "Error: measurement noise should be a double\n");
        return false;
    }

    gyroscope.measurement_variance_rad2__s2 = json[KEY_NOISE_VARIANCE].GetDouble();

    if (json.HasMember(KEY_DECIMATE_BY) && json[KEY_DECIMATE_BY].IsInt())
        gyroscope.decimate_by = json[KEY_DECIMATE_BY].GetInt();

    return true;
}


static bool copy_json_to_accelerometer(Value & json, rc_AccelerometerIntrinsics & accelerometer)
{
    if(!json.IsObject()) {
        fprintf(stderr, "Error: accelerometer is not an object\n");
        return false;
    }
    if(!require_keys(json, {KEY_SCALE_AND_ALIGNMENT, KEY_IMU_BIAS,
                KEY_IMU_BIAS_VARIANCE, KEY_NOISE_VARIANCE}))
        return false;

    if(!copy_json_to_rc_matrix(json[KEY_SCALE_AND_ALIGNMENT], accelerometer.scale_and_alignment) ||
       !copy_json_to_rc_vector(json[KEY_IMU_BIAS], accelerometer.bias_m__s2) ||
       !copy_json_to_rc_vector(json[KEY_IMU_BIAS_VARIANCE], accelerometer.bias_variance_m2__s4))
        return false;

    if(!json[KEY_NOISE_VARIANCE].IsDouble()) {
        fprintf(stderr, "Error: measurement noise should be a double\n");
        return false;
    }

    accelerometer.measurement_variance_m2__s4 = json[KEY_NOISE_VARIANCE].GetDouble();

    if (json.HasMember(KEY_DECIMATE_BY) && json[KEY_DECIMATE_BY].IsInt())
        accelerometer.decimate_by = json[KEY_DECIMATE_BY].GetInt();

    return true;
}

static bool copy_json_to_thermometer(Value &json, rc_ThermometerIntrinsics &thermometer)
{
    if (!json.IsObject()) {
        fprintf(stderr, "Error: thermometer is not an object\n");
        return false;
    }

    if (json.HasMember(KEY_NOISE_VARIANCE)) {
        if(json[KEY_NOISE_VARIANCE].IsDouble())
            thermometer.measurement_variance_C2 = json[KEY_NOISE_VARIANCE].GetDouble();
        else {
            fprintf(stderr, "Error: measurement noise should be a double\n");
            return false;
        }
    }

    return true;
}

static bool copy_json_to_imus(Value & json, std::vector<sensor_calibration_imu> & imus)
{
    if(!json.IsArray()) {
        fprintf(stderr, "Error: imus is not an array\n");
        return false;
    }

    for(size_t i = 0; i < json.Size(); i++) {
        sensor_calibration_imu imu;
        Value & json_imu = json[i];
        if(!json_imu.IsObject()) {
            fprintf(stderr, "Error: imu array element is not an object\n");
            return false;
        }
        if(!require_keys(json_imu, {KEY_EXTRINSICS, KEY_IMU_ACCELEROMETER, KEY_IMU_GYROSCOPE})) return false;

        copy_json_to_accelerometer(json_imu[KEY_IMU_ACCELEROMETER], imu.intrinsics.accelerometer);
        copy_json_to_gyroscope(json_imu[KEY_IMU_GYROSCOPE], imu.intrinsics.gyroscope);
        if(json_imu.HasMember(KEY_IMU_THERMOMETER))
            copy_json_to_thermometer(json_imu[KEY_IMU_THERMOMETER], imu.intrinsics.thermometer);
        copy_json_to_extrinsics(json_imu[KEY_EXTRINSICS], imu.extrinsics);
        imus.push_back(imu);
    }
    return true;
}

static bool copy_json_to_two_elements_uint32(Value & json, const char * key, uint32_t & val0, uint32_t & val1)
{
    if(!json[key].IsArray() || json[key].Size() != 2) {
        fprintf(stderr, "Error: %s is not a 2 element array\n", key);
        return false;
    }
    val0 = json[key][0].GetInt();
    val1 = json[key][1].GetInt();
    return true;
}

static bool copy_json_to_two_elements_double(Value & json, const char * key, double & val0, double & val1)
{
    if(!json[key].IsArray() || json[key].Size() != 2) {
        fprintf(stderr, "Error: %s is not a 2 element array\n", key);
        return false;
    }
    val0 = json[key][0].GetDouble();
    val1 = json[key][1].GetDouble();
    return true;
}

static bool copy_json_to_camera(Value & json, sensor_calibration_camera & camera)
{
    if(!json.IsObject()) {
        fprintf(stderr, "Error: camera is not an object\n");
        return false;
    }

    if(!require_keys(json, {KEY_EXTRINSICS, KEY_CAMERA_IMAGE_SIZE, KEY_CAMERA_CENTER,
               KEY_CAMERA_FOCAL_LENGTH, KEY_CAMERA_DISTORTION})) return false;

    if(!copy_json_to_two_elements_uint32(json, KEY_CAMERA_IMAGE_SIZE, camera.intrinsics.width_px, camera.intrinsics.height_px) ||
       !copy_json_to_two_elements_double(json, KEY_CAMERA_CENTER, camera.intrinsics.c_x_px, camera.intrinsics.c_y_px) ||
       !copy_json_to_two_elements_double(json, KEY_CAMERA_FOCAL_LENGTH, camera.intrinsics.f_x_px, camera.intrinsics.f_y_px))
        return false;

    Value & distortion = json[KEY_CAMERA_DISTORTION];
    if(!distortion.IsObject()) {
        fprintf(stderr, "Error: distortion is not an object\n");
        return false;
    }

    require_key(distortion, KEY_CAMERA_DISTORTION_TYPE);
    std::string type = distortion[KEY_CAMERA_DISTORTION_TYPE].GetString();
    if(type == "fisheye") {
        require_key(distortion, KEY_CAMERA_DISTORTION_W);
        camera.intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
        camera.intrinsics.w = distortion[KEY_CAMERA_DISTORTION_W].GetDouble();
    }
    else if(type == "polynomial") {
        require_key(distortion, KEY_CAMERA_DISTORTION_K);
        camera.intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
        if(distortion[KEY_CAMERA_DISTORTION_K].Size() != 3) {
            fprintf(stderr, "Error: distortion size is not 3, but polynomial was selected\n");
            return false;
        }
        for(int i = 0; i < 3; i++)
            camera.intrinsics.distortion[i] = distortion[KEY_CAMERA_DISTORTION_K][i].GetDouble();
    }
    else if (type == "kannalabrandt4") {
        require_key(distortion, KEY_CAMERA_DISTORTION_K);
        camera.intrinsics.type = rc_CALIBRATION_TYPE_KANNALA_BRANDT4;
        if (distortion[KEY_CAMERA_DISTORTION_K].Size() != 4) {
            fprintf(stderr, "Error: distortion size is not 4, but kannalabrandt4 was selected\n");
            return false;
        }
        for (int i = 0; i < 4; i++)
            camera.intrinsics.distortion[i] = distortion[KEY_CAMERA_DISTORTION_K][i].GetDouble();
    }
    else if(type == "undistorted") {
        camera.intrinsics.type = rc_CALIBRATION_TYPE_UNDISTORTED;
    }
    else {
        fprintf(stderr, "Error: unknown distortion type\n");
        camera.intrinsics.type = rc_CALIBRATION_TYPE_UNKNOWN;
        return false;
    }

    copy_json_to_extrinsics(json[KEY_EXTRINSICS], camera.extrinsics);
    return true;
}

static bool copy_json_to_cameras(Value & json, std::vector<sensor_calibration_camera> & cameras)
{
    if(!json.IsArray()) {
        fprintf(stderr, "Error: cameras or depths is not an array\n");
        return false;
    }

    for(size_t i = 0; i < json.Size(); i++) {
        sensor_calibration_camera camera;
        if(!copy_json_to_camera(json[i], camera))
            return false;

        cameras.push_back(camera);
    }
    return true;
}

static bool copy_json_to_coordinates(Value & json,  calibration_coordinates &coords)
{
    if (json.HasMember(KEY_WORLD_UP))              if (!copy_json_to_rc_vector(json[KEY_WORLD_UP],              coords.world_up))              return false;
    if (json.HasMember(KEY_WORLD_INITIAL_FORWARD)) if (!copy_json_to_rc_vector(json[KEY_WORLD_INITIAL_FORWARD], coords.world_initial_forward)) return false;
    if (json.HasMember(KEY_BODY_FORWARD))          if (!copy_json_to_rc_vector(json[KEY_BODY_FORWARD],          coords.body_forward))          return false;
    return true;
}

static bool copy_json_to_append_calibration(Value & json, calibration &cal)
{
    if(json.HasMember(KEY_VELOCIMETERS))
        if(!copy_json_to_velocimeters(json[KEY_VELOCIMETERS], cal.velocimeters))
            return false;

    if(json.HasMember(KEY_COORDINATES))
        if (!(cal.coordinates.first = copy_json_to_coordinates(json[KEY_COORDINATES], cal.coordinates.second)))
           return false;

    return true;
}

static bool copy_json_to_calibration(Value & json, calibration & cal)
{
    if(!require_keys(json, {KEY_DEVICE_ID, KEY_DEVICE_TYPE, KEY_VERSION, KEY_CAMERAS, KEY_DEPTHS, KEY_IMUS}))
        return false;

    cal.device_id = json[KEY_DEVICE_ID].GetString();
    cal.device_type = json[KEY_DEVICE_TYPE].GetString();
    cal.version = json[KEY_VERSION].GetInt();
    if(!copy_json_to_cameras(json[KEY_CAMERAS], cal.cameras))
        return false;

    if(!copy_json_to_cameras(json[KEY_DEPTHS], cal.depths))
        return false;

    if(!copy_json_to_imus(json[KEY_IMUS], cal.imus))
        return false;

    return copy_json_to_append_calibration(json, cal);
}

bool calibration_serialize(const calibration &cal, std::string &jsonString)
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

bool calibration_deserialize(const std::string &jsonString, calibration &cal)
{
    Document json;
    if (json.Parse(jsonString.c_str()).HasParseError()) {
        std::cout << "JSON parse error (" << json.GetParseError() << ") at offset " << json.GetErrorOffset() << "\n";
        return false;
    }

    int version = 0;
    if (json.HasMember(KEY_CALIBRATION_VERSION)) version = json[KEY_CALIBRATION_VERSION].GetInt();
    if (json.HasMember(KEY_VERSION)) version = json[KEY_VERSION].GetInt();
    if(!version) {
        fprintf(stderr, "Error: no calibration version\n");
        return false;
    }

    if(version < 10) {
        calibration_json legacy_calibration;
        bool success = calibration_deserialize(jsonString, legacy_calibration);
        if(success)
            success = calibration_convert(legacy_calibration, cal);
        return success;
    }

    return copy_json_to_calibration(json, cal);
}

bool calibration_append_deserialize(const std::string &jsonString, calibration &cal)
{
    Document json;
    if (json.Parse(jsonString.c_str()).HasParseError()) {
        std::cout << "JSON parse error (" << json.GetParseError() << ") at offset " << json.GetErrorOffset() << "\n";
        return false;
    }

    return copy_json_to_append_calibration(json, cal);
}
