#pragma once

#ifndef rs_sf_image_io_h
#define rs_sf_image_io_h

#include <fstream>
#include <iostream>
#include <string>
#include "rs_shapefit.h"
#include "json/json.h"

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

bool rs_sf_image_write(std::string& filename, const rs_sf_image* img)
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

rs_sf_image_ptr rs_sf_image_read(std::string& filename, const int frame_id = 0)
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
        sscanf_s(tmp.c_str(), "%d %d", &dst->img_w, &dst->img_h);
        std::getline(stream_data, tmp);
        if (std::stoi(tmp) == 65535) dst->byte_per_pixel = 2;
        dst->frame_id = frame_id;

        auto* pbuf = stream_data.rdbuf();
        dst = make_image_ptr(&*dst);
        pbuf->sgetn((char*)dst->data, dst->num_char());
        stream_data.close();
    }
    return std::move(dst);
}

struct rs_sf_file_stream
{
    std::vector<rs_sf_image_ptr> image;
    rs_sf_intrinsics depth_intrinsics;
    int num_frame;

    rs_sf_file_stream(const std::string& path, const int frame_num)
    {
        this->depth_intrinsics = read_calibration(path, num_frame);
        if (num_frame <= frame_num) return;

        const auto suffix = std::to_string(frame_num) + ".pgm";
        image.emplace_back(rs_sf_image_read(path + "depth_" + suffix, frame_num));
        image.emplace_back(rs_sf_image_read(path + "ir_" + suffix, frame_num));
    }

    std::vector<rs_sf_image> get_images() const { return{ *image[0], *image[1] }; }

    static void write_frame(const std::string& path, const rs_sf_image* depth, const rs_sf_image* ir, const rs_sf_image* displ)
    {
        rs_sf_image_write(path + "depth_" + std::to_string(depth->frame_id), depth);
        rs_sf_image_write(path + "ir_" + std::to_string(ir->frame_id), ir);
        rs_sf_image_write(path + "displ_" + std::to_string(displ->frame_id), displ);
    }

    static rs_sf_intrinsics read_calibration(const std::string& path, int& num_frame)
    {
        rs_sf_intrinsics depth_intrinsics;
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


#include "example.hpp"
#include <map>

struct rs_sf_gl_context
{
    GLFWwindow* win = nullptr;
    std::string key;
    texture_buffer texture;

    rs_sf_gl_context(const std::string& _key, int w = 1280, int h = 480 * 2) : key(_key) {
        glfwInit();
        win = glfwCreateWindow(w, h, key.c_str(), nullptr, nullptr);
        glfwSetWindowSize(win, w / 2, h / 2);
        glfwMakeContextCurrent(win);
    }

    virtual ~rs_sf_gl_context() {
        if (win) {
            glfwDestroyWindow(win);
            glfwTerminate();
        }
    }

    bool imshow(const rs_sf_image* image, int num_images = 1, const char* text = nullptr)
    {
        if (!glfwWindowShouldClose(win))
        {
            // Wait for new images
            glfwPollEvents();

            // Clear the framebuffer
            int w, h;
            glfwGetFramebufferSize(win, &w, &h);
            glViewport(0, 0, w, h);
            glClear(GL_COLOR_BUFFER_BIT);

            // Draw the images
            glPushMatrix();
            glfwGetWindowSize(win, &w, &h);
            glOrtho(0, w, h, 0, -1, +1);

            auto tiles = static_cast<int>(ceil(sqrt(num_images)));// frames.size())));
            auto tile_w = static_cast<float>(w) / tiles;
            auto tile_h = static_cast<float>(h) / (num_images <= 2 ? 1 : tiles);

            rs_format stream_format[] = { RS_FORMAT_RAW8, RS_FORMAT_Z16, RS_FORMAT_RGB8 };
            for (int index = 0; index < num_images; ++index)
            {
                auto col_id = index / tiles;
                auto row_id = index % tiles;
                auto& frame = image[index];

                texture.upload((void*)frame.data, frame.img_w, frame.img_h, stream_format[frame.byte_per_pixel - 1]);
                texture.show({ row_id * tile_w, col_id * tile_h, tile_w, tile_h }, 1);
            }

            if (text != nullptr)
                draw_text(20 + w / 2, h - 20, text);

            glPopMatrix();
            glfwSwapBuffers(win);

            return true;
        }
        return false;
    }
};



#endif // !rs_sf_image_io_h

