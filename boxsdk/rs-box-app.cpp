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
    std::string calibration_read_path = "", calibration_save_path = "";
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "-calibration_read")) { calibration_read_path = argv[++i]; }
        else if (!strcmp(argv[i], "-co") || !strcmp(argv[i], "-calibration_save")) { calibration_save_path = argv[++i]; }
        else {
            printf("usages:\n box-app.exe [--calibration FILE]");
            return 0;
        }
    }

    rs2::context  ctx;
    rs2::pipeline pipe;

    auto list = ctx.query_devices();
    if (list.size() <= 0) { std::runtime_error("No device detected."); return EXIT_FAILURE; }

    // Assume the first device is our depth camera
    std::string header = std::string(list[0].get_info(RS2_CAMERA_INFO_NAME)) + " Box Scan Example ";
   
    // Declare box detection
    rs2::box_measure boxscan(list[0], read_calibration(calibration_read_path).get());

    // Setup camera pipeline
    auto config = boxscan.get_camera_config();
    auto pipeline = pipe.start(config);

    // save calibration if needed
    save_calibration(calibration_save_path, pipeline);

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    
    box_depth_stablize stablize;
    rs2::box_vector box = boxscan.get_boxes();

    // Start application
    for (window app(boxscan.stream_w() * 3 / 2, boxscan.stream_h(), header.c_str()); app;)
    {
        rs2::frameset frameset; //frame set container
    
        while (!frameset.first_or_default(RS2_STREAM_DEPTH)
            || (!frameset.first_or_default(RS2_STREAM_COLOR) && !frameset.first_or_default(RS2_STREAM_INFRARED)))
        {
            frameset = pipe.wait_for_frames(); //wait until a pair of frames.
        }

        if (auto box_frame = boxscan.process(box.size()&&!app.dense_request()?stablize(frameset):frameset)) //process new frame pair
        {
            // select depth display on the smaller left window
            auto depth_display = color_map(box_frame[app.plane_request() ? RS2_STREAM_PLANE : RS2_STREAM_DEPTH_DENSE]);

            // display the selected output frames
            app.render_ui(depth_display, box_frame[RS2_STREAM_COLOR]);

            // draw box wireframe and text info if any
            if ((box = boxscan.get_boxes()))
            {
                // check box fitness and update depth display on the left window
                if (app.boxca_request()) { app.render_raycast_depth_frame(boxscan, box_frame, box[0], depth_display); }

                app.render_box_on_depth_frame(box[0].project_box_onto_frame(box_frame, RS2_STREAM_DEPTH)); //draw wireframe on depth image
                app.render_box_on_color_frame(box[0].project_box_onto_frame(box_frame, RS2_STREAM_COLOR)); //draw wireframe on color image
                app.render_box_dim(box[0].str()); //draw box information text
                
            }
        }

        if (app.reset_request()) { boxscan.reset(); } //if reset button clicked or bad box does not fit
        boxscan.configure(RS2_STREAM_PLANE, app.plane_request());       //update output request
        boxscan.configure(RS2_STREAM_DEPTH_DENSE, app.dense_request()); //update output request
    }

    return EXIT_SUCCESS;
}
#endif
