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
        float* cam_pose;

        int num_pixel() const { return img_w * img_h; }
        int num_char() const { return num_pixel() * byte_per_pixel; }
    };

    struct rs_sf_planefit;

    enum rs_sf_status {
        RS_SF_INVALID_ARG = -2,
        RS_SF_FAILED = -1,
        RS_SF_SUCCESS = 0
    };

    enum rs_sf_planefit_option
    {
        RS_SF_PLANEFIT_OPTION_TRACK = 0,
        RS_SF_PLANEFIT_OPTION_RESET = 1,
    };

    enum rs_sf_planefit_draw_opion
    {
        RS_SF_PLANEFIT_DRAW_ORIGINAL = 0,
        RS_SF_PLANEFIT_DRAW_SCALED = 1
    };

    RS_SHAPEFIT_DECL rs_sf_planefit* rs_sf_planefit_create(const rs_sf_intrinsics* camera);
    RS_SHAPEFIT_DECL void rs_sf_planefit_delete(rs_sf_planefit* obj);

    RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_depth_image(rs_sf_planefit* obj, const rs_sf_image* image, rs_sf_planefit_option option = RS_SF_PLANEFIT_OPTION_TRACK);
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_draw_planes(const rs_sf_planefit* obj, rs_sf_image* rgb, const rs_sf_image* src = nullptr);
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_draw_plane_ids(const rs_sf_planefit* obj, rs_sf_image* mono, rs_sf_planefit_draw_opion option = RS_SF_PLANEFIT_DRAW_ORIGINAL );

#ifdef __cplusplus
}

#include <memory>

struct rs_sf_image_auto : public rs_sf_image
{
    virtual ~rs_sf_image_auto() {}
    std::unique_ptr<unsigned char[]> src;
    float src_pose[12];
    void set_pose(const float p[12]) { 
        if (p) memcpy(cam_pose = src_pose, p, sizeof(float) * 12); 
        else cam_pose = nullptr; 
    }
};

template<int Channel>
struct rs_sf_image_impl : public rs_sf_image_auto
{
    rs_sf_image_impl(const rs_sf_image* ref) 
    {
        img_h = ref->img_h; img_w = ref->img_w; byte_per_pixel = Channel;
        data = (src = std::make_unique<unsigned char[]>(num_char())).get();
        if (ref->data && num_char()==ref->num_char()) memcpy(data, ref->data, num_char());
        set_pose(ref->cam_pose);
    }
    rs_sf_image_impl(int w, int h, const void* v = nullptr, const float pose[12] = nullptr) {
        img_h = h; img_w = w; byte_per_pixel = Channel;
        data = (src = std::make_unique<unsigned char[]>(num_char())).get();
        if (v) memcpy(data, v, num_char());
        set_pose(pose);
    }
};

typedef rs_sf_image_impl<1> rs_sf_image_mono;
typedef rs_sf_image_impl<2> rs_sf_image_depth;
typedef rs_sf_image_impl<3> rs_sf_image_rgb;

struct rs_sf_planefit_ptr : public std::unique_ptr<rs_sf_planefit, void(*)(rs_sf_planefit*)>
{
    rs_sf_planefit_ptr(const rs_sf_intrinsics* camera = nullptr) :
        std::unique_ptr<rs_sf_planefit, void(*)(rs_sf_planefit*)>(
        (camera ? rs_sf_planefit_create(camera) : nullptr), rs_sf_planefit_delete) {}
};

#endif

#endif //rs_shapefit_h