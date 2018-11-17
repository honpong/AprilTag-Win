//
//  rs_sf_device_manager.cpp
//  shapefit-dev
//
//  Created by Hon Pong (Gary) Ho on 11/14/18.
//
#include "rs_sf_camera.hpp"
#include "rs_sf_device_manager.hpp"
#include <librealsense2/rsutil.h>
#include <iostream>
#include <iomanip>

std::string rs_sf_device_manager::get_device_name() const
{
    return std::string(_device.get_info(RS2_CAMERA_INFO_NAME));
}

bool rs_sf_device_manager::search_device_name(const std::string& device_name)
{
    for (auto dev : _ctx.query_devices()){
        _device = dev;
        if(get_device_name()==device_name){
            return true;
        }
    }
    _device = nullptr;
    return false;
}

void rs_sf_device_manager::add_stream_request(const rs_sf_stream_select& stream_request)
{
    _streams.emplace_back(stream_request);
}

void rs_sf_device_manager::find_stream_profiles()
{
    for(auto& stream : _streams){
        for(auto& sensor : _device.query_sensors()){
            for(auto profile : sensor.get_stream_profiles()){
                if((profile.stream_type()==stream.type) &&
                   (profile.stream_index()==stream.index || stream.index<0) &&
                   (profile.fps()==stream.fps || stream.fps<0) &&
                   (profile.format()==stream.format)){
                    if(profile.is<rs2::motion_stream_profile>() ||
                       ((profile.as<rs2::video_stream_profile>().width()  == stream.width) &&
                        (profile.as<rs2::video_stream_profile>().height() == stream.height))){
                           stream.index   = profile.stream_index();
                           stream.fps     = profile.fps();
                           stream.sensor  = sensor;
                           stream.profile = profile;
                       }
                }
            }
        }
    }
}

void rs_sf_device_manager::extract_stream_calibrations()
{
    // set up calibrations
    for(auto& stream : _streams){
        if(!stream.profile){ continue; }
        if(stream.profile.is<rs2::motion_stream_profile>()){
            stream.imu_intriniscs = stream.profile.as<rs2::motion_stream_profile>().get_motion_intrinsics();
        } else if(stream.profile.is<rs2::video_stream_profile>()){
            stream.cam_intrinsics = stream.profile.as<rs2::video_stream_profile>().get_intrinsics();
        }
        stream.extrinsics.resize(num_streams());
        for( int t=0; t<num_streams(); ++t){
            if(!_streams[t].profile){ continue; }
            try{
                stream.extrinsics[t] = stream.profile.get_extrinsics_to(_streams[t].profile);
            }catch(...){}
        }
    }
}

void rs_sf_device_manager::print_requested_streams() const
{
    for(auto& stream : _streams){
        if(!stream.profile){ continue; }
        auto& s = stream.profile;
        std::cout << "stream name  :" << s.stream_name() << std::endl;
        std::cout << "stream type  :" << rs2_stream_to_string(s.stream_type()) << std::endl;
        std::cout << "stream index :" << s.stream_index() << std::endl;
        std::cout << "stream id    :" << s.unique_id() << std::endl;
        std::cout << "stream format:" << rs2_format_to_string(s.format()) << std::endl;
        if(s.is<rs2::video_stream_profile>()){
            auto ss = s.as<rs2::video_stream_profile>();
            std::cout << "stream w,h  :" << ss.width() << "," << ss.height() << std::endl;
        }
        std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
    }
}

void rs_sf_device_manager::print_calibrations() const
{
    std::cout << "--------------------------------------------------------" << std::endl;
    std::cout << "                  Print Calibration                     " << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;
    for(auto& stream : _streams){
        if(!stream.profile){ continue; }
        auto& s = stream.profile;
        std::cout << std::setprecision(3);
        std::cout << "stream " << (rs2_stream)s.stream_type() << " index " << s.stream_index() << std::endl <<
        "fx :" << stream.cam_intrinsics.fx    << ", fy :" << stream.cam_intrinsics.fy     << std::endl <<
        "px :" << stream.cam_intrinsics.ppx   << ", py :" << stream.cam_intrinsics.ppy    << std::endl <<
        "w  :" << stream.cam_intrinsics.width << ", h  :" << stream.cam_intrinsics.height << std::endl;
        for (int s=0; s<num_streams(); ++s){
            std::cout << "    extrinsics to " << (rs2_stream)_streams[s].type << ", index " << _streams[s].index;
            std::cout << " rotation: ";
            for(auto& r : stream.extrinsics[s].rotation){ std::cout << r << " "; }
            std::cout << " translation: ";
            for(auto& t : stream.extrinsics[s].translation){ std::cout << t << " ";}
            std::cout << std::endl;
        }
        std::cout << "--------------------------------------------------------" << std::endl;
    }
}

void rs_sf_device_manager::print(const rs2::error& e) {
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
}
