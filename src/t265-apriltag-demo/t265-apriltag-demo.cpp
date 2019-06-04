//
//  t265-apriltag-demo
//
//  Created by Hon Pong (Gary) Ho on 05/29/19.
//

#include "t265-apriltag-demo.hpp"
#include "t265-apriltag.hpp"

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define PATH_SEPARATER '\\'
#define DEFAULT_FOLDER "."
#else
#define PATH_SEPARATER '/'
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/Desktop/temp/data/")
#define DEFAULT_FOLDER "."
#endif

#define DEFAULT_FILE "t265-apriltag.bag"
#define DEFAULT_JSON "calibration.json"
#define VERSION_STRING "v0.1"

struct app_data_t
{
    std::string data_path = std::string(DEFAULT_FOLDER) + PATH_SEPARATER + DEFAULT_FILE;
    std::string json_path = std::string(DEFAULT_FOLDER) + PATH_SEPARATER + DEFAULT_JSON;
    bool is_live    = true;
    bool is_record  = false;
    bool is_replay  = false;
    bool is_ui      = true;
    bool exit_request = false;
    bool is_tag_detect = true;
    bool is_playback_loop = true;

    const int fisheye_sensor_idx = 1;
    const std::string app_name = std::string("t265-apriltag-demo ") + VERSION_STRING;

    bool init(int argc, char* argv[])
    {
        for (int i = 1; i < argc; ++i) {
            if (!strcmp(argv[i],      "--record")) { is_record = is_live = true; }
            else if (!strcmp(argv[i], "--replay")) { is_replay = true; is_live = is_record = false; }
            else if (!strcmp(argv[i], "--no_ui"))  { is_ui     = false;     }
            else if (!strcmp(argv[i], "--path"  )) { data_path = argv[++i]; }
            else if (!strcmp(argv[i], "--json"  )) { json_path = argv[++i]; }
            else if (!strcmp(argv[i], "--no_tag")) { is_tag_detect = false; }
            else if (!strcmp(argv[i], "--once"  )) { is_playback_loop = false; }
            else {
                printf("usages:\n t265-apriltag-demo \n");
                printf("  [--record][--replay][--once][--no_ui][--no_tag][--path PATH_TO_BAG_FILE][--json PATH_TO_JSON]\n\n");
                printf("--record : enable recording.                                          \n");
                printf("--replay : replay a recorded T265 .bag file with .json calibration.   \n");
                printf("--once   : replay only once.                                          \n");
                printf("--no_ui  : do not display UI.                                         \n");
                printf("--no_tag : do not start Apriltag detection.                           \n");
                printf("--path   : path to T265 .bag file.                                    \n");
                printf("--json   : json to custom .json calibration file from this program.   \n");
                return false;
            }
        }    
        return true;
    }

    void set_exit(bool flag = true) { exit_request = flag; }
    bool is_exit() const { return exit_request; }

} g_app_data;


std::string print(cv_tag_pose& tagpose)
{
    std::stringstream ss;
    ss << "tag " << tagpose.id << ", pose t:" << std::fixed << std::right << std::setprecision(3) << std::setw(6)
    << tagpose.translation[0] << "," << tagpose.translation[1] << "," << tagpose.translation[2] << "," << "| R:"
    << tagpose.rotation(0,0) << "," << tagpose.rotation(0,1) << "," << tagpose.rotation(0,2) << ","
    << tagpose.rotation(1,0) << "," << tagpose.rotation(1,1) << "," << tagpose.rotation(1,2) << ","
    << tagpose.rotation(2,0) << "," << tagpose.rotation(2,1) << "," << tagpose.rotation(2,2);
    return ss.str();
}

std::string print(const rs2_pose& t265_pose)
{
    std::stringstream ss; ss << std::fixed << std::right << std::setprecision(3) << std::setw(6);
    ss << "t265 pose: " << t265_pose.translation.x << "," << t265_pose.translation.y << "," << t265_pose.translation.z << "|"
    << t265_pose.rotation.w << "," << t265_pose.rotation.x << "," << t265_pose.rotation.y << "," << t265_pose.rotation.z;
    return ss.str();
}

void detection_callback(std::shared_ptr<apriltag_detection_array> tags, rs2_pose& t265_pose)
{
    if(!g_app_data.is_ui)
    {
        //1. print t265 pose output
        std::cout << print(t265_pose) << std::endl;
        
        for(int t=0, nt = tags->num_detections(); t<nt; ++t)
        {
            //2. print tag detection from fisheye and tag pose from distorted corners
            auto cvpose = tags->get_tag_pose(t);
            if(cvpose.id >= 0){ std::cout << "  distorted " << print(cvpose) << std::endl; }
            
            //3. print tag detection from fisheye, undistorted corners to a undistorted image, and tag pose from undistorted corners
            cvpose = tags->get_tag_undistorted_pose(t);
            if(cvpose.id >= 0){ std::cout << "undistorted " << print(cvpose) << std::endl; }
        }
        std::cout << "-------------------------------------------------------------------------------------------------" << std::endl;
    }
}

int run_detection_loop(std::shared_ptr<rs2::pipeline>& pipe, rs2_intrinsics& intr, rs2_extrinsics& extr)
{
    std::unique_ptr<apriltag> apt;
    std::shared_ptr<apriltag_detection_array> tags;
    cv::Mat display, src;

    rs2::frameset frames;
    const int fisheye_width = intr.width;
    const int fisheye_height = intr.height;
    
    auto device = pipe->get_active_profile().get_device();

    for (g_app_data.set_exit(false); !g_app_data.is_exit(); )
    {
        if ((bool)(frames = pipe->wait_for_frames()))
        {
            auto fisheye_frame = frames.get_fisheye_frame(g_app_data.fisheye_sensor_idx);
            auto t265_pose = frames.get_pose_frame().get_pose_data();
            cv::Mat(fisheye_height, fisheye_width, CV_8UC1, (void*)fisheye_frame.get_data()).copyTo(src);
           
            fisheye_frame = rs2::frame();
            frames = {};
            tags = nullptr;
            
            if(g_app_data.is_tag_detect)
            {
                if(!apt){ apt = std::make_unique<apriltag>(); }
                tags = apt->detect(src, intr);
                detection_callback(tags, t265_pose);
            }
            
            if (g_app_data.is_ui)
            {
                cv::cvtColor(src, display, cv::COLOR_GRAY2RGB);
                
                if(tags)
                {
                    cv::Mat undistort_img;
                    cv::cvtColor(apt->undistort(src, intr),undistort_img, cv::COLOR_GRAY2RGB);
                    
                    tags->draw_detections(display, &undistort_img);
                    
                    cv::hconcat(display, undistort_img, display);
                }
                cv::imshow(g_app_data.app_name, display);
            }
            
            switch (cv::waitKey(1)) {
                case 'q': case 27: g_app_data.set_exit(); break;
                case 'a': case 'A': g_app_data.is_tag_detect = !g_app_data.is_tag_detect; break;
                case 'u': case 'U': g_app_data.is_ui = !g_app_data.is_ui; break;
            }
        }
        else { g_app_data.set_exit(); }

        if (device.as<rs2::playback>() && !g_app_data.is_playback_loop)
        {
            const auto total_playback_time = device.as<rs2::playback>().get_duration();
            const auto current_playback_time = std::chrono::nanoseconds(device.as<rs2::playback>().get_position());
            if (current_playback_time >= total_playback_time) { g_app_data.set_exit(); }
        }
    }

    return 0;
}

int run()
{
    if (g_app_data.is_live)
    {
        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);
        cfg.enable_stream(RS2_STREAM_FISHEYE, 1);
        cfg.enable_stream(RS2_STREAM_FISHEYE, 2);

        if (g_app_data.is_record) {
            cfg.enable_record_to_file(g_app_data.data_path);
            std::cout << "Record to file :" << g_app_data.data_path << std::endl;
        }

        auto pipe           = std::make_shared<rs2::pipeline>();
        auto pipe_profile   = pipe->start(cfg);
        auto device         = pipe->get_active_profile().get_device();
        auto fisheye_stream = pipe_profile.get_stream(RS2_STREAM_FISHEYE, g_app_data.fisheye_sensor_idx);
       
        // Get fisheye sensor intrinsics parameters
        auto intrinsics_live = fisheye_stream.as<rs2::video_stream_profile>().get_intrinsics();
        // This is the pose of the fisheye sensor relative to the T265 coordinate system
        auto extrinsics_live = fisheye_stream.get_extrinsics_to(pipe_profile.get_stream(RS2_STREAM_POSE));

        std::cout << std::endl << "CALIBRATION FROM LIVE STREAM:" << std::endl;
        print(intrinsics_live, extrinsics_live);

        if (g_app_data.is_record)
        {
            // write calibration to json file
            write_calibration(device, intrinsics_live, extrinsics_live, g_app_data.json_path);
            std::cout << "Calibration file written to :" << g_app_data.json_path << std::endl;
        }

        run_detection_loop(pipe, intrinsics_live, extrinsics_live);
        pipe->stop();
    }
    
    if (g_app_data.is_replay)
    {
        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);
        cfg.enable_stream(RS2_STREAM_FISHEYE, 1);
        cfg.enable_stream(RS2_STREAM_FISHEYE, 2);
        cfg.enable_device_from_file(g_app_data.data_path);
        std::cout << "DATA FROM FILE : " << g_app_data.data_path << std::endl;

        auto pipe = std::make_shared<rs2::pipeline>();
        auto pipe_profile = pipe->start(cfg);
        auto device = pipe->get_active_profile().get_device();
        auto fisheye_stream = pipe_profile.get_stream(RS2_STREAM_FISHEYE, g_app_data.fisheye_sensor_idx);
        auto fisheye_calibration = read_calibration(g_app_data.json_path);

        std::cout << "--------------------------------------------------------------------------------------\n";
        std::cout << "CALIBRATION FROM FILE : " << g_app_data.json_path << std::endl;
        print(fisheye_calibration.intr, fisheye_calibration.f2p);

        run_detection_loop(pipe, fisheye_calibration.intr, fisheye_calibration.f2p);
        pipe->stop();
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (g_app_data.init(argc, argv)) {
        return run();
    }
    return -1;
}
