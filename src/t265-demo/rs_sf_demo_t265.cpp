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

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define PATH_SEPARATER '\\'
#define DEFAULT_PATH "." //"C:\\temp\\t265-capture\\"
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
#define VERSION_STRING "v0.3"

int g_camera_id = 0;
std::string g_str_origin = "[filename,tx,ty,tz,rw,rx,ry,rz] | Origin in WGS84 coordinate: Not provided at command line input.";
std::string g_pose_path = DEFAULT_PATH;
std::string g_script_name = "t265-insight.bat";

cv::Size size_win_fisheye(848 / 4, 800 / 4);

int scn_width = 800 + size_win_fisheye.width;
int scn_height = 600;
const int num_buttons = 6;

inline cv::Size size_rgb() { return cv::Size(800, scn_height); }
inline cv::Size size_screen() { return cv::Size(scn_width, scn_height); }

inline cv::Rect win_rgb() { return cv::Rect(0, 0, size_rgb().width, size_rgb().height); }
inline cv::Rect win_fisheye() { return cv::Rect(win_rgb().width, 0, size_win_fisheye.width, size_win_fisheye.height); }
inline cv::Rect win_text() { return cv::Rect(0, 0, scn_width - win_fisheye().width, scn_height); }
inline cv::Rect win_buttons() { return cv::Rect(win_fisheye().x, win_fisheye().height, win_fisheye().width, win_rgb().height - win_fisheye().height); }

inline cv::Size size_button() { return cv::Size(win_fisheye().width, (scn_height - win_fisheye().height) / num_buttons); }
inline cv::Rect win_exit() { return cv::Rect(scn_width - size_button().width, scn_height - size_button().height, size_button().width, size_button().height); }
inline cv::Rect win_capture() { return cv::Rect(win_exit().x, win_exit().y - size_button().height, size_button().width, size_button().height); }
inline cv::Rect win_bat() { return cv::Rect(win_capture().x, win_capture().y - size_button().height, size_button().width, size_button().height); }
inline cv::Rect win_cam2() { return cv::Rect(win_bat().x, win_bat().y - size_button().height, size_button().width, size_button().height); }
inline cv::Rect win_cam1() { return cv::Rect(win_cam2().x, win_cam2().y - size_button().height, size_button().width, size_button().height); }
inline cv::Rect win_cam0() { return cv::Rect(win_cam1().x, win_cam1().y - size_button().height, size_button().width, size_button().height); }

inline cv::Point label(const cv::Rect& rect) { return cv::Point(rect.x, rect.y + rect.height * 2 / 3); }
std::vector<std::string>& operator<<(std::vector<std::string>& buf, const std::string& msg) { buf.emplace_back(msg); return buf; }

bool g_t265 = true;

struct app_data {
    bool exit_request = false;
    bool capture_request = false;
    bool script_request = false;
    bool cam0_request = false;
    bool cam1_request = false;
    bool cam2_request = false;
    int highlight_exit_button = 0;
    int highlight_capture_button = 0;
    int highlight_script_button = 0;
    
    void set_exit_request() { exit_request = true; highlight_exit_button = 5; }
    void set_capture_request() { capture_request = true; highlight_capture_button = 5; }
    void set_script_request() { script_request = true; highlight_script_button = 5; }
    bool is_highlight_capture_button() { highlight_capture_button = std::max(0, highlight_capture_button - 1); return highlight_capture_button > 0; }
    bool is_highlight_script_button() { highlight_script_button = std::max(0, highlight_script_button - 1); return highlight_script_button > 0; }
    bool is_highlight_exit_button() { highlight_exit_button = std::max(0, highlight_exit_button - 1); return highlight_exit_button > 0; }
} g_app_data;

void run()
{
    int camera_id = g_camera_id;    
    std::map<int, int> counters;
    std::map<int, std::string> stream_names;
    std::mutex mutex;
    
    bool t265_available = g_t265;
    std::string folder_path = g_pose_path;
    std::ofstream index_file;
    std::string last_file_written;

    std::string window_name = "T265-RGB Capture App for Insight " + std::string(VERSION_STRING) + " @ " + folder_path;
    cv::namedWindow(window_name);

    for(g_app_data.exit_request=false; !g_app_data.exit_request; )
    {
        cv::VideoCapture cap(camera_id);
        cap.set(CV_CAP_PROP_FRAME_WIDTH, 4096);
        cap.set(CV_CAP_PROP_FRAME_HEIGHT, 2160);

		auto cap_width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
		auto cap_height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

        std::unique_ptr<rs2::pipeline> pipe;
        if (t265_available)
        {
            // Declare RealSense pipeline, encapsulating the actual device and sensors
            try {
                pipe = std::make_unique<rs2::pipeline>();
                // Create a configuration for configuring the pipeline with a non default profile
                rs2::config cfg;
                // Add pose stream
                cfg.enable_stream(RS2_STREAM_FISHEYE, 0);
                cfg.enable_stream(RS2_STREAM_FISHEYE, 1);
                cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);
                
                // Start pipeline with chosen configuration
                auto profiles = pipe->start();
                auto _streams = profiles.get_streams();
                
                //Collect the enabled streams names
                for (auto p : profiles.get_streams()) {
                    stream_names[p.unique_id()] = p.stream_name();
                    printf("%s\n", p.stream_name().c_str());
                    
                    /**
                     auto vp = p.as<rs2::video_stream_profile>();
                     if (vp) {
                     auto fi = vp.get_intrinsics();
                     printf("fisheye fx %.3f fy %.3f, px %.3f py %.3f, size %d %d,", fi.fx, fi.fy, fi.ppx, fi.ppy, fi.width, fi.height);
                     for (auto m : fi.coeffs) {
                     printf("%.3f ", m);
                     }
                     printf("\n");
                     }
                     */
                }
            }
            catch (...) {
                t265_available = false;
            }
        }
        
        for(bool switch_request=false; !switch_request && !g_app_data.exit_request;)
        {
            cv::Mat img, screen_img, cvIr;
            std::vector<std::string> scn_msg;
            
            if(cap.isOpened()){
                scn_height = (int)((scn_width - size_win_fisheye.width)* cap_height / cap_width);
                screen_img.create(size_screen(), CV_8UC3);
                screen_img(win_buttons()).setTo(0);
                cap >> img;
                cv::resize(img, screen_img(win_rgb()), size_rgb(), 0, 0, CV_INTER_NN);
                scn_msg << "RGB cam id:" + std::to_string(camera_id) + "  w:" + std::to_string(img.cols) + " h:" + std::to_string(img.rows);
            }
            else{
                screen_img.create(size_screen(), CV_8UC3);
                screen_img.setTo(0);
            }
            
            rs2::frame p;
            if (t265_available) {
                try {
                    rs2::frameset fs = pipe->wait_for_frames();

                    if (fs) {
                        auto f = fs.first_or_default(RS2_STREAM_FISHEYE);
                        cv::Mat simg;
                        if (f) {
                            auto vf = f.as<rs2::video_frame>();
                            cv::Mat cvvf(vf.get_height(), vf.get_width(), CV_8UC1, (void*)vf.get_data());
                            cv::resize(cvvf, simg, cv::Size(), 0.25, 0.25, CV_INTER_NN);
                            cv::cvtColor(simg, screen_img(win_fisheye()), CV_GRAY2RGB, 3);
                        }

                        p = fs.first_or_default(RS2_STREAM_POSE);
                        if (p)
                        {
                            auto pf = p.as<rs2::pose_frame>();
                            if (pf)
                            {
                                const auto print = [&scn_msg](const std::string& conf, const rs2_pose& p) {
                                    std::stringstream ss;
                                    scn_msg << "T265 Confidence: " + conf;
                                    ss << std::fixed << std::right << std::setprecision(3) << std::setw(6);
                                    ss << p.translation.x << "," << p.translation.y << "," << p.translation.z;
                                    scn_msg << "Translation: " + ss.str();
                                    std::stringstream sr;
                                    sr << std::fixed << std::right << std::setprecision(3) << std::setw(6);
                                    sr << p.rotation.w << "," << p.rotation.x << "," << p.rotation.y << "," << p.rotation.z;
                                    scn_msg << "Rotation: " + sr.str();
                                };

                                auto pd = pf.get_pose_data();
                                switch (pd.tracker_confidence) {
                                case 1: print("Low", pd); break;
                                case 2: print("Medium", pd); break;
                                case 3: print("High", pd); break;
                                default: scn_msg << "T265 Tracking Failed";
                                }
                            }
                        }
                    }
                }
                catch (...) { t265_available = false; }
            }
            scn_msg << "Script: " + g_script_name;
            if (!last_file_written.empty()) {
                scn_msg << "Last write:" + last_file_written;
            }
            
            //////////////////////////////////////////////////////////////////////////////////////
            for (int i = 0; i < (int)scn_msg.size(); ++i) {
                cv::putText(screen_img, scn_msg[i], cv::Point(win_text().x + 10, win_text().y + 20 * (1+i)), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255));
            }
            
            cv::rectangle(screen_img, win_exit(), cv::Scalar(255, 255, 255), g_app_data.is_highlight_exit_button() ? 3 : 1);
            cv::putText(screen_img, "  EXIT", label(win_exit()), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255));
            cv::rectangle(screen_img, win_capture(), cv::Scalar(255, 255, 255), g_app_data.is_highlight_capture_button() ? 3 : 1);
            cv::putText(screen_img, "  CAPTURE",  label(win_capture()), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255));
            cv::rectangle(screen_img, win_bat(), cv::Scalar(255, 255, 255), g_app_data.is_highlight_script_button() ? 3 : 1);
            cv::putText(screen_img, "  CALL SCRIPT", label(win_bat()), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255));
            cv::rectangle(screen_img, win_cam2(), cv::Scalar(255, 255, 255), camera_id == 2 ? 3 : 1);
            cv::putText(screen_img, "  CAM 2", label(win_cam2()), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255));
            cv::rectangle(screen_img, win_cam1(), cv::Scalar(255, 255, 255), camera_id == 1 ? 3 : 1);
            cv::putText(screen_img, "  CAM 1", label(win_cam1()), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255));
            cv::rectangle(screen_img, win_cam0(), cv::Scalar(255, 255, 255), camera_id == 0 ? 3 : 1);
            cv::putText(screen_img, "  CAM 0", label(win_cam0()), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255));
            
            //////////////////////////////////////////////////////////////////////////////////////
            if (g_app_data.capture_request)
            {
                auto t = std::time(NULL);
                auto tm = *std::localtime(&t);
                std::stringstream ss;
                ss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");
                
                std::string filename = "rgb_" + ss.str() + ".jpg";
                last_file_written = folder_path + filename;
                cv::imwrite(last_file_written, img, { CV_IMWRITE_JPEG_QUALITY, 100 });
                
                if (!index_file.is_open()) {
                    index_file.open(folder_path + "pose.txt", std::ios_base::out | std::ios_base::trunc);
                    index_file << g_str_origin << std::endl;
                }
                index_file << filename;
                
                if (p) {
                    auto pf = p.as<rs2::pose_frame>();
                    if (pf) {
                        auto pd = pf.get_pose_data();
                        index_file << "," << pd.translation.x << "," << pd.translation.y << "," << pd.translation.z;
                        index_file << "," << pd.rotation.w << "," << pd.rotation.x << "," << pd.rotation.y << "," << pd.rotation.z;
                    }
                }
                
                index_file << std::endl;
                g_app_data.capture_request = false; //capture request handled
            }
            if (g_app_data.script_request)
            {
                std::async(std::launch::async, [&]() {
                    return system(("START \"SCRIPT\" /SEPARATE /B " + g_script_name + " " + folder_path).c_str());
                //system(("CALL " + g_script_name + " " + folder_path).c_str());
                });
                g_app_data.script_request = false; //bat process request handled
            }
            
            if (g_app_data.cam0_request && camera_id != 0) { camera_id = 0; switch_request = true; }
            else if (g_app_data.cam1_request && camera_id != 1) { camera_id = 1; switch_request = true; }
            else if (g_app_data.cam2_request && camera_id != 2) { camera_id = 2; switch_request = true; }
            g_app_data.cam0_request = false;
            g_app_data.cam1_request = false;
            g_app_data.cam2_request = false;
            
            cv::imshow(window_name, screen_img.clone());
            switch (cv::waitKey(1))
            {
                case 'q': case 27: return;
                case '0': if (camera_id != 0) { camera_id = 0; switch_request = true; break; }
                case '1': if (camera_id != 1) { camera_id = 1; switch_request = true; break; }
                case '2': if (camera_id != 2) { camera_id = 2; switch_request = true; break; }
                case '3': if (camera_id != 3) { camera_id = 3; switch_request = true; break; }
                case '4': if (camera_id != 4) { camera_id = 4; switch_request = true; break; }
                case '5': if (camera_id != 5) { camera_id = 5; switch_request = true; break; }
                default: break;
            }
            
            cv::setMouseCallback(window_name, [](int event, int x, int y, int flags, void* userdata) {
                switch (event) {
                    case cv::EVENT_LBUTTONUP:
                        if (win_exit().contains(cv::Point(x, y))) { g_app_data.set_exit_request(); };
                        if (win_capture().contains(cv::Point(x, y))) { g_app_data.set_capture_request(); }
                        if (win_bat().contains(cv::Point(x, y))) { g_app_data.set_script_request(); }
                        if (win_cam0().contains(cv::Point(x, y))) { g_app_data.cam0_request = true; }
                        if (win_cam1().contains(cv::Point(x, y))) { g_app_data.cam1_request = true; }
                        if (win_cam2().contains(cv::Point(x, y))) { g_app_data.cam2_request = true; }
                    default: break;
                }
            }, &g_app_data);
        }
    }
}

int main(int argc, char* argv[])
{
    bool is_live = true, is_capture = false, is_replay = false; int laser_option = 0;
    std::string data_path = DEFAULT_PATH, camera_json_path = "."; //camera.json location
    std::vector<int> capture_size = { 640,480 };
    
    for (int i = 1; i < argc; ++i) {
        if      (!strcmp(argv[i], "--live"))            { is_live = true; is_replay = false; }
        else if (!strcmp(argv[i], "--no_t265"))         { g_t265 = false; }
        else if (!strcmp(argv[i], "--origin"))          { g_str_origin = argv[++i]; }
        else if (!strcmp(argv[i], "--path"))            { g_pose_path = argv[++i]; }
        else if (!strcmp(argv[i], "--cam"))             { g_camera_id = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--script"))          { g_script_name = argv[++i]; }
        //else if (!strcmp(argv[i], "--capture"))         { is_capture = true; is_live = false; }
        //else if (!strcmp(argv[i], "--path"))            { data_path = camera_json_path = argv[++i]; }
        //else if (!strcmp(argv[i], "--replay"))          { is_replay = true; is_live = false; }
        //else if (!strcmp(argv[i], "--replay_once"))     { is_replay = true; is_live = false; g_replay_once = true;}
        //else if (!strcmp(argv[i], "--hd"))              { capture_size = { 1280,720 }; }
        //else if (!strcmp(argv[i], "--qhd"))             { capture_size = { 640,360 }; }
        //else if (!strcmp(argv[i], "--vga"))             { capture_size = { 640,480 }; }
        else {
            printf("usages:\n t265-demo.exe [--no_t265][--origin STR][--cam ID][--script FILENAME][--path OUTPUT_PATH]\n");
            printf("\n");
            printf("--no_tm2: Capture RGB only without t265 connected.\n");
            printf("--origin: STR will be added to the first line of output pose.txt.   \n");
            printf("--cam   : ID set the initial camera ID, default 0. Rear-facing tablet cam usually has ID=1\n");
            printf("--script: FILENAME of an external script, default script is t265-insight.bat \n");
            printf("--path  : OUTPUT_PATH to the capture files, default is the runtime working directory.\n\n");
            return 0;
        }
    }
    if (data_path.back() != '\\' && data_path.back() != '/'){ data_path.push_back(PATH_SEPARATER); }
    if (camera_json_path.back() != '\\' && camera_json_path.back() != '/'){ camera_json_path.push_back(PATH_SEPARATER); }
    if (g_pose_path.back() != '\\' && g_pose_path.back() != '/'){ g_pose_path.push_back(PATH_SEPARATER); }
    
    run();
    return 0;
}
