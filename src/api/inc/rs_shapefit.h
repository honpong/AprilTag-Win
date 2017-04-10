/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once

#ifndef rs_shapefit_h
#define rs_shapefit_h

#if (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))
#ifdef RS_SHAPEFIT_EXPORTS
#define RS_SHAPEFIT_DECL __declspec(dllexport)
#else
#define RS_SHAPEFIT_DECL
#endif
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
        int img_w;                    /**< image width                     */
        int img_h;                    /**< image height                    */
        int byte_per_pixel;           /**< number of byte per pixel        */
        unsigned long long frame_id;  /**< frame identifier                */
        unsigned char* data;          /**< image data                      */
        float* cam_pose;              /**< camera pose of this image       */
        rs_sf_intrinsics* intrinsics; /**< camera intrinsics of this image */

        inline int num_pixel() const { return img_w * img_h; }
        inline int num_char() const { return num_pixel() * byte_per_pixel; }
    };

    struct rs_sf_plane
    {
        int plane_id;      /**< plane identifier [1-254], 0 end of plane array */ 
        int contour_id;    /**< contour idenfifier, starts from 0              */
        int num_points;    /**< number of contour points of this contour       */
        float(*pos)[3];    /**< array of 3d points of this contour             */
        float equation[4]; /**< plane equation                                 */
    };

    struct rs_sf_box
    {
        float center[3];   /**< box center in 3d world coordinate           */
        float axis[3][3];  /**< box axis and lengths in 3d world coordinate */
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
    };

    enum rs_shapefit_capability
    {
        RS_SHAPEFIT_PLANE = 0,
        RS_SHAPEFIT_BOX = 1,
    };

    enum rs_sf_fit_option
    {
        RS_SF_OPTION_ASYNC_WAIT = 0,        /**< Wait ms in async mode, set -1 for sync process */
        RS_SF_OPTION_TRACKING = 1,          /**< 0:TRACK, 1:SINGLE FRAME       */
        RS_SF_OPTION_PLANE_NOISE = 2,       /**< 0:LOW, 1:MEDIUM, 2:HIGH       */
        RS_SF_OPTION_PLANE_RES = 3,         /**< 0:LOW, 1:MEDIUM, 2:HIGH       */
        RS_SF_OPTION_BOX_PLANE_RES = 4,     /**< 0:LOW,           2:HIGH       */
        RS_SF_OPTION_GET_PLANE_ID = 5,      /**< 0:ORIGINAL, 1:SCALED, 2:REMAP */
        RS_SF_OPTION_COUNT = 6,
        RS_SF_MAX_PLANE_COUNT = 256,        /**< required buffers by rs_sf_planefit_get_planes() */
    };

    /// Object manipulation -------------------------------------------------------------------------------------
    /** Create a shapefit instance.
    * @param[in] camera     depth camera intrinsics.
    * @param[in] capability capability of the shapefit instance.
    * @return shapefit instance.  */
    RS_SHAPEFIT_DECL rs_shapefit* rs_shapefit_create(const rs_sf_intrinsics* camera, rs_shapefit_capability capability = RS_SHAPEFIT_PLANE);
   
    /** Delete a shapefit instance.
    * @param[in] obj shapefit instance to be deleted */
    RS_SHAPEFIT_DECL void rs_shapefit_delete(rs_shapefit* obj);

    /// General Processing --------------------------------------------------------------------------------------   
    /** Process a depth image by a shapefit object. 
    * @param[in] obj      shapefit object.
    * @param[in] image    input depth image. 
    * @return RS_SF_SUCCESS if no error. */
    RS_SHAPEFIT_DECL rs_sf_status rs_shapefit_depth_image(rs_shapefit* obj, const rs_sf_image* depth);

    /** Set shapefit option
    * @param[in] obj      shapefit object.
    * @param[in] option   shapefit option, @see rs_sf_fit_option.
    * @param[in] value    option value. *
    * @return RS_SF_SUCCESS if no error. */
    RS_SHAPEFIT_DECL rs_sf_status rs_shapefit_set_option(rs_shapefit* obj, rs_sf_fit_option option, double value);

    /** Set shapefit object to process synchronously. 
    * @param[in] obj      shapefit object.
    * @param[in] flag     1: sync process, 0:async process waiting time in milliseconds 
    * @return RS_SF_SUCCESS if no error. */
    inline rs_sf_status rs_shapefit_set_synchronous_process(rs_shapefit* obj, double flag = 1) {
        return rs_shapefit_set_option(obj, RS_SF_OPTION_ASYNC_WAIT, -flag);
    }

    /// Plane Fitting Functions --------------------------------------------------------------------------------
    /** Get plane id map. Plane id ranges from 1-254. Details of a plane id can be obtained by 
    * rs_sf_plane_get_planes() and matching the plane_id of the rs_sf_plane array. 
    * @param[in]     obj      shapefit object with plane fitting capability.
    * @param[in|out] id_map   single channel unsigned char image buffer. 
    * @return RS_SF_SUCCESS if no error. */
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_get_plane_ids(const rs_shapefit* obj, rs_sf_image* id_map);
    
    /** Get an array of plane details. 
    * @param[in]     obj          shapefit object with plane fitting capability. 
    * @param[in|out] rgb          optional rgb image buffer for drawing plane contours in white.
    * @param[in|out] planes       optional array of rs_sf_plane buffers.
    * @param[in]     point_buffer float buffer for contour 3d positions will assign to the planes[.].pos pointers.
    *                             length of this buffer should be at least 3x number of pixels in input depth image. 
    * @return RS_SF_SUCCESS if no error. */
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_get_planes(const rs_shapefit* obj, rs_sf_image* rgb = nullptr, rs_sf_plane planes[RS_SF_MAX_PLANE_COUNT] = nullptr, float* point_buffer = nullptr);

    /** Draw plane pixels by its plane_id according to a color table. 
    * @param[in]     obj          shapefit object with plane fitting capability.
    * @param[in|out] rgb          rgb image buffer for drawing color plane pixels.
    * @param[in]     bkg          optional background image for the output rgb image.
    * @return RS_SF_SUCCESS if no error. */
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_draw_planes(const rs_shapefit* obj, rs_sf_image* rgb, const rs_sf_image* bkg = nullptr);

    /// Box Fitting Functions --------------------------------------------------------------------------------
    /** Get a box.
    * @param[in]     obj          shapefit object with box fitting capability. 
    * @param[in]     box_id       box identifier, 0-based.
    * @param[in]     box          return buffer for the box.
    * @return RS_SF_SUCCESS if no error. */
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_boxfit_get_box(const rs_shapefit* obj, int box_id, rs_sf_box* box);

    /** Draw box wireframes in an rgb image. 
    * @param[in]     obj          shapefit object with box fitting capability.
    * @param[in|out] rgb          rgb image buffer for drawing box wireframes. 
    * @param[in]     bkg          optional background image for the output rgb image.
    * @param[in]     color        optional color selection for box wireframes. 
    * @return RS_SF_SUCCESS if no error. */
    RS_SHAPEFIT_DECL rs_sf_status rs_sf_boxfit_draw_boxes(const rs_shapefit* obj, rs_sf_image* rgb, const rs_sf_image* bkg = nullptr, const unsigned char color[3] = nullptr);
   
#ifdef __cplusplus
}

#include <memory>
inline void rs_sf_memcpy(void* dst, const void* src, size_t len) { memcpy(dst, src, len); }

struct rs_sf_image_auto : public rs_sf_image
{
    virtual ~rs_sf_image_auto() {}
    rs_sf_image_auto& set_pose(const float p[12]) { 
        if (p) rs_sf_memcpy(cam_pose = cam_pose_data, p, sizeof(float) * 12);
        else cam_pose = nullptr; 
		return *this;
    }
    rs_sf_image_auto& set_intrinsics(const rs_sf_intrinsics* intr) {
        if (intr) rs_sf_memcpy(intrinsics = &intrinsics_data, intr, sizeof(rs_sf_intrinsics));
        else intrinsics = nullptr;
        return *this;
    }
    std::unique_ptr<unsigned char[]> src;
    float cam_pose_data[12];
    rs_sf_intrinsics intrinsics_data;
};

template<int Channel> struct rs_sf_image_impl : public rs_sf_image_auto
{
    rs_sf_image_impl(const rs_sf_image* ref) 
    {
        img_h = ref->img_h; img_w = ref->img_w; byte_per_pixel = Channel; frame_id = ref->frame_id;
        data = (src = std::make_unique<unsigned char[]>(num_char())).get();
        if (ref->data && num_char()==ref->num_char()) rs_sf_memcpy(data, ref->data, num_char());
        set_pose(ref->cam_pose); set_intrinsics(ref->intrinsics);       
    }
    rs_sf_image_impl(int w, int h, int fid = -1, const void* v = nullptr, const float pose[12] = nullptr, const rs_sf_intrinsics* i = nullptr) {
        img_h = h; img_w = w; byte_per_pixel = Channel; frame_id = fid;
        data = (src = std::make_unique<unsigned char[]>(num_char())).get();
        if (v) rs_sf_memcpy(data, v, num_char());
        set_pose(pose); set_intrinsics(i);
    }
};

typedef std::unique_ptr<rs_sf_image_auto> rs_sf_image_ptr;
typedef rs_sf_image_impl<1> rs_sf_image_mono;
typedef rs_sf_image_impl<2> rs_sf_image_depth;
typedef rs_sf_image_impl<3> rs_sf_image_rgb;

template<rs_shapefit_capability capability = RS_SHAPEFIT_PLANE> struct rs_shapefit_ptr : public std::unique_ptr<rs_shapefit, void(*)(rs_shapefit*)>
{
    rs_shapefit_ptr(const rs_sf_intrinsics* camera = nullptr, const rs_shapefit_capability cap = capability) :
        std::unique_ptr<rs_shapefit, void(*)(rs_shapefit*)>(
        (camera ? rs_shapefit_create(camera, cap) : nullptr), rs_shapefit_delete) {}
};

typedef rs_shapefit_ptr<> rs_sf_shapefit_ptr;
typedef rs_shapefit_ptr<RS_SHAPEFIT_PLANE> rs_sf_planefit_ptr;
typedef rs_shapefit_ptr<RS_SHAPEFIT_BOX> rs_sf_boxfit_ptr;

#endif //__cplusplus

#endif //! rs_shapefit_h