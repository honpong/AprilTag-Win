#include "rs_shapefit.h"
#include "rs_sf_camera.hpp"
#include "rs_sf_image_io.hpp"
#include "rs_sf_gl_context.hpp"
#include "rs_sf_pose_tracker.h"

#include <chrono>

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define PATH_SEPARATER '\\'
#define DEFAULT_PATH "c:\\temp\\shapefit\\b\\"
#else
#define PATH_SEPARATER '/'
#define DEFAULT_PATH (std::string(getenv("HOME"))+"/temp/shapefit/1/")
#endif

int capture_frames(const std::string& path, const int image_set_size, const int cap_size[2]);
int capture_frames2(const std::string& path, const int image_set_size, const int cap_size[2]);
int run_shapefit_live(const rs_shapefit_capability cap, const int cap_size[2]);
int run_shapefit_offline(const std::string& path, const rs_shapefit_capability cap);
bool run_shapefit(rs_shapefit* planefitter, rs_sf_image img[]);

int main(int argc, char* argv[])
{
    bool is_live = false, is_capture = false;
    std::string path = DEFAULT_PATH;
    int num_frames = 200; std::vector<int> capture_size = { 640,480 };
    rs_shapefit_capability sf_option = RS_SHAPEFIT_PLANE;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--cbox") == 0) { sf_option = RS_SHAPEFIT_BOX_COLOR; }
        else if (strcmp(argv[i], "--box") == 0) { sf_option = RS_SHAPEFIT_BOX; }
        else if (strcmp(argv[i], "--plane") == 0) { sf_option = RS_SHAPEFIT_PLANE; }
        else if (strcmp(argv[i], "--live") == 0) { is_live = true; }
        else if (strcmp(argv[i], "--capture") == 0) { is_capture = true; is_live = false; }
        else if (strcmp(argv[i], "--num_frame") == 0) { num_frames = atoi(argv[++i]); }
        else if (strcmp(argv[i], "--path") == 0) { path = argv[++i]; }
        else if (strcmp(argv[i], "--replay") == 0) { is_live = false; }
        else if (strcmp(argv[i], "--hd") == 0) { capture_size = { 1280,720 }; }
        else if (strcmp(argv[i], "--qhd") == 0) { capture_size = { 640,360 }; }
        else if (strcmp(argv[i], "--vga") == 0) { capture_size = { 640,480 }; }
        else {
            printf("usages:\n rs_shapefit_app [--cbox|--box|--plane][--live|--replay][--path PATH][--capture][--num_frame NUM] \n");
            return 0;
        }
    }
    if (path.back() != '\\' && path.back() != '/'){ path.push_back(PATH_SEPARATER); }
    if (is_capture) capture_frames2(path, num_frames, capture_size.data());
    if (is_live) return run_shapefit_live(sf_option, capture_size.data());
    return run_shapefit_offline(path, sf_option);
}

int capture_frames2(const std::string& path, const int image_set_size, const int cap_size[2])
{
    const int img_w = 640, img_h = 480;
    auto rs_data_src = rs_sf_create_camera_imu_stream(img_w, img_h);
    
    rs_sf_gl_context win("capture", img_w * 2, img_h);
    int count = 0;
    while(++count > 30){
        auto data = rs_data_src->get_data();
    }
    std::exit(0);
    return 0;
}

int capture_frames(const std::string& path, const int image_set_size, const int cap_size[2]) {

    const int img_w = cap_size[0], img_h = cap_size[1];
    auto rs_cam = rs_sf_create_camera_stream(img_w, img_h);

    struct dataset { rs_sf_image_ptr depth, ir, displ; };
    std::deque<dataset> image_set;

    rs_sf_gl_context win("capture", img_w * 2, img_h);
    while (1) {
 
        auto frames = rs_cam->get_images();
        image_set.push_back({});
        auto depth_data = (unsigned short*)(image_set.back().depth = make_depth_ptr(&frames[RS_SF_STREAM_DEPTH]))->data;
        //auto ir_data = (image_set.back().ir = make_mono_ptr(&frames[RS_SF_STREAM_INFRARED]))->data;
        auto color_data = (image_set.back().ir = make_rgb_ptr(&frames[RS_SF_STREAM_COLOR]))->data;
        auto displ_data = (image_set.back().displ = make_mono_ptr(img_w * 2, img_h))->data;
 
        for (int y = 0, p = 0; y < img_h; ++y) {
            for (int x = 0; x < img_w; ++x, ++p) {
                displ_data[y*img_w * 2 + x] = static_cast<unsigned char>(depth_data[p] * 255.0 / 4000.0);
                displ_data[(y * 2 + 1)*img_w + x] = color_data[p * 3];
            }
        }

        while ((int)image_set.size() > image_set_size) image_set.pop_front();
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

    rs_sf_file_stream::write_calibration(path, *rs_cam->get_intrinsics(RS_SF_STREAM_DEPTH), *rs_cam->get_intrinsics(RS_SF_STREAM_COLOR), *rs_cam->get_extrinsics(RS_SF_STREAM_COLOR, RS_SF_STREAM_DEPTH), (int)image_set.size(), rs_cam->get_depth_unit());
    return 0;
}

int run_shapefit_live(rs_shapefit_capability cap, const int cap_size[2]) try
{
    const int img_w = cap_size[0], img_h = cap_size[1];
    auto rs_cam = rs_sf_create_camera_stream(img_w, img_h);
    auto shapefit = rs_sf_shapefit_ptr(rs_cam->get_intrinsics(), cap, rs_cam->get_depth_unit());

   // bool sp_init = rs_sf_setup_scene_perception(
   //    intrinsics.fx, intrinsics.fy,
   //     intrinsics.ppx, intrinsics.ppy,
   //     intrinsics.width, intrinsics.height,
   //     320, 240, RS_SF_LOW_RESOLUTION);

    while(1)
    {
        auto image = rs_cam->get_images();
        if (!run_shapefit(shapefit.get(), image)) break;
    }

    //if (sp_init) rs_sf_pose_tracking_release();
    return 0;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

int run_shapefit_offline(const std::string& path, const rs_shapefit_capability shapefit_capability)
{
    rs_sf_shapefit_ptr shapefitter;
    rs_sf_file_stream data(path);
    //bool sp_init = false;
    while (true)
    {
        if (!shapefitter) {
            shapefitter = rs_sf_shapefit_ptr(data.get_intrinsics(), shapefit_capability, data.get_depth_unit());
            rs_shapefit_set_option(shapefitter.get(), RS_SF_OPTION_ASYNC_WAIT, -1);
            //sp_init = rs_sf_setup_scene_perception(
            //    data.depth_intrinsics.fx, data.depth_intrinsics.fy,
            //    data.depth_intrinsics.ppx, data.depth_intrinsics.ppy,
            //    data.depth_intrinsics.width, data.depth_intrinsics.height,
            //    320, 240, RS_SF_LOW_RESOLUTION);
        }

        if (!run_shapefit(shapefitter.get(), data.get_images())) break;

        if (data.is_end_of_stream())
            shapefitter = nullptr;
    }

    //if (sp_init) rs_sf_pose_tracking_release();
    return 0;
}

bool run_shapefit(rs_shapefit * shapefitter, rs_sf_image img[])
{
    static rs_sf_gl_context win("shape fitting", img[RS_SF_STREAM_DEPTH].img_w * 2, img[RS_SF_STREAM_DEPTH].img_h * 2);

    auto* img_d = img + RS_SF_STREAM_DEPTH;
    auto* img_ir = img + RS_SF_STREAM_INFRARED;
    auto* img_c = img + RS_SF_STREAM_COLOR;

    if (!img_c || !img_c->data) img_c = img_ir;

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
    rs_shapefit_set_option(shapefitter, RS_SF_OPTION_BOX_PLANE_RES, 1);
    //rs_shapefit_set_option(shapefitter, RS_SF_OPTION_PLANE_RES, 1);
    //rs_shapefit_set_option(shapefitter, RS_SF_OPTION_ASYNC_WAIT, -1);
    if (rs_shapefit_depth_image(shapefitter, img_d) < 0) return false;
    std::chrono::duration<float, std::milli> last_frame_compute_time = std::chrono::steady_clock::now() - start_time;

    // color display buffer
    rs_sf_image_rgb rgb_plane(img_d->img_w/2,img_d->img_h/2, img_d->frame_id, nullptr, img_d->cam_pose, img_d->intrinsics), rgb_box(img_c);

    // draw plane color
    rs_sf_planefit_draw_planes(shapefitter, &rgb_plane);

    // display either box wireframe or plane_id
    if (rs_sf_boxfit_draw_boxes(shapefitter, &rgb_box, img_c) != RS_SF_SUCCESS) {
        rs_shapefit_set_option(shapefitter, RS_SF_OPTION_GET_PLANE_ID, 2);
        rgb_box.byte_per_pixel = 1;
        rs_sf_planefit_get_plane_ids(shapefitter, &rgb_box);
    } 
    // display plane contours
    //rs_sf_planefit_get_planes(shapefitter, &rgb_plane);
    rs_sf_boxfit_draw_boxes(shapefitter, &rgb_plane);

    // time measure
    std::ostringstream text; 
    text << (int)last_frame_compute_time.count() << "ms/frame";
    
    rs_sf_box box = {};
    if (rs_sf_boxfit_get_box(shapefitter, 0, &box) == RS_SF_SUCCESS) {
        text << ", box " << int(std::sqrt(box.dim_sqr(0)) * 1000) << " x " << int(std::sqrt(box.dim_sqr(1)) * 1000) << " x " << int(std::sqrt(box.dim_sqr(2)) * 1000) << " mm^3";

        //rs_sf_boxfit_raycast_boxes(shapefitter, &box, img_d);
    }

    //rs_sf_image_write("c:\\temp\\shapefit\\live\\plane_"" + std::to_string(img->frame_id), &pid);
    //rs_sf_image_write("c:\\temp\\shapefit\\live\\color_" + std::to_string(img->frame_id), &rgb_box);

    // gl drawing
    rs_sf_image show[] = { *img_d, *img_c, rgb_plane, rgb_box };
    return win.imshow(show, 4, text.str().c_str());
}
