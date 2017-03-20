#pragma once

#ifndef rs_sf_image_io_h
#define rs_sf_image_io_h

#include <fstream>
#include <iostream>
#include <string>
#include "rs_shapefit.h"


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

std::unique_ptr<rs_sf_image_auto> rs_sf_image_read(std::string& filename, const int frame_id = 0)
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
        dst->src = std::make_unique<uint8_t[]>(dst->num_char());
        dst->data = dst->src.get();
        pbuf->sgetn((char*)dst->data, dst->num_char());
        dst->cam_pose = nullptr;

        stream_data.close();
    }

    return std::move(dst);
}

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

