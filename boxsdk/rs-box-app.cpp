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
int main(int argc, char* argv[])
{
    int w = 640, h = 480;

    rs2::context  ctx;
    rs2::config   config;
    rs2::pipeline pipe;

    auto list = ctx.query_devices();
    if (list.size() <= 0) std::runtime_error("No device detected.");

    auto dev = list[0];
    auto camera_name = dev.get_info(RS2_CAMERA_INFO_NAME);

    std::string header = " Box Scan Example ";
    if (!strcmp(camera_name, "Intel RealSense 410")) {
        w = 1280; h = 720;
    }
    else if (!strcmp(camera_name, "Intel RealSense SR300")) {
    }
    header = camera_name + header;

    config.enable_stream(RS2_STREAM_DEPTH, 0, w, h, RS2_FORMAT_Z16);
    config.enable_stream(RS2_STREAM_COLOR, 0, w, h, RS2_FORMAT_RGB8);

    auto pipeline_profile = pipe.start(config);

    rs2::box_measure boxscan(dev);

    window app(w, h, header.c_str());
    app.on_key_release = [&app](int key) { if (key == 'q' || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(&*app, GL_TRUE); };
    texture depth_image, color_image, plane_image;

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
  
    while (app)
    {
        rs2::frameset frameset;

        while (!frameset.first_or_default(RS2_STREAM_DEPTH)
            || !frameset.first_or_default(RS2_STREAM_COLOR))
        {
            frameset = pipe.wait_for_frames();
        }
      
        auto boxes_frame = boxscan.process(frameset);
        auto depth_frame = frameset.get_depth_frame();
        auto color_frame = frameset.get_color_frame();
        auto depth_color = color_map(depth_frame);

        std::string box_msg;
        auto box = boxscan.get_boxes();
        if (box.size()) { box_msg += box[0].str(); }
        depth_image.render(depth_color, { 0, 0, app.width() / 2, app.height() / 2 }, "");
        color_image.render(color_frame, { app.width() / 2, 0, app.width() / 2, app.height() / 2 }, box_msg.c_str());
        plane_image.render(boxes_frame, { 0, app.height() / 2, app.width() / 2, app.height() / 2 }, box_msg.c_str());
        draw_text(app.width() / 2 + 15, app.height() / 2 + 20, box_msg.c_str());
    }

    return EXIT_SUCCESS;
}
#endif
