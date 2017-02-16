#pragma once
#include <memory>

struct rs_sf_intrinsics
{
    unsigned int img_w, img_h;
    float cam_px, cam_py, cam_fx, cam_fy;
    int reserved;
    float reserved2[5];
};

struct rs_sf_image
{
    std::unique_ptr<unsigned char[]> data;
    int img_w, img_h, byte_per_pixel;
    int frame_id;

    int num_pixel() { return img_w * img_h; }
    int num_char() { return num_pixel() * byte_per_pixel; }
    void set_data_zeros() { memset(data.get(), 0, num_char()); }

#if (defined(__OPENCV_ALL_HPP__) | defined(OPENCV_ALL_HPP))
    inline cv::Mat cvmat() const { return cv::Mat(img_h, img_w, CV_MAKETYPE(CV_8U, byte_per_pixel), data.get()); }
#endif
};

typedef std::unique_ptr<rs_sf_image> rs_sf_image_ptr;
struct rs_sf_planefit;