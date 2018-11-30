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
int live_demo(const int cap_size[2]);

int main(int argc, char* argv[])
{
    bool is_live = false, is_capture = false, is_replay = false; int laser_option = 2;
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
    if (is_live)    live_demo(capture_size.data());
    return 0;
}

struct d435i_dataset : public rs_sf_dataset
{
    struct img_data : public rs_sf_data_buf {
        img_data(rs_sf_data_ptr& ref) : rs_sf_data_buf(*ref), _src(ref) { image.cam_pose = _pose; }
        rs_sf_data_ptr _src;
        float          _pose[12] = {1.0f,0,0,0,0,1.0f,0,0,0,0,1.0f,0};
    };
    static rs_sf_data_ptr add_pose_data(rs_sf_data_ptr& ref) { return std::make_shared<img_data>(ref); }
    
    enum stream {DEPTH, IR_L, IR_R, COLOR, GYRO, ACCEL};
    d435i_dataset()
    {
        resize(6);
        at(DEPTH).resize(2);
        at(IR_L).resize(2);
        at(IR_R).resize(2);
        at(COLOR).resize(1);
    }
    
    d435i_dataset& operator<<(rs_sf_dataset_ptr& data) {
        if(!data){ return *this; }
        
        for(auto s : {DEPTH,IR_L,IR_R}){
            for(auto& d : data->at(s)){
                at(s)[(d->sensor_type & RS_SF_SENSOR_LASER_OFF)?1:0] = add_pose_data(d);
            }
        }
        for(auto& d : data->at(COLOR)){
            at(COLOR)[0] = add_pose_data(d);
        }
        for(auto s : {GYRO, ACCEL}){
            if(data->size()>s && !data->at(s).empty()){
                at(s).splice(at(s).end(), data->at(s));
            }
        }
        return *this;
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
};

int capture_frames(const std::string& path, const int image_set_size, const int cap_size[2], int laser_option)
{
    const int img_w = 640, img_h = 480;
    rs_sf_gl_context win("capture", img_w * 3, img_h * 3);
    
    std::unique_ptr<rs_sf_data_writer> recorder;
    d435i_dataset buf;
 
    try {
        for(auto rs_data_src = rs_sf_create_camera_imu_stream(img_w, img_h, laser_option);;)
        {
            auto new_data = rs_data_src->wait_for_data();
            
            if(!recorder){ recorder = rs_sf_create_data_writer(rs_data_src.get(), path);}
            recorder->write(*new_data);
        
            auto images = (buf << new_data).images();
            if(!win.imshow(images.data(),images.size())){break;}
        }
    }catch(std::exception& e){ fprintf(stderr, "%s\n", e.what()); }
    recorder.reset();
    return 0;
}

int replay_frames(const std::string& path)
{
    auto rs_data_src = rs_sf_create_camera_imu_stream(path,true);
    const int img_w = rs_data_src->get_stream_info()[0].intrinsics.cam_intrinsics.width;
    const int img_h = rs_data_src->get_stream_info()[0].intrinsics.cam_intrinsics.height;
    
    d435i_dataset buf;
    auto tracker = rs2::camera_imu_tracker::create();
    if(tracker){ if(!tracker->init(path+"camera.json", false)){ return -1; }}
    
    for(rs_sf_gl_context win("replay", img_w*3, img_h*3); ;)
    {
        auto new_data = rs_data_src->wait_for_data();
        if(!new_data || new_data->empty()){ rs_data_src = rs_sf_create_camera_imu_stream(path,false); continue; }
        
        auto images = (buf << new_data).images();
        if(tracker){
            tracker->process(buf.laser_off_data());
            tracker->wait_for_image_pose(images);
        }
        
        if(!win.imshow(images.data(),images.size())){break;}
    }
    return 0;
}

int live_demo(const int cap_size[2])
{
    auto rs_data_src = rs_sf_create_camera_imu_stream(cap_size[0],cap_size[1],0);
    const int img_w = rs_data_src->get_stream_info()[0].intrinsics.cam_intrinsics.width;
    const int img_h = rs_data_src->get_stream_info()[0].intrinsics.cam_intrinsics.height;
    
    rs_shapefit_capability cap = rs_shapefit_capability::RS_SHAPEFIT_BOX;
    auto boxfit = rs_sf_shapefit_ptr(&rs_data_src->get_stream_info()[d435i_dataset::DEPTH].intrinsics.cam_intrinsics,cap,rs_data_src->get_depth_unit());
    d435i_dataset buf;
    
    for(rs_sf_gl_context win("live demo", img_w*3, img_h*3); ;)
    {
        auto new_data = rs_data_src->wait_for_data();
        auto images = (buf << new_data).images();
        if(!buf.full_laser_off_imageset()){ continue; }
        rs_shapefit_depth_image(boxfit.get(), images.data());
        rs_sf_planefit_draw_planes(boxfit.get(), &images[d435i_dataset::COLOR]);
        if(!win.imshow(images.data(),images.size())){break;}
    }
    return 0;
}
