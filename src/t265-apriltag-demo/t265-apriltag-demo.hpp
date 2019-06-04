//
//  rs_sf_demo_t265.hpp
//  shapefit
//
//  Created by Hon Pong (Gary) Ho on 2/28/19.
//

#ifndef rs_sf_demo_t265_h
#define rs_sf_demo_t265_h

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <opencv2/opencv.hpp>

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <iostream>
#include <map>
#include <chrono>
#include <mutex>
#include <thread>
#include <future>

#include "json/json.h"

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

#endif /* rs_sf_demo_t265_h */
