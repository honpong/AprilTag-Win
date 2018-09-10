#pragma once

#ifndef rs_sf_image_io_h
#define rs_sf_image_io_h

#include "rs_shapefit.h"
#include "rs_sf_camera.hpp"
#include "json/json.h"

#include <fstream>
#include <iostream>
#include <string>

#define make_mono_ptr(...) std::make_unique<rs_sf_image_mono>(__VA_ARGS__)
#define make_depth_ptr(...) std::make_unique<rs_sf_image_depth>(__VA_ARGS__)
#define make_rgb_ptr(...) std::make_unique<rs_sf_image_rgb>(__VA_ARGS__)

rs_sf_image_ptr make_image_ptr(const rs_sf_image* ref, int byte_per_pixel = -1)
{
    byte_per_pixel = (byte_per_pixel <= 0 ? ref->byte_per_pixel : byte_per_pixel);
    switch (byte_per_pixel) {
    case 1: return make_mono_ptr(ref);
    case 2: return make_depth_ptr(ref);
    case 3: default: return make_rgb_ptr(ref);
    }
}

bool rs_sf_image_write(const std::string& filename, const rs_sf_image* img)
{
    if (!img || !img->data) return false;

    std::ofstream stream_data;
    stream_data.open((filename + (img->byte_per_pixel == 3 ? ".ppm" : ".pgm")).c_str(), 
        std::ios_base::out | std::ios_base::binary);
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

rs_sf_image_ptr rs_sf_image_read(const std::string& filename, const int frame_id = 0)
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

struct rs_sf_file_stream : rs_sf_image_stream 
{
    std::string folder_path, file_prefix[RS_SF_STREAM_COUNT] = { "","depth_","color_","ir_","ir2_","fisheye_" }, file_format[RS_SF_STREAM_COUNT] = { "",".pgm",".ppm",".pgm",".pgm",".pgm" };
    int total_num_frame, next_frame_num;
    std::vector<rs_sf_image_ptr> image;
    rs_sf_image images[RS_SF_STREAM_COUNT];
    rs_sf_intrinsics intrinsics[RS_SF_STREAM_COUNT];
    rs_sf_extrinsics color_to_depth;
    float depth_unit;

    rs_sf_file_stream(const std::string& path) : folder_path(path), total_num_frame(0), next_frame_num(0), image(RS_SF_STREAM_COUNT)
    {
        intrinsics[RS_SF_STREAM_DEPTH] = read_depth_calibration(folder_path, total_num_frame, depth_unit);
        intrinsics[RS_SF_STREAM_COLOR] = read_color_calibration(folder_path, total_num_frame, color_to_depth);
        intrinsics[RS_SF_STREAM_INFRARED] = intrinsics[RS_SF_STREAM_COLOR];
    }

    rs_sf_intrinsics* get_intrinsics(int stream = RS_SF_STREAM_DEPTH) override { return &intrinsics[stream]; }
    rs_sf_extrinsics* get_extrinsics(int from, int to) override { return &color_to_depth; }
    float get_depth_unit() override { return depth_unit; }
    
    rs_sf_image* get_images() override
    {
        next_frame_num = get_next_frame_num();
        for (auto& i : images) i = {};
        for (auto&& stream : { RS_SF_STREAM_DEPTH, RS_SF_STREAM_COLOR, RS_SF_STREAM_INFRARED }) {
            const auto file_path = folder_path + file_prefix[stream] + std::to_string(next_frame_num) + file_format[stream];
            images[stream] = *(image[stream] = rs_sf_image_read(file_path, next_frame_num));
            images[stream].intrinsics = image[stream]->intrinsics = get_intrinsics(stream);
        }
        images[RS_SF_STREAM_COLOR] = image[RS_SF_STREAM_COLOR]->set_pose(color_to_depth);
        ++next_frame_num;
        return images;
    }

    bool is_end_of_stream() const { return next_frame_num >= total_num_frame; }
    int get_next_frame_num() const { return next_frame_num % total_num_frame; }

    static void write_frame(const std::string& path, const rs_sf_image* depth, const rs_sf_image* color, const rs_sf_image* displ)
    {
        rs_sf_image_write(path + "depth_" + std::to_string(depth->frame_id), depth);
        rs_sf_image_write(path + "color_" + std::to_string(color->frame_id), color);
        rs_sf_image_write(path + "displ_" + std::to_string(displ->frame_id), displ);
    }
    
    static Json::Value read_json(const std::string& path)
    {
        Json::Value json_data;
        std::ifstream infile;
        infile.open(path + "calibration.json", std::ifstream::binary);
        infile >> json_data;
        return json_data;
    }
    
    static rs_sf_intrinsics read_intrinsics(const Json::Value json_intr)
    {
        rs_sf_intrinsics intrinsics = {};
        intrinsics.fx    = json_intr["fx"].asFloat();
        intrinsics.fy    = json_intr["fy"].asFloat();
        intrinsics.ppx   = json_intr["ppx"].asFloat();
        intrinsics.ppy   = json_intr["ppy"].asFloat();
        intrinsics.width = json_intr["width"].asInt();
        intrinsics.height= json_intr["height"].asInt();
        intrinsics.model = (rs_sf_distortion)json_intr["model"].asInt();
        for (int c = 0; c < json_intr["coeff"].size(); ++c)
            intrinsics.coeffs[c] = json_intr["coeff"][c].asFloat();
        return intrinsics;
    }

    static rs_sf_intrinsics read_depth_calibration(const std::string& path, int& num_frame, float& depth_unit)
    {
        Json::Value json_data = read_json(path);
        num_frame = json_data["depth_cam"]["num_frame"].asInt();
        depth_unit = json_data["depth_cam"].get("depth_unit", 0.001f).asFloat();
        return read_intrinsics(json_data["depth_cam"]["intrinsics"]);
    }
    
    static rs_sf_intrinsics read_color_calibration(const std::string& path, int& num_frame, rs_sf_extrinsics& color_to_depth)
    {
        Json::Value json_data = read_json(path);
        
        num_frame = json_data["color_cam"]["num_frame"].asInt();
        for (int r = 0; r < 9; ++r ){ color_to_depth.rotation[r] = json_data["color_cam"]["to_depth"]["rotation"][r].asFloat(); }
        for (int t = 0; t < 3; ++t ){ color_to_depth.translation[t] = json_data["color_cam"]["to_depth"]["translation"][t].asFloat(); }
        
        return read_intrinsics(json_data["color_cam"]["intrinsics"]);
    }
    
    static void write_calibration(const std::string& path,
                                  const rs_sf_intrinsics& depth_intrinsics,
                                  const rs_sf_intrinsics& color_intrinsics,
                                  const rs_sf_extrinsics& color_to_depth,
                                  int num_frame, float depth_unit)
    {
        auto write_intr = [](const rs_sf_intrinsics& intr){
            Json::Value json_intr;
            json_intr["fx"]     = intr.fx;
            json_intr["fy"]     = intr.fy;
            json_intr["ppx"]    = intr.ppx;
            json_intr["ppy"]    = intr.ppy;
            json_intr["height"] = intr.height;
            json_intr["width"]  = intr.width;
            json_intr["model"]  = intr.model;
            for (const auto& c : intr.coeffs)
                json_intr["coeff"].append(c);
            return json_intr;
        };
        
        Json::Value root;
        root["calibration_file_version"] = 1;
        root["depth_cam"]["intrinsics"] = write_intr(depth_intrinsics);
        root["depth_cam"]["depth_unit"] = depth_unit;
        root["depth_cam"]["num_frame"] = num_frame;

        root["color_cam"]["intrinsics"] = write_intr(color_intrinsics);
        root["color_cam"]["num_frame"] = num_frame;
        for (const auto& r : color_to_depth.rotation)
            root["color_cam"]["to_depth"]["rotation"].append(r);
        for (const auto& t : color_to_depth.translation)
            root["color_cam"]["to_depth"]["translation"].append(t);
        
        try {
            Json::StyledStreamWriter writer;
            std::ofstream outfile;
            outfile.open(path + "calibration.json");
            writer.write(outfile, root);
        }
        catch (...) {
            printf("fail to write calibration to %s \n",path.c_str());
        }
    }
    
};


#endif // !rs_sf_image_io_h

