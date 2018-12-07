//
//  rs_sf_device_manager.cpp
//  shapefit-dev
//
//  Created by Hon Pong (Gary) Ho on 11/14/18.
//
#include "rs_sf_camera.hpp"
#include "rs_sf_device_manager.hpp"
#include "rs2-custom-calibration-mm.h"
#include <librealsense2/rsutil.h>
#include <librealsense2/rs_advanced_mode.hpp>
#include <iostream>
#include <iomanip>

std::string rs_sf_device_manager::get_device_name() const
{
    return std::string(_device.get_info(RS2_CAMERA_INFO_NAME));
}

bool rs_sf_device_manager::search_device_name(const std::string& device_name)
{
    std::cout << "\n\n----------------------------------------" << std::endl;
    std::cout << "Enumerate RealSense Devices ...             " << std::endl;
    for (auto dev : _ctx.query_devices()){
        _device = dev;
        printf("%s Plugged In.\n", get_device_name().c_str());
        if(get_device_name()==device_name){
            read_custom_fw_data();
            return true;
        }
    }
    _device = nullptr;
    return false;
}

void rs_sf_device_manager::read_custom_fw_data()
{
    try {
        // to use the custom calibration read/write api, the device should be in advanced mode
        if (_device.is<rs400::advanced_mode>())
        {
            rs400::advanced_mode advanced = _device.as<rs400::advanced_mode>();
            if (!advanced.is_enabled()) {
                advanced.toggle_advanced_mode(true);
            }
            
            // initialize custom calibration R/W API
            auto m_dcApi = std::make_unique<crw::MMCalibrationRWAPI>();
            
            // mydev should be a pointer to rs2::device from librealsense
            if (m_dcApi->Initialize(&_device)==DC_MM_SUCCESS)
            {
                // the custom data area has DC_MM_CUSTOM_DATA_SIZE (504 bytes) of storage space. User can
                // define their data own format and write to the device and retrieve it later from
                // the device
                
                // on a new device, the storage is not initialzed, so should write data before perform
                // read. reading before writing any data will fail.
                
                uint8_t mmCustom[DC_MM_CUSTOM_DATA_SIZE];
                memset((void*)&mmCustom, 0x0, sizeof(mmCustom));
                
                // read the custom data back from the device
                if( m_dcApi->ReadMMCustomData(mmCustom)==DC_MM_SUCCESS ){
                    std::cout << "Custom FW Data " << sizeof(mmCustom) << " bytes ";
                    for(int c=0; c<DC_MM_CUSTOM_DATA_SIZE; ++c){
                        if(c%10==0){ std::cout << std::endl; }
                        std::cout << (int)mmCustom[c] << ",";
                    }
                    std::cout << "\n----------------------------------------\n" << std::endl;
                }
            }
        }
    }catch(std::exception& e){ fprintf(stderr,"WARNING: unable to read custom FW data \n%s\n",e.what()); }
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
                   (profile.fps()==(int)stream.fps || stream.fps<0.0f) &&
                   (profile.format()==stream.format)){
                    if(profile.is<rs2::motion_stream_profile>() ||
                       ((profile.as<rs2::video_stream_profile>().width()  == stream.width) &&
                        (profile.as<rs2::video_stream_profile>().height() == stream.height))){
                           stream.index   = profile.stream_index();
                           stream.fps     = (float)profile.fps();
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
            try {
                stream.imu_intriniscs = stream.profile.as<rs2::motion_stream_profile>().get_motion_intrinsics();
            }catch(...){ std::cout << "WARNING: error getting imu intrsincis " << (rs2_stream)stream.profile.stream_type() << " " << stream.profile.stream_index() << std::endl; }
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
        std::cout << "stream name   : " << s.stream_name() << std::endl;
        std::cout << "stream type   : " << rs2_stream_to_string(s.stream_type()) << std::endl;
        std::cout << "stream index  : " << s.stream_index() << std::endl;
        std::cout << "stream id     : " << s.unique_id() << std::endl;
        std::cout << "stream fps    : " << s.fps() << "Hz " << std::endl;
        std::cout << "stream format : " << rs2_format_to_string(s.format()) << std::endl;
        if(s.is<rs2::video_stream_profile>()){
            auto ss = s.as<rs2::video_stream_profile>();
            std::cout << "stream w,h    : " << ss.width() << "," << ss.height() << std::endl;
        }
        std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
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
        std::cout << std::setprecision(3) << (rs2_stream)s.stream_type() << " stream " << s.stream_index() << std::endl <<
        "fx : " << stream.cam_intrinsics.fx    << ", fy : " << stream.cam_intrinsics.fy     << std::endl <<
        "px : " << stream.cam_intrinsics.ppx   << ", py : " << stream.cam_intrinsics.ppy    << std::endl <<
        "w  : " << stream.cam_intrinsics.width << ", h  : " << stream.cam_intrinsics.height << std::endl;
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
