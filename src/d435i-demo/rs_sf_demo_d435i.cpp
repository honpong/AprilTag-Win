//
//  rs_sf_demo_d435i.cpp
//  d435i-demo
//
//  Created by Hon Pong (Gary) Ho on 11/6/18.
//

#include "rs_shapefit.h"
#include "rs_sf_camera.hpp"
#include "rs_sf_image_io.hpp"
#include "rs_sf_gl_context.hpp"
#include "rs_sf_pose_tracker.h"

#include <chrono>

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define PATH_SEPARATER '\\'
#define DEFAULT_PATH "c:\\temp\\shapefit\\b\\"
#else
#define PATH_SEPARATER '/'
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/temp/shapefit/1/")
#define DEFAULT_PATH (std::string(getenv("HOME"))+"/Desktop/temp/data/")
#endif

int capture_frames(const std::string& path, const int image_set_size, const int cap_size[2]);
int capture_frames2(const std::string& path, const int image_set_size, const int cap_size[2], int laser_option);

int main(int argc, char* argv[])
{
    bool is_live = false, is_capture = false; int laser_option = 2;
    std::string path = DEFAULT_PATH;
    int num_frames = 200; std::vector<int> capture_size = { 640,480 };
    rs_shapefit_capability sf_option = RS_SHAPEFIT_PLANE;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--cbox") == 0) { sf_option = RS_SHAPEFIT_BOX_COLOR; }
        else if (strcmp(argv[i], "--box") == 0) { sf_option = RS_SHAPEFIT_BOX; }
        else if (strcmp(argv[i], "--plane") == 0) { sf_option = RS_SHAPEFIT_PLANE; }
        else if (strcmp(argv[i], "--live") == 0) { is_live = true; }
        else if (strcmp(argv[i], "--capture") == 0) { is_capture = true; is_live = false; }
        else if (strcmp(argv[i], "--num_frame") == 0) { num_frames = atoi(argv[++i]); }
        else if (strcmp(argv[i], "--path") == 0) { path = argv[++i]; }
        else if (strcmp(argv[i], "--replay") == 0) { is_live = false; }
        else if (strcmp(argv[i], "--hd") == 0) { capture_size = { 1280,720 }; }
        else if (strcmp(argv[i], "--qhd") == 0) { capture_size = { 640,360 }; }
        else if (strcmp(argv[i], "--vga") == 0) { capture_size = { 640,480 }; }
        else if (strcmp(argv[i], "--laser_off") == 0) { laser_option = 0; }
        else if (strcmp(argv[i], "--laser_on") == 0) { laser_option = 1; }
        else {
            printf("usages:\n d435i-demo [--cbox|--box|--plane][--live|--replay][--path PATH][--capture][--num_frame NUM] \n");
            printf("                     [--hd|--qhd|--vga][--laser_off|--laser_on] \n");
            return 0;
        }
    }
    if (path.back() != '\\' && path.back() != '/'){ path.push_back(PATH_SEPARATER); }
    capture_frames2(path, num_frames, capture_size.data(), laser_option);
    
    return 0;
}

int capture_frames2(const std::string& path, const int image_set_size, const int cap_size[2], int laser_option)
{
    const int img_w = 640, img_h = 480;
    rs_sf_gl_context win("capture", img_w * 3, img_h * 3);
    
    std::unique_ptr<rs_sf_data_writer> recorder;
    rs_sf_data_ptr laser[2][3];
 
    for(auto rs_data_src = rs_sf_create_camera_imu_stream(img_w, img_h, laser_option);;)
    //for(auto rs_data_src = rs_sf_create_camera_imu_stream(path);;)
    {
        auto buf = rs_data_src->wait_for_data(std::chrono::milliseconds(330));
        //if(buf.empty()){ recorder.reset(); rs_data_src = rs_sf_create_camera_imu_stream(path); continue;}
        
        if(!recorder){ recorder = rs_sf_create_data_writer(rs_data_src.get(), path);}
        recorder->write(buf);
        
        if(!buf[0].empty()&&!buf[1].empty()&&!buf[2].empty()&&!buf[3].empty()){
            for(auto s : {0,1,2}){
                laser[(buf[s][0]->sensor_type&RS_SF_SENSOR_LASER_OFF)?1:0][s]=buf[s][0];
            }
            if(laser[0][0]&&laser[0][1]&&laser[0][2]){
                std::vector<rs_sf_image> images = {laser[0][0]->image,laser[0][1]->image,laser[0][2]->image};
                if(laser[1][0]&&laser[1][1]&&laser[1][2]){
                    images.emplace_back(laser[1][0]->image);
                    images.emplace_back(laser[1][1]->image);
                    images.emplace_back(laser[1][2]->image);
                }
                images.emplace_back(buf[3][0]->image);
                if(!win.imshow(images.data(),images.size())){break;}
            }
        }
    }
    recorder.reset();
    return 0;
}
