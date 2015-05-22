//
//  sensor_fusion.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/2/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//


#include "sensor_fusion.h"
#include "filter.h"

sensor_fusion::sensor_fusion()
{
    isSensorFusionRunning = false;
    isProcessingVideo = false;
    auto cam_fn = [this](camera_data &&data)
    {
        bool docallback = true;
        if(!isSensorFusionRunning)
        {
        } else if(isProcessingVideo) {
            docallback = filter_image_measurement(&sfm, data.image, data.width, data.height, data.stride, data.timestamp);
            if(status_callback) status_callback();
            if(docallback && camera_callback) camera_callback(std::move(data));
        } else {
            //We're not yet processing video, but we do want to send updates for the video preview. Make sure that rotation is initialized.
            docallback =  sfm.gravity_init;
            if(status_callback) status_callback();
            if(docallback && camera_callback) camera_callback(std::move(data));
        }
    };
    
    auto acc_fn = [this](accelerometer_data &&data)
    {
        if(!isSensorFusionRunning) return;
        filter_accelerometer_measurement(&sfm, data.accel_m__s2, data.timestamp);
        if(status_callback) status_callback();
    };
    
    auto gyr_fn = [this](gyro_data &&data)
    {
        if(!isSensorFusionRunning) return;
        filter_gyroscope_measurement(&sfm, data.angvel_rad__s, data.timestamp);
    };
    
    // TODO should this be ELIMINATE_DROPS
    queue = std::make_unique<fusion_queue>(cam_fn, acc_fn, gyr_fn, fusion_queue::latency_strategy::MINIMIZE_DROPS, std::chrono::microseconds(33333), std::chrono::microseconds(10000), std::chrono::microseconds(10000)); //Have to make jitter high - ipad air 2 accelerometer has high latency, we lose about 10% of samples with jitter at 8000
}

void sensor_fusion::set_device(const corvis_device_parameters &dc)
{
    device = dc;
    filter_initialize(&sfm, dc);
}

void sensor_fusion::set_location(double latitude_degrees, double longitude_degrees, double altitude_meters)
{
    queue->dispatch_async([this, latitude_degrees, altitude_meters]{
        filter_compute_gravity(&sfm, latitude_degrees, altitude_meters);
    });
}

void sensor_fusion::start_calibration(bool threaded)
{
    isSensorFusionRunning = true;
    isProcessingVideo = false;
    filter_initialize(&sfm, device);
    filter_start_static_calibration(&sfm);
    if(threaded) queue->start_singlethreaded(false);
    else queue->start_async(false);
}

/*void sensor_fusion::start_inertial_only()
{
    filter_initialize(&sfm, device);
}*/

void sensor_fusion::start(bool threaded, camera_control_interface &cam)
{
    isSensorFusionRunning = true;
    isProcessingVideo = false;
    filter_initialize(&sfm, device);
    filter_start_hold_steady(&sfm);
    if(threaded) queue->start_async(true);
    else queue->start_singlethreaded(true);
}

void sensor_fusion::start_unstable(bool threaded, camera_control_interface &cam)
{
    isSensorFusionRunning = true;
    isProcessingVideo = false;
    filter_initialize(&sfm, device);
    filter_start_dynamic(&sfm);
    if(threaded) queue->start_async(true);
    else queue->start_singlethreaded(true);
}

void sensor_fusion::start_offline()
{
    queue->start_singlethreaded(true);
    sfm.ignore_lateness = true;
    // TODO: Note that we call filter initialize, and this can change
    // device_parameters (specifically a_bias_var and w_bias_var)
    filter_initialize(&sfm, device);
    filter_start_dynamic(&sfm);
    isSensorFusionRunning = true;
    isProcessingVideo = true;
}

void sensor_fusion::stop()
{
    queue->stop_sync();
    isSensorFusionRunning = false;
    isProcessingVideo = false;
}

void sensor_fusion::reset(sensor_clock::time_point time, const transformation &initial_pose_m)
{
    queue->stop_sync();
    filter_initialize(&sfm, device);
}

void sensor_fusion::receive_image(camera_data &&data)
{
    queue->receive_camera(std::move(data));
}

void sensor_fusion::receive_accelerometer(accelerometer_data &&data)
{
    queue->receive_accelerometer(std::move(data));
}

void sensor_fusion::receive_gyro(gyro_data &&data)
{
    queue->receive_gyro(std::move(data));
}

static sensor_clock::time_point last_log;

// TODO: call trigger_log when we update position if stream is enabled
// or if period has expired (the SOW also mentions a trigger condition
// I think?)
void sensor_fusion::trigger_log() const
{
    if(!log_function) return;

    last_log = sensor_clock::now();
    char buffer[1024];
    // TODO: should this be time since epoch to now, even if we are
    // running faster than realtime?
    float seconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>
        (sensor_clock::now().time_since_epoch()).count()/1.e6;
    quaternion q = to_quaternion(sfm.s.W.v);
    //timestamp tx ty tz qx qy qz qw (all floats)
    sprintf_s(buffer, "%.9f %.9f %.9f %.9f %.9f %.9f %.9f %.9f\n", seconds_since_epoch, sfm.s.T.v[0], sfm.s.T.v[1], sfm.s.T.v[2], q.x(), q.y(), q.z(), q.w());
    size_t nbytes = strlen(buffer);
    log_function(log_handle, buffer, nbytes);
}

