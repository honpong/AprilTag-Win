/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017-2019 Intel Corporation. All Rights Reserved.

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
#include <iostream>
#include <fstream>
#include <mutex>

static float* operator<<(float* dst, const rc_Pose& src){
    if(!dst){ return dst; }
    dst[0] = src.R.v[0][0]; dst[1] = src.R.v[0][1]; dst[ 2] = src.R.v[0][2]; dst[ 3] = src.T.x;
    dst[4] = src.R.v[1][0]; dst[5] = src.R.v[1][1]; dst[ 6] = src.R.v[1][2]; dst[ 7] = src.T.y;
    dst[8] = src.R.v[2][0]; dst[9] = src.R.v[2][1]; dst[10] = src.R.v[2][2]; dst[11] = src.T.z;
    return dst;
}
// dst = r * src + t
static void transform(float dst[12], const float src[12], const float r[9], const float t[3])
{
    // compute dst image pose
    dst[0] = r[0] * src[0] + r[1] * src[4] + r[2] * src[8];
    dst[1] = r[0] * src[1] + r[1] * src[5] + r[2] * src[9];
    dst[2] = r[0] * src[2] + r[1] * src[6] + r[2] * src[10];
    dst[3] = r[0] * src[3] + r[1] * src[7] + r[2] * src[11] + t[0];
    dst[4] = r[3] * src[0] + r[4] * src[4] + r[5] * src[8];
    dst[5] = r[3] * src[1] + r[4] * src[5] + r[5] * src[9];
    dst[6] = r[3] * src[2] + r[4] * src[6] + r[5] * src[10];
    dst[7] = r[3] * src[3] + r[4] * src[7] + r[5] * src[11] + t[1];
    dst[8] = r[6] * src[0] + r[7] * src[4] + r[8] * src[8];
    dst[9] = r[6] * src[1] + r[7] * src[5] + r[8] * src[9];
    dst[10] = r[6] * src[2] + r[7] * src[6] + r[8] * src[10];
    dst[11] = r[6] * src[3] + r[7] * src[7] + r[8] * src[11] + t[2];
}

static float* operator*=(float dst[12], const rc_Pose& rc_pose)
{
    float src[12]; src << rc_pose;
    float p[12] = {
        dst[0]*src[0]+dst[1]*src[4]+dst[2]*src[8],
        dst[0]*src[1]+dst[1]*src[5]+dst[2]*src[9],
        dst[0]*src[2]+dst[1]*src[6]+dst[2]*src[10],
        dst[0]*src[3]+dst[1]*src[7]+dst[2]*src[11]+dst[3],

        dst[4]*src[0]+dst[5]*src[4]+dst[6]*src[8],
        dst[4]*src[1]+dst[5]*src[5]+dst[6]*src[9],
        dst[4]*src[2]+dst[5]*src[6]+dst[6]*src[10],
        dst[4]*src[3]+dst[5]*src[7]+dst[6]*src[11]+dst[7],
        
        dst[8]*src[0]+dst[9]*src[4]+dst[10]*src[8],
        dst[8]*src[1]+dst[9]*src[5]+dst[10]*src[9],
        dst[8]*src[2]+dst[9]*src[6]+dst[10]*src[10],
        dst[8]*src[3]+dst[9]*src[7]+dst[10]*src[11]+dst[11]
    };
    rs_sf_memcpy(dst, p, sizeof(p));
    return dst;
}

struct rc_imu_camera_tracker : public rs2::camera_imu_tracker
{
    typedef rs_sf_data_ptr data_packet;
    std::unique_ptr<rc_Tracker,decltype(&rc_destroy)> _tracker {nullptr,nullptr};

    //bool is_realtime{ false }, qvga{ false }, async{ false }, use_depth{ true };
    //bool fast_path{ false }, to_zero_biases{ false }, use_odometry{ false }, stereo_configured{ false }, dynamic_calibration{ false };
    
    bool _async{ false };
    bool _fast_path{ false };
    bool _dynamic_calibration{ false };
    rc_TrackerQueueStrategy _queue_strategy{ rc_QUEUE_MINIMIZE_DROPS /*rc_QUEUE_MINIMIZE_LATENCY*/ };
    bool _strategy_override{ false };
    
    virtual ~rc_imu_camera_tracker()
    {
        if(_tracker){ rc_stopTracker(_tracker.get()); }
        _tracker.reset();
    }
    
    bool init(const std::string& calibration_file, bool async) override
    {
        std::ifstream json_file(calibration_file);
        std::string json_str((std::istreambuf_iterator<char>(json_file)), std::istreambuf_iterator<char>());
        if (!json_file.is_open() || json_str.empty()) {
            fprintf(stderr,"Error: failed to open JSON calibration for camera tracker ... \n");
            return false;
        }
        return init(json_str.c_str(), async);
    }
    
    bool init(const char* calibration_data, bool async) override
    {
        if(_tracker!=nullptr){ reset_tracker(); return false; }
        _tracker = std::unique_ptr<rc_Tracker,void(*)(rc_Tracker*)>(rc_create(), rc_destroy);
        
        if(!rc_setCalibration(_tracker.get(), calibration_data)){
            fprintf(stderr, "Error: failed to load JSON calibration into camera tracker ... \n");
            return false;
        }
        
        // setting data callback from the rc tracker
        rc_setDataCallback(_tracker.get(), [](void* handle, rc_Tracker* tracker, const rc_Data* data){
            ((rc_imu_camera_tracker*)handle)->data_callback(tracker,data);}, this);
        
        _async = async;
        return start_tracker();
    }
    
    bool start_tracker()
    {
        return reset_tracker();
    }
    
    bool reset_tracker()
    {
        if(!_tracker){ return false; }
        rc_stopTracker(_tracker.get());
        rc_configureQueueStrategy(_tracker.get(), (_strategy_override) ? _queue_strategy :
                                  (_async ? rc_QUEUE_MINIMIZE_LATENCY : _queue_strategy));
        return rc_startTracker(_tracker.get(),
                               (_async ? rc_RUN_ASYNCHRONOUS : rc_RUN_SYNCHRONOUS) |
                               (_fast_path ? rc_RUN_FAST_PATH : rc_RUN_NO_FAST_PATH) |
                               (_dynamic_calibration ? rc_RUN_DYNAMIC_CALIBRATION : rc_RUN_STATIC_CALIBRATION));
    }
    
    bool process(data_packet& data) override
    {
        if(!_tracker||!data){ return false; }
        rc_Timestamp timestamp_us = static_cast<rc_Timestamp>(data->timestamp_us);
        switch(data->sensor_type)
        {
            case RS_SF_SENSOR_DEPTH:
            case RS_SF_SENSOR_DEPTH_LASER_OFF:
                rc_receiveImage(_tracker.get(), 0, rc_FORMAT_DEPTH16, timestamp_us, 0,
                                data->image.img_w, data->image.img_h, data->image.img_w, data->image.data,
                                [](void* ptr){ delete (data_packet*)ptr; }, new data_packet(data));
                break;
            case RS_SF_SENSOR_INFRARED:
                break;
            case RS_SF_SENSOR_INFRARED_LASER_OFF:
                rc_receiveImage(_tracker.get(), data->sensor_index - 1, rc_FORMAT_GRAY8, timestamp_us, 0,
                                data->image.img_w, data->image.img_h, data->image.img_w, data->image.data,
                                [](void* ptr){ delete (data_packet*)ptr; }, new data_packet(data));
                break;
            case RS_SF_SENSOR_COLOR:
                /*
                rc_receiveImage(_tracker.get(), 2, rc_FORMAT_RGB8, timestamp_us, 0,
                                data->image.img_w, data->image.img_h, data->image.img_w, data->image.data,
                                [](void* ptr){ ((data_packet*)ptr)->reset(); }, new data_packet(data));
                 */
                break;
            case RS_SF_SENSOR_GYRO: {
                const rc_Vector angular_velocity_rad__s = *(rc_Vector*)&data->imu;
                rc_receiveGyro(_tracker.get(), 0, timestamp_us, angular_velocity_rad__s);
                break;
            }
            case RS_SF_SENSOR_ACCEL: {
                const rc_Vector acceleration_m__s2 = *(rc_Vector*)&data->imu;
                rc_receiveAccelerometer(_tracker.get(), 0, timestamp_us, acceleration_m__s2);
                break;
            }
            default:
                break;
        }
        return true;
    }
    
    struct rc_tracker_output : private rc_Data, public rc_PoseTime
    {
        static const rc_DataPath _data_path = rc_DATA_PATH_SLOW;

        rc_TrackerConfidence _confidence;
        float                _path_length;
        rc_PoseVelocity      _pose_velocity;
        rc_PoseAcceleration  _pose_acceleration;
    
        rc_tracker_output(rc_Tracker* tracker, const rc_Data& ref)
        : rc_Data(ref), rc_PoseTime(rc_getPose(tracker, &_pose_velocity, &_pose_acceleration, _data_path))
        {
            _confidence  = rc_getConfidence(tracker);
            _path_length = rc_getPathLength(tracker);
        }
        
        rc_tracker_output() = default;
    };
    
    rc_tracker_output _last_output_pose = {};
    std::mutex        _last_pose_mutex;
    void data_callback(rc_Tracker* tracker, const rc_Data* data)
    {
        if(!data){ return; }
        
        switch(data->type){
            case rc_SENSOR_TYPE_IMAGE:
            case rc_SENSOR_TYPE_GYROSCOPE:
            case rc_SENSOR_TYPE_ACCELEROMETER: {
                std::lock_guard<std::mutex> lk(_last_pose_mutex);
                _last_output_pose = rc_tracker_output(tracker,*data);
                //std::cout << "pose confidence : " << _last_output_pose.load()._confidence << std::endl;
            }   break;
            default:
                // not used
                break;
        }
    }
    
    bool wait_for_image_pose(std::vector<rs_sf_image>& images) override
    {
        rc_tracker_output _pose;
        {
            std::lock_guard<std::mutex> lk(_last_pose_mutex);
            _pose = _last_output_pose;
        }

#ifndef NDEBUG
        std::cout << _pose._confidence << " pose Q: " << _pose.pose_m.Q.x << " " << _pose.pose_m.Q.y << " " << _pose.pose_m.Q.z << " " << _pose.pose_m.Q.w
        << ", T:" << _pose.pose_m.T.x << " " << _pose.pose_m.T.y << " " << _pose.pose_m.T.z << std::endl;
#endif
        
        if(_pose._confidence == rc_E_CONFIDENCE_NONE){ return false; }
        for(auto& img : images){
            img.cam_pose *= _pose.pose_m;
            //img.cam_pose << _pose.pose_m;
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

std::unique_ptr<rs2::camera_imu_tracker> rs2::camera_imu_tracker::create(){
    return std::make_unique<rc_imu_camera_tracker>();
}
#else
std::unique_ptr<rs2::camera_imu_tracker> rs2::camera_imu_tracker::create(){ return nullptr; }
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
