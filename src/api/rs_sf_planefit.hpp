/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
//
//  rs_sf_planefit.hpp
//  algo-core
//
//  Created by Hon Pong (Gary) Ho
//
#pragma once
#ifndef rs_sf_planefit_hpp
#define rs_sf_planefit_hpp

#include "rs_sf_util.h"

struct rs_sf_planefit : public rs_shapefit
{
    struct parameter
    {
        bool compute_full_pt_cloud = false;
        bool search_around_missing_z = false;
        bool filter_plane_map = false;
        bool refine_plane_map = false;
        bool hole_fill_plane_map = true;

#ifndef _DEBUG
        int  img_x_dn_sample = 5;
        int  img_y_dn_sample = 5;
#else
        int  img_x_dn_sample = 10;
        int  img_y_dn_sample = 10;
#endif
        int  candidate_gx_dn_sample = 3;
        int  candidate_gy_dn_sample = 3;
        int  track_gx_dn_sample = 6;
        int  track_gy_dn_sample = 6;

        float min_z_value = 0.05f;      //meter
        float max_z_value = 6.0f;       //meter
        float max_fit_err_thr = 0.015f; //meter
        float max_normal_thr = 0.75f;

        int min_num_plane_pt = 200;
        int max_num_plane_output = MAX_VALID_PID;
    };

    rs_sf_planefit(const rs_sf_intrinsics* camera);
    virtual ~rs_sf_planefit() override { run_task(-1); }
    virtual rs_sf_status set_option(rs_sf_fit_option option, double value) override;
    virtual rs_sf_status set_locked_inputs(const rs_sf_image *img) override;
    virtual rs_sf_status set_locked_outputs() override;
    virtual rs_sf_status process_depth_image();
    virtual rs_sf_status track_depth_image();

    int num_detected_planes() const;
    int max_detected_pid() const;
    rs_sf_status get_plane_index_map(rs_sf_image* map, int hole_filled = -1) const;
    rs_sf_status mark_plane_src_on_map(rs_sf_image* map) const;
    rs_sf_status get_planes(rs_sf_plane dst[RS_SF_MAX_PLANE_COUNT], float* point_buffer) const; // return unit meter

protected:

    struct plane;
    struct pt3d_group;
    struct pt3d {
        enum : unsigned char {
            MASK_KNOWN_POS = 0x01,
            MASK_VALID_POS = 0x02,
            MASK_KNOWN_NORMAL = 0x04,
            MASK_VALID_NORMAL = 0x08,
            MASK_KNOWN_PLANE = 0x10,
            MASK_VALID_PLANE = 0x20,
            MASK_BOUNDARY = 0x40,
            MASK_CHECKED = 0x80,
            SET_VALID_POS = (MASK_KNOWN_POS | MASK_VALID_POS),
            SET_VALID_NORMAL = (MASK_KNOWN_NORMAL | MASK_VALID_NORMAL),
            CLEAR_BOUNDARY = (~MASK_BOUNDARY & 0xff),
            CLEAR_CHECKED = (~MASK_CHECKED & 0xff),
        }; 
        unsigned char state;
        plane* best_plane;
        v3 pos, normal;
        int p; i2 pix; pt3d_group *grp;       
        inline void clear_all_state() { state = 0; best_plane = nullptr; }
        inline void clear_check_flag() { state &= CLEAR_CHECKED; }
        inline void clear_boundary_flag() { state &= CLEAR_BOUNDARY; }
        inline void set_valid_pos() { state |= SET_VALID_POS; }
        inline void set_invalid_pos() { state |= MASK_KNOWN_POS; }
        inline void set_valid_normal() { state |= SET_VALID_NORMAL; }
        inline void set_invalid_normal() { state |= MASK_KNOWN_NORMAL; }
        inline void set_checked() { state |= MASK_CHECKED; }
        inline void set_boundary() { state |= MASK_BOUNDARY; }
        inline bool is_known_pos() const { return (state & MASK_KNOWN_POS) != 0; }
        inline bool is_known_normal() const { return (state & MASK_KNOWN_NORMAL) != 0; }
        inline bool is_valid_pos() const { return (state & MASK_VALID_POS) != 0; }
        inline bool is_valid_normal() const { return (state & MASK_VALID_NORMAL) != 0; }
        inline bool is_valid_plane() const { return (state & MASK_VALID_PLANE) != 0; }
        inline bool is_checked() const { return (state & MASK_CHECKED) != 0; }
        inline bool is_boundary() const { return (state & MASK_BOUNDARY) != 0; }
    };

    typedef std::vector<pt3d> vec_pt3d;
    typedef std::vector<pt3d*> vec_pt_ref;
    typedef std::list<pt3d*> list_pt_ref;

    struct pt3d_group { pt3d *pt0; int gp; vec_pt_ref pt; };
    struct plane {
        v3 normal; float d; pt3d* src; int pid;
        vec_pt_ref pts, best_pts, edge_grp[2], fine_pts;
        const plane* past_plane;
        plane(const v3& _nor, float _d, pt3d* _src, const plane* _past_plane = nullptr)
            : normal(_nor), d(_d), src(_src), pid(INVALID_PID), past_plane(_past_plane) {}
        bool non_empty() const { return best_pts.size() > 1; }
    };

    typedef std::vector<plane> vec_plane;
    typedef std::vector<plane*> vec_plane_ref;
    typedef std::vector<pt3d_group> vec_pt3d_group;
    typedef std::unique_ptr<rs_sf_image_depth> image_depth_ptr;
    typedef std::unique_ptr<rs_sf_image_rgb>   image_color_ptr;

    struct scene {
        bool is_full_pt_cloud;
        pose_t cam_pose;
        image_depth_ptr src_depth_img;
        image_color_ptr src_color_img;
        vec_pt3d pt_img;
        vec_pt3d_group pt_grp;
        vec_plane planes;
        vec_plane_ref tracked_pid, sorted_plane_ptr;
        inline void swap(scene& ref) {
            std::swap(is_full_pt_cloud, ref.is_full_pt_cloud);
            std::swap(cam_pose, ref.cam_pose);
            std::swap(src_depth_img, ref.src_depth_img);
            std::swap(src_color_img, ref.src_color_img);
            pt_img.swap(ref.pt_img);
            pt_grp.swap(ref.pt_grp);
            planes.swap(ref.planes);
            tracked_pid.swap(ref.tracked_pid);
            sorted_plane_ptr.swap(ref.sorted_plane_ptr);
        }
        inline void reset() {
            cam_pose.set_pose();
            planes.clear();
            tracked_pid.clear();
            sorted_plane_ptr.clear();
        }
    };

    // state memory
    parameter m_param;
    scene m_view, m_ref_view;
    int m_grid_w, m_grid_h, m_grid_neighbor[9];

    // call after parameter updated
    void parameter_updated();

    // for child classes
    plane* get_tracked_plane(int pid) const;
    int src_h() const { return m_intrinsics.height; }
    int src_w() const { return m_intrinsics.width; }
    int num_pixels() const { return src_h()*src_w(); }
    int num_pixel_groups() const { return m_grid_h * m_grid_w; }
    bool is_valid_new_plane(const plane& new_plane) const { return (int)new_plane.pts.size() > m_param.min_num_plane_pt; }
    bool is_valid_plane(const plane& plane) const { return (int)plane.best_pts.size() > m_param.min_num_plane_pt; }
    bool is_valid_past_plane(const plane& past_plane) const { return past_plane.pid > INVALID_PID && past_plane.non_empty(); }
    void refine_plane_boundary(plane& dst);

private:

    // temporary memory
    vec_pt_ref m_inlier_buf;
    int m_plane_pt_reserve, m_track_plane_reserve;

    // initalization
    void init_img_pt_groups(scene& view);

    // per frame detection
    i2 project_grid_i(const v3& cam_pt) const;
    v3 unproject(const float u, const float v, const float z) const;
    bool is_within_pt_group_fov(const int x, const int y) const;
    bool is_within_pt_img_fov(const int x, const int y) const;
    bool is_valid_raw_z(const float z) const;
    float get_z_in_meter(const pt3d& pt) const;
    void compute_pt3d(pt3d& pt, bool search_around = false) const;
    void compute_pt3d_normal(pt3d& pt_query, pt3d& pt_right, pt3d& pt_below) const;
    void image_to_pointcloud(scene& current_view, bool force_full_pt_cloud = false);
    void img_pt_group_to_normal(vec_pt3d_group& pt_groups);
    void img_pointcloud_to_planecandidate(const vec_pt3d_group& pt_groups, vec_plane& img_planes, int candidate_gy_dn_sample = -1, int candidate_gx_dn_sample = -1);
    bool is_inlier(const plane& candidate, const pt3d& p);
    void grow_planecandidate(vec_pt3d_group& pt_groups, vec_plane& plane_candidates);
    void grow_inlier_buffer(pt3d_group pt_group[], plane& plane_candidate, const vec_pt_ref& seeds, bool reset_best_plane_ptr = true);
    void non_max_plane_suppression(vec_pt3d_group& pt_groups, vec_plane& plane_candidates);
    void filter_plane_ptr_to_plane_img(vec_pt3d_group& pt_groups);
    void sort_plane_size(vec_plane& planes, vec_plane_ref& sorted_planes);

    // plane tracking
    bool is_tracked_pid(int pid) const { return INVALID_PID < pid && pid <= MAX_VALID_PID; }
    void save_current_scene_as_reference();
    void map_candidate_plane_from_past(scene& current_view, const scene& past_view);
    void combine_planes_from_the_same_past(scene& current_view);
    void assign_planes_pid(vec_plane_ref& tracked_pid, const vec_plane_ref& sorted_planes);

    // output utility 
    void pt_groups_planes_to_full_img(vec_pt3d& pt_img, vec_plane_ref& sorted_planes);
    void upsize_pt_cloud_to_plane_map(const scene& ref_view, rs_sf_image * dst) const;
    void hole_fill_custom_plane_map(rs_sf_image* map) const;

    // pose buffer
    float r00, r01, r02, r10, r11, r12, r20, r21, r22, t0, t1, t2;
    float inv_cam_fx, inv_cam_fy;
    float& cam_px = m_intrinsics.ppx;
    float& cam_py = m_intrinsics.ppy;
    float& cam_fx = m_intrinsics.fx;
    float& cam_fy = m_intrinsics.fy;
    //rs_sf_distortion& model = m_intrinsics.model;
    float m_depth_img_to_meter = 0.001f;
};

#endif // ! rs_sf_planefit_hpp

