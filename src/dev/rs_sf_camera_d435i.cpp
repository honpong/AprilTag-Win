//
//  rs_sf_camera_d435i.cpp
//  shapefit-dev
//
//  Created by Hon Pong (Gary) Ho on 11/6/18.
//

#include "rs_sf_camera.hpp"
#include "rs_sf_device_manager.hpp"
#include "rs_sf_file_io.hpp"
#include <thread>
#include <mutex>
#include <atomic>
#include <iomanip>
#include <cmath>

#ifndef RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED
#define RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED RS2_OPTION_EMITTER_ENABLED
#endif

struct rs_sf_d435i_writer;
struct rs_sf_d435i_camera : public rs_sf_data_stream, rs_sf_device_manager
{
    rs_sf_d435i_camera(int w, int h, const rs_sf_stream_request& request)
    {
        if(search_device_name("Intel RealSense D435I")){ printf("Intel RealSense D435I Camera Found\n"); }
        else if(search_device_name("Intel RealSense D435" )){ printf("Intel RealSense D435  Camera Found\n");}
        if(!_device){ throw std::runtime_error("No Intel RealSense D435/D435I Found"); }

		auto GYHZ = request.gyro_fps  > 0 ? (float)request.gyro_fps : 200.0f;
		auto ACHZ = request.accel_fps > 0 ? (float)request.accel_fps : 63.0f;

        add_stream_request({RS2_STREAM_DEPTH,   -1,  30, RS2_FORMAT_Z16,            w,  h});
        add_stream_request({RS2_STREAM_INFRARED, 1,  30, RS2_FORMAT_Y8 ,            w,  h});
        add_stream_request({RS2_STREAM_INFRARED, 2,  30, RS2_FORMAT_Y8 ,            w,  h});
        add_stream_request({RS2_STREAM_COLOR,   -1,  30, RS2_FORMAT_RGB8,           w,  h});
        add_stream_request({RS2_STREAM_GYRO,    -1,GYHZ, RS2_FORMAT_MOTION_XYZ32F, -1, -1});
        add_stream_request({RS2_STREAM_ACCEL,   -1,ACHZ, RS2_FORMAT_MOTION_XYZ32F, -1, -1});
        
        find_stream_profiles();
        print_requested_streams();
        
        extract_stream_calibrations();
        print_calibrations();
        
        open(request.laser);
        start();
    }
    
    virtual ~rs_sf_d435i_camera()
    {
        for(auto s : {0,3,4}){
            if(_streams[s].sensor && _pipeline_streaming[s]){
                _pipeline_streaming[s] = false;
                _streams[s].sensor.stop();
                std::lock_guard<std::mutex> lk(_pipeline_mutex[s]);
                _streams[s].sensor.close();
                _streams[s].sensor = nullptr;
                _streams[s].profile = {};
            }
        }
    }
    
    std::atomic<rs_sf_serial_number> _data_packet_count = {0};
    rs_sf_serial_number generate_serial_number()   { return _data_packet_count++; }
    std::string         get_device_name() override { return rs_sf_device_manager::get_device_name(); }
    float               get_depth_unit()  override { return 0.001f; }
    string_vec          get_device_info() override {
        switch(_laser_option)
        {
            case -1: return {rs2_option_to_string(RS2_OPTION_EMITTER_ENABLED)            + std::string("=UNKNOWN")};
            case  0: return {rs2_option_to_string(RS2_OPTION_EMITTER_ENABLED)            + std::string("=0")};
            case  1: return {rs2_option_to_string(RS2_OPTION_EMITTER_ENABLED)            + std::string("=1")};
            case  2: return {rs2_option_to_string(RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED) + std::string("=1")};
        }
        return {""};
    }
    bool                set_laser(int option) override {
        if(_laser_option==option){ return true; }
        if(_laser_option==2)     { return false;}
        try{
            //printf("TRY SETTING EMITTOR %d \n",option);
            _streams[0].sensor.set_option(RS2_OPTION_EMITTER_ENABLED,(float)option);
            _laser_option = option;
            return true;
        }catch(...){}
        return false;
    }
    
    int _laser_option = -1;
    void open(int laser_option)
    {
        // open the depth camera stream
        _streams[0].sensor.open({_streams[0].profile,_streams[1].profile,_streams[2].profile});
        
        try{
            switch(laser_option)
            {
                case 0:
                    _streams[0].sensor.set_option(RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED, 0);
                    _streams[0].sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 0);
                    break;
                case 1:
                    _streams[0].sensor.set_option(RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED, 0);
                    _streams[0].sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1);
                    break;
                case 2:
                    _streams[0].sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1);
                    _streams[0].sensor.set_option(RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED, 1);
                    break;
            }
        }catch(...){ fprintf(stderr,"WARNING: error setting laser option %d!\n", _laser_option); }

        try{
            _laser_option = (int)_streams[0].sensor.get_option(RS2_OPTION_EMITTER_ENABLED);
            if(laser_option==2 &&
               _streams[0].sensor.supports(RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED) &&
               _streams[0].sensor.get_option(RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED)){
                _laser_option=2;
            }
        }catch(...){
            fprintf(stderr,"WARNING: error getting laser option %d! set to %d\n",_laser_option, laser_option);
            _laser_option = laser_option;
        }
        
        // open the color camera stream
        _streams[3].sensor.open(_streams[3].profile);
        
        // open the motion sensor stream
        if(_streams[4].profile && _streams[5].profile){
            _streams[4].sensor.set_option(RS2_OPTION_ENABLE_MOTION_CORRECTION, 0);
            _streams[4].sensor.open({_streams[4].profile,_streams[5].profile});
        }
    }
    
    struct rs_sf_data_auto : public rs_sf_data_buf
    {
        virtual ~rs_sf_data_auto() {}
        
        rs_sf_data_auto(rs2::frame f, rs_sf_stream_select& stream, const rs_sf_serial_number& new_serial_number, int laser_option = -1) : _f(f)
        {
            sensor_index  = _f.get_profile().stream_index();
            sensor_type   = (rs_sf_sensor_t)(_f.get_profile().stream_type());
            timestamp_us  = _f.get_timestamp() * std::chrono::microseconds(std::chrono::milliseconds(1)).count();
            exposure_time_us = -0.1;
            serial_number = new_serial_number;
            frame_number  = _f.get_frame_number();
            
            if (_f.is<rs2::video_frame>()){
                image                = {};
                image.byte_per_pixel = _f.as<rs2::video_frame>().get_bytes_per_pixel();
                image.data           = (unsigned char*)_f.get_data();
                image.frame_id       = frame_number;
                image.img_h          = _f.as<rs2::video_frame>().get_height();
                image.img_w          = _f.as<rs2::video_frame>().get_width();
                image.intrinsics     = (rs_sf_intrinsics*)&stream.cam_intrinsics;
                
                if(sensor_type == RS_SF_SENSOR_DEPTH ||
                   sensor_type == RS_SF_SENSOR_INFRARED){
                    if(laser_option==0 || (laser_option==2 &&
                                           _f.supports_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE) &&
                                           _f.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE)==0)){
                        sensor_type = (rs_sf_sensor_t)(sensor_type|RS_SF_SENSOR_LASER_OFF);
                    }
                }
                try{
                    if(sensor_type != RS_SF_SENSOR_COLOR && //exposure time from D435x color sensor is known to be garbage
                       _f.supports_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE)){
                        exposure_time_us = (double)_f.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
                    }
                }catch(...){}
            }
            else if(_f.is<rs2::motion_frame>()){
                rs_sf_memcpy(imu.v, _f.get_data(), sizeof(imu.v));
                _f = nullptr;
            }
        }
        rs2::frame _f;
    };
    
    std::vector<bool>       _pipeline_streaming;
    std::vector<std::mutex> _pipeline_mutex;
    rs_sf_dataset           _pipeline_buffer;
    bool start()
    {
        if(!_pipeline_buffer.empty()) return false;
        
        _pipeline_buffer.resize(num_streams());
        _pipeline_streaming.resize(num_streams());
        std::vector<std::mutex> mutexes(num_streams());
        _pipeline_mutex.swap(mutexes);
        
        // stereo depth sensors
        _pipeline_streaming[0] = _pipeline_streaming[1] = _pipeline_streaming[2] = true;
        _streams[0].sensor.start([this](rs2::frame f){
            if(!_pipeline_streaming[0]){ return; }
            for(int s : {0,1,2}){
                if(f.get_profile().stream_type() ==_streams[s].type &&
                   f.get_profile().stream_index()==_streams[s].index){
                    auto new_data = std::make_shared<rs_sf_data_auto>(f,_streams[s],generate_serial_number(),_laser_option);
                    std::lock_guard<std::mutex> lk(_pipeline_mutex[s]);
                    _pipeline_buffer[s].emplace_back(new_data);
                }
            }
        });
        
        // color sensor
        _pipeline_streaming[3] = true;
        _streams[3].sensor.start([this](rs2::frame f){
            if(!_pipeline_streaming[3]){ return; }
            auto new_data = std::make_shared<rs_sf_data_auto>(f,_streams[3],generate_serial_number());
            std::lock_guard<std::mutex> lk(_pipeline_mutex[3]);
            _pipeline_buffer[3].emplace_back(new_data);
        });
        
        // imu motion sensor
        if(_streams[4].profile && _streams[5].profile){
            _pipeline_streaming[4] = _pipeline_streaming[5] = true;
            _streams[4].sensor.start([this](rs2::frame f){
                if(!_pipeline_streaming[4]){ return; }
                for(int s : {4,5}){
                    if(f.get_profile().stream_type() ==_streams[s].type &&
                       f.get_profile().stream_index()==_streams[s].index){
                        auto new_data = std::make_shared<rs_sf_data_auto>(f,_streams[s],generate_serial_number());
                        std::lock_guard<std::mutex> lk(_pipeline_mutex[s]);
                        _pipeline_buffer[s].emplace_back(new_data);
                    }
                }
            });
        }
        
        return false;
    }
    
    rs_sf_dataset_ptr wait_for_data(const std::chrono::milliseconds& wait_time_ms) override
    {
        static const auto time_inc = std::chrono::milliseconds(1);
        rs_sf_dataset dst; dst.resize(num_streams());
        
        for(auto wait_time = std::chrono::milliseconds(0); wait_time < wait_time_ms; wait_time+=time_inc)
        {
            //look for data in all streams
            for(int s=0; s<num_streams(); ++s){
                if(_pipeline_buffer[s].empty()) continue;
                std::lock_guard<std::mutex> lk(_pipeline_mutex[s]);
                dst[s].splice(dst[s].end(), _pipeline_buffer[s]);
                //_pipeline_buffer[s] should be empty at this point
            }
            
            //send out data if available
            if(!dst[0].empty()&&!dst[1].empty()&&!dst[2].empty()&&!dst[3].empty()){break;}
            std::this_thread::sleep_for(time_inc);
        }
        return std::make_shared<rs_sf_dataset>(dst);
    }
    
    stream_info_vec get_stream_info() override
    {
        stream_info_vec dst; dst.reserve(num_streams());
        for(auto& stream : _streams)
        {
            if(stream.profile){
                rs_sf_stream_info info;
                info.type  = (rs_sf_sensor_t)stream.type;
                info.index = stream.index;
                info.fps   = stream.fps;
                info.format = stream.format;
                switch(info.type){
                    case RS_SF_SENSOR_GYRO:
                    case RS_SF_SENSOR_ACCEL:
                        info.intrinsics.imu_intrinsics = *(rs_sf_imu_intrinsics*)&stream.imu_intriniscs;
                        break;
                    default:
                        info.intrinsics.cam_intrinsics = *(rs_sf_intrinsics*)&stream.cam_intrinsics;
                        break;
                }
                
                info.extrinsics.reserve(num_streams());
                for(int ss=0; ss<num_streams(); ++ss)
                {
                    if(_streams[ss].profile){
                        info.extrinsics.emplace_back(*(rs_sf_extrinsics*)&stream.extrinsics[ss]);
                    }
                }
                dst.emplace_back(info);
            }
        }
        return dst;
    }
};

struct rs_sf_d435i_writer : public rs_sf_file_io, rs_sf_data_writer
{
    std::ofstream index_file;
    //std::ofstream accel_file;
    //std::ofstream gyro_file;
    
    rs_sf_data_stream* _src;
    
    rs_sf_d435i_writer(rs_sf_data_stream* src, const std::string& path) : rs_sf_file_io(path), _src(src)
    {
        if(dynamic_cast<rs_sf_file_io*>(src)&&dynamic_cast<rs_sf_file_io*>(src)->_folder_path==path){
            _folder_path.append("clone").push_back(PATH_SEPARATER);
            fprintf(stderr,"WARNING: cannot overwrite source directory! Write into %s\n",_folder_path.c_str());
        }
        
        RS_SF_CLEAR_DIRECTORY(_folder_path)
        
        index_file.open(get_index_filepath().c_str(), std::ios_base::out | std::ios_base::trunc);
        
        write_calibrations();
    }
    
    ~rs_sf_d435i_writer()
    {
        for(auto file : {&index_file,/*&accel_file,&gyro_file*/})
        {
            if(file->is_open()){file->close();}
        }
    }
    
    std::string generate_data_filename(const rs_sf_data& item)
    {
        switch(item.sensor_type){
            case RS_SF_SENSOR_ACCEL:
            case RS_SF_SENSOR_GYRO:
                return std::to_string(item.imu.x) + " " + std::to_string(item.imu.y) + " " + std::to_string(item.imu.z);
            default:
                return get_stream_name(item.sensor_type, item.sensor_index)+ std::to_string(_next_frame_num[item.sensor_type][item.sensor_index]++) + _file_format[item.sensor_type];
        }
    }
    
    bool write_header(const std::string& out_filename_or_imu_data, const rs_sf_data& data, const rs_sf_serial_number& dataset_number)
    {
        static const std::string sep = ",";
        
        if(index_file.is_open()){
            std::stringstream os; os
            << dataset_number           << sep << std::setprecision(10) << std::fixed
            << data.timestamp_us        << sep << std::setprecision(2)  << std::fixed
            << data.exposure_time_us    << sep
            << data.serial_number       << sep
            << data.frame_number        << sep
            << data.sensor_type         << sep
            << data.sensor_index        << sep
            << out_filename_or_imu_data << std::endl;
            index_file << os.str();
        }else{
            fprintf(stderr, "WARNING: fail to write to dataset header!\n");
            return false;
        }
        return true;
    }
    
    bool write_calibrations() const
    {
        Json::Value json_root;
        json_root["calibration_file_version"] = RS_SF_CALIBRATION_FILE_VERSION;
        json_root["device_name"] = _src->get_device_name();
        for(auto s : _src->get_device_info())
            json_root["device_info"].append(s);
        
        auto stream_info = _src->get_stream_info();
        for(auto& info : stream_info)
        {
            auto stream_name = get_stream_name(info.type,info.index);
            json_root["sensor_name"].append(stream_name);
        }

        for(auto& info : stream_info)
        {
            Json::Value cam;
            cam["sensor_type"]  = info.type;
            cam["sensor_index"] = info.index;
            cam["sensor_fps"]   = info.fps;
            cam["data_format"]  = info.format;
            cam["intrinsics"]   = write_intrinsics(info.type, info.intrinsics);
            for(int s=0; s<stream_info.size(); ++s)
            {
                cam["extrinsics"][json_root["sensor_name"][s].asString()]=write_extrinsics(info.extrinsics[s]);
            }
            json_root["sensor"][get_stream_name(info.type,info.index)]=cam;
        }
        
        try {
            Json::StyledStreamWriter writer;
            std::ofstream outfile;
            outfile.open(get_calibration_filepath());
            writer.write(outfile, json_root);
        }
        catch (...) {
            fprintf(stderr,"WARNING: fail to write calibration to %s \n",_folder_path.c_str());
            return false;
        }
        return true;
    }
    
    std::atomic<rs_sf_serial_number> _dataset_count = {0};
    std::mutex _write_mutex;
    bool write(const rs_sf_dataset& data) override
    {
        auto my_data_set_number = _dataset_count++;
        
        std::deque<const rs_sf_data*> items;
        for(const auto& stream : data){
            for(const auto& item : stream){
                items.emplace_back(item.get());
            }
        }
        
        std::sort(items.begin(), items.end(), [](const rs_sf_data*& a, const rs_sf_data*& b){ return a->serial_number < b->serial_number; });
        
        std::lock_guard<std::mutex> lk(_write_mutex);
        for(auto& item : items){
            auto filename = generate_data_filename(*item);
            if(!write_header(filename, *item, my_data_set_number)){ return false; }
            if((item->sensor_type&0xf)<RS_SF_SENSOR_GYRO){
                std::async(std::launch::async,[this,item=item,filename=filename](){
                    rs_sf_image_write(_folder_path+filename,&item->image);
                });
            }
        }
        return true;
    }
};

template<typename T>
static T& operator>>(T& ss, rs_sf_sensor_t& sensor_type) { ss >> *(int*)&sensor_type; return ss; }

struct rs_sf_d435i_file_stream : public rs_sf_file_io, rs_sf_data_stream
{
    struct rs_sf_data_auto : public rs_sf_data_buf
    {
        virtual ~rs_sf_data_auto() {}
        
        rs_sf_data_auto(const std::string& index_line, const rs_sf_d435i_file_stream* src)
        {
            char sep;
            std::stringstream is(index_line); is
            >> _dataset_number   >> sep
            >> timestamp_us      >> sep
            >> exposure_time_us  >> sep
            >> serial_number     >> sep
            >> frame_number      >> sep
            >> sensor_type       >> sep
            >> sensor_index      >> sep;
            std::getline(is,_in_filename);
            if(_in_filename.back()=='\r'){ _in_filename.pop_back(); }
            switch(sensor_type)
            {
                case RS_SF_SENSOR_ACCEL:
                case RS_SF_SENSOR_GYRO:
                    is = std::stringstream(_in_filename);
                    is >> imu.x >> imu.y >> imu.z;
                    break;
                default:
                    if(src!=nullptr){
                        image = *(_image_ptr = rs_sf_image_read(src->_folder_path + _in_filename, frame_number));
                    }
                    break;
            }
        };
        
        rs_sf_timestamp time_diff(const rs_sf_data_auto& ref) const { return timestamp_us - ref.timestamp_us; }
        bool operator==(const rs_sf_data_auto& ref) const { return
            sensor_type == ref.sensor_type  &&
            sensor_index== ref.sensor_index &&
            timestamp_us== ref.timestamp_us;
        }
        
        rs_sf_serial_number _dataset_number;
        std::string         _in_filename;
        rs_sf_image_ptr     _image_ptr;
    };
    
    friend struct rs_sf_data_auto;
    typedef std::shared_ptr<rs_sf_data_auto> rs_sf_data_ptr;
    
    struct rs_sf_virtual_stream_info : public rs_sf_stream_info
    {
        std::string     _virtual_stream_name;

        rs_sf_data_ptr _last_data, _first_data;
        int             _num_data = 0, _num_problem_frames = 0;
        
        rs_sf_timestamp _min_deviation = std::numeric_limits<rs_sf_timestamp>::max();
        rs_sf_timestamp _max_deviation = std::numeric_limits<rs_sf_timestamp>::min();
        rs_sf_timestamp _sum_intervals = 0;
        rs_sf_timestamp _expected_interval, _diff_tolerance;
        
        rs_sf_virtual_stream_info(const rs_sf_file_io& src, const rs_sf_stream_info& ref, const rs_sf_sensor_t& laser_option = RS_SF_SENSOR_LASER_ON, const bool half_fps = false)
        : rs_sf_stream_info(ref)
        {
            type = rs_sf_sensor_t(ref.type | laser_option);
            fps  = half_fps ? ref.fps/2.0f : ref.fps;
            _virtual_stream_name = src.get_stream_name(type, index);
            
            _expected_interval = std::chrono::microseconds(std::chrono::seconds(1)).count()/(double)(fps);
            _diff_tolerance = _expected_interval * 0.1f;
        }
        
        void init(rs_sf_data_ptr& first_data)
        {
            _last_data = _first_data = first_data; _num_data = 1;
        }
        
        void update(rs_sf_data_ptr&& new_data)
        {
            if(!_first_data){ init(new_data); }
            else{
                auto time_interval = new_data->time_diff(*_last_data);
                auto time_deviation = time_interval - _expected_interval;
                
                if(std::abs(time_deviation)>_diff_tolerance){
                    fprintf(stderr,"Incorrect sample of %s stream: %s\n",_virtual_stream_name.c_str(), new_data->_in_filename.c_str());
                    _num_problem_frames++;
                }
                
                _num_data++;
                _sum_intervals += time_interval;
                _min_deviation = std::min(std::abs(time_deviation),_min_deviation);
                _max_deviation = std::max(std::abs(time_deviation),_max_deviation);
                
                _last_data = new_data;
            }
        }
        
        int expected_num_frames() const { return (int)(1 + (!_first_data?0.0:_last_data->time_diff(*_first_data) / _expected_interval)); }
        int frame_drops() const  { return expected_num_frames() - _num_data; }
        
        std::string print() const {
            if(!_first_data){ return ""; }
            std::stringstream os; os << std::setprecision(4) << std::fixed <<
            " Stat : "         << _virtual_stream_name << " | " << _num_data <<
            " frames, ("       <<  frame_drops() << " drops, " << _num_problem_frames << " issues) |" <<
            " timestamp avg: " << 0.001 * _sum_intervals/_num_data << "ms |" <<
            " max deviation: " << 0.001 * _max_deviation           << "ms |" <<
            " min deviation: " << 0.001 * _min_deviation           << "ms |";
            return os.str();
        }
    };
    
    const rs_sf_d435i_file_stream* NO_IMAGE_FILE_READ = nullptr;
    
    std::ifstream                          _index_file;
    //std::ifstream                          _accel_file;
    //std::ifstream                          _gyro_file;
    
    bool                                   _init_check_data;
    int                                    _num_streams;
    int                                    _garbage_dataset_num;
    std::string                            _device_name;
    std::vector<std::string>               _device_info;
    std::vector<std::string>               _stream_name;
    std::vector<rs_sf_stream_info>         _streams;
    std::deque<rs_sf_data_ptr>             _data_buffer;
    std::vector<rs_sf_virtual_stream_info> _virtual_streams;
    
    rs_sf_d435i_file_stream(const std::string& path, bool request_check_data) : rs_sf_file_io(path), _init_check_data(request_check_data), _garbage_dataset_num(10)
    {
        read_calibrations();
        
        init_virtual_streams();
        if(_init_check_data){ check_data(); }
        
        _index_file.open(get_index_filepath(), std::ios_base::in);
        _index_file.seekg(0);
    }
    
    ~rs_sf_d435i_file_stream()
    {
        for(auto s : {&_index_file,/*&_accel_file,&_gyro_file*/}){
            if(s->is_open()){ s->close(); }
        }
    }
    
    int             num_streams() const { return _num_streams; }
    float           get_depth_unit() override { return 0.001f; }
    stream_info_vec get_stream_info() override { return _streams; }
    std::string     get_device_name() override { return _device_name; }
    string_vec      get_device_info() override { return _device_info; }
    bool            is_offline_stream() override { return true; }
    
    void read_calibrations()
    {
        auto device_json = read_json(get_calibration_filepath());
        auto stream_names = device_json["sensor_name"];
        
        _device_name = device_json["device_name"].asString();
        for(int s=0; s<(int)device_json["device_info"].size(); ++s)
            _device_info.emplace_back(device_json["device_info"][s].asString());
        
        _num_streams = stream_names.size();
        _streams.resize(_num_streams);
        _stream_name.resize(_num_streams);
        
        for(int s=0; s<_num_streams; ++s)
        {
            _stream_name[s] = device_json["sensor_name"][s].asString();
            auto info       = device_json["sensor"][_stream_name[s]];
            
            _streams[s].type       = (rs_sf_sensor_t)info["sensor_type"].asInt();
            _streams[s].index      = (rs_sf_uint16_t)info["sensor_index"].asInt();
            _streams[s].format     = info["data_format"].asInt();
            _streams[s].fps        = info["sensor_fps"].asFloat();
            _streams[s].intrinsics = read_intrinsics(_streams[s].type, info["intrinsics"]);
            _streams[s].extrinsics.resize(_num_streams);
            for(int ss=0; ss<_num_streams; ++ss){
                _streams[s].extrinsics[ss] = read_extrinsics(info["extrinsics"][stream_names[ss].asString()]);
            }
        }
    }
    
    bool is_interlaced_IR_projection() const
    {
        for(auto s : _device_info){
            if(s=="Emitter On And Off Enabled=1"){ return true; }
            if(s==rs2_option_to_string(RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED)){ return true; }
        }
        return false;
    }
    
    void init_virtual_streams()
    {
        bool is_interlaced_ir = is_interlaced_IR_projection();
        
        for(auto& s : _streams)
        {
            if(s.type==RS_SF_SENSOR_DEPTH||s.type==RS_SF_SENSOR_INFRARED)
            {
                _virtual_streams.emplace_back(*this, s, RS_SF_SENSOR_LASER_ON, is_interlaced_ir);
                _virtual_streams.emplace_back(*this, s, RS_SF_SENSOR_LASER_OFF, is_interlaced_ir);
            }
            else{
                _virtual_streams.emplace_back(*this, s, RS_SF_SENSOR_LASER_ON, false);
            }
        }
    }
    
    rs_sf_virtual_stream_info& virtual_stream_of(const rs_sf_data& data){
        for(auto& s : _virtual_streams){
            if(data.sensor_type == s.type &&
               data.sensor_index == s.index){
                return s;
            }
        }
        return _virtual_streams[0];
    }
    
    void check_data()
    {
        _index_file.open(get_index_filepath(), std::ios_base::in);
        _index_file.seekg(0);
        
        if(!_index_file.is_open()){ throw std::runtime_error("ERROR, unable to read " + _folder_path); }
        
        try{
            for(std::string line; !_index_file.eof(); ){
                std::getline(_index_file, line);
                if(line.empty()){ continue; }
                auto new_data = std::make_shared<rs_sf_data_auto>(line,NO_IMAGE_FILE_READ);
                if(new_data->_dataset_number> _garbage_dataset_num){
                    virtual_stream_of(*new_data).update(std::move(new_data));
                }
            }
        }catch(...){}

        std::cout << "--------------------------------------------------------" << std::endl;
        for(auto& s : _virtual_streams)
        {
            std::cout << s.print() << "\n";
        }
        std::cout << "--------------------------------------------------------" << std::endl;

        _index_file.close();
    }
    
    rs_sf_dataset_ptr wait_for_data(const std::chrono::milliseconds& wait_time_ms) override
    {
        rs_sf_dataset dst;
        if(!_index_file.is_open()) { return nullptr; }

        try{
            for(std::string line; !_index_file.eof(); )
            {
                std::getline(_index_file, line);
                if(line.empty()){ continue; }
                
                _data_buffer.emplace_back(std::make_shared<rs_sf_data_auto>(line,this));
                
                if(_data_buffer.front()->_dataset_number!=_data_buffer.back()->_dataset_number){
                    break; //end of dataset
                }
            }
        }catch(...){}
        
        try{
            if(!_data_buffer.empty())
            {
                dst.resize(num_streams());
                const auto dataset_number = _data_buffer.front()->_dataset_number;
                while(!_data_buffer.empty() && (dataset_number==_data_buffer.front()->_dataset_number))
                {
                    auto& data = _data_buffer.front();
                    switch(data->sensor_type)
                    {
                        case RS_SF_SENSOR_DEPTH:
                        case RS_SF_SENSOR_DEPTH_LASER_OFF:
                            dst[0].emplace_back(std::move(data));
                            break;
                        case RS_SF_SENSOR_INFRARED:
                        case RS_SF_SENSOR_INFRARED_LASER_OFF:
                            dst[data->sensor_index].emplace_back(std::move(data));
                            break;
                        case RS_SF_SENSOR_COLOR:
                            dst[3].emplace_back(std::move(data));
                            break;
                        case RS_SF_SENSOR_GYRO:
                            dst[4].emplace_back(std::move(data));
                            break;
                        case RS_SF_SENSOR_ACCEL:
                            dst[5].emplace_back(std::move(data));
                            break;
                        default:
                            break;
                    }
                    _data_buffer.pop_front();
                }
            }
        }catch(...){
            fprintf(stderr, "WARNING: error in reading data from %s \n", _folder_path.c_str());
        }
        return std::make_shared<rs_sf_dataset>(dst);
    }
};

std::unique_ptr<rs_sf_data_stream> rs_sf_create_camera_imu_stream(int w, int h, const rs_sf_stream_request& req){
    return std::make_unique<rs_sf_d435i_camera>(w, h, req); }

std::unique_ptr<rs_sf_data_writer> rs_sf_create_data_writer(rs_sf_data_stream* src, const std::string& path){
    return std::make_unique<rs_sf_d435i_writer>(src, path); }

std::unique_ptr<rs_sf_data_stream> rs_sf_create_camera_imu_stream(const std::string& folder_path, bool check_data){
    return std::make_unique<rs_sf_d435i_file_stream>(folder_path, check_data); }
