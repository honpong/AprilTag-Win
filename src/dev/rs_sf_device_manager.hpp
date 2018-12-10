//
//  rs_sf_device_manager.hpp
//  shapefit-core
//
//  Created by Hon Pong (Gary) Ho on 11/14/18.
//

#ifndef rs_sf_device_manager_h
#define rs_sf_device_manager_h

#include <librealsense2/rs.hpp>

struct rs_sf_stream_select
{
    rs2_stream  type;
    int         index;
    float       fps;
    rs2_format  format;
    int         width;
    int         height;
    
    rs2_timestamp_domain        timestamp_domain;
    rs2::sensor                 sensor;
    rs2::stream_profile         profile;
    union {
        rs2_intrinsics              cam_intrinsics;
        rs2_motion_device_intrinsic imu_intriniscs;
    };
    std::vector<rs2_extrinsics> extrinsics;
};

struct rs_sf_device_manager
{
protected:
    rs2::context                     _ctx;
    rs2::device                      _device;
    std::vector<rs_sf_stream_select> _streams;
    
    int  num_streams() const { return (int)_streams.size(); }
    bool search_device_name(const std::string& device_name);
    std::string get_device_name() const;
    void add_stream_request(const rs_sf_stream_select& stream_request);
    void find_stream_profiles();
    void extract_stream_calibrations();
    
    void print_requested_streams() const;
    void print_calibrations() const;
    
    void read_custom_fw_data();
    
    static void print(const rs2::error& e);
};

#endif /* rs_sf_device_manager_h */
