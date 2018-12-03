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
    bool is_live = true, is_capture = false, is_replay = false; int laser_option = 1;
    std::string path = DEFAULT_PATH;
    int num_frames = 200; std::vector<int> capture_size = { 640,480 };
    rs_shapefit_capability sf_option = RS_SHAPEFIT_PLANE;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--cbox"))                 { sf_option = RS_SHAPEFIT_BOX_COLOR; }
        else if (!strcmp(argv[i], "--box"))             { sf_option = RS_SHAPEFIT_BOX; }
        else if (!strcmp(argv[i], "--plane"))           { sf_option = RS_SHAPEFIT_PLANE; }
        else if (!strcmp(argv[i], "--live"))            { is_live = true; }
        else if (!strcmp(argv[i], "--capture"))         { is_capture = true; is_live = false; }
        else if (!strcmp(argv[i], "--num_frame"))       { num_frames = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--path"))            { path = argv[++i]; }
        else if (!strcmp(argv[i], "--replay"))          { is_replay = true; is_live = false; }
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

#include <functional>
enum stream {DEPTH, IR_L, IR_R, COLOR, GYRO, ACCEL};
typedef std::function<std::unique_ptr<rs_sf_data_stream>()> stream_maker;
struct d435i_buffered_stream : public rs_sf_data_stream, rs_sf_dataset
{
    struct img_data : public rs_sf_data_buf {
        img_data(rs_sf_data_ptr& ref) : rs_sf_data_buf(*ref), _src(ref) { image.cam_pose = _pose; }
        rs_sf_data_ptr _src;
        float          _pose[12] = {1.0f,0,0,0,0,1.0f,0,0,0,0,1.0f,0};
    };
    
    rs_sf_data_ptr add_pose_data(rs_sf_data_ptr& ref, const stream s) {
        auto data = std::make_shared<img_data>(ref);
        if(has_imu()){
            auto* ext = (const float*)(&_stream_info[s].extrinsics[DEPTH]);
            for(auto s : {0,1,2,4,5,6,8,9,10,3,7,11}){
                data->image.cam_pose[s] = *ext++;
            }
        }else { data->image.cam_pose = nullptr; }
        data->image.intrinsics = &_stream_info[s].intrinsics.cam_intrinsics;
        return data;
    }
    
    std::unique_ptr<rs_sf_data_stream> _src;
    rs_sf_dataset_ptr wait_for_data(const std::chrono::milliseconds& wait_time_ms) override { return _src->wait_for_data(); }
    std::string       get_device_name() override { return _src->get_device_name(); }
    string_vec        get_device_info() override { return _src->get_device_info(); }
    stream_info_vec   get_stream_info() override { return _src->get_stream_info(); }
    float             get_depth_unit()  override { return _src->get_depth_unit();  }
    
    stream_info_vec _stream_info;
    rs_sf_intrinsics intrinsics(const stream& s) const { return _stream_info[s].intrinsics.cam_intrinsics; }
    
    stream_maker _maker;
    d435i_buffered_stream(stream_maker&& init) : _maker(init) { reset(); }
    
    int width()  const { return intrinsics(DEPTH).width; }
    int height() const { return intrinsics(DEPTH).height;}
    bool has_imu() const { return _stream_info.size() > 4; }
    
    void reset() {
        _src = _maker();
        
        resize(6);
        at(DEPTH).resize(2);
        at(IR_L).resize(2);
        at(IR_R).resize(2);
        at(COLOR).resize(1);
        
        _stream_info = get_stream_info();
    }
    
    rs_sf_dataset_ptr wait_and_buffer_data(bool reset_imu_buffer = true)
    {
        if(reset_imu_buffer) { reset_imu_buffers(); }
        
        if(!_src){ return nullptr; }
        
        auto data = _src->wait_for_data();
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
                at(s).insert(at(s).end(), data->at(s).begin(), data->at(s).end());
            }
        }
        return data;
    }
    
    void reset_imu_buffers() { at(GYRO).clear(); at(ACCEL).clear(); }
    
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

#include "d435i_default_json.h"
struct d435i_demo_pipeline
{
    rs_shapefit_capability _cap = RS_SHAPEFIT_BOX_COLOR;
    std::string            _path;
    d435i_buffered_stream  _src;
    rs_sf_shapefit_ptr     _boxfit;
    std::unique_ptr<rs2::camera_imu_tracker> _tracker;
    
    d435i_demo_pipeline(const std::string& path, stream_maker&& maker) : _path(path), _src(std::move(maker)) { init_algo_middleware(); }
    
    int init_algo_middleware()
    {
        rs_sf_intrinsics intr[2] = {_src.intrinsics(DEPTH),_src.intrinsics(COLOR)};
        _boxfit  = rs_sf_shapefit_ptr(intr, _cap, _src.get_depth_unit());
        
        if(_src.has_imu()){
            _tracker = rs2::camera_imu_tracker::create();
            
            if(_tracker &&
               !_tracker->init(_path+"camera.json", !_src.is_offline_stream()) &&
               !_tracker->init(default_camera_json, !_src.is_offline_stream())) { return -1; }
        }
        return 0;
    }
    
    int reset()
    {
        _src.reset();
        return init_algo_middleware();
    }
    
    std::unique_ptr<rs_sf_image_rgb> _boxwire;
    std::vector<rs_sf_image> exec()
    {
        std::vector<rs_sf_image> images;
        for(;images.empty();){
            auto new_data = _src.wait_and_buffer_data();
            if(!new_data||new_data->empty()){
                if(reset()<0){ return images;}
                else         { continue;     }
            }
            
            images = _src.images();
            if(_tracker){
                _tracker->process(_src.laser_off_data());
                _tracker->wait_for_image_pose(images);
            }
            
            rs_sf_image boxfit_images[2] = {images[DEPTH], images[COLOR]};
            rs_shapefit_depth_image(_boxfit.get(), boxfit_images);
            
            images[COLOR].intrinsics = images[IR_L].intrinsics;
            rs_sf_planefit_draw_planes(_boxfit.get(), &images[COLOR], &images[IR_L]);
            
            _boxwire = std::make_unique<rs_sf_image_rgb>(&images[IR_L]);
            rs_sf_boxfit_draw_boxes(_boxfit.get(), &(images[IR_R]=*_boxwire), &images[IR_L]);
            rs_sf_boxfit_draw_boxes(_boxfit.get(), &images[COLOR]);
        }
        return images;
    }
};
 
int capture_frames(const std::string& path, const int image_set_size, const int cap_size[2], int laser_option)
{
    const int img_w = 640, img_h = 480;
    rs_sf_gl_context win("capture", img_w * 3, img_h * 3);
    
    std::unique_ptr<rs_sf_data_writer> recorder;
 
    try {
        for(d435i_buffered_stream rs_data_src([&](){return rs_sf_create_camera_imu_stream(img_w, img_h, laser_option);});;)
        {
            auto new_data = rs_data_src.wait_and_buffer_data();
            
            if(!recorder){ recorder = rs_sf_create_data_writer(&rs_data_src, path);}
            recorder->write(*new_data);
        
            auto images = rs_data_src.images();
            if(!win.imshow(images.data(),images.size())){break;}
        }
    }catch(std::exception& e){ fprintf(stderr, "%s\n", e.what()); }
    recorder.reset();
    return 0;
}

int replay_frames(const std::string& path)
{
    bool check_data = true;
    d435i_demo_pipeline pipe(path, [&](){return rs_sf_create_camera_imu_stream(path, check_data);});
    for(rs_sf_gl_context win("replay", pipe._src.width()*3, pipe._src.height()*3); ;check_data=false)
    {
        auto images = pipe.exec();
        if(!win.imshow(images.data(),images.size())){break;}
    }
    return 0;
}

int live_demo(const int cap_size[2], const std::string& path)
{
    d435i_demo_pipeline pipe(path, [&](){return rs_sf_create_camera_imu_stream(cap_size[0],cap_size[1],0);});
    for(rs_sf_gl_context win("live demo", pipe._src.width()*3, pipe._src.height()*3); ;)
    {
        auto images = pipe.exec();
        if(!win.imshow(images.data(),images.size())){break;}
    }
    return 0;
}
