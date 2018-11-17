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

struct rs_sf_d435i_writer;
struct rs_sf_d435i_camera : public rs_sf_data_stream, rs_sf_device_manager
{
    rs_sf_d435i_camera(int w, int h, int laser)
    {
        if(search_device_name("Intel RealSense D435I")){ printf("Intel RealSense D435I Camera Found\n"); }
        else if(search_device_name("Intel RealSense D435" )){ printf("Intel RealSense D435  Camera Found\n");}
        if(!_device){ throw std::runtime_error("No Intel RealSense D435/D435I Found"); }

        add_stream_request({RS2_STREAM_DEPTH,   -1,  30, RS2_FORMAT_Z16,            w,  h});
        add_stream_request({RS2_STREAM_INFRARED, 1,  30, RS2_FORMAT_Y8 ,            w,  h});
        add_stream_request({RS2_STREAM_INFRARED, 2,  30, RS2_FORMAT_Y8 ,            w,  h});
        add_stream_request({RS2_STREAM_COLOR,   -1,  30, RS2_FORMAT_RGB8,           w,  h});
        add_stream_request({RS2_STREAM_ACCEL,   -1, 125, RS2_FORMAT_MOTION_XYZ32F, -1, -1});
        add_stream_request({RS2_STREAM_GYRO,    -1, 200, RS2_FORMAT_MOTION_XYZ32F, -1, -1});
        
        find_stream_profiles();
        print_requested_streams();
        
        extract_stream_calibrations();
        print_calibrations();
        
        open(laser);
        start();
    }
    
    virtual ~rs_sf_d435i_camera()
    {
        for(auto s : {0,3,4}){
            if(_streams[s].sensor){
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
    std::string         get_device_info() override {
        switch(_laser_option)
        {
            case -1: return rs2_option_to_string(RS2_OPTION_EMITTER_ENABLED)            + std::string("=UNKNOWN");
            case  0: return rs2_option_to_string(RS2_OPTION_EMITTER_ENABLED)            + std::string("=0");
            case  1: return rs2_option_to_string(RS2_OPTION_EMITTER_ENABLED)            + std::string("=1");
            case  2: return rs2_option_to_string(RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED) + std::string("=1");
        }
        return "";
    }
    
    int _laser_option = -1;
    void open(int laser_option)
    {
        // open the depth camera stream
        _streams[0].sensor.open({_streams[0].profile,_streams[1].profile,_streams[2].profile});
        try{
            switch(laser_option)
            {
                case 0: _streams[0].sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 0);            break;
                case 1: _streams[0].sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1);            break;
                case 2: _streams[0].sensor.set_option(RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED, 1); break;
            }
        }catch(...){ printf("WARNING: error setting laser option %d!\n", laser_option); }

        try{
            _laser_option = _streams[0].sensor.get_option(RS2_OPTION_EMITTER_ENABLED);
            if(laser_option==2 && _streams[0].sensor.get_option(RS2_OPTION_EMITTER_ON_AND_OFF_ENABLED)){
                _laser_option=2;
            }
        }catch(...){ printf("WARNING: error getting laser option %d!\n",_laser_option); }
        
        // open the color camera stream
        _streams[3].sensor.open(_streams[3].profile);
        
        // open the motion sensor stream
        if(_streams[4].profile && _streams[5].profile){
            _streams[4].sensor.open({_streams[4].profile,_streams[5].profile});
        }
    }
    
    struct rs_sf_data_auto : public rs_sf_data_buf
    {
        virtual ~rs_sf_data_auto() {}
        
        rs_sf_data_auto(rs2::frame f, rs_sf_stream_select& stream, const rs_sf_serial_number& new_serial_number) : _f(f)
        {
            static const int _stream_to_byte_per_pixel[RS_SF_STREAM_COUNT] = { 0,2,3,1,1 };
            sensor_index  = _f.get_profile().stream_index();
            sensor_type   = (rs_sf_sensor_t)(_f.get_profile().stream_type());
            //timestamp_us  = _f.supports_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL) ? _f.get_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL) : _f.get_timestamp();
            timestamp_us  = _f.get_timestamp();
            serial_number = new_serial_number;
            frame_number  = _f.get_frame_number();
            
            if (_f.is<rs2::video_frame>()){
                image.byte_per_pixel = _stream_to_byte_per_pixel[stream.type];
                image.data           = (unsigned char*)_f.get_data();
                image.frame_id       = frame_number;
                image.img_h          = _f.as<rs2::video_frame>().get_height();
                image.img_w          = _f.as<rs2::video_frame>().get_width();
                image.intrinsics     = (rs_sf_intrinsics*)&stream.cam_intrinsics;
                
                if(_f.supports_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE) &&
                   _f.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE)==0){
                    sensor_type = (rs_sf_sensor_t)(sensor_type|RS_SF_SENSOR_LASER_OFF);
                }
            }
            else if(_f.is<rs2::motion_frame>()){
                rs_sf_memcpy(vec, _f.get_data(), sizeof(vec));
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
                    std::lock_guard<std::mutex> lk(_pipeline_mutex[s]);
                    _pipeline_buffer[s].emplace_back(std::make_shared<rs_sf_data_auto>(f,_streams[s],generate_serial_number()));
                }
            }
        });
        
        // color sensor
        _pipeline_streaming[3] = true;
        _streams[3].sensor.start([this](rs2::frame f){
            if(!_pipeline_streaming[3]){ return; }
            std::lock_guard<std::mutex> lk(_pipeline_mutex[3]);
            _pipeline_buffer[3].emplace_back(std::make_shared<rs_sf_data_auto>(f,_streams[3],generate_serial_number()));
        });
        
        // imu motion sensor
        _pipeline_streaming[4] = _pipeline_streaming[5] = true;
        if(_streams[4].profile && _streams[5].profile){
            if(!_pipeline_streaming[4]){ return; }
            _streams[4].sensor.start([this](rs2::frame f){
                for(int s : {4,5}){
                    if(f.get_profile().stream_index()==_streams[s].index){
                        std::lock_guard<std::mutex> lk(_pipeline_mutex[s]);
                        _pipeline_buffer[s].emplace_back(std::make_shared<rs_sf_data_auto>(f,_streams[s],generate_serial_number()));
                    }
                }
            });
        }
        
        return false;
    }
    
    rs_sf_dataset wait_for_data(const std::chrono::milliseconds& wait_time_us) override
    {
        static const auto time_inc = std::chrono::microseconds(10);
        rs_sf_dataset dst; dst.resize(num_streams());
        
        for(auto wait_time = std::chrono::microseconds(0); /*wait_time < wait_time_us*/; wait_time+=time_inc)
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
        return dst;
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
    std::ofstream accel_file;
    std::ofstream gyro_file;
    
    rs_sf_data_stream* _src;
    
    rs_sf_d435i_writer(rs_sf_data_stream* src, const std::string& path) : rs_sf_file_io(path), _src(src)
    {
        if(dynamic_cast<rs_sf_file_io*>(src)&&dynamic_cast<rs_sf_file_io*>(src)->_folder_path==path){
            _folder_path.append("clone").push_back(PATH_SEPARATER);
            printf("WARNING: cannot overwrite source directory! Write into %s\n",_folder_path.c_str());
        }
        
        RS_SF_CLEAR_DIRECTORY(_folder_path)
        
        index_file.open(get_index_filepath().c_str(), std::ios_base::out | std::ios_base::trunc);
        
        write_calibrations();
    }
    
    ~rs_sf_d435i_writer()
    {
        for(auto file : {&index_file,&accel_file,&gyro_file})
        {
            if(file->is_open()){file->close();}
        }
    }
    
    std::string generate_data_filename(const rs_sf_data& item)
    {
        switch(item.sensor_type){
            case RS_SF_SENSOR_ACCEL:
            case RS_SF_SENSOR_GYRO:
                return std::to_string(item.vec[0]) + " " + std::to_string(item.vec[1]) + " " + std::to_string(item.vec[2]);
            default:
                return get_stream_name(item.sensor_type, item.sensor_index)+ std::to_string(_next_frame_num[item.sensor_type][item.sensor_index]++) + _file_format[item.sensor_type];
        }
    }
    
    bool write_header(const std::string& out_filename, const rs_sf_data& data, const rs_sf_serial_number& dataset_number)
    {
        static const std::string sep = ",";
        
        if(index_file.is_open()){
            std::stringstream os; os
            << dataset_number     << sep << std::setprecision(17)
            << data.timestamp_us  << sep
            << data.serial_number << sep
            << data.frame_number  << sep
            << data.sensor_type   << sep
            << data.sensor_index  << sep
            << out_filename       << std::endl;
            index_file << os.str();
        }else{
            printf("WARNING: fail to write to dataset header!\n");
            return false;
        }
        return true;
    }
    
    bool write_calibrations() const
    {
        Json::Value json_root;
        json_root["calibration_file_version"] = RS_SF_CALIBRATION_FILE_VERSION;
        json_root["device_name"] = _src->get_device_name();
        json_root["device_info"] = _src->get_device_info();

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
            printf("fail to write calibration to %s \n",_folder_path.c_str());
            return false;
        }
        return true;
    }
    
    std::atomic<rs_sf_serial_number> _dataset_count = {0};
    bool write(rs_sf_dataset& data) override
    {
        auto my_data_set_number = _dataset_count++;
        
        std::deque<const rs_sf_data*> items;
        for(const auto& stream : data){
            for(const auto& item : stream){
                items.emplace_back(item.get());
            }
        }
        std::sort(items.begin(), items.end(), [](const rs_sf_data*& a, const rs_sf_data*& b){ return a->timestamp_us < b->timestamp_us; });
        for(auto& item : items){
            auto filename = generate_data_filename(*item);
            if(!write_header(filename, *item, my_data_set_number)){ return false; }
            if((item->sensor_type&0xf)<RS_SF_SENSOR_ACCEL){
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
        
        rs_sf_data_auto(const std::string& index_line, rs_sf_d435i_file_stream* src)
        {
            char sep;
            std::stringstream is(index_line); is
            >> _dataset_number   >> sep
            >> timestamp_us      >> sep
            >> serial_number     >> sep
            >> frame_number      >> sep
            >> sensor_type       >> sep
            >> sensor_index      >> sep;
            std::getline(is,_in_filename);
            switch(sensor_type)
            {
                case RS_SF_SENSOR_ACCEL:
                case RS_SF_SENSOR_GYRO:
                    is = std::stringstream(_in_filename);
                    is >> vec[0] >> vec[1] >> vec[2];
                    break;
                default:
                    image = *(_image_ptr = rs_sf_image_read(src->_folder_path + _in_filename, frame_number));
                    break;
            }
        };
        
        rs_sf_serial_number _dataset_number;
        std::string         _in_filename;
        rs_sf_image_ptr     _image_ptr;
    };
    
    friend struct rs_sf_data_auto;
    
    std::ifstream                                _index_file;
    std::ifstream                                _accel_file;
    std::ifstream                                _gyro_file;
    
    int                                          _num_streams;
    std::string                                  _device_name;
    std::string                                  _device_info;
    std::vector<std::string>                     _stream_name;
    std::vector<rs_sf_stream_info>               _streams;
    std::deque<std::shared_ptr<rs_sf_data_auto>> _data_buffer;
    
    rs_sf_d435i_file_stream(const std::string& path) : rs_sf_file_io(path)
    {
        read_calibrations();
        
        _index_file.open(get_index_filepath(), std::ios_base::in);
        _index_file.seekg(0);
    }
    
    ~rs_sf_d435i_file_stream()
    {
        for(auto s : {&_index_file,&_accel_file,&_gyro_file}){
            if(s->is_open()){ s->close(); }
        }
    }
    
    int             num_streams() const { return _num_streams; }
    float           get_depth_unit() override { return 0.001f; }
    stream_info_vec get_stream_info() override { return _streams; }
    std::string     get_device_name() override { return _device_name; }
    std::string     get_device_info() override { return _device_info; }
    
    void read_calibrations()
    {
        auto device_json = read_json(get_calibration_filepath());
        auto stream_names = device_json["sensor_name"];
        
        _device_name = device_json["device_name"].asString();
        _device_info = device_json["device_info"].asString();
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
            _streams[s].fps        = info["fps"].asInt();
            _streams[s].intrinsics = read_intrinsics(_streams[s].type, info["intrinsics"]);
            _streams[s].extrinsics.resize(_num_streams);
            for(int ss=0; ss<_num_streams; ++ss){
                _streams[s].extrinsics[ss] = read_extrinsics(info["extrinsics"][stream_names[ss].asString()]);
            }
        }
    }
    
    rs_sf_dataset wait_for_data(const std::chrono::milliseconds& wait_time_us) override
    {
        rs_sf_dataset dst;
        if(!_index_file.is_open()) { return dst; }

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
                        case RS_SF_SENSOR_ACCEL:
                            dst[4].emplace_back(std::move(data));
                            break;
                        case RS_SF_SENSOR_GYRO:
                            dst[5].emplace_back(std::move(data));
                            break;
                        default:
                            break;
                    }
                    _data_buffer.pop_front();
                }
            }
        }catch(...){
            printf("error in reading data from %s \n", _folder_path.c_str());
        }
        return dst;
    }
};

std::unique_ptr<rs_sf_data_stream> rs_sf_create_camera_imu_stream(int w, int h, int laser){
    return std::make_unique<rs_sf_d435i_camera>(w, h, laser); }

std::unique_ptr<rs_sf_data_writer> rs_sf_create_data_writer(rs_sf_data_stream* src, const std::string& path){
    return std::make_unique<rs_sf_d435i_writer>(src, path); }

std::unique_ptr<rs_sf_data_stream> rs_sf_create_camera_imu_stream(const std::string& folder_path){
    return std::make_unique<rs_sf_d435i_file_stream>(folder_path); }
