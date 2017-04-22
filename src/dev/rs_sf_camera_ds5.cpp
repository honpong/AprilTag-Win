#include "rs_sf_camera.hpp"

#if defined(__APPLE__) | defined(__OSX__) | defined(APPLE)
struct rs_sf_camera_stream : rs_sf_image_stream
{
    rs_sf_camera_stream(int w, int h) {}
    virtual rs_sf_image* get_images() { return nullptr; }
    virtual rs_sf_intrinsics* get_intrinsics() { return nullptr; }
};
#else

#include <../../thirdparty/LibRealSense/librealsense/include/librealsense/rsutil.hpp>
struct rs_sf_camera_stream : rs_sf_image_stream
{
    rs_sf_camera_stream(int w, int h) : image{}, curr_depth(w*h * 2), prev_depth(w*h * 2)
	{
		try {
			auto list = ctx.query_devices();
			if (list.size() == 0) throw std::runtime_error("No device detected.");

			device = list[0];
			config.enable_stream(RS_SF_STREAM_DEPTH, w, h, 30, RS_SF_FORMAT_Z16);
			config.enable_stream(RS_SF_STREAM_INFRARED, w, h, 30, RS_SF_FORMAT_Y8);

			stream = config.open(device);
			intrinsics = stream.get_intrinsics(RS_SF_STREAM_DEPTH);

			stream.start(syncer);
			device.set_option(RS_OPTION_EMITTER_ENABLED, 1);
			//device.set_option(RS_OPTION_ENABLE_AUTO_EXPOSURE, 1);
		}
		catch (const rs::error & e) { print(e); }
	}

    virtual rs_sf_intrinsics* get_intrinsics() override { return (rs_sf_intrinsics*)&intrinsics; }

    virtual rs_sf_image* get_images() override try
    {
        for (auto frames = syncer.wait_for_frames();; frames = syncer.wait_for_frames())
        {
            for (auto&& f : frames) {
                auto& img = image[f.get_stream_type()];
                img = {};
                img.data = (unsigned char*)f.get_data();
                img.img_h = f.get_height();
                img.img_w = f.get_width();
                img.frame_id = f.get_frame_number();
                img.byte_per_pixel = stream_to_byte_per_pixel[f.get_stream_type()];

                if (f.get_stream_type() == RS_SF_STREAM_DEPTH) {
                    std::swap(curr_depth, prev_depth);
                    rs_sf_memcpy(curr_depth.data(), img.data, img.num_char());
                    img.data = prev_depth.data();
                }
            }
            if (frames.size() > 1) return image;
            if (frames.size() == 0) return nullptr;
        }
    }
	catch (const rs::error & e) { print(e);	return nullptr; }
	
protected:
    rs_sf_image image[RS_SF_STREAM_COUNT];
    int stream_to_byte_per_pixel[RS_SF_STREAM_COUNT] = { 0,2,3,1,1 };
    std::vector<unsigned char> curr_depth, prev_depth;

private:
    rs::context ctx;
    rs::device device;
    rs::util::syncer syncer;
    rs::util::config config;
    rs_intrinsics intrinsics;
    rs::util::Config<>::multistream stream;

	void print(const rs::error& e) {
		std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
	}
};

#endif // realsense defined only

std::unique_ptr<rs_sf_image_stream> rs_sf_create_camera_stream(int w, int h) {
    return std::make_unique<rs_sf_camera_stream>(w, h);
}
