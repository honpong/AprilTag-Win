#include "rs_sf_camera.hpp"

#if 0
//defined(__APPLE__) | defined(__OSX__) | defined(APPLE)
#else

#include <librealsense2/rsutil.h>
#include <iostream>

struct rs_sf_camera_stream : rs_sf_image_stream
{
    rs_sf_camera_stream(int w, int h) : image{}, intrinsics{}, extrinsics{}
    {
        try {
            auto list = ctx.query_devices();
            if (list.size() == 0) throw std::runtime_error("No device detected.");

            device = list[0];
            std::string camera_name = device.get_info(RS2_CAMERA_INFO_NAME);
            if (camera_name == "Intel RealSense SR300") { depth_unit = 0.000125f; }

            config.enable_stream(RS_SF_STREAM_DEPTH, 0, w, h, RS_SF_FORMAT_Z16, 30);
            config.enable_stream(RS_SF_STREAM_COLOR, 0, w, h, RS_SF_FORMAT_RGB8, 30);

            auto pprofile = pipe.start(config);
            auto depth_profile = pprofile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
            auto color_profile = pprofile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
            
            intrinsics[RS_SF_STREAM_DEPTH] = depth_profile.get_intrinsics();
            intrinsics[RS_SF_STREAM_COLOR] = color_profile.get_intrinsics();
            extrinsics[RS_SF_STREAM_DEPTH][RS_SF_STREAM_COLOR] = depth_profile.get_extrinsics_to(color_profile);
            extrinsics[RS_SF_STREAM_COLOR][RS_SF_STREAM_DEPTH] = color_profile.get_extrinsics_to(depth_profile);
            buffer(extrinsics[RS_SF_STREAM_COLOR][RS_SF_STREAM_DEPTH]);
            
            // Go over the device's sensors
            for (rs2::sensor& sensor : device.query_sensors()){
                // Check if the sensor if a depth sensor
                if (auto dpt = sensor.as<rs2::depth_sensor>()){
                    depth_unit = std::max(0.001f, dpt.get_depth_scale());
                }
            }
            
            //device.set_option(RS_OPTION_EMITTER_ENABLED, 1);
            //device.set_option(RS_OPTION_ENABLE_AUTO_EXPOSURE, 1);

        }
        catch (const rs2::error & e) { print(e); }
    }

    virtual rs_sf_intrinsics* get_intrinsics(int stream) override { return (rs_sf_intrinsics*)&intrinsics[stream]; }
    virtual rs_sf_extrinsics* get_extrinsics(int from, int to) override { return (rs_sf_extrinsics*)&extrinsics[from][to]; }

    virtual rs_sf_image* get_images() override try
    {
        for (auto frames = pipe.wait_for_frames();; frames = pipe.wait_for_frames())
        {
            for (auto&& f : frames) {
                auto stream_type = f.get_profile().stream_type();
                auto& img = image[stream_type];
                img = {};
                img.data = (unsigned char*)f.get_data();
                img.img_h = f.as<rs2::video_frame>().get_height();
                img.img_w = f.as<rs2::video_frame>().get_width();
                img.frame_id = f.get_frame_number();
                img.byte_per_pixel = stream_to_byte_per_pixel[stream_type];

                if (stream_type == RS_SF_STREAM_COLOR){ img.cam_pose = color_pose_buf; }
            }
            if (frames.first_or_default(RS2_STREAM_DEPTH) && frames.first_or_default(RS2_STREAM_COLOR)) return image;
            if (frames.size() == 0) return nullptr;
        }
    }
    catch (const rs2::error & e) {
        print(e);
        return nullptr; 
    }

    virtual float get_depth_unit() override {
        return depth_unit;
    }

protected:
    rs_sf_image image[RS_SF_STREAM_COUNT];
    int stream_to_byte_per_pixel[RS_SF_STREAM_COUNT] = { 0,2,3,1,1 };
    float depth_unit = 0.001f;

private:
    rs2::context ctx;
    rs2::device device;
    rs2::pipeline pipe;
    rs2::config config;
    rs2_intrinsics intrinsics[RS_SF_STREAM_COUNT];
    rs2_extrinsics extrinsics[RS_SF_STREAM_COUNT][RS_SF_STREAM_COUNT];
    float color_pose_buf[12];
    void buffer(const rs2_extrinsics& extr){
        const float* src = (const float*)&extr;
        for(int p : {0,1,2,4,5,6,8,9,10,3,7,11}){ color_pose_buf[p] = *src++; }
    }
    
    void print(const rs2::error& e) {
        std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    }
};

#endif // realsense defined only

std::unique_ptr<rs_sf_image_stream> rs_sf_create_camera_stream(int w, int h) {
    return std::make_unique<rs_sf_camera_stream>(w, h);
}
