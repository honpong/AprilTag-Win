#pragma once

#ifndef rs_shapefit_h
#define rs_shapefit_h

#if (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))
#ifdef RS_SHAPEFIT_EXPORTS
#define RS_SHAPEFIT_DECL __declspec(dllexport)
#else
#define RS_SHAPEFIT_DECL __declspec(dllimport)
#endif
#else
#define RS_SHAPEFIT_DECL
#endif

#ifdef __cplusplus
extern "C"
{
#endif

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
    };

    enum rs_sf_status {
        RS_SF_INVALID_ARG = -2,
        RS_SF_FAILED = -1,
        RS_SF_SUCCESS = 0
    };

    struct rs_sf_planefit;
    enum rs_sf_planefit_option
    {
        RS_SF_PLANEFIT_OPTION_TRACK = 0,
        RS_SF_PLANEFIT_OPTION_RESET = 1,
    };

    RS_SHAPEFIT_DECL rs_sf_planefit* rs_sf_planefit_create(const rs_sf_intrinsics* camera);
    RS_SHAPEFIT_DECL void rs_sf_planefit_delete(rs_sf_planefit* obj);

    RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_depth_image(rs_sf_planefit* obj, const rs_sf_image* image, rs_sf_planefit_option option = RS_SF_PLANEFIT_OPTION_TRACK);

#ifdef __cplusplus
}
#endif

#endif //rs_shapefit_h