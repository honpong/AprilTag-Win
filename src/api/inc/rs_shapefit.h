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

#define RS_SHAPEFIT_API_MAJOR_VERSION 1
#define RS_SHAPEFIT_API_MINOR_VERSION 0
#define RS_SHAPEFIT_API_PATCH_VERSION 0

#ifdef __cplusplus
extern "C"
{
#endif

    struct rs_sf_intrinsics
    {
        int           width;     /* width of the image in pixels */
        int           height;    /* height of the image in pixels */
        float         ppx;       /* horizontal coordinate of the principal point of the image, as a pixel offset from the left edge */
        float         ppy;       /* vertical coordinate of the principal point of the image, as a pixel offset from the top edge */
        float         fx;        /* focal length of the image plane, as a multiple of pixel width */
        float         fy;        /* focal length of the image plane, as a multiple of pixel height */
        int           model;     /* distortion model of the image */
        float         coeffs[5]; /* distortion coefficients */
    };

    struct rs_sf_image
    {
        unsigned char* data;
        int img_w, img_h, byte_per_pixel;
        unsigned long long frame_id;
        float* cam_pose;

        int num_pixel() const { return img_w * img_h; }
        int num_char() const { return num_pixel() * byte_per_pixel; }
    };

    struct rs_sf_box
    {
        float center[3];
        float axis[3][3];
    };

    struct rs_shapefit;

    enum rs_sf_status
    {
        RS_SF_INVALID_OBJ_HANDLE = -3,
        RS_SF_INVALID_ARG = -2,
        RS_SF_FAILED = -1,
        RS_SF_SUCCESS = 0,
        RS_SF_BUSY = 1,
        RS_SF_INDEX_OUT_OF_BOUND = 2,
        RS_SF_INDEX_INVALID = 3
    };

    enum rs_shapefit_option
    {
        RS_SHAPEFIT_PLANE = 0,
        RS_SHAPEFIT_BOX = 1,
    };

    enum rs_sf_fit_option
    {
        RS_SF_OPTION_MAX_PROCESS_DELAY = 0, /**< Max delay in ms in processing */
        RS_SF_OPTION_TRACKING = 1,          /**< 0:TRACK, 1:SINGLE FRAME       */
        RS_SF_OPTION_PLANE_NOISE = 2,       /**< 0:LOW, 1:MEDIUM, 2:HIGH       */
        RS_SF_OPTION_PLANE_RES = 3,         /**< 0:LOW,           2:HIGH       */
        RS_SF_OPTION_BOX_PLANE_RES = 4,     /**< 0:LOW,           2:HIGH       */
        RS_SF_OPTION_DRAW_PLANES = 5,       /**< 0:OVERLAY, 1:OVERWRITE        */
        RS_SF_OPTION_GET_PLANE_ID = 6,      /**< 0:ORIGINAL, 1:SCALED, 2:REMAP */
        RS_SF_OPTION_COUNT = 7,
    };

    RS_SHAPEFIT_DECL rs_shapefit* rs_shapefit_create(const rs_sf_intrinsics* camera, rs_shapefit_option option = RS_SHAPEFIT_PLANE);
    RS_SHAPEFIT_DECL void rs_shapefit_delete(rs_shapefit* obj);

    /// General Processing
    RS_SHAPEFIT_DECL rs_sf_status rs_shapefit_set_option(rs_shapefit* obj, rs_sf_fit_option option, double value);
    RS_SHAPEFIT_DECL rs_sf_status rs_shapefit_depth_image(rs_shapefit* obj, const rs_sf_image* image);

    /// Plane Fitting Functions
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_get_plane_ids(const rs_shapefit* obj, rs_sf_image* mono);
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_get_equation(const rs_shapefit* obj, int pid, float equation[4]);
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_draw_planes(const rs_shapefit* obj, rs_sf_image* rgb, const rs_sf_image* src = nullptr);

    /// Box Fitting Functions
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_boxfit_get_box(const rs_shapefit* obj, int box_id, rs_sf_box* dst);
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_boxfit_draw_boxes(const rs_shapefit* obj, rs_sf_image* rgb, const rs_sf_image* src = nullptr);
   
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
        img_h = ref->img_h; img_w = ref->img_w; byte_per_pixel = Channel; frame_id = ref->frame_id;
        data = (src = std::make_unique<unsigned char[]>(num_char())).get();
        if (ref->data && num_char()==ref->num_char()) memcpy(data, ref->data, num_char());
        set_pose(ref->cam_pose);
    }
    rs_sf_image_impl(int w, int h, int fid = -1, const void* v = nullptr, const float pose[12] = nullptr) {
        img_h = h; img_w = w; byte_per_pixel = Channel; frame_id = fid;
        data = (src = std::make_unique<unsigned char[]>(num_char())).get();
        if (v) memcpy(data, v, num_char());
        set_pose(pose);
    }
};

typedef std::unique_ptr<rs_sf_image_auto> rs_sf_image_ptr;
typedef rs_sf_image_impl<1> rs_sf_image_mono;
typedef rs_sf_image_impl<2> rs_sf_image_depth;
typedef rs_sf_image_impl<3> rs_sf_image_rgb;

template<rs_shapefit_option option = RS_SHAPEFIT_PLANE>
struct rs_shapefit_ptr : public std::unique_ptr<rs_shapefit, void(*)(rs_shapefit*)>
{
    rs_shapefit_ptr(const rs_sf_intrinsics* camera = nullptr, const rs_shapefit_option opt = option) :
        std::unique_ptr<rs_shapefit, void(*)(rs_shapefit*)>(
        (camera ? rs_shapefit_create(camera, opt) : nullptr), rs_shapefit_delete) {}
};

typedef rs_shapefit_ptr<> rs_sf_shapefit_ptr;
typedef rs_shapefit_ptr<RS_SHAPEFIT_PLANE> rs_sf_planefit_ptr;
typedef rs_shapefit_ptr<RS_SHAPEFIT_BOX> rs_sf_boxfit_ptr;

#endif //__cplusplus

#endif //rs_shapefit_h