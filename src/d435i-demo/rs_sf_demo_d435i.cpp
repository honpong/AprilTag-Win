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
#include "d435i_default_json.h"
#include "rs_sf_demo_d435i.hpp"

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define PATH_SEPARATER '\\'
#define DEFAULT_PATH "C:\\temp\\data\\"
#define STREAM_REQUEST(l,t) (rs_sf_stream_request{l,400,250,t})
#define GET_CAPTURE_DISPLAY_IMAGE(src) src.one_image()
#define DECIMATE_ACCEL 10
#define DECIMATE_GYRO  2
#define IGNORE_IR_TIMESTAMP 1
#else
#define PATH_SEPARATER '/'
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/temp/shapefit/1/")
#define DEFAULT_PATH (std::string(getenv("HOME"))+"/Desktop/temp/data/")
#define STREAM_REQUEST(l,t) (rs_sf_stream_request{l,-1,-1,t})
#define GET_CAPTURE_DISPLAY_IMAGE(src) src.images()
#define DECIMATE_ACCEL 1
#define DECIMATE_GYRO  1
#define IGNORE_IR_TIMESTAMP 0
#endif

int capture_frames(const std::string& path, const int cap_size[2], int laser_option);
int replay_frames(const std::string& path);
int live_play(const int cap_size[2], const std::string& path);
rs_shapefit_capability g_sf_option = RS_SHAPEFIT_BOX_COLOR;
int g_ts_option = 0;
int g_accel_dec = DECIMATE_ACCEL;
int g_gyro_dec = DECIMATE_GYRO;

int main(int argc, char* argv[])
{
    bool is_live = true, is_capture = false, is_replay = false; int laser_option = 0;
    std::string data_path = DEFAULT_PATH, camera_json_path = "."; //camera.json location
    std::vector<int> capture_size = { 640,480 };

    for (int i = 1; i < argc; ++i) {
        if      (!strcmp(argv[i], "--cbox"))            { g_sf_option = RS_SHAPEFIT_BOX_COLOR; }
        else if (!strcmp(argv[i], "--box"))             { g_sf_option = RS_SHAPEFIT_BOX; }
        else if (!strcmp(argv[i], "--plane"))           { g_sf_option = RS_SHAPEFIT_PLANE; }
        else if (!strcmp(argv[i], "--live"))            { is_live = true; }
        else if (!strcmp(argv[i], "--capture"))         { is_capture = true; is_live = false; }
        else if (!strcmp(argv[i], "--path"))            { data_path = camera_json_path = argv[++i]; }
        else if (!strcmp(argv[i], "--replay"))          { is_replay = true; is_live = false; }
        else if (!strcmp(argv[i], "--hd"))              { capture_size = { 1280,720 }; }
        else if (!strcmp(argv[i], "--qhd"))             { capture_size = { 640,360 }; }
        else if (!strcmp(argv[i], "--vga"))             { capture_size = { 640,480 }; }
        else if (!strcmp(argv[i], "--laser_off"))       { laser_option = 0; }
        else if (!strcmp(argv[i], "--laser_on"))        { laser_option = 1; }
        else if (!strcmp(argv[i], "--laser_interlaced")){ laser_option = 2; }
        else if (!strcmp(argv[i], "--ts"))              { g_ts_option = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--decimate"))        { g_accel_dec = atoi(argv[++i]); g_gyro_dec = atoi(argv[++i]); }
        else {
            printf("usages:\n d435i-demo [--cbox|--box|--plane][--live|--replay][--capture][--path PATH]\n");
            printf("                     [--hd|--qhd|--vga][--laser_off|--laser_on|--laser_interlaced][--ts 0,1,2,7,9][--decimate accel gyro] \n");
            return 0;
        }
    }
    if (data_path.back() != '\\' && data_path.back() != '/'){ data_path.push_back(PATH_SEPARATER); }
    if (camera_json_path.back() != '\\' && camera_json_path.back() != '/'){ camera_json_path.push_back(PATH_SEPARATER); }
    if (is_capture) capture_frames(data_path, capture_size.data(), laser_option);
    if (is_replay)  replay_frames(data_path);
    if (is_live)    live_play(capture_size.data(),camera_json_path);
    return 0;
}

#include <chrono>
#include <functional>
enum stream {DEPTH, IR_L, IR_R, COLOR, GYRO, ACCEL};
enum laser_stream {ANY_BUF=0, ON_BUF=0, OFF_BUF=1};
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
        auto* ext = (const float*)(&_stream_info[s].extrinsics[DEPTH]);
        for (auto s : { 0,1,2,4,5,6,8,9,10,3,7,11 }) {
            data->image.cam_pose[s] = *ext++;
        }
        data->image.intrinsics = &_stream_info[s].intrinsics.cam_intrinsics;
        return data;
    }
    
    std::unique_ptr<rs_sf_data_stream> _src;
    rs_sf_dataset_ptr wait_for_data(const std::chrono::milliseconds& wait_time_ms) override { return _src->wait_for_data(); }
    std::string       get_device_name()   override { return _src->get_device_name();  }
    string_vec        get_device_info()   override { return _src->get_device_info();  }
    stream_info_vec   get_stream_info()   override { return _src->get_stream_info();  }
    float             get_depth_unit()    override { return _src->get_depth_unit();   }
    bool              is_offline_stream() override { return _src->is_offline_stream();}
    bool              set_laser(int option) override { return _src->set_laser(option);}
    int               get_laser()         override { return _src->get_laser(); }
    
    stream_info_vec _stream_info;
    rs_sf_intrinsics intrinsics(const stream& s) const { return _stream_info[s].intrinsics.cam_intrinsics; }
    
    stream_maker _maker;
    d435i_buffered_stream(stream_maker&& init) : _maker(init) { reset(); }
    
    int width()  const { return intrinsics(DEPTH).width; }
    int height() const { return intrinsics(DEPTH).height;}
    bool has_imu() const { return _stream_info.size() > 4; }
    
    rs_sf_timestamp _first_timestamp = 0;
    rs_sf_timestamp _last_timestamp  = 0;
    rs_sf_serial_number _first_frame_number = -1;
    rs_sf_serial_number _last_frame_number = -1;

    void reset() {
        _src = _maker();
        
        resize(6);
        at(DEPTH).resize(2);
        at(IR_L).resize(2);
        at(IR_R).resize(2);
        at(COLOR).resize(1);
        
        _first_timestamp = _last_timestamp = 0;
        _first_frame_number = _last_frame_number = -1;
        _stream_info = get_stream_info();
    }
    
    rs_sf_dataset_ptr wait_and_buffer_data(bool reset_imu_buffer = true)
    {
        if(reset_imu_buffer) { reset_imu_buffers(); }
        reset_img_buffers();
        
        if(!_src){ return nullptr; }
        
        auto data = _src->wait_for_data();
        if(!data || data->empty() ){ return nullptr; }

#ifdef IGNORE_IR_TIMESTAMP
        for (auto dimg : data->at(DEPTH)) {
            for (auto s : { IR_L, IR_R }) {
                for (auto ir : data->at(s)) {
                    if (ir->frame_number == dimg->frame_number) { ir->timestamp_us = dimg->timestamp_us; }
                }
            }
        }
#endif //IGNORE_IR_TIMESTAMP
        
        for(auto s : {DEPTH,IR_L,IR_R}){
            for(auto& d : data->at(s)){
                at(s)[(d->sensor_type & RS_SF_SENSOR_LASER_OFF)?OFF_BUF:ON_BUF] = add_pose_data(d,s);
            }
        }
        for(auto& d : data->at(COLOR)){
            at(COLOR)[ANY_BUF] = add_pose_data(d,COLOR);
        }
        for(auto s : {GYRO, ACCEL}){
            if(data->size()>s && !data->at(s).empty()){
                at(s).insert(at(s).end(), data->at(s).begin(), data->at(s).end());
            }
        }
        
        update_timestamp_difference();
        return data;
    }
    
    void update_timestamp_difference() {
        for(auto& s : *this){
            for(auto& d : s){
                if(d && (d->sensor_type == RS_SF_SENSOR_COLOR || d->sensor_type == RS_SF_SENSOR_DEPTH || d->sensor_type == RS_SF_SENSOR_DEPTH_LASER_OFF)){
                    if (_first_frame_number == -1) { _first_frame_number = d->frame_number; }
                    if (_last_frame_number < d->frame_number) { _last_frame_number = d->frame_number; }
                    if (_first_timestamp == 0) { _first_timestamp = d->timestamp_us; }
                    if(_last_timestamp < d->timestamp_us){_last_timestamp=d->timestamp_us;}
                }
            }
        }
  
        for (auto s : { GYRO, ACCEL }) {
            for (auto& imu : at(s)) {
                if (!imu) { continue; }
                auto factor = std::max(1.0,_last_timestamp / imu->timestamp_us);
                imu->timestamp_us *= std::pow(10.0,(int)std::round(std::log10(factor)));
            }
        }
    }
    
    std::chrono::seconds total_runtime() const {
        //return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::microseconds((unsigned long long)(_last_timestamp-_first_timestamp)));
        return std::chrono::seconds((unsigned long long)((_last_frame_number - _first_frame_number) * (1.0 / 30.0)));
    }
    
    void reset_imu_buffers() { at(GYRO).clear(); at(ACCEL).clear(); }
    void reset_img_buffers() {
        if(_src->get_laser()==0){ at(DEPTH)[ON_BUF].reset(); at(IR_L)[ON_BUF].reset(); at(IR_R)[ON_BUF].reset(); }
        if(_src->get_laser()==1){ at(DEPTH)[OFF_BUF].reset(); at(IR_L)[OFF_BUF].reset(); at(IR_R)[OFF_BUF].reset(); }
    }
    
    bool empty() const {
        if(rs_sf_dataset::empty()){ return true; }
        for(const auto& d : *this){ if(!d.empty()){ return false; }}
        return true;
    }
    
    bool laser_images(const laser_stream& lb = ON_BUF) const {
        if(at(DEPTH)[lb] && at(IR_L)[lb] && at(IR_R)[lb] && at(COLOR)[ANY_BUF]){ return true; }
        return false;
    }
    
    std::vector<rs_sf_image> images() const {
        std::vector<rs_sf_image> dst;
        for(auto lb : {ON_BUF, OFF_BUF}){
            if(laser_images(lb)){
                for(auto s : {DEPTH,IR_L,IR_R}){dst.emplace_back(at(s)[lb]->image);}
            }
        }
        if(at(COLOR)[ANY_BUF]){ dst.emplace_back(at(COLOR)[ANY_BUF]->image); }
        return dst;
    }

    std::vector<rs_sf_image> one_image() const {
        if (at(IR_L)[ ON_BUF]) { return{ at(IR_L)[ ON_BUF]->image }; }
        if (at(IR_L)[OFF_BUF]) { return{ at(IR_L)[OFF_BUF]->image }; }
        return{ at(COLOR)[ANY_BUF]->image };
    }
    
    std::vector<rs_sf_data_ptr> data_vec(bool laser_off_only) const {
        std::vector<rs_sf_data_ptr> dst;
        dst.reserve(size());
        for(auto& stream : *this){
            for(auto& data : stream){
                if(!data){ continue; }
                switch(data->sensor_type){
                    case RS_SF_SENSOR_DEPTH_LASER_ON:
                    case RS_SF_SENSOR_INFRARED_LASER_ON:
                        if(laser_off_only){ break; }
                    case RS_SF_SENSOR_DEPTH_LASER_OFF:
                    case RS_SF_SENSOR_INFRARED_LASER_OFF:
                    case RS_SF_SENSOR_GYRO:
                    case RS_SF_SENSOR_ACCEL:
                    case RS_SF_SENSOR_COLOR:
                        dst.emplace_back(data);
                    default: break;
                }
            }
        }
        return dst;
    }
};


#include <deque>
#include <array>
struct d435i_exec_pipeline
{
    rs_shapefit_capability _cap;
    std::string            _path;
    d435i_buffered_stream  _src;
    rs_sf_shapefit_ptr     _boxfit;
    std::chrono::seconds   _drop_time{0};
    std::unique_ptr<rs_sf_image_rgb>         _boxwire;
    std::unique_ptr<rs2::camera_imu_tracker> _imu_tracker, _gpu_tracker;
    rs2::camera_imu_tracker* _tracker = nullptr;
    int                      _decimate_accel = g_accel_dec;
    int                      _decimate_gyro = g_gyro_dec;
    std::deque<std::array<float,3>> _box_dimension_queue;
    const int                       _rolling_average_length = 50000;

    d435i_exec_pipeline(const std::string& path, stream_maker&& maker) : _path(path), _src(std::move(maker)) { select_camera_tracking(true); }
    
    int init_algo_middleware()
    {
        bool sync = _src.is_offline_stream();
        
        rs_sf_intrinsics intr[2] = {_src.intrinsics(DEPTH),_src.intrinsics(COLOR)};
        _boxfit  = rs_sf_shapefit_ptr(intr, _cap = g_sf_option, _src.get_depth_unit());
        rs_shapefit_set_option(_boxfit.get(), RS_SF_OPTION_BOX_SCAN_MODE, 1);
        rs_shapefit_set_option(_boxfit.get(), RS_SF_OPTION_PLANE_NOISE, 2); //noisy planes
        rs_shapefit_set_option(_boxfit.get(), RS_SF_OPTION_BOX_BUFFER, 21); //more buffering
        rs_shapefit_set_option(_boxfit.get(), RS_SF_OPTION_MAX_NUM_BOX, 1); //output single box
        if(sync){ rs_shapefit_set_option(_boxfit.get(), RS_SF_OPTION_ASYNC_WAIT, -1); }

        // assume we are on Windows
        if (!_gpu_tracker) {
            _gpu_tracker = rs2::camera_imu_tracker::create_gpu();
            if(_gpu_tracker){
                printf("SP Tracker Available \n");
                _gpu_tracker->init(&intr[0], (int)RS_SF_MED_RESOLUTION);
            }
        }else {
            _gpu_tracker->init(nullptr, (int)RS_SF_MED_RESOLUTION);
        }
        
        if (_src.has_imu()) {
            _drop_time = std::chrono::seconds(sync ? 0 : 3);
            _imu_tracker = rs2::camera_imu_tracker::create();

            if (_imu_tracker &&
                !_imu_tracker->init(_path + "camera.json", !sync, _decimate_accel, _decimate_gyro) &&
                !_imu_tracker->init(default_camera_json, !sync, _decimate_accel, _decimate_gyro)) {
                _imu_tracker = nullptr;
            }
        }

        set_camera_tracker_ptr();
        
        return 0;
    }
    
    int reset(bool reset_src = true)
    {
        if(reset_src){ _src.reset(); }
        return init_algo_middleware();
    }
    
    bool _use_primary = false;
    bool select_camera_tracking(bool use_primary) {
        if(_use_primary!=use_primary){
            _use_primary = use_primary;
            reset(false);
        }
        return _use_primary;
    }
    
    void set_camera_tracker_ptr() {
        if(_use_primary){
            if(_gpu_tracker){ _tracker = _gpu_tracker.get(); _src.set_laser(1); }
            else            { _tracker = _imu_tracker.get(); _src.set_laser(0); }
        }else{
            if(_gpu_tracker){ _tracker = _imu_tracker.get(); _src.set_laser(0); }
            else            { _tracker = nullptr;            _src.set_laser(1); }
        }
        _src.reset_img_buffers();
    }
    
    std::string _app_hint = "";
    std::shared_ptr<rs_sf_box> _box;
    std::vector<rs_sf_image> exec_once()
    {
        std::vector<rs_sf_image> images;
        for(;images.empty();){
            auto new_data = _src.wait_and_buffer_data();
            if(!new_data||new_data->empty()){
                if(reset()<0){ return images;}
                else         { continue; }
            }
            
            images = _src.images();
            _app_hint = "Move Tablet Around";
            if(_src.total_runtime() >= _drop_time)
            {
                if( _tracker != nullptr ){
                    _tracker->process(_src.data_vec(_tracker->require_laser_off()));
                    switch (_tracker->wait_for_image_pose(images)){
                        case rs2::camera_imu_tracker::HIGH:   _app_hint="High Quality   "; break;
                        case rs2::camera_imu_tracker::MEDIUM: _app_hint="Medium Quality "; break;
                        default:                              _app_hint="Move Around / Reset"; break;
                    }
                }
                else {
                    _app_hint = "bypass";
                }
            }
                
            rs_sf_image boxfit_images[2] = {images[DEPTH], images[COLOR]};
            rs_shapefit_depth_image(_boxfit.get(), boxfit_images);
            
            if(try_get_box()==false){
                //images[COLOR].intrinsics = images[IR_L].intrinsics;
                //rs_sf_planefit_draw_planes(_boxfit.get(), &images[COLOR], &images[IR_L]);
                rs_sf_planefit_draw_planes(_boxfit.get(), &images[COLOR], &images[COLOR]);
            }
                
            _boxwire = std::make_unique<rs_sf_image_rgb>(&images[IR_L]);
            rs_sf_boxfit_draw_boxes(_boxfit.get(), &(images[IR_R]=*_boxwire), &images[IR_L]);
            rs_sf_boxfit_draw_boxes(_boxfit.get(), &images[COLOR]);
            
        }
        return images;
    }
    
    bool try_get_box() {
        rs_sf_box box;
        if(RS_SF_SUCCESS==rs_sf_boxfit_get_box(_boxfit.get(), 0, &box)){
            _box = std::make_shared<rs_sf_box>(box);
        }else{
            _box = nullptr;
        }
        return _box!=nullptr;
    }
    
    static std::array<float,3> to_array(const rs_sf_box& box){
        return {
            std::sqrt(box.dim_sqr(0))*1000.0f,
            std::sqrt(box.dim_sqr(1))*1000.0f,
            std::sqrt(box.dim_sqr(2))*1000.0f};
    }
    
    std::string rolling_average_box_dimension() {
        while(_box_dimension_queue.size()>_rolling_average_length){ _box_dimension_queue.pop_front(); }
        
        float box_queue_size = (float)_box_dimension_queue.size();
        float box_dimension[3] = {0.0f,0.0f,0.0f};
        for(auto& d : _box_dimension_queue){
            box_dimension[0] += d[0];
            box_dimension[1] += d[1];
            box_dimension[2] += d[2];
        }
        return
        std::to_string((int)std::round(box_dimension[0]/box_queue_size))+"x"+
        std::to_string((int)std::round(box_dimension[1]/box_queue_size))+"x"+
        std::to_string((int)std::round(box_dimension[2]/box_queue_size));
    }
    
    std::string box_dim_string()
    {
        if(_box){
            _box_dimension_queue.emplace_back(to_array(*_box));
            return rolling_average_box_dimension();
        }
        _box_dimension_queue.clear();
        return "";
    }
};
 
int capture_frames(const std::string& path, const int cap_size[2], int laser_option) try
{
    d435i_buffered_stream src([&](){return rs_sf_create_camera_imu_stream(cap_size[0], cap_size[1], STREAM_REQUEST(laser_option,g_ts_option));});
    auto recorder = rs_sf_create_data_writer(&src, path);
    for(rs_sf_gl_context win("capture", src.width(), src.height());;)
    {
        recorder->write(*src.wait_and_buffer_data());
        auto images = GET_CAPTURE_DISPLAY_IMAGE(src);
        if(!win.imshow(images.data(),(int)images.size())){break;}
    }
    return 0;
}catch(std::exception& e){ fprintf(stderr, "%s\n", e.what()); return -1; }

int replay_frames(const std::string& path) try
{
    bool check_data = true;
    d435i_exec_pipeline pipe(path, [&](){return rs_sf_create_camera_imu_stream(path, check_data);});
    for(rs_sf_gl_context win("replay", pipe._src.width()*3, pipe._src.height()*3); ;check_data=false)
    {
        auto images = pipe.exec_once();
        if(!win.imshow(images.data(),(int)images.size(),pipe._app_hint.c_str())){break;}
    }
    return 0;
}catch(std::exception& e){ fprintf(stderr, "%s\n", e.what()); return -1; }

int live_play(const int cap_size[2], const std::string& path) try
{
    if(false){
        d435i_exec_pipeline pipe(path, [&](){return rs_sf_create_camera_imu_stream(cap_size[0],cap_size[1],STREAM_REQUEST(0,g_ts_option));});
        for(rs_sf_gl_context win("live demo", pipe._src.width()*3, pipe._src.height()*3); ;)
        {
            auto images = pipe.exec_once();
            if(!win.imshow(&images[3],1)){break;}
        }
    }else{
        d435i_exec_pipeline pipe(path, [&](){return rs_sf_create_camera_imu_stream(cap_size[0], cap_size[1],STREAM_REQUEST(0,g_ts_option));});
        for(d435i::window app(pipe._src.width()*3/2, pipe._src.height(),/*800,1280,*/ pipe._src.get_device_name()+" Box Scan Example"); app;)
        {
            auto images = pipe.exec_once();
            app.render_ui(&images[DEPTH], &images[COLOR], true, pipe._app_hint.c_str());
            app.render_box_dim(pipe.box_dim_string());
        
            pipe.select_camera_tracking(app.dense_request());
            if(app.reset_request()){ pipe.reset(false); }
        }
    }
    return 0;
}catch(std::exception& e){ fprintf(stderr, "%s\n", e.what()); return -1; }
