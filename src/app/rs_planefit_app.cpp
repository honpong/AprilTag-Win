#include <librealsense/rs.hpp>
#include <librealsense/rsutil.hpp>
#include <opencv2/opencv.hpp>
#include <fstream>
#include "json/json.h"
#include "rs_shapefit.h"

int planefit_one_frame(const std::string& path);
int capture_one_frame(const std::string& path);

int main(int argc, char* argv[])
{
    std::string path = "c:\\temp\\shapefit\\";
    //return capture_one_frame(path);
    return planefit_one_frame(path);
}

int planefit_one_frame(const std::string & path)
{
    auto depth_data = cv::imread(path + "depth.png", CV_LOAD_IMAGE_UNCHANGED);
    auto ir_data = cv::imread(path + "ir.png", CV_LOAD_IMAGE_UNCHANGED);

    rs_sf_image depth_image = {};
    depth_image.data = depth_data.data;
    depth_image.img_w = depth_data.cols;
    depth_image.img_h = depth_data.rows;
    depth_image.byte_per_pixel = 2;

    ////////////////////////////////////////////////////////////////////////////////
    Json::Value calibration_data;
    std::ifstream infile;
    infile.open(path + "calibration.json", std::ifstream::binary);
    infile >> calibration_data;

    Json::Value json_depth_intrinsics = calibration_data["depth_cam"]["intrinsics"];
    rs_sf_intrinsics depth_intrinsics;
    depth_intrinsics.cam_fx = json_depth_intrinsics["fx"].asFloat();
    depth_intrinsics.cam_fy = json_depth_intrinsics["fy"].asFloat();
    depth_intrinsics.cam_px = json_depth_intrinsics["ppx"].asFloat();
    depth_intrinsics.cam_py = json_depth_intrinsics["ppy"].asFloat();
    depth_intrinsics.img_w = json_depth_intrinsics["width"].asInt();
    depth_intrinsics.img_h = json_depth_intrinsics["height"].asInt();


    ////////////////////////////////////////////////////////////////////////////////
    auto planefitter = rs_sf_planefit_create(&depth_intrinsics);
    rs_sf_planefit_depth_image(planefitter, &depth_image);
    rs_sf_planefit_delete(planefitter);
    return 0;
}

int capture_one_frame(const std::string& path) {

    rs::context ctx;
    auto list = ctx.query_devices();
    if (list.size() == 0) throw std::runtime_error("No device detected.");

    auto dev = list[0];
    rs::util::config config;
    config.enable_stream(RS_STREAM_DEPTH, 640, 480, 30, RS_FORMAT_Z16);
    config.enable_stream(RS_STREAM_INFRARED, 640, 480, 30, RS_FORMAT_Y8);

    auto stream = config.open(dev);
    auto intrinsics = stream.get_intrinsics(RS_STREAM_DEPTH);

    rs::util::syncer syncer;
    stream.start(syncer);
    dev.set_option(RS_OPTION_EMITTER_ENABLED, 1);
    dev.set_option(RS_OPTION_ENABLE_AUTO_EXPOSURE, 1);
    
    cv::Mat depth_image, raw_ir_image, displ_image;
    while (1) {
        auto fs = syncer.wait_for_frames();
        if (fs.size() == 0) break;
        if (fs.size() < 2) continue;

        rs::frame* frames[RS_STREAM_COUNT];
        for (auto& f : fs) { frames[f.get_stream_type()] = &f; }

        auto depth_data = frames[RS_STREAM_DEPTH]->get_data();
        auto img_w = frames[RS_STREAM_DEPTH]->get_width();
        auto img_h = frames[RS_STREAM_DEPTH]->get_height();
        depth_image = cv::Mat(img_h, img_w, CV_16U, (void*)depth_data).clone();
        raw_ir_image = cv::Mat(img_h, img_w, CV_8U, (void*)frames[RS_STREAM_INFRARED]->get_data()).clone();
        depth_image.convertTo(displ_image, CV_8U, 255.0 / 8000.0);
        cv::hconcat(displ_image, raw_ir_image, displ_image);
        cv::imshow("DS5 depth", displ_image);
        if (cv::waitKey(1) == 'q') break;
    }

    cv::imwrite(path + "depth.png", depth_image);
    cv::imwrite(path + "ir.png", raw_ir_image);
    cv::imwrite(path + "displ.png", displ_image);
   
    Json::Value json_intr;
    json_intr["fx"] = intrinsics.fx;
    json_intr["fy"] = intrinsics.fy;
    json_intr["ppx"] = intrinsics.ppx;
    json_intr["ppy"] = intrinsics.ppy;
    json_intr["model"] = intrinsics.model;
    json_intr["height"] = intrinsics.height;
    json_intr["width"] = intrinsics.width;
    for (const auto& c : intrinsics.coeffs)
        json_intr["coeff"].append(c);

    Json::Value root;
    root["depth_cam"]["intrinsics"] = json_intr;

    try {
        Json::StyledStreamWriter writer;
        std::ofstream outfile;
        outfile.open(path + "calibration.json");
        writer.write(outfile, root);
    }catch(...){}

    return 0;
}
