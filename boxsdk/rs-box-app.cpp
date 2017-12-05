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

#if 0
#define STB_IMAGE_IMPLEMENTATION
#include "../third-party/stb_image.h"
#include <fstream>
void write_icon(){
    int icon_w, icon_h, icon_n;
    auto *icon_data = stbi_load("C:\\temp\\unit.png", &icon_w, &icon_h, &icon_n, 0);


    std::ofstream myfile; myfile.open("C:\\temp\\unit.txt", std::ios::out);
    for (int y = 0; y < icon_h; ++y, myfile << std::endl) {
        printf("\r%d", y);
        for (int x = 0; x < icon_w; ++x, myfile << ",") {
            myfile << "0x";
            for (int c = 0; c < 1; ++c) {
                char tmp[32];
                sprintf(tmp, "%02x", icon_data[(y*icon_w + x) * icon_n + c]);
                myfile << tmp;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    write_icon(); return 0;
#else

int main(int argc, char* argv[])
{
#endif

    rs2::context  ctx;
    rs2::pipeline pipe;

    auto list = ctx.query_devices();
    if (list.size() <= 0) std::runtime_error("No device detected.");

    auto dev = list[0];
    std::string header = std::string(dev.get_info(RS2_CAMERA_INFO_NAME)) + " Box Scan Example ";
   
    // Declare box detection
    rs2::box_measure boxscan(dev);

    // Setup camera pipeline
    auto config = boxscan.get_camera_config();
    auto pipeline = pipe.start(config);

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    // Start application
    for(window app(boxscan.stream_w() * 3 / 2, boxscan.stream_h(), header.c_str());app;)
    {
        rs2::frameset frameset;

        while (!frameset.first_or_default(RS2_STREAM_DEPTH)
            || !frameset.first_or_default(RS2_STREAM_COLOR)
            )
        {
            frameset = pipe.wait_for_frames();
        }

        auto box_frame = boxscan.process(frameset);
        auto box = boxscan.get_boxes();

        app.render_ui(color_map(frameset.get_depth_frame()), frameset.get_color_frame());
        //app.render_ui(box_frame[1], frameset.get_color_frame());

        if (box.size() && box_frame.size())
        {
            app.render_box_on_depth_frame(box[0].project_box_onto_frame(box_frame.state(RS2_STREAM_DEPTH)).end_pt);
            app.render_box_on_color_frame(box[0].project_box_onto_frame(box_frame.state(RS2_STREAM_COLOR)).end_pt);
            app.render_box_dim(box[0].str()); 
        }

        if (app.reset_request()) {
            boxscan.reset();
       }
    }

    return EXIT_SUCCESS;
}

#endif
