#include <librealsense/rs.hpp>
#include <librealsense/rsutil.hpp>
#include "rs_shapefit.h"
#include "rs_sf_image_io.h"
//#include "rs_sf_pose_tracker.h"

const std::string default_path = "c:\\temp\\shapefit\\a\\";
int capture_frames(const std::string& path, const int image_set_size);
int run_shapefit_live(const rs_shapefit_option opt);
int run_shapefit_offline(const std::string& path, const rs_shapefit_option opt);
bool run_shapefit(rs_shapefit* planefitter, rs_sf_image img[2]);

int main(int argc, char* argv[])
{
    bool is_live = true, is_capture = false;
    std::string path = default_path;
    int num_frames = 200;
    rs_shapefit_option sf_option = RS_SHAPEFIT_PLANE;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-box") == 0) { sf_option = RS_SHAPEFIT_BOX; }
        else if (strcmp(argv[i], "-plane") == 0) { sf_option = RS_SHAPEFIT_PLANE; }
        else if (strcmp(argv[i], "-live") == 0) { is_live = true; }
        else if (strcmp(argv[i], "-capture") == 0) { is_capture = true; is_live = false; }
        else if (strcmp(argv[i], "-num_frames") == 0) { num_frames = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-path") == 0) { path = argv[++i]; }
        else if (strcmp(argv[i], "-replay") == 0) { is_live = false; }
        else {
            printf("usages:\n rs_shapefit_app [-box|-plane][-live|-replay][-path PATH][-capture][-num_frame NUM] \n");
            return 0;
        }
    }
    if (path.back() != '\\' && path.back() != '/') path.push_back('\\');
    if (is_capture) capture_frames(path, num_frames);
    if (is_live) return run_shapefit_live(sf_option);
    return run_shapefit_offline(path, sf_option);
}

int capture_frames(const std::string& path, const int image_set_size) {

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

    struct dataset { rs_sf_image_ptr depth, ir, displ; };
    std::deque<dataset> image_set;

    rs_sf_gl_context win("capture", 1280, 480);
    rs_sf_image_ptr prev_depth;
    while (1) {
        auto fs = syncer.wait_for_frames();
        if (fs.size() == 0) break;
        if (fs.size() < 2) {
            if (fs[0].get_format() == RS_STREAM_DEPTH)
                prev_depth = make_depth_ptr(fs[0].get_width(),fs[0].get_height(),-1,fs[0].get_data());
            continue;
        }

        rs::frame* frames[RS_STREAM_COUNT];
        for (auto& f : fs) { frames[f.get_stream_type()] = &f; }

        auto img_w = frames[RS_STREAM_DEPTH]->get_width();
        auto img_h = frames[RS_STREAM_DEPTH]->get_height();

        image_set.push_back({});
        image_set.back().depth = std::move(prev_depth);
        prev_depth = make_depth_ptr(img_w, img_h, -1, frames[RS_STREAM_DEPTH]->get_data());
        auto depth_data = (unsigned short*)image_set.back().depth->data;
        auto ir_data = (image_set.back().ir = make_mono_ptr(img_w, img_h, -1, frames[RS_STREAM_INFRARED]->get_data()))->data;
        auto displ_data = (image_set.back().displ = make_mono_ptr(img_w * 2, img_h))->data;
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
        rs_sf_file_stream::write_frame(path, dataset.depth.get(), dataset.ir.get(), dataset.displ.get());
    }
    printf("\n");

    rs_sf_file_stream::write_calibration(path, *(rs_sf_intrinsics*)&intrinsics, (int)image_set.size());
    return 0;
}

int run_shapefit_live(rs_shapefit_option opt) try
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
    auto shapefit = rs_sf_shapefit_ptr((rs_sf_intrinsics*)&intrinsics, opt);
    
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

        rs_sf_image image[] = { {nullptr,0,0,2},{nullptr,0,0,1} };
        image[0].data = (unsigned char*)prev_depth.data(); // (unsigned char*)frames[RS_STREAM_DEPTH]->get_data();
        image[1].data = (unsigned char*)frames[RS_STREAM_INFRARED]->get_data();
        image[0].img_w = image[1].img_w = frames[RS_STREAM_DEPTH]->get_width();
        image[0].img_h = image[1].img_h = frames[RS_STREAM_DEPTH]->get_height();
        image[0].frame_id = image[1].frame_id = frame_id++;

        if (!run_shapefit(shapefit.get(), image)) break;
        memcpy(prev_depth.data(), frames[RS_STREAM_DEPTH]->get_data(), image[0].num_char());
    }

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

int run_shapefit_offline(const std::string& path, const rs_shapefit_option shapefit_option)
{
    rs_sf_shapefit_ptr shapefitter;
    int frame_num = 0;
    bool sp_init = false;
    while (true)
    {
        rs_sf_file_stream data(path, frame_num++);
        if (data.image.size() == 0) {
            frame_num = 0;
            shapefitter = nullptr;
            continue;
        }

        if (!shapefitter) {
            shapefitter = rs_sf_shapefit_ptr(&data.depth_intrinsics, shapefit_option);
            //sp_init = rs_sf_setup_scene_perception(
            //    data.depth_intrinsics.fx, data.depth_intrinsics.fy,
            //    data.depth_intrinsics.ppx, data.depth_intrinsics.ppy,
            //    data.depth_intrinsics.width, data.depth_intrinsics.height,
            //    320, 240, RS_SF_LOW_RESOLUTION);
        }

        if (!run_shapefit(shapefitter.get(), data.get_images().data())) break;
    }

    //if (sp_init) rs_sf_pose_tracking_release();
    return 0;
}

bool run_shapefit(rs_shapefit * shapefitter, rs_sf_image img[2])
{
    static rs_sf_gl_context win("shape fitting"); 
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
        img[0].cam_pose = img[1].cam_pose = camPose;
    }
    */

    // do plane fit    
    rs_shapefit_set_option(shapefitter, RS_SF_OPTION_PLANE_NOISE, 1);
    //rs_shapefit_set_option(shapefitter, RS_SF_OPTION_MAX_PROCESS_DELAY, -1);
    //rs_shapefit_set_option(shapefitter, RS_SF_OPTION_BOX_PLANE_RES, 1);
    if (rs_shapefit_depth_image(shapefitter, img) < 0) return false;
    std::chrono::duration<float, std::milli> last_frame_compute_time = std::chrono::steady_clock::now() - start_time;

    // time measure
    char text[256];
    sprintf(text, "%.0fms/frame", last_frame_compute_time.count());

    // color display buffer
    rs_sf_image_rgb rgb(&img[1]);

    // display either box wireframe or colored planes
    if (rs_sf_boxfit_draw_boxes(shapefitter, &rgb, &img[1]) != RS_SF_SUCCESS)
        rs_sf_planefit_draw_planes(shapefitter, &rgb, &img[1]);
        
    // plane map display
    rs_sf_image_mono pid(&img[0]);
    rs_shapefit_set_option(shapefitter, RS_SF_OPTION_GET_PLANE_ID, 2);
    rs_sf_planefit_get_plane_ids(shapefitter, &pid);

    //rs_sf_image_write(path + "..\\live\\plane_" + std::to_string(img->frame_id), &pid);
    //rs_sf_image_write(path + "..\\live\\color_" + std::to_string(img->frame_id), &rgb);

    // gl drawing
    rs_sf_image show[] = { img[0], img[1], pid, rgb };
    return win.imshow(show, 4, text);
}
