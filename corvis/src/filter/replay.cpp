//
//  replay.cpp
//
//  Created by Eagle Jones on 4/8/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "replay.h"

#include <iostream>
#include <fstream>
#include <string.h>
#include "filter_setup.h"
#include "device_parameters.h"
#include "cor.h"
#include "packet.h"
#include "sensor_fusion_queue.h"

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        cerr << "Usage: replay filename device.\n";
        return -1;
    }
    ifstream file(argv[1], ios::binary | ios::ate);
    if(file.bad())
    {
        cerr << "Couldn't open file " << argv[1] << " for reading.\n";
        return -1;
    }
    auto end = file.tellg();
    file.seekg (0, ios::beg);
    auto begin = file.tellg();

    cerr << "size is: " << (end-begin) << " bytes.\n";
    
    int packets_dispatched = 0;
    uint64_t bytes_dispatched = 0;
    bool is_running = true;
    
    corvis_device_type device_type = get_device_by_name(argv[2]);
    corvis_device_parameters dc;
    get_parameters_for_device(device_type, &dc);
    filter_setup cor_setup(&dc);
    
    
    auto camf = [&cor_setup](const camera_data &x) { filter_image_measurement(&cor_setup.sfm, x.image, x.width, x.height, x.stride, std::chrono::duration_cast<std::chrono::microseconds>(x.timestamp.time_since_epoch()).count()); };
    auto accf = [&cor_setup](const accelerometer_data &x) { filter_accelerometer_measurement(&cor_setup.sfm, x.accel_m__s2, std::chrono::duration_cast<std::chrono::microseconds>(x.timestamp.time_since_epoch()).count()); };
    auto gyrf = [&cor_setup](const gyro_data &x) { filter_gyroscope_measurement(&cor_setup.sfm, x.angvel_rad__s, std::chrono::duration_cast<std::chrono::microseconds>(x.timestamp.time_since_epoch()).count()); };
    
    fusion_queue queue(camf, accf, gyrf, fusion_queue::latency_strategy::ELIMINATE_DROPS, std::chrono::microseconds(33000), std::chrono::microseconds(10000), std::chrono::microseconds(5000));
    
    queue.start_offline(true);
    
    cor_setup.sfm.ignore_lateness = true;
    cor_time_init();
    filter_start_dynamic(&cor_setup.sfm);
    
    uint64_t first_timestamp;
    /*, realtime_offset;
     uint64_t time_started, now;*/
    
    packet_header_t header;
    file.read((char *)&header, 16);
    if(file.bad() || file.eof()) is_running = false;
    first_timestamp = header.time;
    cerr << "First timestamp: " << first_timestamp << endl;
    
    /*
     time_started = get_timestamp();
     now = time_started;
     
     // filter->ignore_lateness is not set on realtime replay, so we must adjust
     // the timestamps to be relative to the current system time
     realtime_offset = 0;
     if(isRealtime)
     realtime_offset = now - first_timestamp;
     */
    while (is_running) {
        packet_t * packet = (packet_t *)malloc(header.bytes);
        packet->header = header;
        
        file.read((char *)packet->data, header.bytes - 16);
        if(file.bad() || file.eof())
        {
            is_running = false;
        }
        else
        {
            
            /*
             // Adjust the time of the packet to be consistent with the start time of replay
             packet->header.time += realtime_offset;
             
             now = get_timestamp();
             float delta_seconds = (1.*packet->header.time - now)/1e6;
             if(isRealtime && delta_seconds > 0) {
             [NSThread sleepForTimeInterval:delta_seconds];
             }
             */
            switch(header.type)
            {
                case packet_camera:
                {
                    int width, height;
                    sscanf((char *)packet->data, "P5 %d %d", &width, &height);
                    camera_data d;
                    d.image = packet->data + 16;
                    d.width = width;
                    d.height = height;
                    d.stride = width;
                    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    d.image_handle = std::unique_ptr<void, void(*)(void *)>(packet, free);
                    queue.receive_camera(std::move(d));
                    //                    filter_image_measurement(&cor_setup.sfm, packet->data + 16, width, height, width, header.time);
                    break;
                }
                case packet_accelerometer:
                {
                    accelerometer_data d;
                    d.accel_m__s2[0] = ((float *)packet->data)[0];
                    d.accel_m__s2[1] = ((float *)packet->data)[1];
                    d.accel_m__s2[2] = ((float *)packet->data)[2];
                    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    queue.receive_accelerometer(std::move(d));
                    free(packet);
                    break;
                }
                case packet_gyroscope:
                {
                    gyro_data d;
                    d.angvel_rad__s[0] = ((float *)packet->data)[0];
                    d.angvel_rad__s[1] = ((float *)packet->data)[1];
                    d.angvel_rad__s[2] = ((float *)packet->data)[2];
                    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    queue.receive_gyro(std::move(d));
                    free(packet);
                    break;
                }
            }
            queue.dispatch_offline(false);
            bytes_dispatched += header.bytes;
            packets_dispatched++;
        }
        
        file.read((char *)&header, 16);
        if(file.bad() || file.eof()) is_running = false;
    }
    while(queue.dispatch_offline(true)) {}
    fprintf(stderr, "Distance is %f cm, total path length %f cm\n", cor_setup.sfm.s.total_distance * 100, norm(cor_setup.sfm.s.T.v) * 100);
    fprintf(stderr, "Dispatched %d packets %.2f Mbytes\n", packets_dispatched, bytes_dispatched/1.e6);
    return 0;
}
