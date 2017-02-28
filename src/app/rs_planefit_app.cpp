#include <librealsense/rs.hpp>
#include <librealsense/rsutil.hpp>
#include <fstream>
#include "json/json.h"
#include "rs_shapefit.h"
#include "rs_sf_image_io.h"

std::string path = "c:\\temp\\shapefit\\f\\";
const int image_set_size = 200;

struct frame_data { rs_sf_image images[2]; std::unique_ptr<unsigned char[]> src[2]; rs_sf_intrinsics depth_intrinsics; };
int capture_frames(const std::string& path);
std::unique_ptr<frame_data> load_one_frame(const std::string& path, const int frame_num = -1);
int run_planefit_live();
int run_planefit_offline(const std::string& path);

int main(int argc, char* argv[])
{
    //return run_planefit_live();
    capture_frames(path);
    return run_planefit_offline(path);
}

std::unique_ptr<frame_data> load_one_frame(const std::string& path, const int frame_num)
{
    auto suffix = (frame_num >= 0 ? "_" + std::to_string(frame_num) : "") + ".pgm";
    auto depth_data = rs_sf_image_read(path + "depth" + suffix, frame_num);
    auto ir_data = rs_sf_image_read(path + "ir" + suffix, frame_num);

    auto r_data = std::make_unique<frame_data>();
    r_data->src[0] = std::move(depth_data->src);
    r_data->src[1] = std::move(ir_data->src); 
    r_data->images[0].data = depth_data->data;
    r_data->images[0].img_w = depth_data->img_w;
    r_data->images[0].img_h = depth_data->img_h;
    r_data->images[0].byte_per_pixel = 2;
    r_data->images[1].data = ir_data->data;
    r_data->images[1].img_w = ir_data->img_w;
    r_data->images[1].img_h = ir_data->img_h;
    r_data->images[1].byte_per_pixel = 1;
    r_data->images[0].frame_id = r_data->images[1].frame_id = frame_num;

    ////////////////////////////////////////////////////////////////////////////////
    Json::Value calibration_data;
    std::ifstream infile;
    infile.open(path + "calibration.json", std::ifstream::binary);
    infile >> calibration_data;

    Json::Value json_depth_intrinsics = calibration_data["depth_cam"]["intrinsics"];
    auto& depth_intrinsics = r_data->depth_intrinsics;
    depth_intrinsics.cam_fx = json_depth_intrinsics["fx"].asFloat();
    depth_intrinsics.cam_fy = json_depth_intrinsics["fy"].asFloat();
    depth_intrinsics.cam_px = json_depth_intrinsics["ppx"].asFloat();
    depth_intrinsics.cam_py = json_depth_intrinsics["ppy"].asFloat();
    depth_intrinsics.img_w = json_depth_intrinsics["width"].asInt();
    depth_intrinsics.img_h = json_depth_intrinsics["height"].asInt();

    return std::move(r_data);
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
    
    struct dataset { std::unique_ptr<rs_sf_image_auto> depth, ir, displ; };
    std::deque<dataset> image_set;

    rs_sf_gl_context win("display");

    while (1) {
        auto fs = syncer.wait_for_frames();
        if (fs.size() == 0) break;
        if (fs.size() < 2) continue;

        rs::frame* frames[RS_STREAM_COUNT];
        for (auto& f : fs) { frames[f.get_stream_type()] = &f; }

        auto img_w = frames[RS_STREAM_DEPTH]->get_width();
        auto img_h = frames[RS_STREAM_DEPTH]->get_height();

        image_set.push_back({});
        auto depth_data = (unsigned short*)(image_set.back().depth = std::make_unique<rs_sf_image_depth>(img_w, img_h, frames[RS_STREAM_DEPTH]->get_data()))->data;
        auto ir_data = (image_set.back().ir = std::make_unique<rs_sf_image_mono>(img_w, img_h, frames[RS_STREAM_INFRARED]->get_data()))->data;
        auto displ_data = (image_set.back().displ = std::make_unique<rs_sf_image_mono>(img_w * 2, img_h))->data;
        for (int y = 0, p = 0; y < img_h; ++y) {
            for (int x = 0; x < img_w; ++x, ++p) {
                displ_data[y*img_w * 2 + x] = static_cast<unsigned char>(depth_data[p] * 255.0 / 8000.0);
                displ_data[(y * 2 + 1)*img_w + x] = ir_data[p];
            }
        }

        while (image_set.size() > image_set_size) image_set.pop_front();
        printf("\r image set size %d    ", (int)image_set.size());

        if (!win.imshow(image_set.back().displ.get())) break;
    }

    printf("\n");
    int frame_id = 0;
    for (auto& dataset : image_set) {
        printf("\r writing frame %d    ", frame_id);
        std::string suffix = std::to_string(frame_id++) + ".pgm";
        rs_sf_image_write(path + "depth_" + suffix, dataset.depth.get());
        rs_sf_image_write(path + "ir_" + suffix, dataset.ir.get());
        rs_sf_image_write(path + "displ_" + suffix, dataset.displ.get());
    }
    printf("\n");

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
    root["depth_cam"]["num_frame"] = (int)image_set.size();

    try {
        Json::StyledStreamWriter writer;
        std::ofstream outfile;
        outfile.open(path + "calibration.json");
        writer.write(outfile, root);
    }catch(...){}

    return 0;
}

int run_planefit_live() try
{
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

    auto win = rs_sf_gl_context("display");
    std::vector<unsigned short> prev_depth(480 * 640);
    int frame_id = 0;
    while (1) {
        auto fs = syncer.wait_for_frames();
        if (fs.size() == 0) break;
        if (fs.size() < 2) continue;

        rs::frame* frames[RS_STREAM_COUNT];
        for (auto& f : fs) { frames[f.get_stream_type()] = &f; }

        rs_sf_image image[3];

        image[0].data = (unsigned char*)prev_depth.data(); // (unsigned char*)frames[RS_STREAM_DEPTH]->get_data();
        image[1].data = (unsigned char*)frames[RS_STREAM_INFRARED]->get_data();
        image[0].img_w = image[1].img_w = frames[RS_STREAM_DEPTH]->get_width();
        image[0].img_h = image[1].img_h = frames[RS_STREAM_DEPTH]->get_height();
        image[0].byte_per_pixel = 2;
        image[1].byte_per_pixel = 1;
        image[0].frame_id = image[1].frame_id = frame_id++;

        auto start_time = std::chrono::steady_clock::now();
        if (rs_sf_planefit_depth_image(planefitter, image /*,RS_SF_PLANEFIT_OPTION_RESET*/)) break;
        std::chrono::duration<float, std::milli> last_frame_compute_time = std::chrono::steady_clock::now() - start_time;

        rs_sf_image_rgb rgb(&image[1]);
        rs_sf_planefit_draw_planes(planefitter, &rgb, &image[1]);
        image[2] = rgb;

        if (!win.imshow(&image[1], 2)) break;
        memcpy(prev_depth.data(), frames[RS_STREAM_DEPTH]->get_data(), image[0].num_char()); 
    }

    if (planefitter) rs_sf_planefit_delete(planefitter);
    return 0;
}
catch (const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}


int run_planefit_offline(const std::string& path)
{
    rs_sf_planefit* planefitter = nullptr;
    auto win = rs_sf_gl_context("display", 640, 480);
    int frame_num = 0;
    while (true)
    {
        auto data = load_one_frame(path, frame_num++);
        if (!planefitter) planefitter = rs_sf_planefit_create(&data->depth_intrinsics);
        auto start_time = std::chrono::steady_clock::now();
        if (rs_sf_planefit_depth_image(planefitter, data->images  /*, RS_SF_PLANEFIT_OPTION_RESET */))
            break;
        std::chrono::duration<float, std::milli> last_frame_compute_time = std::chrono::steady_clock::now() - start_time;

        rs_sf_image_rgb rgb(&data->images[1]);
        rs_sf_planefit_draw_planes(planefitter, &rgb, &data->images[1]);

        if (win.imshow(&rgb) == false) break;
    }
    return 0;
}
