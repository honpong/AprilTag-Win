//
//  sensor_fusion.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/2/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include <future>
#include "sensor_fusion.h"
#include "filter.h"

/*
 The filter has multiple reference frames:
 w: The world reference frame, with z aligned to gravity, and rotation around z arbitrary, and centered at initial accel position
 a: Accelerometer reference frame
 c: Camera reference frame
 w': Camera-centered world reference frame, with alignement as above, but centered around initial camera position
 
 The existing output of the filter is:
 g__w_at = g__w_a0 * g__a0_at
 
 We want instead to have:
 g__w'_ct = g__w'_w * g__w_at * g__a_c
 
 g__w'_w = g__w'_c0 * g__c_a * g__a0_w
 
 both g__w'_c0 and g__a0_w are R-only - respective world centers are at instrument centers, and I want R__w'_w = I
 
 R__w'_c0 * R__c_a * R__a0_w = I
 
 R__w'_c0 = R__w_a0 * R__a_c
 
 so:
 
 g__w'_w = (I, R__w_a0 * R__a_c * T__c_a)
 = (I, R__w_a0 * R__a_c * R__c_a * -T__a_c)
 = (I, R__w_a0 * -T__a_c)
*/
transformation sensor_fusion::accel_to_camera_world_transform() const
{
    return transformation(quaternion(), sfm.s.initial_orientation * -sfm.s.Tc.v);
}

v4 sensor_fusion::accel_to_camera_position(const v4& x) const
{
    return transformation_apply(accel_to_camera_world_transform(), x);
}

v4 sensor_fusion::camera_to_accel_position(const v4& x) const
{
    return transformation_apply(invert(accel_to_camera_world_transform()), x);
}

transformation sensor_fusion::accel_to_camera_transformation(const transformation &x) const
{
    transformation cam_transform(to_quaternion(sfm.s.Wc.v), sfm.s.Tc.v);
    return compose(accel_to_camera_world_transform(), compose(x, cam_transform));
}

transformation sensor_fusion::get_transformation() const
{
    transformation filter_transform(to_quaternion(sfm.s.W.v), sfm.s.T.v);    
    if(sfm.qr.valid)
    {
        return accel_to_camera_transformation(compose(sfm.qr.origin, filter_transform));
    }

    return accel_to_camera_transformation(filter_transform);
}

v4 sensor_fusion::filter_to_external_position(const v4& x) const
{
    if(sfm.qr.valid)
    {
        return accel_to_camera_position(transformation_apply(sfm.qr.origin, x));
    }
    return accel_to_camera_position(x);
}


void sensor_fusion::flush_and_reset()
{
    isSensorFusionRunning = false;
    queue->stop_sync();
    queue->dispatch_sync([this]{
        filter_initialize(&sfm, device);
    });
    sfm.camera_control.focus_unlock();
    sfm.camera_control.release_platform_specific_object();
    
    isProcessingVideo = false;
    processingVideoRequested = false;
}

RCSensorFusionErrorCode sensor_fusion::get_error()
{
    RCSensorFusionErrorCode error = RCSensorFusionErrorCodeNone;
    if(sfm.numeric_failed) error = RCSensorFusionErrorCodeOther;
    else if(sfm.speed_failed) error = RCSensorFusionErrorCodeTooFast;
    else if(sfm.detector_failed) error = RCSensorFusionErrorCodeVision;
    return error;
}

void sensor_fusion::update_status()
{
    status s;
    //Updates happen synchronously in the calling (filter) thread
    s.error = get_error();
    s.progress = filter_converged(&sfm);
    s.run_state = sfm.run_state;
    
    s.confidence = RCSensorFusionConfidenceNone;
    if(s.run_state == RCSensorFusionRunStateRunning)
    {
        if(s.error == RCSensorFusionErrorCodeVision)
        {
            s.confidence = RCSensorFusionConfidenceLow;
        }
        else if(sfm.has_converged)
        {
            s.confidence = RCSensorFusionConfidenceHigh;
        }
        else
        {
            s.confidence = RCSensorFusionConfidenceMedium;
        }
    }
    if(s == last_status) return;
    
    // queue actions related to failures before queuing callbacks to the sdk client.
    if(s.error == RCSensorFusionErrorCodeOther)
    {
        //Sensor fusion has already been reset by get_error, but it could have gotten random data inbetween, so do full reset
        if(threaded) std::async(std::launch::async, [this] { flush_and_reset(); });
        else flush_and_reset();
    }
    else if(last_status.run_state == RCSensorFusionRunStateStaticCalibration && s.run_state == RCSensorFusionRunStateInactive && s.error == RCSensorFusionErrorCodeNone)
    {
        isSensorFusionRunning = false;
        //TODO: save calibration
    }
    
    if((s.error == RCSensorFusionErrorCodeVision && s.run_state != RCSensorFusionRunStateRunning)) {
        //refocus if either we tried to detect and failed, or if we've recently moved during initialization
        isProcessingVideo = false;
        processingVideoRequested = true;
        
        // Request a refocus
        //TODO: is it possible for *this to become invalid before this is called?
        sfm.camera_control.focus_once_and_lock([this](uint64_t timestamp, float position)
                                               {
                                                   if(processingVideoRequested && !isProcessingVideo) {
                                                       isProcessingVideo = true;
                                                       processingVideoRequested = false;
                                                   }
                                               });
    }
    
    if(status_callback) {
        if(threaded) std::async(std::launch::async, status_callback, s);
        else status_callback(s);
    }

    last_status = s;
}

std::vector<sensor_fusion::feature_point> sensor_fusion::get_features() const
{
    std::vector<feature_point> features;
    features.reserve(sfm.s.features.size());
    for(auto i: sfm.s.features)
    {
        if(i->is_valid()) {
            feature_point p;
            p.id = i->id;
            p.x = (float)i->current[0];
            p.y = (float)i->current[1];
            p.original_depth = (float)i->v.depth();
            p.stdev = (float)i->v.stdev_meters(sqrt(i->variance()));
            v4 ext_pos = filter_to_external_position(i->world);
            p.worldx = (float)ext_pos[0];
            p.worldy = (float)ext_pos[1];
            p.worldz = (float)ext_pos[2];
            p.initialized = i->is_initialized();
            features.push_back(p);
        }
    }
    return features;
}

void sensor_fusion::update_data(camera_data &&image)
{
    auto d = std::make_unique<data>();
    
    //perform these operations synchronously in the calling (filter) thread
    d->total_path_m = sfm.s.total_distance;
    camera_parameters cp;
    cp.fx = (float)sfm.s.focal_length.v;
    cp.fy = (float)sfm.s.focal_length.v;
    cp.cx = (float)sfm.s.center_x.v;
    cp.cy = (float)sfm.s.center_y.v;
    cp.skew = 0;
    cp.k1 = (float)sfm.s.k1.v;
    cp.k2 = (float)sfm.s.k2.v;
    cp.k3 = (float)sfm.s.k3.v;
    d->camera_intrinsics = cp;
    
    if(sfm.qr.valid)
    {
        d->origin_qr_code = sfm.qr.data;
    }
    
    d->transform = get_transformation();
    d->time = sfm.last_time;

    d->features = get_features();

    if(camera_callback) {
        //if(threaded) std::async(std::launch::async, camera_callback, std::move(d), std::move(image));
        //else
        camera_callback(std::move(d), std::move(image));
    }
}

sensor_fusion::sensor_fusion(fusion_queue::latency_strategy strategy)
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
            update_status();
            if(docallback) update_data(std::move(data));
        } else {
            //We're not yet processing video, but we do want to send updates for the video preview. Make sure that rotation is initialized.
            docallback =  sfm.gravity_init;
            update_status();
            if(docallback) update_data(std::move(data));
        }
    };
    
    auto acc_fn = [this](accelerometer_data &&data)
    {
        if(!isSensorFusionRunning) return;
        filter_accelerometer_measurement(&sfm, data.accel_m__s2, data.timestamp);
        update_status();
    };
    
    auto gyr_fn = [this](gyro_data &&data)
    {
        if(!isSensorFusionRunning) return;
        filter_gyroscope_measurement(&sfm, data.angvel_rad__s, data.timestamp);
    };
    
    queue = std::make_unique<fusion_queue>(cam_fn, acc_fn, gyr_fn, strategy, std::chrono::microseconds(10000)); //Have to make jitter high - ipad air 2 accelerometer has high latency, we lose about 10% of samples with jitter at 8000
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

void sensor_fusion::start_calibration(bool thread)
{
    threaded = thread;
    isSensorFusionRunning = true;
    isProcessingVideo = false;
    filter_initialize(&sfm, device);
    filter_start_static_calibration(&sfm);
    if(threaded) queue->start_async(false);
    else queue->start_singlethreaded(false);
}

/*void sensor_fusion::start_inertial_only()
{
    filter_initialize(&sfm, device);
}*/

void sensor_fusion::start(bool thread, camera_control_interface &cam)
{
    threaded = thread;
    isSensorFusionRunning = true;
    isProcessingVideo = false;
    filter_initialize(&sfm, device);
    filter_start_hold_steady(&sfm);
    if(threaded) queue->start_async(true);
    else queue->start_singlethreaded(true);
}

void sensor_fusion::start_unstable(bool thread, camera_control_interface &cam)
{
    threaded = thread;
    isSensorFusionRunning = true;
    isProcessingVideo = false;
    filter_initialize(&sfm, device);
    filter_start_dynamic(&sfm);
    if(threaded) queue->start_async(true);
    else queue->start_singlethreaded(true);
}

void sensor_fusion::start_offline()
{
    threaded = false;
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
    // TODO: we currently ignore initial_pose_m
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

    last_log = sfm.last_time;

    transformation transform = get_transformation();
    
    m4 R = to_rotation_matrix(transform.Q);
    v4 T = transform.T;

    std::stringstream s(std::stringstream::out);
    s << sfm.last_time.time_since_epoch().count() << " " << " " <<
            R(0, 0) << " " << R(0, 1) << " " << R(0, 2) << " " << T(0) << " " <<
            R(1, 0) << " " << R(1, 1) << " " << R(1, 2) << " " << T(1) << " " <<
            R(2, 0) << " " << R(2, 1) << " " << R(2, 2) << " " << T(2);

    log_function(log_handle, s.str().c_str(), s.str().length());
}

