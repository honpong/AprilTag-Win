//
//  t265-apriltag-demo
//
//  Created by Hon Pong (Gary) Ho on 05/29/19.
//

#include "t265-apriltag-demo.hpp"

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define PATH_SEPARATER '\\'
#define DEFAULT_PATH ".\\" //capture\\" //"C:\\temp\\t265-capture\\"
#define DEFAULT_SCRIPT "t265-insight.bat"
//#define BIN_COMMAND ("START \"SCRIPT\" /WAIT /B " + g_script_name + " " + folder_path).c_str()
//#define SCRIPT_COMMAND ("START \"SCRIPT\" /WAIT /B " + g_script_name + " " + folder_path + " py").c_str()
#define BIN_COMMAND ("CALL " + g_script_name + " " + folder_path).c_str()
#define SCRIPT_COMMAND ("CALL " + g_script_name + " " + folder_path + " py").c_str()

#else
#define PATH_SEPARATER '/'
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/temp/shapefit/1/")
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/Desktop/temp/data/")
#define DEFAULT_PATH "./"
#define DEFAULT_SCRIPT "t265-insight.sh"
#define SCRIPT_COMMAND ("chmod +x " + g_script_name + ";./" + g_script_name + " " + folder_path).c_str()
#define BIN_COMMAND SCRIPT_COMMAND

#endif
#define DEFAULT_CAMERA_JSON default_camera_json
#define STREAM_REQUEST(l) (rs_sf_stream_request{l,-1,-1,g_ir_fps,g_color_fps,g_replace_color})
#define VERSION_STRING "v0.1"


/**
#include "apriltag.h"
#include "tag36h11.h"
#include "tag36h10.h"
#include "tag36artoolkit.h"
#include "tag25h9.h"
#include "tag25h7.h"
#include "common/getopt.h"

struct apriltag
{
    apriltag() { init(); }
    ~apriltag() { destory(); }

    getopt_t *getopt = nullptr;
    const char* famname = nullptr;
    apriltag_family_t *tf = nullptr;
    apriltag_detector_t *td = nullptr;

    void destory()
    {
        apriltag_detector_destroy(td);
        if (!strcmp(famname, "tag36h11"))
            tag36h11_destroy(tf);
        else if (!strcmp(famname, "tag36h10"))
            tag36h10_destroy(tf);
        else if (!strcmp(famname, "tag36artoolkit"))
            tag36artoolkit_destroy(tf);
        else if (!strcmp(famname, "tag25h9"))
            tag25h9_destroy(tf);
        else if (!strcmp(famname, "tag25h7"))
            tag25h7_destroy(tf);
        getopt_destroy(getopt);

        getopt = nullptr;
        famname = nullptr;
        tf = nullptr;
        td = nullptr;
    }

    void init() 
    {
        getopt = getopt_create();

        getopt_add_bool(getopt, 'h', "help", 0, "Show this help");
        getopt_add_bool(getopt, 'd', "debug", 0, "Enable debugging output (slow)");
        getopt_add_bool(getopt, 'q', "quiet", 0, "Reduce output");
        getopt_add_string(getopt, 'f', "family", "tag36h11", "Tag family to use");
        getopt_add_int(getopt, '\0', "border", "1", "Set tag family border size");
        getopt_add_int(getopt, 't', "threads", "4", "Use this many CPU threads");
        getopt_add_double(getopt, 'x', "decimate", "1.0", "Decimate input image by this factor");
        getopt_add_double(getopt, 'b', "blur", "0.0", "Apply low-pass blur to input");
        getopt_add_bool(getopt, '0', "refine-edges", 1, "Spend more time trying to align edges of tags");
        getopt_add_bool(getopt, '1', "refine-decode", 0, "Spend more time trying to decode tags");
        getopt_add_bool(getopt, '2', "refine-pose", 0, "Spend more time trying to precisely localize tags");

        // Initialize tag detector with options
        famname = getopt_get_string(getopt, "family");
        if (!strcmp(famname, "tag36h11"))
            tf = tag36h11_create();
        else if (!strcmp(famname, "tag36h10"))
            tf = tag36h10_create();
        else if (!strcmp(famname, "tag36artoolkit"))
            tf = tag36artoolkit_create();
        else if (!strcmp(famname, "tag25h9"))
            tf = tag25h9_create();
        else if (!strcmp(famname, "tag25h7"))
            tf = tag25h7_create();
        else {
            printf("Unrecognized tag family name. Use e.g. \"tag36h11\".\n");
            exit(-1);
        }
        tf->black_border = getopt_get_int(getopt, "border");

        td = apriltag_detector_create();
        apriltag_detector_add_family(td, tf);
        td->quad_decimate = (float)getopt_get_double(getopt, "decimate");
        td->quad_sigma = (float)getopt_get_double(getopt, "blur");
        td->nthreads = getopt_get_int(getopt, "threads");
        td->debug = getopt_get_bool(getopt, "debug");
        td->refine_edges = getopt_get_bool(getopt, "refine-edges");
        td->refine_decode = getopt_get_bool(getopt, "refine-decode");
        td->refine_pose = getopt_get_bool(getopt, "refine-pose");
    }

    std::string find(cv::Mat& frame, cv::Mat& gray)
    {
        // Make an image_u8_t header for the Mat data
#ifdef _MSC_VER
        image_u8_t im{ gray.cols, gray.rows, gray.cols, gray.data };
#else
        image_u8_t im = { .width = gray.cols,
            .height = gray.rows,
            .stride = gray.cols,
            .buf = gray.data
        };
#endif

        zarray_t *detections = apriltag_detector_detect(td, &im);
        
        int num_tag_detected = zarray_size(detections);
        // Draw detection outlines
        for (int i = 0; i < num_tag_detected; i++) {
            apriltag_detection_t *det;
            zarray_get(detections, i, &det);
            cv::line(frame, cv::Point2d(det->p[0][0], det->p[0][1]),
                cv::Point2d(det->p[1][0], det->p[1][1]),
                cv::Scalar(0, 0xff, 0), 2);
            cv::line(frame, cv::Point2d(det->p[0][0], det->p[0][1]),
                cv::Point2d(det->p[3][0], det->p[3][1]),
                cv::Scalar(0, 0, 0xff), 2);
            cv::line(frame, cv::Point2d(det->p[1][0], det->p[1][1]),
                cv::Point2d(det->p[2][0], det->p[2][1]),
                cv::Scalar(0xff, 0, 0), 2);
            cv::line(frame, cv::Point2d(det->p[2][0], det->p[2][1]),
                cv::Point2d(det->p[3][0], det->p[3][1]),
                cv::Scalar(0xff, 0, 0), 2);

            std::stringstream ss;
            ss << det->id;
            cv::String text = ss.str();
            int fontface = cv::FONT_HERSHEY_SCRIPT_SIMPLEX;
            double fontscale = 1.0;
            int baseline;
            cv::Size textsize = getTextSize(text, fontface, fontscale, 2,
                &baseline);
            cv::putText(frame, text, cv::Point2d(det->c[0] - textsize.width / 2,
                det->c[1] + textsize.height / 2),
                fontface, fontscale, cv::Scalar(0xff, 0x99, 0), 2);
        }
        zarray_destroy(detections);

        return std::to_string(num_tag_detected) + " tags detected";
    }
};
*/

void run(const std::string& data_path)
{
    auto pipe = std::make_shared<rs2::pipeline>();

    rs2::config cfg;
    cfg.enable_record_to_file(data_path);
    
    pipe->start(cfg);
    auto device = pipe->get_active_profile().get_device();
    
    // Declare frameset and frames which will hold the data from the camera
    rs2::frameset frames;
    int width = -1, height = -1;
    cv::Mat rgb, disp;
    
    for(bool exit_request=false; !exit_request; )
    {
        frames = pipe->wait_for_frames();
        auto color_frame = frames.get_color_frame();
        
        if(width==-1||height==-1){
            width = color_frame.get_width();
            height = color_frame.get_height();
        }
        
        cv::Mat src(height,width, CV_8UC3, (void*)color_frame.get_data());
        cv::cvtColor(src, rgb, cv::COLOR_RGB2BGR);
        cv::resize(rgb, rgb, cv::Size(), 0.5, 0.5);
        cv::hconcat(rgb, rgb, disp);
        cv::imshow("record-playback", disp);
        
        switch(cv::waitKey(1)){
            case 'q': case 27: exit_request=true; break;
        }
    }
    pipe->stop();
    device = nullptr;
    
    cfg = {};
    cfg.enable_device_from_file(data_path);
    
    pipe = std::make_shared<rs2::pipeline>();
    pipe->start(cfg);
    
    device = pipe->get_active_profile().get_device();
    for(bool exit_request=false; !exit_request; )
    {
        frames = pipe->wait_for_frames();
        if(frames)
        {
            auto color_frame = frames.get_color_frame();
            cv::Mat src(height,width, CV_8UC3, (void*)color_frame.get_data());
            cv::resize(src, rgb, cv::Size(), 0.5, 0.5);
            cv::cvtColor(rgb, disp(cv::Rect(rgb.cols,0,rgb.cols,rgb.rows)), cv::COLOR_RGB2BGR);
            cv::imshow("record-playback", disp);
            
            switch(cv::waitKey(1))
            {
                case 'q': case 27: exit_request=true; break;
            }
        }
        else{ exit_request = true; }

        auto total_playback_time = device.as<rs2::playback>().get_duration();
        auto current_playback_time = std::chrono::nanoseconds(device.as<rs2::playback>().get_position());

        if( current_playback_time >= total_playback_time){
            exit_request=true;
        }
    }
}

int main(int argc, char* argv[])
{
    bool is_live = true, is_capture = false, is_replay = false; int laser_option = 0;
    std::string data_file = "t265_data.bag";
    std::string data_path = DEFAULT_PATH + data_file, camera_json_path = "."; //camera.json location
    std::vector<int> capture_size = { 640,480 };
    
    for (int i = 1; i < argc; ++i) {
        if      (!strcmp(argv[i], "--live"))            { is_live = true; is_replay = false; }
        else if (!strcmp(argv[i], "--path"))            { data_path = argv[++i];}
        //else if (!strcmp(argv[i], "--no_t265"))         { g_t265 = false; }
        //else if (!strcmp(argv[i], "--origin"))          { g_str_origin = argv[++i]; }
        //else if (!strcmp(argv[i], "--path"))            { g_pose_path = argv[++i]; }
        //else if (!strcmp(argv[i], "--cam"))             { g_camera_id = atoi(argv[++i]); }
        //else if (!strcmp(argv[i], "--script"))          { g_script_name = argv[++i]; }
        //else if (!strcmp(argv[i], "--interval"))        { g_auto_capture_interval_s = atoi(argv[++i]); }
        //else if (!strcmp(argv[i], "--fisheye"))         { g_cam_fisheye = 3; }
        //else if (!strcmp(argv[i], "--apriltag"))        { g_app_data.apriltag_mode = true; }
        //else if (!strcmp(argv[i], "--no_blur_est"))     { g_app_data.estimate_blur = false; }
        else {
            printf("usages:\n t265-demo.exe [--no_t265][--origin STR][--cam ID][--interval SEC][--script FILENAME][--path OUTPUT_PATH]\n");
            printf("\n");
            printf("--no_tm2  : Capture RGB only without t265 connected.\n");
            printf("--origin  : STR will be added to the first line of output pose.txt.   \n");
            printf("--cam     : ID set the initial camera ID, default 0. Rear-facing tablet cam usually has ID=1\n");
            printf("--interval: SEC number of seconds per auto captured image, default is 2 seconds. \n");
            printf("--script  : FILENAME of an external script, default script is t265-insight.bat \n");
            printf("--path    : OUTPUT_PATH to the capture files, default is the .\\capture\\.\n\n");
            return 0;
        }
    }
    //if (data_path.back() != '\\' && data_path.back() != '/'){ data_path.push_back(PATH_SEPARATER); }
    if (camera_json_path.back() != '\\' && camera_json_path.back() != '/'){ camera_json_path.push_back(PATH_SEPARATER); }
    
    run(data_path);
    return 0;
}
