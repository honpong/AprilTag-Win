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
    int         fps;
    rs2_format  format;
    int         width;
    int         height;
    
    rs2::sensor                 sensor;
    rs2::stream_profile         profile;
    rs2_intrinsics              intrinsics;
    std::vector<rs2_extrinsics> extrinsics;
};

struct rs_sf_device_manager
{
protected:
    rs2::context                     _ctx;
    rs2::device                      _device;
    std::vector<rs_sf_stream_select> _streams;
    
    int  num_streams() const { return _streams.size(); }
    bool search_device_name(const std::string& device_name);
    std::string get_device_name() const;
    void add_stream_request(const rs_sf_stream_select& stream_request);
    void find_stream_profiles();
    void extract_stream_calibrations();
    
    void print_requested_streams() const;
    void print_calibrations() const;
    
    static void print(const rs2::error& e);
};

#endif /* rs_sf_device_manager_h */
