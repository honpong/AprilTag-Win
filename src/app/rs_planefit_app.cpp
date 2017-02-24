#include <librealsense/rs.hpp>
#include <librealsense/rsutil.hpp>
#include <opencv2/opencv.hpp>
#include <fstream>
#include "json/json.h"
#include "rs_shapefit.h"

const int image_set_size = 200;

struct frame_data { cv::Mat src[2]; rs_sf_image images[2]; rs_sf_intrinsics depth_intrinsics; };
int planefit_one_frame(rs_sf_image* images, rs_sf_intrinsics* depth_intrinsics);
int capture_frames(const std::string& path);
frame_data load_one_frame(const std::string& path, const int frame_num = -1);
int run_planefit_live();
int display_planes_and_wait(const rs_sf_planefit* planefitter, rs_sf_image& bkg_image, float compute_time_ms);

int main(int argc, char* argv[])
{
     return run_planefit_live();

    std::string path = "c:\\temp\\shapefit\\e\\";
    //capture_frames(path);
    //planefit_one_frame(data.images, &data.depth_intrinsics);

    rs_sf_planefit* planefitter = nullptr;
    for (int i = 0; i < image_set_size; ++i)
    {
        auto data = load_one_frame(path, i);
        if (!planefitter) planefitter = rs_sf_planefit_create(&data.depth_intrinsics);

        auto start_time = std::chrono::steady_clock::now();
        rs_sf_planefit_depth_image(planefitter, data.images  /*, RS_SF_PLANEFIT_OPTION_RESET */);
        std::chrono::duration<float, std::milli> last_frame_compute_time = std::chrono::steady_clock::now() - start_time;
        printf("frame %d, duration %.2f ms\n", data.images[0].frame_id, last_frame_compute_time.count());

        if (display_planes_and_wait(planefitter, data.images[1], last_frame_compute_time.count()) == 'q')
            break;
    }
    rs_sf_planefit_delete(planefitter);
    return 0;
}

frame_data load_one_frame(const std::string& path, const int frame_num)
{
    frame_data r_data;
    auto suffix = (frame_num >= 0 ? "_" + std::to_string(frame_num) : "") + ".png";
    auto depth_data = r_data.src[0] = cv::imread(path + "depth" + suffix, CV_LOAD_IMAGE_UNCHANGED);
    auto ir_data = r_data.src[1] = cv::imread(path + "ir" + suffix, CV_LOAD_IMAGE_UNCHANGED);

    auto& input_images = r_data.images;
    input_images[0].data = depth_data.data;
    input_images[0].img_w = depth_data.cols;
    input_images[0].img_h = depth_data.rows;
    input_images[0].byte_per_pixel = 2;
    input_images[1].data = ir_data.data;
    input_images[1].img_w = ir_data.cols;
    input_images[1].img_h = ir_data.rows;
    input_images[1].byte_per_pixel = 1;
    input_images[0].frame_id = input_images[1].frame_id = frame_num;

    ////////////////////////////////////////////////////////////////////////////////
    Json::Value calibration_data;
    std::ifstream infile;
    infile.open(path + "calibration.json", std::ifstream::binary);
    infile >> calibration_data;

    Json::Value json_depth_intrinsics = calibration_data["depth_cam"]["intrinsics"];
    auto& depth_intrinsics = r_data.depth_intrinsics;
    depth_intrinsics.cam_fx = json_depth_intrinsics["fx"].asFloat();
    depth_intrinsics.cam_fy = json_depth_intrinsics["fy"].asFloat();
    depth_intrinsics.cam_px = json_depth_intrinsics["ppx"].asFloat();
    depth_intrinsics.cam_py = json_depth_intrinsics["ppy"].asFloat();
    depth_intrinsics.img_w = json_depth_intrinsics["width"].asInt();
    depth_intrinsics.img_h = json_depth_intrinsics["height"].asInt();

    return r_data;
}

int planefit_one_frame(rs_sf_image* input_images, rs_sf_intrinsics* depth_intrinsics)
{
    ////////////////////////////////////////////////////////////////////////////////
    auto planefitter = rs_sf_planefit_create(depth_intrinsics);
    rs_sf_planefit_depth_image(planefitter, input_images);
    rs_sf_planefit_delete(planefitter);
    return 0;
}

int capture_frames(const std::string& path) {

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
    
    struct dataset { cv::Mat depth, ir, displ; };
    std::deque<dataset> image_set;
    while (1) {
        auto fs = syncer.wait_for_frames();
        if (fs.size() == 0) break;
        if (fs.size() < 2) continue;

        rs::frame* frames[RS_STREAM_COUNT];
        for (auto& f : fs) { frames[f.get_stream_type()] = &f; }

        auto depth_data = frames[RS_STREAM_DEPTH]->get_data();
        auto img_w = frames[RS_STREAM_DEPTH]->get_width();
        auto img_h = frames[RS_STREAM_DEPTH]->get_height();
        auto depth_image = cv::Mat(img_h, img_w, CV_16U, (void*)depth_data).clone();
        auto raw_ir_image = cv::Mat(img_h, img_w, CV_8U, (void*)frames[RS_STREAM_INFRARED]->get_data()).clone();
        cv::Mat displ_image;
        depth_image.convertTo(displ_image, CV_8U, 255.0 / 8000.0);
        cv::hconcat(displ_image, raw_ir_image, displ_image);
        cv::imshow("DS5 depth", displ_image);
    
        image_set.push_back({ depth_image,raw_ir_image,displ_image });
        while (image_set.size() > image_set_size) image_set.pop_front();
        if (cv::waitKey(1) == 'q') break;
    }

    int frame_id = 0;
    for (auto& dataset : image_set) {
        std::string suffix = std::to_string(frame_id++) + ".png";
        cv::imwrite(path + "depth_" + suffix, dataset.depth);
        cv::imwrite(path + "ir_" + suffix, dataset.ir);
        cv::imwrite(path + "displ_" + suffix , dataset.displ);
    }

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
    root["depth_cam"]["num_frame"] = image_set_size;

    try {
        Json::StyledStreamWriter writer;
        std::ofstream outfile;
        outfile.open(path + "calibration.json");
        writer.write(outfile, root);
    }catch(...){}

    return 0;
}

int run_planefit_live(){

    rs::context ctx;
    auto list = ctx.query_devices();
    if (list.size() == 0) throw std::runtime_error("No device detected.");

    auto dev = list[0];
    rs::util::config config;
    config.enable_stream(RS_STREAM_DEPTH, 640, 480, 30, RS_FORMAT_Z16);
    config.enable_stream(RS_STREAM_INFRARED, 640, 480, 30, RS_FORMAT_Y8);

    auto stream = config.open(dev);
    auto intrinsics = stream.get_intrinsics(RS_STREAM_DEPTH); 
    auto planefitter = rs_sf_planefit_create((rs_sf_intrinsics*)&intrinsics);

    rs::util::syncer syncer;
    stream.start(syncer);
    dev.set_option(RS_OPTION_EMITTER_ENABLED, 1);
    dev.set_option(RS_OPTION_ENABLE_AUTO_EXPOSURE, 1);

    cv::Mat prev_depth(480, 640, CV_16U);
    int frame_id = 0;
    while (1) {
        auto fs = syncer.wait_for_frames();
        if (fs.size() == 0) break;
        if (fs.size() < 2) continue;

        rs::frame* frames[RS_STREAM_COUNT];
        for (auto& f : fs) { frames[f.get_stream_type()] = &f; }

        rs_sf_image image[2];

        image[0].data = prev_depth.data; // (unsigned char*)frames[RS_STREAM_DEPTH]->get_data();
        image[1].data = (unsigned char*)frames[RS_STREAM_INFRARED]->get_data();
        image[0].img_w = image[1].img_w = frames[RS_STREAM_DEPTH]->get_width();
        image[0].img_h = image[1].img_h = frames[RS_STREAM_DEPTH]->get_height();
        image[0].byte_per_pixel = 2;
        image[1].byte_per_pixel = 1;
        image[0].frame_id = image[1].frame_id = frame_id++;

        auto start_time = std::chrono::steady_clock::now();
        rs_sf_planefit_depth_image(planefitter, image /*,RS_SF_PLANEFIT_OPTION_RESET*/ );
        std::chrono::duration<float, std::milli> last_frame_compute_time = std::chrono::steady_clock::now() - start_time;
        if (display_planes_and_wait(planefitter, image[1], last_frame_compute_time.count()) == 'q') break;

        memcpy(prev_depth.data, frames[RS_STREAM_DEPTH]->get_data(), image[0].num_char());
    }

    if (planefitter) rs_sf_planefit_delete(planefitter);
    return 0;
}

int display_planes_and_wait(const rs_sf_planefit* planefitter, rs_sf_image & bkg_image, float compute_time_ms)
{
    rs_sf_image_rgb rgb(&bkg_image);
    rs_sf_planefit_draw_planes(planefitter, &rgb, &bkg_image);

    //cv::imwrite("C:\\temp\\shapefit\\e\\tail_plane_" + std::to_string(rgb.frame_id) + ".png", disp);

    cv::Mat disp(rgb.img_h, rgb.img_w, CV_8UC3, rgb.data);

    auto time_str = std::to_string(compute_time_ms);
    cv::putText(disp, time_str.substr(0, time_str.find_first_of(".") + 2) + "ms", 
        cv::Point(8, 25), CV_FONT_NORMAL, 1, cv::Scalar(255, 255, 255));

    cv::imshow("planes", disp);

    return cv::waitKey(1);
}
