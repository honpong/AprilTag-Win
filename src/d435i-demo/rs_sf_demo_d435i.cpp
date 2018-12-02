//
//  rs_sf_demo_d435i.cpp
//  d435i-demo
//
//  Created by Hon Pong (Gary) Ho on 11/6/18.
//

#include "rs_shapefit.h"
#include "rs_sf_camera.hpp"
#include "rs_sf_gl_context.hpp"
#include "rs_sf_pose_tracker.h"

#include <chrono>

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define PATH_SEPARATER '\\'
#define DEFAULT_PATH "c:\\temp\\shapefit\\b\\"
#else
#define PATH_SEPARATER '/'
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/temp/shapefit/1/")
#define DEFAULT_PATH (std::string(getenv("HOME"))+"/Desktop/temp/data/")
#endif

int capture_frames(const std::string& path, const int image_set_size, const int cap_size[2], int laser_option);
int replay_frames(const std::string& path);
int live_demo(const int cap_size[2], const std::string& path);

int main(int argc, char* argv[])
{
    bool is_live = false, is_capture = false, is_replay = false; int laser_option = 1;
    std::string path = DEFAULT_PATH;
    int num_frames = 200; std::vector<int> capture_size = { 640,480 };
    rs_shapefit_capability sf_option = RS_SHAPEFIT_PLANE;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--cbox"))                 { sf_option = RS_SHAPEFIT_BOX_COLOR; }
        else if (!strcmp(argv[i], "--box"))             { sf_option = RS_SHAPEFIT_BOX; }
        else if (!strcmp(argv[i], "--plane"))           { sf_option = RS_SHAPEFIT_PLANE; }
        else if (!strcmp(argv[i], "--live"))            { is_live = true; }
        else if (!strcmp(argv[i], "--capture"))         { is_capture = true; }
        else if (!strcmp(argv[i], "--num_frame"))       { num_frames = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--path"))            { path = argv[++i]; }
        else if (!strcmp(argv[i], "--replay"))          { is_replay = true; }
        else if (!strcmp(argv[i], "--hd"))              { capture_size = { 1280,720 }; }
        else if (!strcmp(argv[i], "--qhd"))             { capture_size = { 640,360 }; }
        else if (!strcmp(argv[i], "--vga"))             { capture_size = { 640,480 }; }
        else if (!strcmp(argv[i], "--laser_off"))       { laser_option = 0; }
        else if (!strcmp(argv[i], "--laser_on"))        { laser_option = 1; }
        else if (!strcmp(argv[i], "--laser_interlaced")){ laser_option = 2; }
        else {
            printf("usages:\n d435i-demo [--cbox|--box|--plane][--live|--replay][--path PATH][--capture][--num_frame NUM] \n");
            printf("                     [--hd|--qhd|--vga][--laser_off|--laser_on|--laser_interlaced] \n");
            return 0;
        }
    }
    if (path.back() != '\\' && path.back() != '/'){ path.push_back(PATH_SEPARATER); }
    if (is_capture) capture_frames(path, num_frames, capture_size.data(), laser_option);
    if (is_replay)  replay_frames(path);
    if (is_live)    live_demo(capture_size.data(),path);
    return 0;
}

enum stream {DEPTH, IR_L, IR_R, COLOR, GYRO, ACCEL};
struct d435i_buffered_src : public rs_sf_dataset, std::unique_ptr<rs_sf_data_stream>
{
    struct img_data : public rs_sf_data_buf {
        img_data(rs_sf_data_ptr& ref) : rs_sf_data_buf(*ref), _src(ref) { image.cam_pose = _pose; }
        rs_sf_data_ptr _src;
        float          _pose[12] = {1.0f,0,0,0,0,1.0f,0,0,0,0,1.0f,0};
    };
    
    d435i_buffered_src(std::unique_ptr<rs_sf_data_stream>&& src) : std::unique_ptr<rs_sf_data_stream>(std::move(src))
    {
        resize(6);
        at(DEPTH).resize(2);
        at(IR_L).resize(2);
        at(IR_R).resize(2);
        at(COLOR).resize(1);
        
        _stream_info = get()->get_stream_info();
    }
    
    std::vector<rs_sf_stream_info> _stream_info;
    const rs_sf_intrinsics& intrinsics(const stream s=DEPTH) const { return _stream_info[s].intrinsics.cam_intrinsics; }
    int width( const stream s=DEPTH) const { return intrinsics(s).width; }
    int height(const stream s=DEPTH) const { return intrinsics(s).height;}
    
    rs_sf_data_ptr add_pose_data(rs_sf_data_ptr& ref, const stream s) {
        auto data = std::make_shared<img_data>(ref);
        auto* ext = (const float*)(&_stream_info[s].extrinsics[DEPTH]);
        for(auto s : {0,1,2,4,5,6,8,9,10,3,7,11}){
            data->image.cam_pose[s] = *ext++;
        }
        data->image.intrinsics = &_stream_info[s].intrinsics.cam_intrinsics;
        return data;
    }
    
    rs_sf_dataset_ptr wait_and_buffer_data()
    {
        if(!get()){ return nullptr; }
        
        auto data = get()->wait_for_data();
        if(!data || data->empty() ){ return nullptr; }
        
        for(auto s : {DEPTH,IR_L,IR_R}){
            for(auto& d : data->at(s)){
                at(s)[(d->sensor_type & RS_SF_SENSOR_LASER_OFF)?1:0] = add_pose_data(d,s);
            }
        }
        for(auto& d : data->at(COLOR)){
            at(COLOR)[0] = add_pose_data(d,COLOR);
        }
        for(auto s : {GYRO, ACCEL}){
            if(data->size()>s && !data->at(s).empty()){
                at(s).splice(at(s).end(), data->at(s));
            }
        }
        return data;
    }
    
    bool empty() const {
        if(rs_sf_dataset::empty()){ return true; }
        for(const auto& d : *this){ if(!d.empty()){ return false; }}
        return true;
    }
    
    bool full_laser_on_imageset() const {
        if(at(DEPTH)[0] && at(IR_L)[0] && at(IR_R)[0] && at(COLOR)[0]){ return true; }
        return false;
    }
    
    bool full_laser_off_imageset() const {
        if(at(DEPTH)[1] && at(IR_L)[1] && at(IR_R)[1] && at(COLOR)[0]){ return true; }
        return false;
    }
    
    std::vector<rs_sf_image> images() const {
        std::vector<rs_sf_image> dst;
        if(full_laser_on_imageset()){
            for(auto s : {DEPTH,IR_L,IR_R}){dst.emplace_back(at(s)[0]->image);}
        }
        if(full_laser_off_imageset()){
            for(auto s : {DEPTH,IR_L,IR_R}){dst.emplace_back(at(s)[1]->image);}
        }
        if(at(COLOR)[0]){ dst.emplace_back(at(COLOR)[0]->image); }
        return dst;
    }
    
    std::vector<rs_sf_data_ptr> laser_off_data() const {
        std::vector<rs_sf_data_ptr> dst;
        dst.reserve(size());
        for(auto& stream : *this){
            for(auto& data : stream){
                if(!data){ continue; }
                switch(data->sensor_type){
                    case RS_SF_SENSOR_LASER_OFF:
                    case RS_SF_SENSOR_INFRARED_LASER_OFF:
                    case RS_SF_SENSOR_GYRO:
                    case RS_SF_SENSOR_ACCEL:
                        dst.emplace_back(data);
                    default: break;
                }
            }
        }
        return dst;
    }
    
    rs_sf_shapefit_ptr make_boxfit(const rs_shapefit_capability& cap = RS_SHAPEFIT_BOX_COLOR) const {
        rs_sf_intrinsics intr[2] = {intrinsics(DEPTH),intrinsics(COLOR)};
        return rs_sf_shapefit_ptr(intr, cap, get()->get_depth_unit());
    }
    
    rs_sf_image_ptr _boxwire;
    std::vector<rs_sf_image>& run_boxfit(rs_sf_shapefit_ptr& boxfit, std::vector<rs_sf_image>& images) {
        rs_sf_image boxfit_images[2] = {images[DEPTH], images[COLOR]};
        rs_shapefit_depth_image(boxfit.get(), boxfit_images);
        //rs_sf_planefit_draw_planes(boxfit.get(), &images[d435i_buffered_src::COLOR]);
        
        _boxwire = std::make_unique<rs_sf_image_rgb>(&images[IR_L]);
        rs_sf_boxfit_draw_boxes(boxfit.get(), &(images[IR_R]=*_boxwire), &images[IR_L]);
        rs_sf_boxfit_draw_boxes(boxfit.get(), &images[COLOR]);
        
        return images;
    }
};

int capture_frames(const std::string& path, const int image_set_size, const int cap_size[2], int laser_option)
{
    const int img_w = 640, img_h = 480;
    rs_sf_gl_context win("capture", img_w * 3, img_h * 3);
    
    std::unique_ptr<rs_sf_data_writer> recorder;
 
    try {
        for(d435i_buffered_src rs_data_src(rs_sf_create_camera_imu_stream(img_w, img_h, laser_option));;)
        {
            auto new_data = rs_data_src.wait_and_buffer_data();
            
            if(!recorder){ recorder = rs_sf_create_data_writer(rs_data_src.get(), path);}
            recorder->write(*new_data);
        
            auto images = rs_data_src.images();
            if(!win.imshow(images.data(),images.size())){break;}
        }
    }catch(std::exception& e){ fprintf(stderr, "%s\n", e.what()); }
    recorder.reset();
    return 0;
}

#include "d435i_default_json.h"
int replay_frames(const std::string& path)
{
    d435i_buffered_src rs_data_src(rs_sf_create_camera_imu_stream(path, true));
    rs_sf_shapefit_ptr boxfit;
    std::unique_ptr<rs2::camera_imu_tracker> tracker;

    auto reset_entire_system = [&]() -> int
    {
        rs_data_src = d435i_buffered_src(rs_sf_create_camera_imu_stream(path, false));
        boxfit      = rs_data_src.make_boxfit();
        tracker     = rs2::camera_imu_tracker::create();

        if(tracker &&
           !tracker->init(path+"camera.json", false) &&
           !tracker->init(default_camera_json, false)) { return -1; }

        return 0;
    };
    
    for(rs_sf_gl_context win("replay", rs_data_src.width()*3, rs_data_src.height()*3); ;)
    {
        auto new_data = rs_data_src.wait_and_buffer_data();
        if(!boxfit || !new_data || new_data->empty()){
            if(reset_entire_system()<0){ return -1; }
            continue;
        }

        auto images = rs_data_src.images();
        if(tracker){
            tracker->process(rs_data_src.laser_off_data());
            tracker->wait_for_image_pose(images);
        }

        images = rs_data_src.run_boxfit(boxfit, images);
        if(!win.imshow(images.data(),images.size())){break;}
    }
    return 0;
}

int live_demo(const int cap_size[2], const std::string& path)
{
    auto rs_data_src = d435i_buffered_src(rs_sf_create_camera_imu_stream(cap_size[0],cap_size[1],0));
    const int img_w = rs_data_src->get_stream_info()[0].intrinsics.cam_intrinsics.width;
    const int img_h = rs_data_src->get_stream_info()[0].intrinsics.cam_intrinsics.height;
    
    auto boxfit  = rs_data_src.make_boxfit();
    auto tracker = rs2::camera_imu_tracker::create();
    if(tracker && !tracker->init(path+"camera.json", false)){ return -1; }
    
    for(rs_sf_gl_context win("live demo", img_w*3, img_h*3); ;)
    {
        rs_data_src.wait_and_buffer_data();
        
        auto images = rs_data_src.images();
        if(tracker){
            tracker->process(rs_data_src.laser_off_data());
            tracker->wait_for_image_pose(images);
        }
        
        images = rs_data_src.run_boxfit(boxfit, images);
        if(!win.imshow(images.data(),images.size())){break;}
    }
    return 0;
}
