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
#include "priority.h"
#include "rc_compat.h"
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
    sfm.catchup->slow_path_timestamp = sfm.catchup->state.get_current_time();
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

void sensor_fusion::queue_receive_data(sensor_data &&data, bool catchup)
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

            if (isProcessingVideo && fast_path && catchup) {
                uint64_t in_queue = queue.data_in_queue(data.type, data.id);
                if(!in_queue)
                    fast_path_catchup();
                else
                    sfm.log->warn("Skipped catchup at {}, {} of {} left in queue", sensor_clock::tp_to_micros(data.timestamp), in_queue, data.type);
            }

            update_status();
            if(docallback)
                update_data(&data);

            sfm.relocalization_info = {};
            if (data.id < sfm.s.cameras.children.size()) {
                const bool relocalize_now = sfm.map && sfm.relocalize &&
                        !sfm.s.groups.children.empty() &&
                        (!sfm.relocalization_future.valid() || sfm.relocalization_future.valid_n());
                const bool new_group_created = sfm.s.group_counter > groups;
                const bool compute_descriptors_now = relocalize_now ||
                        (sfm.map && (sfm.save_map || sfm.relocalize) && new_group_created);

                if (sfm.s.cameras.children[data.id]->detecting_space || compute_descriptors_now) {
                    std::unique_ptr<camera_frame_t> camera_frame;
                    if (compute_descriptors_now)
                        camera_frame = filter_create_camera_frame(&sfm, data);

                    auto start = std::chrono::steady_clock::now();
                    sfm.s.cameras.children[data.id]->detected = filter_detect(&sfm, data, camera_frame);
                    auto stop = std::chrono::steady_clock::now();
                    queue.stats.find(data.global_id())->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });

                    if (camera_frame) {
                        filter_compute_orb(&sfm, data, *camera_frame);
                        if (sfm.relocalize) filter_compute_dbow(&sfm, *camera_frame);
                        if (new_group_created) filter_assign_frame(&sfm, *camera_frame);
                        if (relocalize_now) {
                            if (sfm.relocalization_future.valid()) {
                                auto result = sfm.relocalization_future.get();
                                filter_add_relocalization_edges(&sfm, result.edges);
                                sfm.relocalization_info = std::move(result.info);
                            }
                            filter_update_map_index(&sfm);
                            sfm.relocalization_future = std::async(threaded ? std::launch::async : std::launch::deferred,
                                [this] (std::unique_ptr<camera_frame_t>&& camera_frame) {
                                    set_priority(PRIORITY_SLAM_RELOCALIZE);
                                    return filter_relocalize(&sfm, *camera_frame);
                            }, std::move(camera_frame));
                        }
                    }
                }
            }
        } break;

        case rc_SENSOR_TYPE_STEREO: {
            START_EVENT(SF_STEREO_RECEIVE, data.id);
            auto pair = sensor_data::split(std::move(data));

            if ((pair.first.id < sfm.s.cameras.children.size()) ^ (pair.second.id < sfm.s.cameras.children.size()))
                sfm.log->critical("Stereo packet with only one camera ({} but not {}) defined\n", pair.first.id, pair.second.id);
            if (pair.first.id >= sfm.s.cameras.children.size() || pair.second.id >= sfm.s.cameras.children.size())
                break;

            if (sfm.s.cameras.children[pair.first.id ]->detected ||
                sfm.s.cameras.children[pair.second.id]->detected)
                filter_stereo_initialize(&sfm, pair.first.id, pair.second.id);

            uint64_t in_queue = queue.data_in_queue(data.type, data.id);
            queue_receive_data(std::move(pair.first), false);
            queue_receive_data(std::move(pair.second), !in_queue);
            if (in_queue)
                sfm.log->warn("Skipped stereo catchup at {}, {} in queue", sensor_clock::tp_to_micros(data.timestamp), in_queue);

            END_EVENT(SF_STEREO_RECEIVE, 0);
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
            auto prediction_time = (data.timestamp - sfm.mini->slow_path_timestamp);
            queue.time_since_catchup_stats.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(prediction_time).count())});
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

bool sensor_fusion::set_stage(const char *name, const rc_Pose &G_world_stage) {
    if (!sfm.map)
        return false;
    queue.dispatch_async([this, name=std::string(name), G_world_stage]() mutable {
        uint64_t closest_id; transformation Gr_closest_now;
        if (sfm.s.get_closest_group_transformation(closest_id, Gr_closest_now)) {
            transformation G_closest_stage = Gr_closest_now * invert(get_transformation()) * to_transformation(G_world_stage);
            sfm.map->set_stage(std::move(name), closest_id, G_closest_stage);
        } else
            sfm.log->error("tried to create a stage when there were no closests!\n");
    });
    return true;
}

bool sensor_fusion::get_stage(bool next, const char *name, mapper::stage::output &current_stage) {
    if (!sfm.map)
        return false;
    uint64_t closest_id; transformation Gr_closest_now;
    if (!sfm.s.get_closest_group_transformation(closest_id, Gr_closest_now))
        return false;
    return sfm.map->get_stage(next, name, closest_id, get_transformation() * invert(Gr_closest_now), current_stage);
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
    set_priority(PRIORITY_SLAM_FASTPATH);
    queue.dispatch_async([this]() { set_priority(PRIORITY_SLAM_SLOWPATH); });
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

#ifdef RELOCALIZATION_DEBUG
#include "debug/visual_debug.h"
#endif

void sensor_fusion::start_mapping(bool relocalize, bool save_map)
{
    if (!sfm.map) {
        sfm.map = std::make_unique<mapper>();
        sfm.relocalize = relocalize;
        sfm.save_map = save_map;
#ifdef RELOCALIZATION_DEBUG
        visual_debug::create_instance(this);
#endif
    }
    sfm.map->reset();
}

void sensor_fusion::stop_mapping()
{
    sfm.map = nullptr;
}

void sensor_fusion::save_map(rc_SaveCallback write, void *handle)
{
    if (!sfm.map)
        return;
    sfm.map->serialize(write, handle);
}

bool sensor_fusion::load_map(rc_LoadCallback read, void *handle)
{
    if (!sfm.map)
        return false;
    bool deserialize_status = false;
    if ((deserialize_status = mapper::deserialize(read, handle, *sfm.map))) {
        sfm.s.group_counter = sfm.map->get_node_id_offset();
        tracker::feature::next_id = sfm.map->get_feature_id_offset();
    }
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

