#pragma once

#ifndef rs_shapefit_h
#define rs_shapefit_h

struct rs_sf_intrinsics
{
    unsigned int img_w, img_h;
    float cam_px, cam_py, cam_fx, cam_fy;
    int reserved;
    float reserved2[5];
};

struct rs_sf_image
{
    unsigned char* data;
    int img_w, img_h, byte_per_pixel;
    int frame_id;

    int num_pixel() { return img_w * img_h; }
    int num_char() { return num_pixel() * byte_per_pixel; }
    
#if (defined(__OPENCV_ALL_HPP__) | defined(OPENCV_ALL_HPP))
    inline cv::Mat cvmat() const { return cv::Mat(img_h, img_w, CV_MAKETYPE(CV_8U, byte_per_pixel), data); }
#endif
};

enum rs_sf_status {
    RS_SF_INVALID_ARG = -2,
    RS_SF_FAILED = -1,
    RS_SF_SUCCESS = 0
};

struct rs_sf_planefit;

rs_sf_planefit* rs_sf_planefit_create(const rs_sf_intrinsics* camera);
void rs_sf_planefit_delete(rs_sf_planefit* obj);

rs_sf_status rs_sf_planefit_depth_image(rs_sf_planefit* obj, const rs_sf_image* image);

#endif;