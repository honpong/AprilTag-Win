#include "rs_box_sdk.hpp"


int main(int argc, char* argv[])
{
    rs2::context ctx;
    rs2::pipeline pipe;

    auto list = ctx.query_devices();
    if (list.size() <= 0) { std::runtime_error("No device detected."); return EXIT_FAILURE; }

    // Assume the first device is our depth camera
    auto dev = list[0];
    std::string header = std::string(dev.get_info(RS2_CAMERA_INFO_NAME)) + " Box Scan Example ";

    // Declare box detection
    rs2::box_measure boxscan(dev);

    // Setup camera pipeline
    auto config = boxscan.get_camera_config();
    auto pipeline = pipe.start(config);

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    // Start application
    for (bool exit = false; !exit; )
    {
        rs2::frameset frameset; //frame set container

        while (!frameset.first_or_default(RS2_STREAM_DEPTH)
            || (!frameset.first_or_default(RS2_STREAM_COLOR) && !frameset.first_or_default(RS2_STREAM_INFRARED)))
        {
            frameset = pipe.wait_for_frames(); //wait until a pair of frames.
        }

        if (auto box_frame = boxscan.process(frameset)) //process new frame pair
        {
            if (auto box = boxscan.get_boxes())
            {
                printf((box[0].str() + "\n").c_str());
            }
        }
    }
    return EXIT_SUCCESS;
}