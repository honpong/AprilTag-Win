#pragma once
#ifndef rs_sf_planefit_h
#define rs_sf_planefit_h

#include "rs_sf_util.h"

struct rs_sf_planefit : public rs_shapefit
{
    struct parameter
    {
#ifdef _DEBUG
        int img_x_dn_sample = 8;
        int img_y_dn_sample = 8;
        bool hole_fill_plane_map = false;
#else
        int img_x_dn_sample = 4;
        int img_y_dn_sample = 4;
        bool hole_fill_plane_map = true;
#endif
        int candidate_x_dn_sample = 16;
        int candidate_y_dn_sample = 16;
        float max_fit_err_thr = 30.0f;
        float max_normal_thr = 0.7f;
        int min_num_plane_pt = 150;
        float min_z_value = 100.0f;
        float max_z_value = 3500.0f;
        int max_num_plane_output = MAX_VALID_PID;
        int track_x_dn_sample = 16 * 8;
        int track_y_dn_sample = 16 * 8;
        bool compute_full_pt_cloud = true;
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
    struct pt3d {
        v3 pos, normal; int p; i2 pix; plane* best_plane;
        pt3d(const v3& _pos, const v3& _nor, int _p = 0, int x = 0, int y = 0, plane* _bestplane = nullptr)
            : pos(_pos), normal(_nor), p(_p), pix(x, y), best_plane(_bestplane) {}
    };
    typedef std::vector<pt3d*> vec_pt_ref;
    typedef std::list<plane*> list_plane_ref;
    struct plane {
        v3 normal; float d; pt3d* src; vec_pt_ref pts, best_pts; int pid; const plane* past_plane;
        plane(const v3& _nor, float _d, pt3d* _src, int _pid = INVALID_PID, const plane* _past_plane = nullptr)
            : normal(_nor), d(_d), src(_src), pid(_pid), past_plane(_past_plane) {}
    };
    
    typedef std::vector<pt3d> vec_pt3d;
    typedef std::vector<plane> vec_plane;
    typedef std::vector<plane*> vec_plane_ref;
    struct scene {
        vec_pt_ref pt_cloud;
        vec_pt3d pt_img;
        vec_plane planes;
        pose_t cam_pose;
        inline void clear() { pt_cloud.clear(); pt_img.clear(); planes.clear(); cam_pose.set_pose(); }
        inline void swap(scene& ref) { pt_cloud.swap(ref.pt_cloud); pt_img.swap(ref.pt_img); planes.swap(ref.planes); std::swap(cam_pose, ref.cam_pose); }
    };

    // state memory
    parameter m_param;
    scene m_view, m_ref_scene;
    vec_plane_ref m_tracked_pid, m_sorted_plane_ptr;
    
    // debug only
    rs_sf_image ref_img, ir_img;

private:

    // temporary memory
    vec_pt_ref m_inlier_buf;
    int m_plane_pt_reserve;
    int src_h() const { return m_intrinsics.img_h; }
    int src_w() const { return m_intrinsics.img_w; }
    int num_pixels() const { return src_h()*src_w(); }

    // per frame detection
    i2 project_i(const v3& cam_pt) const;
    v3 unproject(const float u, const float v, const float z) const;
    bool is_within_pt_cloud_fov(const int x, const int y) const;
    bool is_valid_raw_z(const float z) const;
    bool is_valid_pt3d_pos(const pt3d& pt) const;
    bool is_valid_pt3d_normal(const pt3d& pt) const;
    void image_to_pointcloud(const rs_sf_image* img, vec_pt3d& pt_img, vec_pt_ref& pt_cloud, pose_t& pose);
    void img_pointcloud_to_normal(vec_pt3d& img_pt_cloud);
    void img_pointcloud_to_planecandidate(vec_pt3d& img_pt_cloud, vec_plane& img_planes, int candidate_y_dn_sample = -1, int candidate_x_dn_sample = -1);
    bool is_inlier(const plane& candidate, const pt3d& p);
    void grow_planecandidate(vec_pt3d& img_pt_cloud, vec_plane& plane_candidates);
    void grow_inlier_buffer(pt3d src_img_pt[], plane& plane_candidate, std::vector<pt3d*>& seeds, bool reset_best_plane_ptr = true);
    void test_planecandidate(vec_pt_ref& pt_cloud, vec_plane& plane_candidates);
    void non_max_plane_suppression(vec_pt_ref& pt_cloud, vec_plane& plane_candidates);
    void sort_plane_size(vec_plane& planes, vec_plane_ref& sorted_planes);

    // plane tracking
    bool is_valid_past_plane(const plane& past_plane) const;
    void save_current_scene_as_reference();
    void map_candidate_plane_from_past(scene& current_view, scene& past_view);
    void combine_planes_from_the_same_past(scene& current_view, scene& past_view);
    void assign_planes_pid(vec_plane_ref& sorted_planes);
    bool is_tracked_pid(int pid);

    // output utility 
    void upsample_best_plane_ptr(scene& view);
    void upsize_pt_cloud_to_plane_map(const vec_pt3d& img_pt_cloud, rs_sf_image* dst) const;
};

#endif // ! rs_sf_planefit_h