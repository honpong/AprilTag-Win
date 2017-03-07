#include <librealsense/rs.hpp>
#include <librealsense/rsutil.hpp>
#include "json/json.h"
#include "rs_shapefit.h"
#include "rs_sf_image_io.h"
//#include "rs_sf_pose_tracker.h"

std::string path = "c:\\temp\\shapefit\\b\\";
const int image_set_size = 200;

int capture_frames(const std::string& path);
int run_planefit_live();
int run_planefit_offline(const std::string& path);
bool run_planefit(rs_sf_planefit* planefitter, rs_sf_image img[2]);

int main(int argc, char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "-live")) {
        if (!strcmp(argv[1], "-capture"))
            capture_frames(path);
        return run_planefit_offline(path);
    }
    return run_planefit_live();
}

struct frame_data {
    rs_sf_image images[2];
    std::unique_ptr<rs_sf_image_auto> src_image[2];
    rs_sf_intrinsics depth_intrinsics;
    int num_frame;

    frame_data(const std::string& path, const int frame_num)
    {
        this->depth_intrinsics = read_calibration(path, num_frame);
        if (num_frame <= frame_num) return;

        const auto suffix = std::to_string(frame_num) + ".pgm";
        images[0] = *(src_image[0] = rs_sf_image_read(path + "depth_" + suffix, frame_num));
        images[1] = *(src_image[1] = rs_sf_image_read(path + "ir_" + suffix, frame_num));
    }

    static void write_frame(const std::string& path, const rs_sf_image* depth, const rs_sf_image* ir, const rs_sf_image* displ)
    {
        rs_sf_image_write(path + "depth_" + std::to_string(depth->frame_id) + ".pgm", depth);
        rs_sf_image_write(path + "ir_" + std::to_string(ir->frame_id) + ".pgm", ir);
        rs_sf_image_write(path + "displ_" + std::to_string(displ->frame_id) + ".pgm", displ);
    }

    static rs_sf_intrinsics read_calibration(const std::string& path, int& num_frame)
    {
        rs_sf_intrinsics depth_intrinsics;
        Json::Value calibration_data;
        std::ifstream infile;
        infile.open(path + "calibration.json", std::ifstream::binary);
        infile >> calibration_data;

        Json::Value json_depth_intrinsics = calibration_data["depth_cam"]["intrinsics"];
        depth_intrinsics.cam_fx = json_depth_intrinsics["fx"].asFloat();
        depth_intrinsics.cam_fy = json_depth_intrinsics["fy"].asFloat();
        depth_intrinsics.cam_px = json_depth_intrinsics["ppx"].asFloat();
        depth_intrinsics.cam_py = json_depth_intrinsics["ppy"].asFloat();
        depth_intrinsics.img_w = json_depth_intrinsics["width"].asInt();
        depth_intrinsics.img_h = json_depth_intrinsics["height"].asInt();

        num_frame = calibration_data["depth_cam"]["num_frame"].asInt();
        return depth_intrinsics;
    }

    static void write_calibration(const std::string& path, const rs_intrinsics& intrinsics, int num_frame)
    {
        Json::Value json_intr, root;
        json_intr["fx"] = intrinsics.fx;
        json_intr["fy"] = intrinsics.fy;
        json_intr["ppx"] = intrinsics.ppx;
        json_intr["ppy"] = intrinsics.ppy;
        json_intr["model"] = intrinsics.model;
        json_intr["height"] = intrinsics.height;
        json_intr["width"] = intrinsics.width;
        for (const auto& c : intrinsics.coeffs)
            json_intr["coeff"].append(c);
        root["depth_cam"]["intrinsics"] = json_intr;
        root["depth_cam"]["num_frame"] = num_frame;

        try {
            Json::StyledStreamWriter writer;
            std::ofstream outfile;
            outfile.open(path + "calibration.json");
            writer.write(outfile, root);
        }
        catch (...) {}
    }
};

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

    rs_sf_gl_context win("capture", 1280, 480);
    std::unique_ptr<rs_sf_image_depth> prev_depth;
    while (1) {
        auto fs = syncer.wait_for_frames();
        if (fs.size() == 0) break;
        if (fs.size() < 2) {
            if (fs[0].get_format() == RS_STREAM_DEPTH)
                prev_depth = std::make_unique<rs_sf_image_depth>(fs[0].get_width(), fs[0].get_height(), fs[0].get_data());
            continue;
        }

        rs::frame* frames[RS_STREAM_COUNT];
        for (auto& f : fs) { frames[f.get_stream_type()] = &f; }

        auto img_w = frames[RS_STREAM_DEPTH]->get_width();
        auto img_h = frames[RS_STREAM_DEPTH]->get_height();

        image_set.push_back({});
        image_set.back().depth = std::move(prev_depth);
        prev_depth = std::make_unique<rs_sf_image_depth>(img_w, img_h, frames[RS_STREAM_DEPTH]->get_data());
        auto depth_data = (unsigned short*)image_set.back().depth->data;
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
        dataset.depth->frame_id = dataset.ir->frame_id = dataset.displ->frame_id = frame_id++;
        frame_data::write_frame(path, dataset.depth.get(), dataset.ir.get(), dataset.displ.get());
    }
    printf("\n");

    frame_data::write_calibration(path, intrinsics, (int)image_set.size());
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

   // bool sp_init = rs_sf_setup_scene_perception(
   //    intrinsics.fx, intrinsics.fy,
   //     intrinsics.ppx, intrinsics.ppy,
   //     intrinsics.width, intrinsics.height,
   //     320, 240, RS_SF_LOW_RESOLUTION);

    std::vector<unsigned short> prev_depth(480 * 640);
    int frame_id = 0;
    while (1) {
        auto fs = syncer.wait_for_frames();
        if (fs.size() == 0) break;
        if (fs.size() < 2) continue;

        rs::frame* frames[RS_STREAM_COUNT];
        for (auto& f : fs) { frames[f.get_stream_type()] = &f; }

        rs_sf_image image[] = { {},{} };
        image[0].data = (unsigned char*)prev_depth.data(); // (unsigned char*)frames[RS_STREAM_DEPTH]->get_data();
        image[1].data = (unsigned char*)frames[RS_STREAM_INFRARED]->get_data();
        image[0].img_w = image[1].img_w = frames[RS_STREAM_DEPTH]->get_width();
        image[0].img_h = image[1].img_h = frames[RS_STREAM_DEPTH]->get_height();
        image[0].byte_per_pixel = 2;
        image[1].byte_per_pixel = 1;
        image[0].frame_id = image[1].frame_id = frame_id++;

        if (!run_planefit(planefitter, image)) break;
        memcpy(prev_depth.data(), frames[RS_STREAM_DEPTH]->get_data(), image[0].num_char());
    }

    if (planefitter) rs_sf_planefit_delete(planefitter);
    //if (sp_init) rs_sf_pose_tracking_release();
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
    int frame_num = 0;
    bool sp_init = false;
    while (true)
    {
        frame_data data(path, frame_num++);
        if (data.src_image[0] == nullptr) {
            frame_num = 0;
            continue;
        }

        if (!planefitter) {
            planefitter = rs_sf_planefit_create(&data.depth_intrinsics);
            //sp_init = rs_sf_setup_scene_perception(
            //    data.depth_intrinsics.cam_fx, data.depth_intrinsics.cam_fy,
            //    data.depth_intrinsics.cam_px, data.depth_intrinsics.cam_py,
            //    data.depth_intrinsics.img_w, data.depth_intrinsics.img_h,
            //    320, 240, RS_SF_LOW_RESOLUTION);
        }

        if (!run_planefit(planefitter, data.images)) break;
    }

    //if (sp_init) rs_sf_pose_tracking_release();
    if (planefitter) rs_sf_planefit_delete(planefitter);
    return 0;
}

bool run_planefit(rs_sf_planefit * planefitter, rs_sf_image img[2])
{
    static rs_sf_gl_context win("display"); 
    //static std::unique_ptr<float[]> buf;
    //static bool was_tracking = false;
    //static std::vector<unsigned short> sdepth(img[0].num_pixel() / 4);
    //static std::vector<unsigned char> scolor(img[1].num_pixel() / 4 * 3);

    auto start_time = std::chrono::steady_clock::now();
    /*
    auto depth_data = (unsigned short*)img[0].data;
    auto ir_data = img[1].data;
    auto s_depth_data = sdepth.data();
    auto s_color_data = scolor.data();
    for (int y = 0, p = 0, w = img[0].img_w; y < 240; ++y) {
        for (int x = 0; x < 320; ++x, ++p) {
            const int p_src = (y * w + x) << 1;
            s_depth_data[p] = depth_data[p_src];
            s_color_data[p * 3] = s_color_data[p * 3 + 1] = s_color_data[p * 3 + 2] = ir_data[p_src];
        }
    }

    float camPose[12];
    bool bResetRequest = false;
    bool track_success = rs_sf_do_scene_perception_tracking(sdepth.data(), scolor.data(), bResetRequest, camPose);
    bool cast_success = rs_sf_do_scene_perception_ray_casting(320, 240, sdepth.data(), buf);
    bool switch_track = (track_success && cast_success) != was_tracking;

    if (was_tracking = (track_success && cast_success)) {
        for (int y = 0, p = 0; y < 480; ++y) {
            for (int x = 0; x < 640; ++x, ++p) {
                depth_data[p] = s_depth_data[(y >> 1) * 320 + (x >> 1)];
            }
        }
        camPose[3] *= 1000.f; camPose[7] *= 1000.0f; camPose[11] *= 1000.0f;
        img[0].cam_pose = img[1].cam_pose = camPose;
    }
    */

    // do plane fit    
    if (rs_sf_planefit_depth_image(planefitter, img /*, !switch_track ? RS_SF_PLANEFIT_OPTION_TRACK : RS_SF_PLANEFIT_OPTION_RESET*/)) return false;
    std::chrono::duration<float, std::milli> last_frame_compute_time = std::chrono::steady_clock::now() - start_time;

    // time measure
    char text[256];
    sprintf(text, "%.2fms/frame", last_frame_compute_time.count());

    // color display
    rs_sf_image_rgb rgb(&img[1]);
    rs_sf_planefit_draw_planes(planefitter, &rgb, &img[1]);

    // plane map display
    rs_sf_image_mono pid(&img[0]);
    rs_sf_planefit_draw_plane_ids(planefitter, &pid, RS_SF_PLANEFIT_DRAW_SCALED);

    // gl drawing
    rs_sf_image show[] = { img[0], img[1], rgb, pid };
    return win.imshow(show, 4, text);
}
