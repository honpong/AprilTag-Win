//
//  rs_sf_demo_t265.cpp
//  t265-demo
//
//  Created by Hon Pong (Gary) Ho on 2/28/19.
//

//#include "rs_shapefit.h"
//#include "rs_sf_camera.hpp"
//#include "rs_sf_gl_context.hpp"
//#include "rs_sf_pose_tracker.h"
//#include "d435i_default_json.h"
#include "rs_sf_demo_t265.hpp"
#include <opencv2/opencv.hpp>

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define PATH_SEPARATER '\\'
#define DEFAULT_PATH "C:\\temp\\data\\"
#define GET_CAPTURE_DISPLAY_IMAGE(src) src.one_image()
#define COLOR_STREAM_REQUEST {}
#else
#define PATH_SEPARATER '/'
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/temp/shapefit/1/")
#define DEFAULT_PATH (std::string(getenv("HOME"))+"/Desktop/temp/data/")
#define GET_CAPTURE_DISPLAY_IMAGE(src) src.images()
#define COLOR_STREAM_REQUEST {if(app.color_request()){ g_replace_color = !g_replace_color; pipe.reset(true); }}

#endif
#define DEFAULT_CAMERA_JSON default_camera_json
#define STREAM_REQUEST(l) (rs_sf_stream_request{l,-1,-1,g_ir_fps,g_color_fps,g_replace_color})
#define VERSION_STRING "v1.3"

int g_ir_fps        = 60;
int g_color_fps     = 15;
int g_accel_dec     = 1;
int g_gyro_dec      = 1;
int g_replace_color = 1;
int g_use_sp        = 0;
int g_force_demo_ui = 0;
int g_tablet_screen = 0;
bool g_bypass_box_detect = false;
bool g_print_cmd_pose    = false;
bool g_replay_once       = false;
bool g_write_rgb_pose    = false;
std::string g_pose_path = ".";

void run()
{
    int camera_id = 0;
    std::string window_name = "hello camera";
    cv::namedWindow(window_name);
    
    for(;;){
        cv::VideoCapture cap(camera_id);
        
        for(bool switch_request=false; !switch_request;)
        {
            cv::Mat img;
            
            if(cap.isOpened()){
                cap >> img;
                cv::putText(img, "press 0, 1, or 2 to switch camera from " + std::to_string(camera_id), cv::Point(25,25), CV_FONT_HERSHEY_PLAIN, 2, cv::Scalar(255,255,255));
            }
            else{
                img = cv::Mat(480,640,CV_8UC3);
                img.setTo(0);
                cv::putText(img, "press 0, 1, or 2 to switch camera from " + std::to_string(camera_id), cv::Point(img.cols/10,img.rows/2), CV_FONT_HERSHEY_PLAIN, 2, cv::Scalar(255,255,255));
            }
            
            cv::imshow(window_name, img);
            switch(cv::waitKey(1))
            {
                case 'q': case 27: return;
                case '0': if(camera_id!=0){ camera_id=0; switch_request=true; break; }
                case '1': if(camera_id!=1){ camera_id=1; switch_request=true; break; }
                case '2': if(camera_id!=2){ camera_id=2; switch_request=true; break; }
                default: break;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    bool is_live = true, is_capture = false, is_replay = false; int laser_option = 0;
    std::string data_path = DEFAULT_PATH, camera_json_path = "."; //camera.json location
    std::vector<int> capture_size = { 640,480 };

    for (int i = 1; i < argc; ++i) {
        if      (!strcmp(argv[i], "--color"))           { g_replace_color = 0; }
        else if (!strcmp(argv[i], "--live"))            { is_live = true; is_replay = false; g_replay_once = false;}
        else if (!strcmp(argv[i], "--capture"))         { is_capture = true; is_live = false; }
        else if (!strcmp(argv[i], "--path"))            { data_path = camera_json_path = argv[++i]; }
        else if (!strcmp(argv[i], "--replay"))          { is_replay = true; is_live = false; }
        else if (!strcmp(argv[i], "--replay_once"))     { is_replay = true; is_live = false; g_replay_once = true;}
        else if (!strcmp(argv[i], "--hd"))              { capture_size = { 1280,720 }; }
        else if (!strcmp(argv[i], "--qhd"))             { capture_size = { 640,360 }; }
        else if (!strcmp(argv[i], "--vga"))             { capture_size = { 640,480 }; }
        else {
            printf("usages:\n t265-demo \n");
            return 0;
        }
    }
    if (data_path.back() != '\\' && data_path.back() != '/'){ data_path.push_back(PATH_SEPARATER); }
    if (camera_json_path.back() != '\\' && camera_json_path.back() != '/'){ camera_json_path.push_back(PATH_SEPARATER); }
    if (g_pose_path.back() != '\\' && g_pose_path.back() != '/'){ g_pose_path.push_back(PATH_SEPARATER); }
    
    run();
    return 0;
}
