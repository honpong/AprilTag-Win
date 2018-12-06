//
//  rs_sf_file_io.hpp
//  shapefit-dev
//
//  Created by Hon Pong (Gary) Ho on 11/14/18.
//
#pragma once

#ifndef rs_sf_file_io_h
#define rs_sf_file_io_h

#include <json/json.h>
#include <fstream>
#include <iostream>
#include <string>
#include <future>
#include <algorithm>

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define RS_SF_CLEAR_DIRECTORY(path) {auto rtn=system(("RMDIR /Q /S " + path + " & MKDIR " + path).c_str());}
#ifndef PATH_SEPARATER
#define PATH_SEPARATER '\\'
#endif
#else
#define RS_SF_CLEAR_DIRECTORY(path) {auto rtn=system(("rm -rf " + path + ";mkdir " + path + " 2>/dev/null;").c_str());}
#ifndef PATH_SEPARATER
#define PATH_SEPARATER '/'
#endif
#endif

struct rs_sf_file_io
{
    const std::string _index_filename       = "index.txt";
    const std::string _calibration_filename = "calibration.json";
    
    std::string _folder_path;
    std::string _file_prefix[RS_SF_SENSOR_TYPE_COUNT] = { "","depth_","color_","ir_","fisheye_","gyro_","accel_"};
    std::string _file_format[RS_SF_SENSOR_TYPE_COUNT] = { "",".pgm",".ppm",".pgm",".pgm","",""};
    
    rs_sf_serial_number _next_frame_num[RS_SF_SENSOR_TYPE_COUNT][3] = {};
    
    rs_sf_file_io(const std::string& path) : _folder_path(path)
    {
        if(_folder_path.back()!=PATH_SEPARATER){ _folder_path.push_back(PATH_SEPARATER); }
        
        for(int i=1; i<5; ++i){
            _file_prefix[i|RS_SF_SENSOR_LASER_OFF]=_file_prefix[i]+"off_";
            _file_format[i|RS_SF_SENSOR_LASER_OFF]=_file_format[i];
        }
    }
    
    std::string get_index_filepath() const
    {
        return _folder_path + _index_filename;
    }
    
    std::string get_calibration_filepath() const
    {
        return _folder_path + _calibration_filename;
    }
    
    std::string get_stream_name(const rs_sf_sensor_t& sensor_type, const rs_sf_uint16_t& sensor_index) const
    {
        return _file_prefix[sensor_type] + (sensor_index>0?std::to_string(sensor_index)+"_":"");
    }
    
    std::string get_stream_name(const rs_sf_data& data) const
    {
        return get_stream_name(data.sensor_type, data.sensor_index);
    }
    
    static Json::Value read_json(const std::string& path)
    {
        Json::Value json_data;
        std::ifstream infile;
        infile.open(path, std::ifstream::binary);
        infile >> json_data;
        return json_data;
    }
    
    static rs_sf_stream_intrinsics read_intrinsics(const rs_sf_sensor_t& type, const Json::Value& json_intr)
    {
        rs_sf_stream_intrinsics intr = {};
        
        switch(type){
            case RS_SF_SENSOR_ACCEL:
            case RS_SF_SENSOR_GYRO:
                for(auto& r : {0,1,2})
                    for(auto& c : {0,1,2,3})
                        intr.imu_intrinsics.data[r][c] = json_intr["data"][r][c].asFloat();
                for(auto& n : {0,1,2})
                    intr.imu_intrinsics.noise_variances[n] = json_intr["noise_variances"][n].asFloat();
                for(auto& b : {0,1,2})
                    intr.imu_intrinsics.bias_variances[b] = json_intr["bias_variances"][b].asFloat();
                break;
            default:
                intr.cam_intrinsics.fx    = json_intr["fx"].asFloat();
                intr.cam_intrinsics.fy    = json_intr["fy"].asFloat();
                intr.cam_intrinsics.ppx   = json_intr["ppx"].asFloat();
                intr.cam_intrinsics.ppy   = json_intr["ppy"].asFloat();
                intr.cam_intrinsics.width = json_intr["width"].asInt();
                intr.cam_intrinsics.height= json_intr["height"].asInt();
                intr.cam_intrinsics.model = (rs_sf_distortion)json_intr["model"].asInt();
                for (int c = 0; c < (int)json_intr["coeff"].size(); ++c)
                    intr.cam_intrinsics.coeffs[c] = json_intr["coeff"][c].asFloat();
                break;
        }
        return intr;
    }
    
    static rs_sf_imu_intrinsics read_imu_intrinsics(const Json::Value& json_intr)
    {
        rs_sf_imu_intrinsics intrinsics;
        return intrinsics;
    }
    
    static rs_sf_extrinsics read_extrinsics(const Json::Value& json_extr)
    {
        rs_sf_extrinsics extrinsics = {};
        for(int r = 0; r < (int)json_extr["rotation"].size(); ++r)
            extrinsics.rotation[r] = json_extr["rotation"][r].asFloat();
        for(int t = 0; t < (int)json_extr["translation"].size(); ++t)
            extrinsics.translation[t] = json_extr["translation"][t].asFloat();
        return extrinsics;
    }
    
    static Json::Value write_intrinsics(const rs_sf_sensor_t& type, const rs_sf_stream_intrinsics& intr)
    {
        Json::Value json_intr;

        switch(type){
            case RS_SF_SENSOR_GYRO:
            case RS_SF_SENSOR_ACCEL:
                for(auto& r : {0,1,2}){
                    for(auto& c : intr.imu_intrinsics.data[r]){
                        json_intr["data"][r].append(c); }
                }
                for(auto& noise : intr.imu_intrinsics.noise_variances){
                    json_intr["noise_variances"].append(noise); }
                for(auto& bias : intr.imu_intrinsics.bias_variances){
                    json_intr["bias_variances"].append(bias); }
            break;
            default:
                json_intr["fx"]     = intr.cam_intrinsics.fx;
                json_intr["fy"]     = intr.cam_intrinsics.fy;
                json_intr["ppx"]    = intr.cam_intrinsics.ppx;
                json_intr["ppy"]    = intr.cam_intrinsics.ppy;
                json_intr["height"] = intr.cam_intrinsics.height;
                json_intr["width"]  = intr.cam_intrinsics.width;
                json_intr["model"]  = intr.cam_intrinsics.model;
                for (const auto& c : intr.cam_intrinsics.coeffs){
                    json_intr["coeff"].append(c); }
            break;
        }
        return json_intr;
    }
    
    static Json::Value write_extrinsics(const rs_sf_extrinsics& extr)
    {
        Json::Value json_extr;
        for(const auto& r : extr.rotation){
            json_extr["rotation"].append(r);
        }
        for(const auto& t : extr.translation){
            json_extr["translation"].append(t);
        }
        return json_extr;
    }
    
    static rs_sf_image_ptr make_image_ptr(const rs_sf_image* ref, int byte_per_pixel = -1)
    {
        byte_per_pixel = (byte_per_pixel <= 0 ? ref->byte_per_pixel : byte_per_pixel);
        switch (byte_per_pixel) {
            case 1: return std::make_unique<rs_sf_image_mono>(ref);
            case 2: return std::make_unique<rs_sf_image_depth>(ref);
            case 3: default: return std::make_unique<rs_sf_image_rgb>(ref);
        }
    }
    
    static rs_sf_image_ptr rs_sf_image_read(const std::string& filename, const rs_sf_serial_number& frame_id)
    {
        std::string tmp;
        std::ifstream stream_data;
        stream_data.open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
        
        auto dst = std::make_unique<rs_sf_image_auto>();
        if (stream_data.is_open())
        {
            std::getline(stream_data, tmp);
            dst->byte_per_pixel = (tmp == "P6" ? 3 : 1);
            std::getline(stream_data, tmp);
            sscanf(tmp.c_str(), "%d %d", &dst->img_w, &dst->img_h);
            std::getline(stream_data, tmp);
            if (std::stoi(tmp) == 65535) dst->byte_per_pixel = 2;
            dst->frame_id = frame_id;
            
            auto* pbuf = stream_data.rdbuf();
            dst = make_image_ptr(&*dst);
            pbuf->sgetn((char*)dst->data, dst->num_char());
            stream_data.close();
        }
        return dst;
    }
    
    static bool rs_sf_image_write(const std::string& filename, const rs_sf_image* img)
    {
        if (!img || !img->data) return false;
        
        std::ofstream stream_data;
        //stream_data.open((filename + (img->byte_per_pixel == 3 ? ".ppm" : ".pgm")).c_str(), std::ios_base::out | std::ios_base::binary);
        stream_data.open(filename.c_str(), std::ios_base::out | std::ios_base::binary );
        
        if (stream_data.is_open())
        {
            stream_data.seekp(0, std::ios_base::beg);
            
            stream_data << (img->byte_per_pixel == 3 ? "P6" : "P5") << std::endl;
            stream_data << img->img_w << " " << img->img_h << std::endl;
            stream_data << (img->byte_per_pixel != 2 ? "255" : "65535") << std::endl;
            
            auto* pbuf = stream_data.rdbuf();
            pbuf->sputn((char*)img->data, img->num_char());
            stream_data.close();
            return true;
        }
        return false;
    }
};

#endif /* rs_sf_file_io_h */
