#pragma once
#ifndef rs_sf_planefit_h
#define rs_sf_planefit_h

#include "rs_sf_util.h"

struct rs_sf_planefit : public rs_shapefit
{
    struct parameter
    {
        bool filter_plane_map = true;
#ifdef _DEBUG
        bool hole_fill_plane_map = false;
        int img_x_dn_sample = 8;
        int img_y_dn_sample = 8;
#else
        bool hole_fill_plane_map = true;
        int img_x_dn_sample = 4;
        int img_y_dn_sample = 4;
#endif
        int candidate_x_dn_sample = 16;
        int candidate_y_dn_sample = 16;
        int track_x_dn_sample = 16 * 6;
        int track_y_dn_sample = 16 * 6;

        float min_z_value = 100.0f;
        float max_z_value = 3500.0f;
        float max_fit_err_thr = 30.0f;
        float max_normal_thr = 0.7f;

        int min_num_plane_pt = 150;
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

    static const int INVALID_PID = 0;
        
    struct plane; 
    struct pt3d_group;
    struct pt3d { v3 pos, normal; bool valid_pos, valid_normal;  plane* best_plane; int p; i2 pix; pt3d_group* grp; };
    typedef std::vector<pt3d*> vec_pt_ref;
    struct pt3d_group { int gp; i2 gpix; vec_pt_ref pt, pl_pt; pt3d *pt0, *pl_pt0; };
    struct plane {
        v3 normal; float d; pt3d* src; vec_pt_ref pts, best_pts; int pid; const plane* past_plane;
        plane(const v3& _nor, float _d, pt3d* _src, int _pid = INVALID_PID, const plane* _past_plane = nullptr)
            : normal(_nor), d(_d), src(_src), pid(_pid), past_plane(_past_plane) {}
    };
    
    typedef std::vector<pt3d> vec_pt3d;
    typedef std::vector<pt3d_group> vec_pt3d_group;
    typedef std::vector<plane> vec_plane;
    typedef std::vector<plane*> vec_plane_ref;
    struct scene {
        vec_pt3d pt_img, plane_img;
        vec_pt3d_group pt_grp;
        vec_plane planes;
        pose_t cam_pose;
        inline void swap(scene& ref) {
            pt_img.swap(ref.pt_img); plane_img.swap(ref.plane_img);
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
    
    // debug only
    rs_sf_image ref_img, ir_img;

    plane* get_tracked_plane(int pid) const;

private:

    // temporary memory
    vec_pt_ref m_inlier_buf;
    int m_plane_pt_reserve, m_track_plane_reserve;
    const int m_grid_w, m_grid_h, m_grid_neighbor[9];
    int src_h() const { return m_intrinsics.img_h; }
    int src_w() const { return m_intrinsics.img_w; }
    int num_pixels() const { return src_h()*src_w(); }
    int num_pixel_groups() const { return m_grid_h * m_grid_w; }

    // initalization
    void init_img_pt_groups(scene& view);

    // per frame detection
    i2 project_grid_i(const v3& cam_pt) const;
    v3 unproject(const float u, const float v, const float z) const;
    bool is_within_pt_group_fov(const int x, const int y) const;
    bool is_valid_raw_z(const float z) const;
    void image_to_pointcloud(const rs_sf_image* img, scene& current_view, pose_t& pose);
    void img_pt_group_to_normal(vec_pt3d_group& pt_groups);
    void img_pointcloud_to_planecandidate(vec_pt3d& img_ot, vec_plane& img_planes, int candidate_y_dn_sample = -1, int candidate_x_dn_sample = -1);
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
    void upsize_pt_cloud_to_plane_map(const vec_pt3d& pt_img, rs_sf_image* dst) const;
    void end_of_process();
};

#endif // ! rs_sf_planefit_h