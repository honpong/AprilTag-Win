#include "rs_sf_camera.hpp"

#if defined(__APPLE__) | defined(__OSX__) | defined(APPLE)
struct rs_sf_camera_stream : rs_sf_image_stream
{
    rs_sf_camera_stream(int w, int h) {}
    virtual rs_sf_image* get_images() { return nullptr; }
    virtual rs_sf_intrinsics* get_intrinsics() { return nullptr; }
    virtual float get_depth_unit() { return 0.001f; }
};
#else

#include <librealsense2/rsutil.h>
#include <iostream>
struct rs_sf_camera_stream : rs_sf_image_stream
{
    rs_sf_camera_stream(int w, int h) : image{}, curr_depth(w*h * 2), prev_depth(w*h * 2)
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
            intrinsics = pprofile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>().get_intrinsics();

            //device.set_option(RS_OPTION_EMITTER_ENABLED, 1);
            //device.set_option(RS_OPTION_ENABLE_AUTO_EXPOSURE, 1);

      
        }
		catch (const rs2::error & e) { print(e); }
	}

    virtual rs_sf_intrinsics* get_intrinsics() override { return (rs_sf_intrinsics*)&intrinsics; }

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

                if (stream_type == RS_SF_STREAM_DEPTH) {
                  //  std::swap(curr_depth, prev_depth);
                  //  rs_sf_memcpy(curr_depth.data(), img.data, img.num_char());
                  //  img.data = prev_depth.data();
                }
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
    std::vector<unsigned char> curr_depth, prev_depth;
    float depth_unit = 0.001f;

private:
    rs2::context ctx;
    rs2::device device;
    rs2::pipeline pipe;
    rs2::config config;
    rs2_intrinsics intrinsics;


	void print(const rs2::error& e) {
		std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
	}
};

#endif // realsense defined only

std::unique_ptr<rs_sf_image_stream> rs_sf_create_camera_stream(int w, int h) {
    return std::make_unique<rs_sf_camera_stream>(w, h);
}
