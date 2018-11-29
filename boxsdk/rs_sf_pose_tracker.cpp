/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include "rs_sf_pose_tracker.h"

static float g_scene_quality = -2.0;
float get_last_failed_sp_quality(void)
{
    return g_scene_quality;
}


#ifdef libSP_FOUND
#include "SP_Core.h"
static int64_t g_frame_number = 0;
static SP_TRACKING_ACCURACY g_tracking_status = SP_TRACKING_ACCURACY::FAILED;
static SP_CameraIntrinsics g_camera_parameters = {};
static const uint64_t g_timestamp_increment = 33000;

void reset_tracking_if_good_input(unsigned short* depth_data, unsigned char* color_data, bool& reset_request)
{
    const float min_acceptable_depth_quality = 0.2f;
    //const float volumeDimension = 4.0f;
    if (!reset_request)
    {
        g_scene_quality = SP_getSceneQuality(depth_data);
        if (g_scene_quality <= min_acceptable_depth_quality)
            return;
    }

    // reset depth fusion
    SP_InputStream stream = {};
    stream.pDepthSrc = depth_data; 
    stream.pFisheyeOrRGBSrc = color_data;
    stream.depthTimestamp = g_frame_number = 0;
    stream.fisheyeOrRGBTimestamp = g_frame_number;

    const float init_pose[12] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    auto status = SP_reset(reinterpret_cast<const float(*)[12]>(init_pose), &stream);
    reset_request = false;
    return;
}

bool _setup_scene_perception(const SP_CameraIntrinsics& camera_parameters, rs_sf_pose_track_resolution resolution)
{
    SP_create();
    //SP_setInertialSupport(0);
    //SP_setGravitySupport(0);
    //SP_setDoSceneReconstruction(1);

    float extrinsics_translation[] = { 0.0f, 0.0f, 0.0f }; // image aligned;
    g_camera_parameters = camera_parameters;
    SP_STATUS SPErr = static_cast<SP_STATUS>(SP_setConfiguration(
        &g_camera_parameters, /*&camera_parameters, extrinsics_translation,*/ (SP_Resolution)resolution /*volume resolution*/));
    if (SPErr != SP_STATUS_SUCCESS)
    {
        return false;
    }

    return true;
}

bool rs_sf_setup_scene_perception(float rfx, float rfy, float rpx, float rpy,
    unsigned int rw, unsigned int rh, int& target_width, int& target_height, rs_sf_pose_track_resolution resolution)
{
    SP_CameraIntrinsics sp_cam_intrinsics = {};

    sp_cam_intrinsics.distortionType = SP_UNDISTORTED;
    sp_cam_intrinsics.focalLengthHorizontal = rfx;
    sp_cam_intrinsics.focalLengthVertical = rfy;
    sp_cam_intrinsics.imageWidth = rw;
    sp_cam_intrinsics.imageHeight = rh;
    sp_cam_intrinsics.principalPointCoordU = rpx;
    sp_cam_intrinsics.principalPointCoordV = rpy;

    if (_setup_scene_perception(sp_cam_intrinsics, resolution))
    {
        SP_getInternalCameraIntrinsics(&sp_cam_intrinsics);
        target_width = sp_cam_intrinsics.imageWidth;
        target_height = sp_cam_intrinsics.imageHeight;
        return true;
    }

    return false;
}

bool rs_sf_do_scene_perception_tracking(unsigned short* depth_data, unsigned char* color_data, bool& reset_request, float cam_pose[12])
{

    if (reset_request || (SP_TRACKING_ACCURACY::FAILED == g_tracking_status && g_frame_number > g_timestamp_increment * 15))
    {
        reset_tracking_if_good_input(depth_data, color_data, reset_request);
    }

    SP_InputStream inputStream = {};
    inputStream.pDepthSrc = depth_data;
    inputStream.pFisheyeOrRGBSrc = color_data;
    g_frame_number += g_timestamp_increment;
    inputStream.depthTimestamp = g_frame_number;
    inputStream.fisheyeOrRGBTimestamp = g_frame_number;

    SP_TRACKING_ACCURACY newTrackingStatus;
    SP_STATUS callStatus = static_cast<SP_STATUS>(SP_doTracking(&newTrackingStatus, &inputStream));

    if (callStatus != SP_STATUS_SUCCESS)
        newTrackingStatus = SP_TRACKING_ACCURACY::FAILED;

    // store latest pose updated by SP_doTracking
    if (newTrackingStatus != SP_TRACKING_ACCURACY::FAILED && cam_pose != nullptr)
    {
        SP_getCameraPose((float(*)[12])cam_pose);
    }

    // output
    g_tracking_status = newTrackingStatus;
    return SP_TRACKING_ACCURACY::FAILED != g_tracking_status;
}

bool rs_sf_do_scene_perception_ray_casting(int image_width, int image_height, unsigned short* dst_depth_data, std::unique_ptr<float[]>& buffer)
{
    if (SP_TRACKING_ACCURACY::FAILED != g_tracking_status)
    {
        const int img_size = image_width*image_height;
        float* vertices_image = buffer.get();
        if (vertices_image == nullptr)
        {
            vertices_image = new float[3 * img_size];
            buffer.reset(vertices_image);
        }

        // store latest pose updated by SP_doTracking
        float pose[12];
        SP_getCameraPose(&pose);

        const float posemi20 = pose[2] * 1000.0f;
        const float posemi21 = pose[6] * 1000.0f;
        const float posemi22 = pose[10] * 1000.0f;
        const float posemi23 = -1000.0f* ((pose[2] * pose[3]) + (pose[6] * pose[7]) + (pose[10] * pose[11]));

        // get raycasted vertices
        //SP_fillDepthImage(dst_depth_data);
        SP_getVerticesImage(vertices_image); //camera u,v are embedding in this matrix

        for (int i = img_size - 1, i3 = i * 3; i >= 0; --i, i3 -= 3)
        {
            const float *const vertex = vertices_image + i3;
            const float vx = vertex[0], vy = vertex[1], vz = vertex[2];
            if (vx != 0 || vy != 0 || vz != 0)
            {
                // model (world) to camera Z
                const float z = posemi20*vx + posemi21*vy + posemi22*vz + posemi23;
                dst_depth_data[i] = static_cast<unsigned short>(z);
            }
            else
            {
                dst_depth_data[i] = 0;
            }
        }
    }

    return SP_TRACKING_ACCURACY::FAILED != g_tracking_status;
}

void rs_sf_pose_tracking_release()
{
    SP_release();
}

#else


#ifdef RC_TRACKER

#include <rc_tracker.h>
#include <fstream>

struct rc_imu_camera_tracker : public rs2::camera_imu_tracker
{
    std::unique_ptr<rc_Tracker,void(*)(rc_Tracker*)> _tracker;
    typedef std::shared_ptr<rs_sf_data> data_packet;
    
    bool init(const std::string& calibration_file) override
    {
        if(_tracker!=nullptr){ return false; }
        _tracker = std::unique_ptr<rc_Tracker,void(*)(rc_Tracker*)>(rc_create(), rc_destroy);
        
        std::ifstream json_file;
        json_file.open(calibration_file, std::ios_base::in);

        if (!json_file.is_open() || !rc_setCalibration(_tracker.get(), (const char *)json_file.rdbuf()))
        {
            fprintf(stderr, "Error: failed to load JSON calibration...\n");
            return false;
        }
        
        json_file.close();
        return true;
    }
    
    bool process(data_packet& data) override
    {
        switch(data->sensor_type)
        {
            case RS_SF_SENSOR_DEPTH:
            case RS_SF_SENSOR_DEPTH_LASER_OFF:
                rc_receiveImage(_tracker.get(), 0, rc_FORMAT_DEPTH16, data->timestamp_us, 0,
                                data->image.img_w, data->image.img_h, data->image.img_w, data->image.data,
                                [](void* ptr){ ((data_packet*)ptr)->reset(); }, new data_packet(data));
                break;
            case RS_SF_SENSOR_INFRARED:
                break;
            case RS_SF_SENSOR_INFRARED_LASER_OFF:
                rc_receiveImage(_tracker.get(), 0, rc_FORMAT_GRAY8, data->timestamp_us, 0,
                                data->image.img_w, data->image.img_h, data->image.img_w, data->image.data,
                                [](void* ptr){ ((data_packet*)ptr)->reset(); }, new data_packet(data));
                break;
            case RS_SF_SENSOR_COLOR:
                break;
            case RS_SF_SENSOR_GYRO: {
                const rc_Vector angular_velocity_rad__s = *(rc_Vector*)&data->imu;
                rc_receiveGyro(_tracker.get(), 0, data->timestamp_us, angular_velocity_rad__s);
                break;
            }
            case RS_SF_SENSOR_ACCEL: {
                const rc_Vector acceleration_m__s2 = *(rc_Vector*)&data->imu;
                rc_receiveAccelerometer(_tracker.get(), 0, data->timestamp_us, acceleration_m__s2);
                break;
            }
            default:
                break;
        }
        return true;
    }
    
    void zero_bias()
    {
        for (rc_Sensor id = 0; true; id++) {
            rc_Extrinsics extrinsics;
            rc_AccelerometerIntrinsics intrinsics;
            if (!rc_describeAccelerometer(_tracker.get(), id, &extrinsics, &intrinsics))
                break;
            for (auto &b : intrinsics.bias_m__s2.v) b = 0;
            for (auto &b : intrinsics.bias_variance_m2__s4.v) b = 1e-3f;
            rc_configureAccelerometer(_tracker.get(), id, nullptr, &intrinsics);
        }
        for (rc_Sensor id = 0; true; id++) {
            rc_Extrinsics extrinsics;
            rc_GyroscopeIntrinsics intrinsics;
            if (!rc_describeGyroscope(_tracker.get(), id, &extrinsics, &intrinsics))
                break;
            for (auto &b : intrinsics.bias_rad__s.v) b = 0;
            for (auto &b : intrinsics.bias_variance_rad2__s2.v) b = 1e-4f;
            rc_configureGyroscope(_tracker.get(), id, &extrinsics, &intrinsics);
        }
    }
    
};

#endif


bool rs_sf_setup_scene_perception(
    float rfx, float rfy, float rpx, float rpy, unsigned int rw, unsigned int rh,
    int& target_width,
    int& target_height,
    rs_sf_pose_track_resolution resolution) {
    return false;
}

bool rs_sf_do_scene_perception_tracking(
    unsigned short* depth_data,
    unsigned char* color_data,
    bool& reset_request,
    float cam_pose[12]) {
    return false;
}

bool rs_sf_do_scene_perception_ray_casting(
    int image_width,
    int image_height,
    unsigned short* depth_data,
    std::unique_ptr<float[]>& buf) {
    return false;
}

void rs_sf_pose_tracking_release() {}

#endif
