#pragma once
#ifndef rs_sf_planefit_h
#define rs_sf_planefit_h

#include "rs_sf_util.h"

struct rs_sf_planefit : public rs_shapefit
{
    struct parameter
    {
        bool compute_full_pt_cloud = false;
        bool filter_plane_map = false;
        bool refine_plane_map = false;
#ifdef _DEBUG
        bool hole_fill_plane_map = false;
        int img_x_dn_sample = 9;
        int img_y_dn_sample = 9;
#else
        bool hole_fill_plane_map = true;
        int img_x_dn_sample = 5;
        int img_y_dn_sample = 5;
#endif
        int candidate_gx_dn_sample = 3;
        int candidate_gy_dn_sample = 3;
        int track_gx_dn_sample = 6;
        int track_gy_dn_sample = 6;

        float min_z_value = 100.0f;
        float max_z_value = 3500.0f;
        float max_fit_err_thr = 30.0f;
        float max_normal_thr = 0.7f;

        int min_num_plane_pt = 200;
        int max_num_plane_output = MAX_VALID_PID;
    };

    rs_sf_planefit(const rs_sf_intrinsics* camera);    
    virtual rs_sf_status process_depth_image(const rs_sf_image* img);
    virtual rs_sf_status track_depth_image(const rs_sf_image* img);

    int num_detected_planes() const;
    int max_detected_pid() const;
    rs_sf_status get_plane_index_map(rs_sf_image* map, int hole_filled = -1) const; 
    rs_sf_status mark_plane_src_on_map(rs_sf_image* map) const;
    rs_sf_status get_plane_equation(int pid, float equ[4]) const;

protected:
        
    struct plane; 
    plane *const NON_EDGE_PT = (plane*)-1;
    static const int INVALID_PID = 0;

    struct pt3d_group;
    typedef unsigned char pt_state;
    static const pt_state PT3D_MASK_KNOWN_POS = 0x1, PT3D_MASK_VALID_POS = 0x2;
    static const pt_state PT3D_MASK_KNOWN_NORMAL = 0x4, PT3D_MASK_VALID_NORMAL = 0x8;
    static const pt_state PT3D_MASK_KNOWN_PLANE = 0x10, PT3D_MASK_VALID_PLANE = 0x20;
    struct pt3d {
        v3 pos, normal;
        pt_state state;
        plane* best_plane;
        int p; i2 pix; pt3d_group *grp;
        inline void clear_all_state() { state = 0; best_plane = nullptr; }
        inline void clear_check_flag() { state &= 0x7f; }
        inline void clear_boundary_flag() { state &= 0xbf; }
        inline void set_valid_pos() { state |= 0x3; }
        inline void set_invalid_pos() { state |= 0x1; }
        inline void set_valid_normal() { state |= 0xf; }
        inline void set_invalid_normal() { state |= PT3D_MASK_KNOWN_NORMAL; }
        inline void set_checked() { state |= 0x80; }
        inline void set_boundary() { state |= 0x40; }
        inline bool is_known_pos() const { return (state & PT3D_MASK_KNOWN_POS) != 0; }
        inline bool is_known_normal() const { return (state & PT3D_MASK_KNOWN_NORMAL) != 0; }
        inline bool is_valid_pos() const { return (state & PT3D_MASK_VALID_POS) != 0; }
        inline bool is_valid_normal() const { return (state & PT3D_MASK_VALID_NORMAL) != 0; }
        inline bool is_valid_plane() const { return (state & PT3D_MASK_VALID_PLANE) != 0; }
        inline bool is_checked() const { return (state & 0x80) != 0; }
        inline bool is_boundary() const { return (state & 0x40) != 0; }
    };
    typedef std::vector<pt3d*> vec_pt_ref;
    struct pt3d_group { int gp; i2 gpix; vec_pt_ref pt; pt3d *pt0; };
    typedef std::list<pt3d*> list_pt_ref;
    struct plane {
        v3 normal; float d; pt3d* src; int pid;
        vec_pt_ref pts, best_pts; 
        list_pt_ref edge_grp[2], fine_pts;
        const plane* past_plane;
        plane(const v3& _nor, float _d, pt3d* _src, const plane* _past_plane = nullptr)
            : normal(_nor), d(_d), src(_src), pid(INVALID_PID), past_plane(_past_plane) {}
    };
    
    typedef std::vector<pt3d> vec_pt3d;
    typedef std::vector<pt3d_group> vec_pt3d_group;
    typedef std::vector<plane> vec_plane;
    typedef std::vector<plane*> vec_plane_ref;
    struct scene {
        bool is_full_pt_cloud;
        vec_pt3d pt_img;
        vec_pt3d_group pt_grp;
        vec_plane planes;
        pose_t cam_pose;
        inline void swap(scene& ref) {
            std::swap(is_full_pt_cloud, ref.is_full_pt_cloud);
            pt_img.swap(ref.pt_img);
            pt_grp.swap(ref.pt_grp);
            planes.swap(ref.planes);
            std::swap(cam_pose, ref.cam_pose);
        }
        inline void reset() { planes.clear(); cam_pose.set_pose(); }
    };
  
    // state memory
    parameter m_param;
    scene m_view, m_ref_view;
    vec_plane_ref m_tracked_pid, m_sorted_plane_ptr;
    
    // input
    rs_sf_image_depth src_depth_img;
    
    // call after parameter updated
    void parameter_updated();

    // for child classes
    plane* get_tracked_plane(int pid) const;
    int src_h() const { return m_intrinsics.height; }
    int src_w() const { return m_intrinsics.width; }
    int num_pixels() const { return src_h()*src_w(); }
    int num_pixel_groups() const { return m_grid_h * m_grid_w; }
    void refine_plane_boundary(plane& dst);

private:

    // temporary memory
    vec_pt_ref m_inlier_buf;
    int m_plane_pt_reserve, m_track_plane_reserve;
    int m_grid_w, m_grid_h, m_grid_neighbor[9];

    // initalization
    void init_img_pt_groups(scene& view);

    // per frame detection
    i2 project_grid_i(const v3& cam_pt) const;
    v3 unproject(const float u, const float v, const float z) const;
    bool is_within_pt_group_fov(const int x, const int y) const;
    bool is_within_pt_img_fov(const int x, const int y) const;
    bool is_valid_raw_z(const float z) const;
    unsigned short& get_raw_z_at(const pt3d& pt) const;
    void compute_pt3d(pt3d& pt) const;
    void compute_pt3d_normal(pt3d& pt_query, pt3d& pt_right, pt3d& pt_below) const;
    void image_to_pointcloud(const rs_sf_image* img, scene& current_view, bool force_full_pt_cloud = false);
    void img_pt_group_to_normal(vec_pt3d_group& pt_groups);
    void img_pointcloud_to_planecandidate(const vec_pt3d_group& pt_groups, vec_plane& img_planes, int candidate_gy_dn_sample = -1, int candidate_gx_dn_sample = -1);
    bool is_inlier(const plane& candidate, const pt3d& p);
    void grow_planecandidate(vec_pt3d_group& pt_groups, vec_plane& plane_candidates);
    void grow_inlier_buffer(pt3d_group pt_group[], plane& plane_candidate, const vec_pt_ref& seeds, bool reset_best_plane_ptr = true);
    void non_max_plane_suppression(vec_pt3d_group& pt_groups, vec_plane& plane_candidates);
    void filter_plane_ptr_to_plane_img(vec_pt3d_group& pt_groups);
    void sort_plane_size(vec_plane& planes, vec_plane_ref& sorted_planes);

    // plane tracking
    bool is_valid_past_plane(const plane& past_plane) const;
    bool is_tracked_pid(int pid) const;
    void save_current_scene_as_reference();
    void map_candidate_plane_from_past(scene& current_view, scene& past_view);
    void combine_planes_from_the_same_past(scene& current_view, scene& past_view);
    void assign_planes_pid(vec_plane_ref& sorted_planes);

    // output utility 
    void pt_groups_planes_to_full_img(vec_pt3d& pt_img, vec_plane_ref& sorted_planes);
    void upsize_pt_cloud_to_plane_map(const vec_pt3d& plane_img, rs_sf_image* dst) const;
    void end_of_process();

    // pose buffer
    float r00, r01, r02, r10, r11, r12, r20, r21, r22, t0, t1, t2;
    float& cam_px = m_intrinsics.ppx;
    float& cam_py = m_intrinsics.ppy;
    float& cam_fx = m_intrinsics.fx;
    float& cam_fy = m_intrinsics.fy;
};

#endif // ! rs_sf_planefit_h