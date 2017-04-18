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
    rs_sf_intrinsics depth_intrinsics;

    rs_sf_file_stream(const std::string& path) : folder_path(path), total_num_frame(0), next_frame_num(0), image(RS_SF_STREAM_COUNT)
    {
        depth_intrinsics = read_calibration(folder_path, total_num_frame);
    }

    rs_sf_intrinsics* get_intrinsics() override { return &depth_intrinsics; }
    
    rs_sf_image* get_images() override
    {
        next_frame_num = get_next_frame_num();
        for (auto&& stream : { RS_SF_STREAM_DEPTH, RS_SF_STREAM_INFRARED }) {
            const auto file_path = folder_path + file_prefix[stream] + std::to_string(next_frame_num) + file_format[stream];
            images[stream] = *(image[stream] = rs_sf_image_read(file_path, next_frame_num));
        }
        ++next_frame_num;
        return images;
    }

    bool is_end_of_stream() const { return next_frame_num >= total_num_frame; }
    int get_next_frame_num() const { return next_frame_num % total_num_frame; }

    static void write_frame(const std::string& path, const rs_sf_image* depth, const rs_sf_image* ir, const rs_sf_image* displ)
    {
        rs_sf_image_write(path + "depth_" + std::to_string(depth->frame_id), depth);
        rs_sf_image_write(path + "ir_" + std::to_string(ir->frame_id), ir);
        rs_sf_image_write(path + "displ_" + std::to_string(displ->frame_id), displ);
    }

    static rs_sf_intrinsics read_calibration(const std::string& path, int& num_frame)
    {
        rs_sf_intrinsics depth_intrinsics = {};
        Json::Value calibration_data;
        std::ifstream infile;
        infile.open(path + "calibration.json", std::ifstream::binary);
        infile >> calibration_data;

        Json::Value json_depth_intrinsics = calibration_data["depth_cam"]["intrinsics"];
        depth_intrinsics.fx = json_depth_intrinsics["fx"].asFloat();
        depth_intrinsics.fy = json_depth_intrinsics["fy"].asFloat();
        depth_intrinsics.ppx = json_depth_intrinsics["ppx"].asFloat();
        depth_intrinsics.ppy = json_depth_intrinsics["ppy"].asFloat();
        depth_intrinsics.width = json_depth_intrinsics["width"].asInt();
        depth_intrinsics.height = json_depth_intrinsics["height"].asInt();

        num_frame = calibration_data["depth_cam"]["num_frame"].asInt();
        return depth_intrinsics;
    }

    static void write_calibration(const std::string& path, const rs_sf_intrinsics& intrinsics, int num_frame)
    {
        Json::Value json_intr, root;
        json_intr["fx"] = intrinsics.fx;
        json_intr["fy"] = intrinsics.fy;
        json_intr["ppx"] = intrinsics.ppx;
        json_intr["ppy"] = intrinsics.ppy;
        json_intr["model"] = intrinsics.model;
        json_intr["height"] = intrinsics.height;
        json_intr["width"] = intrinsics.width;
        for (const auto& c : intrinsics.coeffs)
            json_intr["coeff"].append(c);
        root["depth_cam"]["intrinsics"] = json_intr;
        root["depth_cam"]["num_frame"] = num_frame;

        try {
            Json::StyledStreamWriter writer;
            std::ofstream outfile;
            outfile.open(path + "calibration.json");
            writer.write(outfile, root);
        }
        catch (...) {}
    }
};


#endif // !rs_sf_image_io_h

