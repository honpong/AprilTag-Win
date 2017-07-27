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
#include "Trace.h"

#define STEREO_TIME_THRESHOLD 0.023

transformation sensor_fusion::get_transformation() const
{
    return sfm.origin*sfm.s.get_transformation();
}

void sensor_fusion::set_transformation(const transformation &pose_m)
{
    sfm.origin_set = true;
    sfm.origin = pose_m*invert(sfm.s.get_transformation());
}

void sensor_fusion::update_status()
{
    if(status_callback)
        status_callback();

    // queue actions related to failures before queuing callbacks to the sdk client.
    if(sfm.numeric_failed) {
        sfm.log->error("Numerical error; filter reset.");
        transformation last_transform = get_transformation();
        filter_initialize(&sfm);
        filter_set_origin(&sfm, last_transform, true);
        filter_start(&sfm);
    }
}

void sensor_fusion::update_data(const sensor_data * data)
{
    if(data_callback)
        data_callback(data);
}

sensor_fusion::sensor_fusion(fusion_queue::latency_strategy strategy)
    : queue([this](sensor_data &&data) { queue_receive_data(std::move(data)); },
            strategy, std::chrono::milliseconds(500)),
      isProcessingVideo(false),
      isSensorFusionRunning(false)
{
}

void sensor_fusion::fast_path_catchup()
{
    auto start = std::chrono::steady_clock::now();
    sfm.catchup->state.copy_from(sfm.s);
    std::unique_lock<std::recursive_mutex> mini_lock(mini_mutex);
    // hold the mini_mutex while we manipulate the mini
    // state *and* while we manipulate the queue during
    // catchup so that dispatch_buffered is sure to notice
    // any new data we get while we are doing the filter
    // updates on the catchup state
    queue.dispatch_buffered([this,&mini_lock](sensor_data &data) {
            mini_lock.unlock();
            switch(data.type) {
                case rc_SENSOR_TYPE_ACCELEROMETER: filter_mini_accelerometer_measurement(&sfm, sfm.catchup->observations, sfm.catchup->state, data); break;
                case rc_SENSOR_TYPE_GYROSCOPE:     filter_mini_gyroscope_measurement(&sfm, sfm.catchup->observations, sfm.catchup->state, data); break;
                default: break;
            }
            mini_lock.lock();
        });
    auto stop = std::chrono::steady_clock::now();
    queue.catchup_stats.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    sfm.catchup->valid = true;
    std::swap(sfm.mini, sfm.catchup);
}

void sensor_fusion::queue_receive_data(sensor_data &&data)
{
    switch(data.type) {
        case rc_SENSOR_TYPE_IMAGE: {
            bool docallback = true;
            if(isProcessingVideo)
                docallback = filter_image_measurement(&sfm, data);
            else
                //We're not yet processing video, but we do want to send updates for the video preview. Make sure that rotation is initialized.
                docallback = sfm.s.orientation_initialized;

            if (isProcessingVideo && fast_path && !queue.data_in_queue(data.type, data.id))
                fast_path_catchup();

            update_status();
            if(docallback)
                update_data(&data);

            if (data.id < sfm.s.cameras.children.size())
                if(sfm.s.cameras.children[data.id]->detecting_space)
                    sfm.s.cameras.children[data.id]->detection_future = std::async(threaded ? std::launch::async : std::launch::deferred,
                        [this] (struct filter *f, const sensor_data &&data) {
                            auto start = std::chrono::steady_clock::now();
                            filter_detect(&sfm, std::move(data));
                            auto stop = std::chrono::steady_clock::now();
                            queue.stats.find(data.global_id())->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
                        }, &sfm, std::move(data));
        } break;

        case rc_SENSOR_TYPE_STEREO: {
            bool docallback = true;
            if(isProcessingVideo) {
                START_EVENT(EV_FILTER_IMG_STEREO, 0);
                std::unique_ptr<void, void(*)(void *)> im_copy(const_cast<void*>(data.stereo.image1), [](void *){});
                sensor_data image_data(data.time_us, rc_SENSOR_TYPE_IMAGE, 0, data.stereo.shutter_time_us,
                       data.stereo.width, data.stereo.height, data.stereo.stride1, data.stereo.format, data.stereo.image1, std::move(im_copy));
                docallback = filter_image_measurement(&sfm, image_data);

                if (fast_path && !queue.data_in_queue(data.type, data.id))
                    fast_path_catchup();

                update_status();
                if(docallback)
                    update_data(&image_data); // TODO: visualize stereo data directly so we don't have a data callback here

                if(sfm.s.cameras.children[0]->detecting_space) {
                    sfm.s.cameras.children[0]->detection_future = std::async(threaded ? std::launch::deferred : std::launch::deferred,
                        [this] (struct filter *f, const sensor_data &&data) {
                            START_EVENT(EV_DETECTING_GROUP_STEREO, 0);
                            auto start = std::chrono::steady_clock::now();
                            filter_detect(&sfm, std::move(data));
                            auto stop = std::chrono::steady_clock::now();
                            auto global_id = sensor_data::get_global_id_by_type_id(rc_SENSOR_TYPE_STEREO, 0); //"data" is of type IMAGE, but truly should report stats to STEREO stream
                            queue.stats.find(global_id)->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
                            END_EVENT(EV_DETECTING_GROUP_STEREO, 0);
                        }, &sfm, std::move(image_data));
                    // since image_data is a wrapper type here that
                    // does't have a copy, we need to be sure we are
                    // finished using it before the end of this case
                    // statement
                    sfm.s.cameras.children[0]->detection_future.wait();
                    filter_stereo_initialize(&sfm, 0, 1, data);
                }

                update_status();
                if(docallback)
                    update_data(&data);
                END_EVENT(EV_FILTER_IMG_STEREO, 0);
            }
            else
                //We're not yet processing video, but we do want to send updates for the video preview. Make sure that rotation is initialized.
                docallback = sfm.s.orientation_initialized;
        } break;

        case rc_SENSOR_TYPE_DEPTH: {
            update_status();
            if (filter_depth_measurement(&sfm, data))
                update_data(&data);
        } break;

        case rc_SENSOR_TYPE_ACCELEROMETER: {
            if(!isSensorFusionRunning) return;
            update_status();
            if (filter_accelerometer_measurement(&sfm, data))
                update_data(&data);
        } break;

        case rc_SENSOR_TYPE_GYROSCOPE: {
            update_status();
            if (filter_gyroscope_measurement(&sfm, data))
                update_data(&data);
        } break;
    }
}

void sensor_fusion::queue_receive_data_fast(sensor_data &data)
{
    if(!isSensorFusionRunning || sfm.run_state != RCSensorFusionRunStateRunning || !fast_path || !sfm.mini->valid) return;
    data.path = rc_DATA_PATH_FAST;
    switch(data.type) {
        case rc_SENSOR_TYPE_ACCELEROMETER: {
            auto start = std::chrono::steady_clock::now();
            if(filter_mini_accelerometer_measurement(&sfm, sfm.mini->observations, sfm.mini->state, data))
                update_data(&data);
            auto stop = std::chrono::steady_clock::now();
            queue.stats.find(data.global_id())->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
        } break;

        case rc_SENSOR_TYPE_GYROSCOPE: {
            auto start = std::chrono::steady_clock::now();
            if(filter_mini_gyroscope_measurement(&sfm, sfm.mini->observations, sfm.mini->state, data))
                update_data(&data);
            auto stop = std::chrono::steady_clock::now();
            queue.stats.find(data.global_id())->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
        } break;
        default:
            break;
    }
    data.path = rc_DATA_PATH_SLOW;
}

void sensor_fusion::set_location(double latitude_degrees, double longitude_degrees, double altitude_meters)
{
    queue.dispatch_async([this, latitude_degrees, altitude_meters]{
        filter_compute_gravity(&sfm, latitude_degrees, altitude_meters);
    });
}

void sensor_fusion::start(bool thread, bool fast_path_)
{
    threaded = thread;
    buffering = false;
    fast_path = fast_path_;
    isSensorFusionRunning = true;
    isProcessingVideo = true;
    filter_initialize(&sfm);
    filter_start(&sfm);
    queue.start(thread);
}

void sensor_fusion::pause_and_reset_position()
{
    isProcessingVideo = false;
    queue.dispatch_async([this]() { filter_start_inertial_only(&sfm); });
}

void sensor_fusion::unpause()
{
    isProcessingVideo = true;
    queue.dispatch_async([this]() { filter_start(&sfm); });
}

void sensor_fusion::start_buffering()
{
    buffering = true;
    queue.start_buffering(std::chrono::milliseconds(200));
}

bool sensor_fusion::started()
{
    return isSensorFusionRunning;
}

void sensor_fusion::stop()
{
    queue.stop();
    filter_deinitialize(&sfm);
    isSensorFusionRunning = false;
    isProcessingVideo = false;
}

void sensor_fusion::flush_and_reset()
{
    stop();
    queue.reset();
    filter_initialize(&sfm);
}

void sensor_fusion::reset(sensor_clock::time_point time)
{
    flush_and_reset();
    sfm.last_time = time;
    sfm.s.time_update(time); //This initial time update doesn't actually do anything - just sets current time, but it will cause the first measurement to run a time_update relative to this
    sfm.origin_set = false;
}

void sensor_fusion::start_mapping()
{
    if (!sfm.map)
        sfm.map = std::make_unique<mapper>();
    sfm.map->reset();
}

void sensor_fusion::stop_mapping()
{
    sfm.map = nullptr;
}

void sensor_fusion::save_map(void (*write)(void *handle, const void *buffer, size_t length), void *handle)
{
    std::string json;
    if(sfm.map && sfm.map->serialize(json))
        write(handle, json.c_str(), json.length());
}

bool sensor_fusion::load_map(size_t (*read)(void *handle, void *buffer, size_t length), void *handle)
{
    if(!sfm.map)
        return false;

    std::string json;
    char buffer[1024];
    size_t bytes_read;
    while((bytes_read = read(handle, buffer, 1024)) != 0) {
        json.append(buffer, bytes_read);
    }

    return mapper::deserialize(json, *sfm.map);
}

void sensor_fusion::receive_data(sensor_data && data)
{
    std::unique_lock<std::recursive_mutex> mini_lock(mini_mutex);
    // hold the mini_mutex while we manipulate the mini state *and*
    // while we push data onto the queue so that catchup either
    // updates the mini state before we do or notices that we pushed
    // new data in.
    queue_receive_data_fast(data);
    queue.receive_sensor_data(std::move(data));
}

std::string sensor_fusion::get_timing_stats()
{
     return queue.get_stats() + filter_get_stats(&sfm);
}

