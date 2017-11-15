/*******************************************************************************
 
 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or nondisclosure
 agreement with Intel Corporation and may not be copied or disclosed except in
 accordance with the terms of that agreement
 Copyright(c) 2017 Intel Corporation. All Rights Reserved.
 
*******************************************************************************/
//
//  rs-box-app.cpp
//  boxapp
//
//  Created by Hon Pong (Gary) Ho.
//

#include <rs_box_sdk.hpp>
#include "rs-box-app.hpp"

using namespace rs2;
#if 0
int main(int argc, char* argv[])
{
    rs2::context ctx;
    rs2::context config;
    rs2::pipeline pipe;
    pipe.start();
    return 0;
}
#else 
#define STB_IMAGE_IMPLEMENTATION
#include "../third-party/stb_image.h"
#include <fstream>
void write_icon(){
    int icon_w, icon_h, icon_n;
    auto *icon_data = stbi_load("C:\\temp\\reset_button.png", &icon_w, &icon_h, &icon_n, 0);

    std::ofstream myfile; myfile.open("C:\\temp\\reset_icon.txt", std::ios::out);
    for (int y = 0; y < icon_h; ++y, myfile << std::endl)
        for (int x = 0; x < icon_w; ++x) 
            for(int n=0; n<icon_n; ++n){
            char tmp[64];
            sprintf(tmp, "0x%02x", icon_data[(y*icon_w + x)*icon_n + n]);
            myfile << tmp << ", ";
        }
}

int main(int argc, char* argv[])
{
    //write_icon();

    int w = 640, h = 480;

    rs2::context  ctx;
    rs2::config   config;
    rs2::pipeline pipe;

    auto list = ctx.query_devices();
    if (list.size() <= 0) std::runtime_error("No device detected.");

    auto dev = list[0];
    auto camera_name = dev.get_info(RS2_CAMERA_INFO_NAME);

    std::string header = " Box Scan Example ";
    if (!strcmp(camera_name, "Intel RealSense 415")) {
        w = 1280 / 2; h = 720 / 2;
    }
    else if (!strcmp(camera_name, "Intel RealSense SR300")) {
    }
    header = camera_name + header;

    config.enable_stream(RS2_STREAM_DEPTH, 0, w, h, RS2_FORMAT_Z16);
    config.enable_stream(RS2_STREAM_COLOR, 0, w, h, RS2_FORMAT_RGB8);

    auto pipeline_profile = pipe.start(config);
   
    rs2::box_measure boxscan(dev);

    window app(w * 3 / 2, h, header.c_str());
    app.on_key_release = [&app](int key) { if (key == 'q' || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(&*app, GL_TRUE); };
    texture depth_image, color_image, realsense_logo, close_icon, reset_icon;

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    
    int icon_w, icon_h, icon_n;
    auto *close_data = stbi_load("C:\\temp\\close_icon.png", &icon_w, &icon_h, &icon_n, 0);
    auto *reset_data = stbi_load("C:\\temp\\reset_icon.png", &icon_w, &icon_h, &icon_n, 0);

    while (app)
    {
        rs2::frameset frameset;

        while (!frameset.first_or_default(RS2_STREAM_DEPTH)
            || !frameset.first_or_default(RS2_STREAM_COLOR)
            )
        {
            frameset = pipe.wait_for_frames();
        }

        auto box_frame = boxscan.process(frameset);
        auto depth_frame = frameset.get_depth_frame();
        auto color_frame = frameset.get_color_frame();
        auto depth_color = color_map(depth_frame);

        std::string box_msg;
        auto box = boxscan.get_boxes();
        if (box.size()) { box_msg += "   Box 1 : " + box[0].str(); }

        const float app_h = app.height(), app_hd2 = app_h / 2, app_hd6 = app_h / 6, app_w = app.width(), app_wd3 = app_w / 3;
        depth_image.render(depth_color, { 0, app_h / 4, app_wd3, app_hd2 }, "");
        color_image.render(color_frame, { app_wd3, 0, app_wd3 * 2, app_h }, "");
        realsense_logo.render(rs2::box_measure::get_icon, { 0, 0, app_wd3, app_hd6 });
        if (box.size()) {
            color_image.draw_box(box_frame.project_box_onto_color(box[0]).end_pt);
            depth_image.draw_box(box_frame.project_box_onto_depth(box[0]).end_pt);
            draw_text(0, 15 + app_hd6, box_msg.c_str());
        }

        static int iw = 64, ih = 64;
        close_icon.render(close_data, iw, ih, { app_wd3 / 4 - app_hd6 / 4,app_h * 15 / 16 - app_hd6 / 4, app_hd6 / 2, app_hd6 / 2 }, "" );
        reset_icon.render(reset_data, iw, ih, { app_w / 4 - app_hd6 / 4,app_h * 15 / 16 - app_hd6 / 4, app_hd6 / 2, app_hd6 / 2 }, "");

        glPushMatrix();
        glTranslatef(1, app_h * 7 / 8, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2f(0, 0);
        glVertex2f(app_wd3 / 2, 0);
        glVertex2f(app_wd3 / 2, app_h / 8);
        glVertex2f(0, app_h / 8);
        glEnd();
        glPopMatrix();

        glPushMatrix();
        glTranslatef(1 + app_wd3 / 2, app_h * 7 / 8, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2f(0, 0);
        glVertex2f(app_wd3 / 2, 0);
        glVertex2f(app_wd3 / 2, app_h / 8);
        glVertex2f(0, app_h / 8);
        glEnd();
        glPopMatrix();
     

        //draw_text(app_wd2 + 15, app_hd2 + 20, box_msg.c_str());
        //box_frame.draw_pose_text(app_wd2 + 15, app_hd2 + 40, 20, draw_text);
    }

    return EXIT_SUCCESS;
}

#endif
