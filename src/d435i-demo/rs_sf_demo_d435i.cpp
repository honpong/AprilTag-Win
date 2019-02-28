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
#define GET_CAPTURE_DISPLAY_IMAGE(src) src.one_image()
#define COLOR_STREAM_REQUEST {}
#else
#define PATH_SEPARATER '/'
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/temp/shapefit/1/")
#define DEFAULT_PATH (std::string(getenv("HOME"))+"/Desktop/temp/data/")
#define GET_CAPTURE_DISPLAY_IMAGE(src) src.images()
#define COLOR_STREAM_REQUEST {if(app.color_request()){ g_replace_color = !g_replace_color; pipe.reset(true); }}

#endif
#define DEFAULT_CAMERA_JSON default_camera_json
#define STREAM_REQUEST(l) (rs_sf_stream_request{l,-1,-1,g_ir_fps,g_color_fps,g_replace_color})
#define VERSION_STRING "v1.3"

int capture_frames(const std::string& path, const int cap_size[2], int laser_option);
int replay_frames(const std::string& path);
int live_play(const int cap_size[2], const std::string& path);

rs_shapefit_capability g_sf_option = RS_SHAPEFIT_BOX_COLOR;
int g_ir_fps        = 60;
int g_color_fps     = 15;
int g_accel_dec     = 1;
int g_gyro_dec      = 1;
int g_replace_color = 1;
int g_use_sp        = 0;
int g_force_demo_ui = 0;
int g_tablet_screen = 0;
bool g_bypass_box_detect = false;
bool g_print_cmd_pose    = false;
bool g_replay_once       = false;
bool g_write_rgb_pose    = false;
std::string g_pose_path = ".";

int main(int argc, char* argv[])
{
    bool is_live = true, is_capture = false, is_replay = false; int laser_option = 0;
    std::string data_path = DEFAULT_PATH, camera_json_path = "."; //camera.json location
    std::vector<int> capture_size = { 640,480 };

    for (int i = 1; i < argc; ++i) {
        if      (!strcmp(argv[i], "--color"))           { g_replace_color = 0; }
        else if (!strcmp(argv[i], "--cbox"))            { g_sf_option = RS_SHAPEFIT_BOX_COLOR; }
        else if (!strcmp(argv[i], "--box"))             { g_sf_option = RS_SHAPEFIT_BOX; }
        else if (!strcmp(argv[i], "--plane"))           { g_sf_option = RS_SHAPEFIT_PLANE; }
        else if (!strcmp(argv[i], "--live"))            { is_live = true; is_replay = false; g_replay_once = false;}
        else if (!strcmp(argv[i], "--capture"))         { is_capture = true; is_live = false; }
        else if (!strcmp(argv[i], "--path"))            { data_path = camera_json_path = argv[++i]; }
        else if (!strcmp(argv[i], "--replay"))          { is_replay = true; is_live = false; }
        else if (!strcmp(argv[i], "--replay_once"))     { is_replay = true; is_live = false; g_replay_once = true;}
        else if (!strcmp(argv[i], "--hd"))              { capture_size = { 1280,720 }; }
        else if (!strcmp(argv[i], "--qhd"))             { capture_size = { 640,360 }; }
        else if (!strcmp(argv[i], "--vga"))             { capture_size = { 640,480 }; }
        else if (!strcmp(argv[i], "--laser_off"))       { laser_option = 0; }
        else if (!strcmp(argv[i], "--laser_on"))        { laser_option = 1; }
        else if (!strcmp(argv[i], "--laser_interlaced")){ laser_option = 2; }
        else if (!strcmp(argv[i], "--fps"))             { g_ir_fps = atoi(argv[++i]); g_color_fps = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--decimate"))        { g_accel_dec = atoi(argv[++i]); g_gyro_dec = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--sp"))              { g_use_sp = 1; }
        else if (!strcmp(argv[i], "--demo_ui"))         { g_force_demo_ui = 1; }
        else if (!strcmp(argv[i], "--tablet"))          { g_tablet_screen = 1; }
        else if (!strcmp(argv[i], "--box_off"))         { g_bypass_box_detect = true; }
        else if (!strcmp(argv[i], "--print_pose"))      { g_print_cmd_pose = true; }
        else if (!strcmp(argv[i], "--write_rgb_pose"))  { g_write_rgb_pose = true; g_pose_path = argv[++i]; }
        else {
            printf("usages:\n d435i-demo [--color][--cbox|--box|--plane][--live|--replay][--capture][--path PATH]\n");
            printf("                     [--fps IR COLOR][--hd|--qhd|--vga][--laser_off|--laser_on|--laser_interlaced][--decimate ACCEL GYRO] \n");
            printf("                     [--demo_ui][--tablet][--box_off][--write_rgb_pose PATH] \n");
            return 0;
        }
    }
    if (data_path.back() != '\\' && data_path.back() != '/'){ data_path.push_back(PATH_SEPARATER); }
    if (camera_json_path.back() != '\\' && camera_json_path.back() != '/'){ camera_json_path.push_back(PATH_SEPARATER); }
    if (g_pose_path.back() != '\\' && g_pose_path.back() != '/'){ g_pose_path.push_back(PATH_SEPARATER); }
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

    struct stereo_data : public rs_sf_data_buf {
        stereo_data(rs_sf_data_ptr& ir_a, rs_sf_data_ptr& ir_b) : rs_sf_data_buf(*left(ir_a, ir_b)), _ir_l(left(ir_a, ir_b)), _ir_r(right(ir_a, ir_b)) {
            serial_number = decltype(serial_number)(-1);
            sensor_type   = (_ir_l.sensor_type & RS_SF_SENSOR_LASER_OFF) ? RS_SF_SENSOR_STEREO_LASER_OFF : RS_SF_SENSOR_STEREO_LASER_ON;
            sensor_index  = 0;
            stereo[0]     = &_ir_l;
            stereo[1]     = &_ir_r;
        }
        img_data _ir_l, _ir_r;

        static rs_sf_data_ptr& left( rs_sf_data_ptr& a, rs_sf_data_ptr& b) { return a->sensor_index < b->sensor_index ? a : b; }
        static rs_sf_data_ptr& right(rs_sf_data_ptr& a, rs_sf_data_ptr& b) { return a->sensor_index < b->sensor_index ? b : a; }

        static bool contains(const rs_sf_data_ptr& stereo, const rs_sf_data_ptr& src) {
            if (auto* ptr = dynamic_cast<const stereo_data*>(stereo.get())) {
                if (ptr->_ir_l._src == src || ptr->_ir_r._src == src) { return true; }
            }
            return false;
        }
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
    std::string       get_device_name()     override { return _src->get_device_name();  }
    string_vec        get_device_info()     override { return _src->get_device_info();  }
    stream_info_vec   get_stream_info()     override { return _src->get_stream_info();  }
    float             get_depth_unit()      override { return _src->get_depth_unit();   }
    bool              is_offline_stream()   override { return _src->is_offline_stream();}
    bool              set_laser(int option) override { return _src->set_laser(option);  }
    int               get_laser()           override { return _src->get_laser();        }
    
    stream_info_vec _stream_info;
    rs_sf_intrinsics intrinsics(const stream& s) const { return _stream_info[s].intrinsics.cam_intrinsics; }
    bool is_virtual_color_stream() const { return
        _stream_info[COLOR].extrinsics[IR_L].translation[0] == 0 &&
        _stream_info[COLOR].extrinsics[IR_L].translation[1] == 0 &&
        _stream_info[COLOR].extrinsics[IR_L].translation[2] == 0;
    }
    
    stream_maker _maker;
    d435i_buffered_stream(stream_maker&& init) : _maker(init) { reset(); }
    
    int  width()   const { return intrinsics(DEPTH).width; }
    int  height()  const { return intrinsics(DEPTH).height;}
    bool has_imu() const { return _stream_info.size() > 4; }
    
    rs_sf_timestamp _first_timestamp = 0;
    rs_sf_timestamp _last_timestamp  = 0;
    rs_sf_serial_number _first_frame_number = -1;
    rs_sf_serial_number _last_frame_number  = -1;

    void reset() {
        _src = nullptr;
        _src = _maker();
        
        resize(6);
        at(DEPTH).resize(2);
        at(IR_L).resize(2);
        at(IR_R).resize(2);
        at(COLOR).resize(1);
        
        _first_timestamp    = _last_timestamp = 0;
        _first_frame_number = _last_frame_number = -1;
        _stream_info        = get_stream_info();
    }
    
    rs_sf_dataset_ptr wait_and_buffer_data(bool reset_imu_buffer = true)
    {
        if(reset_imu_buffer) { reset_imu_buffers(); }
        reset_img_buffers();
        
        if(!_src){ return nullptr; }
        
        auto data = _src->wait_for_data();
        if(!data || data->empty() ){ return nullptr; }

#if defined(IGNORE_IR_TIMESTAMP) & (IGNORE_IR_TIMESTAMP > 0)
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

    inline std::vector<rs_sf_data_ptr> data_vec_with_stereo(bool laser_off_only, bool keep_original_image = false) const {
        return pair_stereo_images(data_vec(laser_off_only), keep_original_image);
    }

    static std::vector<rs_sf_data_ptr> pair_stereo_images(std::vector<rs_sf_data_ptr>&& src, const bool keep_original)  
    {
        struct paired_data_list : public std::vector<rs_sf_data_ptr> {
            bool contains(const rs_sf_data_ptr& src) const {
                for (auto& d : *this) { if (stereo_data::contains(d, src)) { return true; } }
                return false;
            }
            void emplace_back_stereo(rs_sf_data_ptr& a, rs_sf_data_ptr& b) {
                emplace_back(std::make_shared<stereo_data>(a, b));
            }
        } dst;
        dst.reserve(src.size());
        for (auto& d1 : src) {
            switch (d1->sensor_type) {
            case RS_SF_SENSOR_INFRARED_LASER_OFF:
            case RS_SF_SENSOR_INFRARED_LASER_ON:
                for (auto& d2 : src) {
                    if (d1->sensor_type == d2->sensor_type && d1->sensor_index != d2->sensor_index && d1->frame_number == d2->frame_number) {
                        if (!dst.contains(d1)) { dst.emplace_back_stereo(d1, d2); }
                        break; //pair found 
                    }
                } 
                if (!keep_original && dst.contains(d1)) { break; } //pair found and not keeping original
            default:
                dst.emplace_back(d1); break;
            }
        }
        return dst;
    }
    
    std::vector<std::string> data_text(bool is_horizontal_display = false) const {
        std::vector<std::stringstream> os; os.resize(size());
        for(int s=0; s<(int)_stream_info.size(); ++s){
            os[s] << std::setfill(' ') << std::right << std::setw(3) << _stream_info[s].fps << "Hz";
        }
        
        for(auto img : {DEPTH, IR_L, IR_R, COLOR}){
            for(auto b : {ON_BUF, OFF_BUF}){
                if(at(img).size() > b && at(img)[b]){ os[img] << std::fixed << std::setprecision(0) << ", " << at(img)[b]->timestamp_us << ", " << std::left << std::setw(23) << (b?" ON":"OFF"); }
            }
        }
        
        for(auto imu : {ACCEL, GYRO}){
            if(at(imu).size() > 0 && at(imu).back()){
                os[imu] << std::fixed << std::setprecision(0) << ", " << at(imu).back()->timestamp_us;
                for(auto& v : at(imu).back()->imu.v){ os[imu] << ", " << std::setprecision(3) << std::setw(6) << v; }
            }
        }
        
        if(is_horizontal_display)
        {
            for(int s=0; s<(int)_stream_info.size(); ++s){
                if(at(s).empty()){ continue; }
                os[s] << " | " << _stream_info[s].type << "," << _stream_info[s].index << "," << _stream_info[s].format;
                os[s] << std::right;
                for(auto& t : _stream_info[s].extrinsics[DEPTH].translation){ os[s] << "," << std::setprecision(3) << std::setw(6) << t; }
                for(auto& r : _stream_info[s].extrinsics[DEPTH].rotation){    os[s] << "," << std::setprecision(3) << std::setw(6) << r; }
            }
        }
        
        std::vector<std::string> dst; dst.reserve(os.size());
        for(auto& s : os){ dst.emplace_back(s.str()); }
        return dst;
    }
};

#if (defined(OPENCV_FOUND) | defined(OpenCV_FOUND)) //& defined(EXIF_FOUND)
#include <opencv2/opencv.hpp>
//#include <libexif/exif-data.h>

#define IMG_SUFFIX ".jpg"
#else
#define IMG_SUFFIX ".ppm"
#endif
#include "rs_sf_file_io.hpp"
struct d435i_pose_writer
{
    std::string   _path { g_pose_path };
    std::string   _prefix { "color_" };
    std::ofstream index_file;
    d435i_pose_writer()
    {
        RS_SF_CLEAR_DIRECTORY(_path);
        std::this_thread::yield();
        
        index_file.open(_path + "pose.txt", std::ios_base::out | std::ios_base::trunc);
    }
    
    bool write(const rs_sf_image& rgb)
    {
        if(!index_file.is_open()){ return false; }
        
        std::string filename = _prefix  + std::to_string(rgb.frame_id) + IMG_SUFFIX;
        index_file << filename;
        auto bluh = convert(rgb.cam_pose);
        index_file << "," << rgb.cam_pose[3] << "," << rgb.cam_pose[7] << "," << rgb.cam_pose[11];
        index_file << "," << bluh.omega << "," << bluh.phi << "," << bluh.kappa;
        for(int i=0; i<12; ++i){ index_file << "," << rgb.cam_pose[i]; }
        
        index_file << std::endl;

#if defined(OPENCV_FOUND) | defined(OpenCV_FOUND)
        cv::Mat img(rgb.img_h, rgb.img_w, CV_8UC3, rgb.data), bgr;
        cv::cvtColor(img,bgr, CV_RGB2BGR);
        cv::imwrite(_path + filename, bgr, {CV_IMWRITE_JPEG_QUALITY, 100});
#else
        rs_sf_file_io::rs_sf_image_write(_path+filename, &rgb);
#endif
        return true;
    }
    
    // 0 = R11, 1 = R12, 2 = R13, 3 = tx
    // 4 = R21, 5 = R22, 6 = R23, 7 = ty
    // 8 = R31, 9 = R32, 10= R33, 11= tz
    struct BLUH_angle { float omega, phi, kappa; };
    static BLUH_angle convert(const float pose[12])
    {
        BLUH_angle dst;
        dst.phi = std::atan2(pose[8], pose[10]) * 180 * M_1_PI; //phi = arctan(R31/R33)
        dst.omega = std::atan2(-pose[9], std::sqrt(pose[1]*pose[1]+pose[5]*pose[5])) * 180 * M_1_PI; //omega = arctan(-R32/sqrt(R12^2+R22^2))
        dst.kappa = std::atan2(pose[1],pose[5]) * 180 * M_1_PI; //kappa = arctan(R12/R22)
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
    std::chrono::seconds   _drop_time{ 0 };
    std::unique_ptr<rs_sf_image_rgb>         _boxwire_img;
    std::unique_ptr<rs2::camera_imu_tracker> _imu_tracker, _gpu_tracker;
    rs2::camera_imu_tracker* _tracker{ nullptr };
    int                      _decimate_accel{ g_accel_dec };
    int                      _decimate_gyro{ g_gyro_dec };
    int                      _use_sp{ g_use_sp }, _use_stereo{ 1 }, _print_data{ 0 }, _use_color{ g_replace_color ? 0 : 1 };
    std::string              _app_hint = "", _tracker_hint = "";
    std::vector<std::string> _screen_text;

    std::deque<std::array<float, 3>> _box_dimension_queue;
    std::array<float, 3>             _box_dimension_sum{ 0.0f,0.0f,0.0f };
    std::string                      _box_dimension_string{ "" };
    const int                        _box_dimension_rolling_length{ 1000 };
    bool                             _bypass_box_detect{ g_bypass_box_detect };
    bool                             _print_cmd_pose{ g_print_cmd_pose };
    bool                             _replay_once{ g_replay_once };
    bool                             _write_rgb_pose{ g_write_rgb_pose };
    std::unique_ptr<d435i_pose_writer> _pose_writer;
    
    d435i_exec_pipeline(const std::string& path, stream_maker&& maker) : _path(path), _src(std::move(maker)) { select_camera_tracking(true); }
    std::string box_dim_string() const { return _box_dimension_string; }

    bool _use_primary = false;
    void select_camera_tracking(bool use_primary) {
        if (_use_primary != use_primary) {
            _use_primary = use_primary;
            reset(false);
        }
    }
    
    void toggle_bypass_box_detect() { _bypass_box_detect = !_bypass_box_detect; }
    
    int reset(bool reset_src = true)
    {
        if (reset_src) { if(_replay_once){ return -1; } _src.reset(); _pose_writer = nullptr; }
        return init_algo_middleware();
    }

    std::vector<rs_sf_image> exec_once()
    {
        std::vector<rs_sf_image> images;
        for (; images.empty();) {
            auto new_data = _src.wait_and_buffer_data();
            if (!new_data || new_data->empty()) {
                if (reset() < 0) { return images; }
                else { continue; }
            }

            images = _src.images();
            _tracker_hint = "Move Tablet Around";
            rs2::camera_imu_tracker::pose_info poseinfo;
            if (_src.total_runtime() >= _drop_time)
            {
                if (_tracker != nullptr) {
                    //_tracker->process(_src.data_vec(_tracker->require_laser_off()));
                    _tracker->process(_src.data_vec_with_stereo(_tracker->require_laser_off(),_use_stereo?false:true));
                    _tracker_hint = _tracker->prefix() + ": ";
                    poseinfo = _tracker->wait_for_image_pose(images);
                    switch(poseinfo._conf) {
                    case rs2::camera_imu_tracker::HIGH:   _tracker_hint += "High Confidence"; break;
                    case rs2::camera_imu_tracker::MEDIUM: _tracker_hint += "Medium Confidence"; break;
                    default:                              _tracker_hint  = "Move Around / Reset"; break;
                    }
                    
                    if(_write_rgb_pose)
                    {
                        switch(poseinfo._conf){
                            case rs2::camera_imu_tracker::HIGH:
                            case rs2::camera_imu_tracker::MEDIUM:
                                if(!_pose_writer){ _pose_writer = std::make_unique<d435i_pose_writer>(); }
                                for(auto i : images){
                                    if(i.byte_per_pixel==3 && i.cam_pose){ _pose_writer->write(i); }
                                }
                                break;
                            default: break;
                        }
                    }
                }
                else {
                    _tracker_hint = "Bypass Camera Tracking";
                }
            }
            
            _screen_text.clear();
            _screen_text.reserve(8);
            _screen_text.emplace_back(_tracker_hint);
            _screen_text.emplace_back(_app_hint);
            
            if(_print_data){
                for(auto& s : _src.data_text(_print_data==1?true:false)){
                    _screen_text.emplace_back(s);
                }
                _screen_text.emplace_back(print_pose(poseinfo, images[0].cam_pose,_print_data));
            }
                                          
            if(_print_cmd_pose){ printf("%s\n", print_pose(poseinfo, images[0].cam_pose, -1).c_str()); }
            
            if (_bypass_box_detect){ _screen_text[0] += ", Box Detect OFF"; continue; }
        
            rs_sf_image boxfit_images[2] = { images[DEPTH], images[COLOR] };
            auto status = rs_shapefit_depth_image(_boxfit.get(), boxfit_images);

            if (try_get_box(status) == false) {
                rs_sf_planefit_draw_planes(_boxfit.get(), &images[COLOR], &images[COLOR]);
            }
            else {
                if (!_boxwire_img) { _boxwire_img = std::make_unique<rs_sf_image_rgb>(&images[IR_L]); _boxwire_img->cam_pose = nullptr; }
                rs_sf_boxfit_draw_boxes(_boxfit.get(), &(images[IR_R] = *_boxwire_img), &images[IR_L]);
                rs_sf_boxfit_draw_boxes(_boxfit.get(), &images[COLOR]);
            }
        }
        return images;
    }

protected:

    int init_algo_middleware()
    {
        bool sync = _src.is_offline_stream();
        rs_sf_intrinsics intr[2] = { _src.intrinsics(DEPTH),_src.intrinsics(COLOR) };

        if (!_gpu_tracker) {
            _gpu_tracker = _use_sp ? rs2::camera_imu_tracker::create_gpu() : nullptr;
            if (_gpu_tracker) {
                printf("SP Tracker Available \n");
                _gpu_tracker->init(&intr[0], (int)RS_SF_MED_RESOLUTION);
            }
        }
        else {
            _gpu_tracker->init(nullptr, (int)RS_SF_MED_RESOLUTION); // reset
        }

        if (_src.has_imu()) {
            _drop_time = std::chrono::seconds(sync ? 0 : 3);
            _imu_tracker = rs2::camera_imu_tracker::create();

            if (_imu_tracker){
                if(     _imu_tracker->init(_path + "camera.json", !sync, _decimate_accel, _decimate_gyro)){ _app_hint = _path + "camera.json"; }
                else if(_imu_tracker->init(DEFAULT_CAMERA_JSON,   !sync, _decimate_accel, _decimate_gyro)){ _app_hint = ""; }
                else {  _imu_tracker = nullptr; }
                
                if(_imu_tracker){
                    _imu_tracker->process({rs2::camera_imu_tracker::make_stereo_msg(_use_stereo?true:false)});
                    _imu_tracker->process({rs2::camera_imu_tracker::make_color_msg(_use_color?true:false)});
                }
            }
        }
        
        set_camera_tracker_ptr();
        
        _boxfit = rs_sf_shapefit_ptr(intr, _cap = (_src.is_virtual_color_stream() && _src.get_laser() ? RS_SHAPEFIT_BOX : g_sf_option), _src.get_depth_unit());
        rs_shapefit_set_option(_boxfit.get(), RS_SF_OPTION_BOX_SCAN_MODE, 1);
        rs_shapefit_set_option(_boxfit.get(), RS_SF_OPTION_PLANE_NOISE, 2); //noisy planes
        //rs_shapefit_set_option(_boxfit.get(), RS_SF_OPTION_BOX_BUFFER, 21); //more buffering
        rs_shapefit_set_option(_boxfit.get(), RS_SF_OPTION_MAX_NUM_BOX, 1); //output single box
        rs_shapefit_set_option(_boxfit.get(), RS_SF_OPTION_ASYNC_WAIT, sync ? -1 : 0);
        
        return 0;
    }

    void set_camera_tracker_ptr() {
        if (_use_primary){
            _tracker = _imu_tracker.get(); _src.set_laser(0);
        }else{
            _tracker = _gpu_tracker.get(); _src.set_laser(1);
        }

        if (_tracker == nullptr)                 { fprintf(stderr, "No camera tracker selected. \n"); }
        else if (_tracker == _imu_tracker.get()) { fprintf(stderr, "IMU camera tracker selected.\n"); }
        else if (_tracker == _gpu_tracker.get()) { fprintf(stderr, "GPU camera tracker selected.\n"); }

        _src.reset_img_buffers();
    }

    bool try_get_box(const rs_sf_status& boxfit_status) {
        rs_sf_box box;
        if (RS_SF_SUCCESS == rs_sf_boxfit_get_box(_boxfit.get(), 0, &box)) {
            if( boxfit_status == RS_SF_SUCCESS ){ return update_box_dim_string(&box); }
            else { return true; }
        }
        else { return update_box_dim_string(nullptr); }
    }

    static std::array<float, 3> to_array(const rs_sf_box& box) {
        return {
            std::sqrt(box.dim_sqr(0))*1000.0f,
            std::sqrt(box.dim_sqr(1))*1000.0f,
            std::sqrt(box.dim_sqr(2))*1000.0f };
    }

    bool update_box_dim_string(const rs_sf_box* box) {
        if (box) {
            _box_dimension_queue.emplace_back(to_array(*box));
            _box_dimension_sum[0] += _box_dimension_queue.back()[0];
            _box_dimension_sum[1] += _box_dimension_queue.back()[1];
            _box_dimension_sum[2] += _box_dimension_queue.back()[2];
            while (_box_dimension_queue.size() > _box_dimension_rolling_length) {
                _box_dimension_sum[0] -= _box_dimension_queue.front()[0];
                _box_dimension_sum[1] -= _box_dimension_queue.front()[1];
                _box_dimension_sum[2] -= _box_dimension_queue.front()[2];
                _box_dimension_queue.pop_front();
            }
            const float box_queue_size = (float)_box_dimension_queue.size();
            _box_dimension_string = 
                std::to_string((int)std::round(_box_dimension_sum[0] / box_queue_size)) + "x" +
                std::to_string((int)std::round(_box_dimension_sum[1] / box_queue_size)) + "x" +
                std::to_string((int)std::round(_box_dimension_sum[2] / box_queue_size));
            return true;
        }
        else {
            _box_dimension_sum = { 0.0f,0.0f,0.0f };
            _box_dimension_queue.clear();
            _box_dimension_string = "";
            return false;
        }
    }
    
    std::string print_pose(const rs2::camera_imu_tracker::pose_info& poseinfo, const float cam_pose[12], int print_mode) const
    {
        std::stringstream pose_str;
        pose_str << " pose " << poseinfo._systime << ", " << poseinfo._conf;
        
        if(print_mode == 1){
            pose_str << "                       |        " << std::fixed;
            for(int p=0; p<12; ++p){ pose_str << (p!=0? ",":"") << std::setprecision(3) << std::setw(6) << cam_pose[p]; }
        }
        if(print_mode == -1){
            pose_str << " | " <<  std::fixed;
            for(int p=0; p<12; ++p){ pose_str << (p!=0? ",":"") << std::setprecision(9) << std::setw(12) << cam_pose[p]; }
        }
        return pose_str.str();
    }
};
 
int capture_frames(const std::string& path, const int cap_size[2], int laser_option) try
{
    d435i_buffered_stream src([&](){return rs_sf_create_camera_imu_stream(cap_size[0], cap_size[1], STREAM_REQUEST(laser_option));});
    auto recorder = rs_sf_create_data_writer(&src, path);
    for(rs_sf_gl_context win("capture", src.width(), src.height());;)
    {
        recorder->write(*src.wait_and_buffer_data());
        auto images = GET_CAPTURE_DISPLAY_IMAGE(src);
        if(!win.imshow(images.data(),(int)images.size())){break;}
    }
    return 0;
}catch(std::exception& e){ fprintf(stderr, "%s\n", e.what()); return -1; }

bool check_data = true;
int run_dev_ui(d435i_exec_pipeline& pipe) try
{
    for(rs_sf_gl_context win("replay", pipe._src.width()*3, pipe._src.height()*3); ;check_data=false)
    {
        auto images = pipe.exec_once();
        if(!win.imshow(images.data(),(int)images.size(),pipe._tracker_hint.c_str())){break;}
    }
    return 0;
}catch(std::exception& e){ fprintf(stderr, "%s\n", e.what()); return -1; }

int run_demo_ui(d435i_exec_pipeline& pipe) try
{
    int app_w = g_tablet_screen ? 800/2 : pipe._src.width()*3/2, app_h = g_tablet_screen ? 1280/2 : pipe._src.height();
    for(d435i::window app(app_w, app_h, pipe._src.get_device_name()+" Box Scan Example"); app;)
    {
        auto images = pipe.exec_once();
        if(g_replay_once && images.size() == 0 ){ break; }
        if(images.size() <= COLOR){ continue; }
        app.render_ui(nullptr, &images[COLOR], true, pipe._screen_text, VERSION_STRING);
        app.render_box_dim(pipe.box_dim_string());
        
        pipe.select_camera_tracking(app.dense_request());
        if(app.stereo_request()){ pipe._use_stereo = 1-pipe._use_stereo; pipe.reset(false); }
        else if(app.reset_request()){ pipe.reset(false); }
        if(app.print_request()){ pipe._print_data = pipe._print_data ? 0 : (app.is_horizontal()?1:2); }
        else { pipe._print_data = pipe._print_data ? (app.is_horizontal() && app.width() >= 800 ?1:2) : 0; }
        if(app.boxde_request()){ pipe.toggle_bypass_box_detect(); }
        COLOR_STREAM_REQUEST
    }
    return 0;
}catch(std::exception& e){ fprintf(stderr, "%s\n", e.what()); return -1; }

int replay_frames(const std::string& path)
{
    d435i_exec_pipeline pipe(path, [&](){return rs_sf_create_camera_imu_stream(path, check_data);});
    //if(pipe._src.has_imu()){ pipe.select_camera_tracking(pipe._gpu_tracker?false:true); } //force to use sp_tracker at replay
    return g_force_demo_ui ? run_demo_ui(pipe) : run_dev_ui(pipe);
}

int live_play(const int cap_size[2], const std::string& path)
{
    d435i_exec_pipeline pipe(path, [&](){return rs_sf_create_camera_imu_stream(cap_size[0], cap_size[1], STREAM_REQUEST(0));});
    //return run_dev_ui(pipe);
    return run_demo_ui(pipe);
}
