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
        transformation last_transform{sfm.s.Q.v, sfm.s.T.v};
        filter_initialize(&sfm);
        sfm.s.T.v = last_transform.T;
        sfm.s.Q.v = last_transform.Q;
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
    START_EVENT(SF_FAST_PATH_CATCHUP, 0);
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
        if(data.type == rc_SENSOR_TYPE_GYROSCOPE)     filter_mini_gyroscope_measurement(&sfm, sfm.catchup->observations, sfm.catchup->state, data);
        mini_lock.lock();
    });
    auto stop = std::chrono::steady_clock::now();
    queue.catchup_stats.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    sfm.catchup->valid = true;
    std::swap(sfm.mini, sfm.catchup);
    END_EVENT(SF_FAST_PATH_CATCHUP, 0);
}

void sensor_fusion::queue_receive_data(sensor_data &&data)
{
    switch(data.type) {
        case rc_SENSOR_TYPE_IMAGE: {
            bool docallback = true;
            uint64_t groups = sfm.s.group_counter;
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

            if (data.id < sfm.s.cameras.children.size()) {
                if (sfm.s.cameras.children[data.id]->node_description_future.valid()) {
                    auto camera_frame = sfm.s.cameras.children[data.id]->node_description_future.get();
                    filter_update_map_index(&sfm, camera_frame);
                    if (sfm.relocalize) {
                        auto result = sfm.relocalization_scheduler.process(threaded,
                            [this] (camera_frame_t&& camera_frame) {
                                return filter_relocalize(&sfm, std::move(camera_frame));
                        }, std::move(camera_frame));
                        if (result.status == scheduler::RESULT_AVAILABLE && result.value)
                            sfm.log->info("relocalized");
                    }
                }

                bool compute_descriptors_now = [&]() {
                    const bool new_group_created = sfm.s.group_counter > groups;
                    if (sfm.map && sfm.map->current_node) {
                        return sfm.relocalize || (sfm.save_map && new_group_created);
                    }
                    return false;
                }();

                if (sfm.s.cameras.children[data.id]->detecting_space || compute_descriptors_now) {
                    camera_frame_t camera_frame;
                    if (compute_descriptors_now)
                        filter_create_camera_frame(&sfm, data, camera_frame);

                    sfm.s.cameras.children[data.id]->detection_future = std::async(threaded ? std::launch::async : std::launch::deferred,
                        [this] (sensor_data &&data, camera_frame_t&& camera_frame) {
                            auto start = std::chrono::steady_clock::now();
                            filter_detect(&sfm, data, camera_frame.frame);
                            auto stop = std::chrono::steady_clock::now();
                            queue.stats.find(data.global_id())->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });

                            if (camera_frame.frame) {
                                sfm.s.cameras.children[data.id]->node_description_future =
                                        std::async(threaded ? std::launch::async : std::launch::deferred,
                                                   [this] (sensor_data &&data, camera_frame_t &&camera_frame) {
                                    filter_compute_orb_and_dbow(&sfm, data, camera_frame);
                                    return camera_frame;
                                }, std::move(data), std::move(camera_frame));
                            }
                        }, std::move(data), std::move(camera_frame));
                }
            }
        } break;

        case rc_SENSOR_TYPE_STEREO: {
            bool docallback = true;
            if(isProcessingVideo) {
                START_EVENT(SF_STEREO_RECEIVE, data.id);
                auto pair { sensor_data::split(std::move(data)) };
                uint64_t groups = sfm.s.group_counter;
                docallback = filter_image_measurement(&sfm, pair.first);

                if (fast_path && !queue.data_in_queue(data.type, data.id))
                    fast_path_catchup();

                update_status();
                if(docallback)
                    update_data(&pair.first); // TODO: visualize stereo data directly so we don't have a data callback here

                if (sfm.s.cameras.children[0]->node_description_future.valid()) {
                    auto camera_frame = sfm.s.cameras.children[0]->node_description_future.get();
                    filter_update_map_index(&sfm, camera_frame);
                    if (sfm.relocalize) {
                        auto result = sfm.relocalization_scheduler.process(threaded,
                            [this] (camera_frame_t&& camera_frame) {
                                return filter_relocalize(&sfm, std::move(camera_frame));
                        }, std::move(camera_frame));
                        if (result.status == scheduler::RESULT_AVAILABLE && result.value)
                            sfm.log->info("relocalized");
                    }
                }

                bool compute_descriptors_now = [&]() {
                    const bool new_group_created = sfm.s.group_counter > groups;
                    if (sfm.map && sfm.map->current_node) {
                        return sfm.relocalize || (sfm.save_map && new_group_created);
                    }
                    return false;
                }();

                if(sfm.s.cameras.children[0]->detecting_space || compute_descriptors_now) {
                    camera_frame_t camera_frame;
                    if (compute_descriptors_now)
                        filter_create_camera_frame(&sfm, pair.first, camera_frame);

                    sfm.s.cameras.children[0]->detection_future = std::async(threaded ? std::launch::deferred : std::launch::deferred,
                        [this] (sensor_data &&data, camera_frame_t&& camera_frame) {
                            START_EVENT(SF_STEREO_DETECT1, 0);
                            auto start = std::chrono::steady_clock::now();
                            filter_detect(&sfm, data, camera_frame.frame);
                            auto stop = std::chrono::steady_clock::now();
                            auto global_id = sensor_data::get_global_id_by_type_id(rc_SENSOR_TYPE_STEREO, 0); //"data" is of type IMAGE, but truly should report stats to STEREO stream
                            queue.stats.find(global_id)->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
                            END_EVENT(SF_STEREO_DETECT1, 0);
                            if (camera_frame.frame) {
                                sfm.s.cameras.children[0]->node_description_future =
                                    std::async(threaded ? std::launch::async : std::launch::deferred,
                                               [this](const sensor_data &&data, camera_frame_t&& camera_frame) {
                                                   filter_compute_orb_and_dbow(&sfm, data, camera_frame);
                                                   return camera_frame;
                                               },
                                               std::move(data), std::move(camera_frame));
                            }
                        }, std::move(pair.first), std::move(camera_frame));
                    sfm.s.cameras.children[0]->detection_future.wait();
                    filter_stereo_initialize(&sfm, 0, 1, pair.second);
                }

                update_status();
                if(docallback)
                    update_data(&data);

                END_EVENT(SF_STEREO_RECEIVE, 0);
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

        case rc_SENSOR_TYPE_THERMOMETER: {
            update_data(&data);
        } break;

        case rc_SENSOR_TYPE_DEBUG: {
        } break;

        case rc_SENSOR_TYPE_VELOCIMETER: {
            update_status();
            if (filter_velocimeter_measurement(&sfm, data))
                update_data(&data);
        } break;
    }
}

void sensor_fusion::queue_receive_data_fast(sensor_data &data)
{
    if(!isSensorFusionRunning || sfm.run_state != RCSensorFusionRunStateRunning || !fast_path || !sfm.mini->valid) return;
    data.path = rc_DATA_PATH_FAST;
    switch(data.type) {
        case rc_SENSOR_TYPE_GYROSCOPE: {
            auto start = std::chrono::steady_clock::now();
            if(filter_mini_gyroscope_measurement(&sfm, sfm.mini->observations, sfm.mini->state, data))
                update_data(&data);
            auto stop = std::chrono::steady_clock::now();
            queue.stats.find(data.global_id())->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
        } break;
        case rc_SENSOR_TYPE_ACCELEROMETER:
        case rc_SENSOR_TYPE_DEPTH:
        case rc_SENSOR_TYPE_IMAGE:
        case rc_SENSOR_TYPE_STEREO:
        case rc_SENSOR_TYPE_THERMOMETER:
        case rc_SENSOR_TYPE_DEBUG:
        case rc_SENSOR_TYPE_VELOCIMETER:
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
    if (!isSensorFusionRunning)
        return;
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

void sensor_fusion::start_mapping(bool relocalize, bool save_map)
{
    if (!sfm.map) {
        sfm.map = std::make_unique<mapper>();
        sfm.relocalize = relocalize;
        sfm.save_map = save_map;
#ifdef RELOCALIZATION_DEBUG
        if(sfm.map) sfm.map->debug = [this](cv::Mat &&image,const uint64_t image_id, const std::string &message, const bool pause) {
            if (data_callback) {
                sensor_data debug_data = {};
                debug_data.type = rc_SENSOR_TYPE_DEBUG;
                debug_data.debug.message = message.c_str();
                debug_data.debug.pause = pause;
                debug_data.id = image_id; // we hack the interface to show the # of desired images
                debug_data.debug.image.width = image.cols;
                debug_data.debug.image.height = image.rows;
                debug_data.debug.image.stride = image.step;
                debug_data.debug.image.format = rc_ImageFormat::rc_FORMAT_RGBA8;
                debug_data.debug.image.image = image.data;
                data_callback(&debug_data); }
        };
#endif
    }
    sfm.map->reset();
}

void sensor_fusion::stop_mapping()
{
    sfm.map = nullptr;
}

#include "jsonstream.h"
#include "rapidjson/writer.h"

void sensor_fusion::save_map(void (*write)(void *handle, const void *buffer, size_t length), void *handle)
{
    if (!sfm.map)
        return;
    rapidjson::Document map_json;
    sfm.map->serialize(map_json, map_json.GetAllocator());
    save_stream stream(write, handle);
    rapidjson::Writer<save_stream> writer(stream);
    map_json.Accept(writer);
}

bool sensor_fusion::load_map(size_t (*read)(void *handle, void *buffer, size_t length), void *handle)
{
    if(!sfm.map)
        return false;
    load_stream stream(read, handle);
    rapidjson::Document doc;
    bool deserialize_status = mapper::deserialize(doc.ParseStream(stream), *sfm.map);
    sfm.s.group_counter = sfm.map->get_node_id_offset();
    tracker::feature::next_id = sfm.map->get_feature_id_offset();

    return deserialize_status;
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

