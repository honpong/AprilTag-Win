//
//  t265-apriltag-demo
//
//  Created by Hon Pong (Gary) Ho on 05/29/19.
//

#include "t265-apriltag-demo.hpp"

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
    bool is_record  = true;
    bool is_replay  = true;
    bool is_ui      = true;
    bool exit_request = false;

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
            else {
                printf("usages:\n t265-apriltag-demo \n");
                printf("  [--record][--replay][--no_ui][--path PATH_TO_BAG_FILE][--json PATH_TO_JSON]\n\n");
                printf("--record : enable recording.                                          \n");
                printf("--replay : replay a recorded T265 .bag file with .json calibration.   \n");
                printf("--no_ui  : do not display UI.                                         \n");
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

void print(const rs2_intrinsics& intr, const rs2_extrinsics& extr)
{
    std::cout << "w: " << intr.width << ", h: " << intr.height << ", fx: " << intr.fx << ", fy: " << intr.fy << ", ppx: " << intr.ppx << ", ppy: " << intr.ppy << std::endl;
    std::cout << "m: " << rs2_distortion_to_string(intr.model) << ", ";
    for (auto m : intr.coeffs) { std::cout << m << ", "; } 
    std::cout << std::endl << "extr: ";
    for (auto r : extr.rotation) { std::cout << r << ", "; }
    std::cout << std::endl << "extt: ";
    for (auto t : extr.translation) { std::cout << t << ", "; }
    std::cout << std::endl;
}

void write_calibration(rs2::device& dev, const rs2_intrinsics& intr0, const rs2_extrinsics& extr0, const std::string& calibration_path)
{
    Json::Value json_intr;
    json_intr["fx"] = intr0.fx;
    json_intr["fy"] = intr0.fy;
    json_intr["ppx"] = intr0.ppx;
    json_intr["ppy"] = intr0.ppy;
    json_intr["height"] = intr0.height;
    json_intr["width"] = intr0.width;
    json_intr["model"] = intr0.model;
    json_intr["model_name"] = rs2_distortion_to_string(intr0.model);
    for (const auto& c : intr0.coeffs) {
        json_intr["coeff"].append(c);
    }

    Json::Value json_extr_pose;
    for (const auto& r : extr0.rotation)
        json_extr_pose["rotation"].append(r);
    for (const auto& t : extr0.translation)
        json_extr_pose["translation"].append(t);

    Json::Value json_root;
    json_root["device"]["serial_numer"] = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
    json_root["fisheye"][0]["intrinsics"] = json_intr;
    json_root["fisheye"][0]["extrinsics"]["pose"] = json_extr_pose;

    try {
        std::ofstream fisheye_calibration_file;
        fisheye_calibration_file.open(calibration_path, std::ios_base::out | std::ios_base::trunc);
        Json::StyledStreamWriter writer;
        writer.write(fisheye_calibration_file, json_root);
    }
    catch (...) {
        printf("WARNING: error in writing fisheye calibration file %s\n", calibration_path.c_str());
    }
}

struct calibration_info {
    std::string serial_number;
    rs2_intrinsics intr;
    rs2_extrinsics f2p;
};

calibration_info read_calibration(const std::string& calibration_path)
{
    Json::Value json_data;
    std::ifstream infile;
    calibration_info cali;
    infile.open(calibration_path, std::ifstream::binary);
    if (infile.is_open()) { 
        infile >> json_data; 
        cali.serial_number = json_data["device"]["serial_number"].asString();
        cali.intr.fx = json_data["fisheye"][0]["intrinsics"]["fx"].asFloat();
        cali.intr.fy = json_data["fisheye"][0]["intrinsics"]["fy"].asFloat();
        cali.intr.ppx = json_data["fisheye"][0]["intrinsics"]["ppx"].asFloat();
        cali.intr.ppy = json_data["fisheye"][0]["intrinsics"]["ppy"].asFloat();
        cali.intr.width = json_data["fisheye"][0]["intrinsics"]["width"].asInt();
        cali.intr.height = json_data["fisheye"][0]["intrinsics"]["height"].asInt();
        cali.intr.model = (rs2_distortion)json_data["fisheye"][0]["intrinsics"]["model"].asInt();
        for (int c = 0; c < 5; ++c) { cali.intr.coeffs[c] = json_data["fisheye"][0]["intrinsics"]["coeff"][c].asFloat(); }
        for (int r = 0; r < 9; ++r) { cali.f2p.rotation[r] = json_data["fisheye"][0]["extrinsics"]["pose"]["rotation"][r].asFloat(); }
        for (int t = 0; t < 3; ++t) { cali.f2p.translation[t] = json_data["fisheye"][0]["extrinsics"]["pose"]["translation"][t].asFloat(); }
    }
    else { fprintf(stderr, "WARNING: unable to read %s\n", calibration_path.c_str()); }
    return cali;
}

int run_detection(std::shared_ptr<rs2::pipeline>& pipe, rs2_intrinsics& intr, rs2_extrinsics& extr)
{
    rs2::frameset frames; 
    int fisheye_width = intr.width;
    int fisheye_height = intr.height;

    auto device = pipe->get_active_profile().get_device();

    for (g_app_data.set_exit(false); !g_app_data.is_exit(); )
    {
        if (frames = pipe->wait_for_frames())
        {
            auto fisheye_frame = frames.get_fisheye_frame(g_app_data.fisheye_sensor_idx);

            if (g_app_data.is_ui)
            {
                cv::Mat src(fisheye_height, fisheye_width, CV_8UC1, (void*)fisheye_frame.get_data());
                cv::imshow(g_app_data.app_name, src.clone());

                switch (cv::waitKey(1)) {
                case 'q': case 27: g_app_data.set_exit(); break;
                }
            }
        }
        else { g_app_data.set_exit(); }

        if (device.as<rs2::playback>())
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

        run_detection(pipe, intrinsics_live, extrinsics_live);
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

        run_detection(pipe, fisheye_calibration.intr, fisheye_calibration.f2p);
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
