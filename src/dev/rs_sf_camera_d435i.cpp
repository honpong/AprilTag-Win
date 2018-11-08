//
//  rs_sf_camera_d435i.cpp
//  shapefit-dev
//
//  Created by Hon Pong (Gary) Ho on 11/6/18.
//

#include "rs_sf_camera.hpp"
#include <librealsense2/rsutil.h>
#include <iomanip>
#include <iostream>

bool rs_sf_device_manager::search_device_name(const std::string device_name) const
{
    for (auto dev : _ctx.query_devices()){
        auto* name = dev.get_info(RS2_CAMERA_INFO_NAME);
        if(name && std::string(name)==device_name){
            return true;
        }
    }
    throw std::runtime_error("Error, " + device_name + " Not Found!");
}

void rs_sf_device_manager::print(const rs2::error& e) {
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
}

struct rs_sf_d435i_stream : public rs_sf_data_stream, rs_sf_device_manager
{
    rs_sf_stream_id _stream_id[6] = {
        {RS2_STREAM_DEPTH,    0},
        {RS2_STREAM_COLOR,    0},
        {RS2_STREAM_INFRARED, 1},
        {RS2_STREAM_INFRARED, 2},
        {RS2_STREAM_ACCEL,    0},
        {RS2_STREAM_GYRO,     0},
    };
    
    rs_sf_d435i_stream(int w, int h)
    {
        //if(!search_device_name("Intel RealSense D435i")){ /*error*/ }
        if(!search_device_name("Intel RealSense D435I")){ /*error*/ }
        
        for(const auto& stream : _stream_id){
            _config.enable_stream((rs2_stream)stream.stream_type, stream.stream_index, w, h);
            _stream_buffer[stream] = {};
        }
        
        // start the pipeline and streaming
        _pprofile = _pipeline.start(_config);
        _device   = _pprofile.get_device();
        
        // get intrinsics and extrinsics
        for(const auto& stream : _stream_id){
            auto profile = _pprofile.get_stream((rs2_stream)stream.stream_type, stream.stream_index);
            for(const auto& stream2 : _stream_id){
                _stream_buffer[stream]._extrinsics[stream2] = profile.get_extrinsics_to(_pprofile.get_stream((rs2_stream)stream2.stream_type, stream2.stream_index));
            }
            if(profile.is<rs2::video_stream_profile>()){
                _stream_buffer[stream]._intrinsics = profile.as<rs2::video_stream_profile>().get_intrinsics();
            }
        }
        
        // get the depth unit
        for(auto sensor : _device.query_sensors()){
            if(sensor.is<rs2::depth_sensor>()){
                for(auto stream : _stream_id){
                    if(stream.stream_type == RS2_STREAM_DEPTH){
                        _stream_buffer[stream]._depth_unit = sensor.as<rs2::depth_sensor>().get_depth_scale();
                    }
                }
            }
        }
        
        print_calibration();
    }
    
    void print_calibration() {
        for(const auto& stream : _stream_id){
            auto& s = _stream_buffer[stream];
            std::cout << std::setprecision(3);
            std::cout << "stream " << rs2_stream_to_string((rs2_stream)stream.stream_type) << " index " << stream.stream_index << std::endl <<
            "fx :" << s._intrinsics.fx    << ", fy :" << s._intrinsics.fy     << std::endl <<
            "px :" << s._intrinsics.ppx   << ", py :" << s._intrinsics.ppy    << std::endl <<
            "w  :" << s._intrinsics.width << ", h  :" << s._intrinsics.height << std::endl;
            for (const auto& s1 : s._extrinsics){
                std::cout << "    extrinsics to " << std::string(rs2_stream_to_string((rs2_stream)s1.first.stream_type)) << ", index " << s1.first.stream_index;
                std::cout << " rotation: ";
                for(auto& r : s1.second.rotation){ std::cout << r << " "; }
                std::cout << " translation: ";
                for(auto& t : s1.second.translation){ std::cout << t << " ";}
                std::cout << std::endl;
            }
        }
    }
    
    rs_sf_data* get_data() override try
    {
        for (auto frames = _pipeline.wait_for_frames();; frames = _pipeline.wait_for_frames())
        {
            for (auto&& f : frames)
            {
                auto stream_profile = f.get_profile();
                auto stream_type  = stream_profile.stream_type();
                auto stream_index = stream_profile.stream_index();
                
                auto& buffer = _stream_buffer[{stream_type, stream_index}];
                auto& img    = buffer._image[stream_index] = {};
                img.data = (unsigned char*)f.get_data();
                img.img_h = f.as<rs2::video_frame>().get_height();
                img.img_w = f.as<rs2::video_frame>().get_width();
                img.frame_id = f.get_frame_number();
                img.byte_per_pixel = _stream_to_byte_per_pixel[stream_type];
            }
            if (frames.size() == 0) return nullptr;
        }
    }
    catch (const rs2::error & e) {
        print(e);
        return nullptr;
    }
    
    rs_sf_intrinsics* get_intrinsics(const rs_sf_stream_id& stream) override {
        //switch(stream){
            //case RS2_STREAM_DEPTH:    return (rs_sf_intrinsics*) rs_sf_device_manager::get_intrinsics(stream);
            //case RS2_STREAM_COLOR:    return (rs_sf_intrinsics*) rs_sf_device_manager::get_intrinsics(stream);
            //case RS2_STREAM_INFRARED: return (rs_sf_intrinsics*) rs_sf_device_manager::get_intrinsics(stream);
        //}
        return nullptr;
    }
    
    rs_sf_extrinsics* get_extrinsics(const rs_sf_stream_id& from_stream, const rs_sf_stream_id& to_stream) override
    {
        return nullptr;
    }
    
    float get_depth_unit() override
    {
        return 0.001f;
    }
};


std::unique_ptr<rs_sf_data_stream> rs_sf_create_camera_imu_stream(int w, int h){
    return std::make_unique<rs_sf_d435i_stream>(w, h); }
