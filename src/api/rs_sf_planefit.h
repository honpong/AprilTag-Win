#pragma once
#include "rs_sf_util.h"

struct rs_sf_planefit
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
        int point_cloud_reserve = -1;
        int candidate_x_dn_sample = 16;
        int candidate_y_dn_sample = 16;
        float max_fit_err_thr = 30.0f;
        float max_normal_thr = 0.7f;
        int min_num_plane_pt = 150;
        float min_z_value = 100.0f;
        float max_z_value = 2500.0f;
        int max_num_plane_output = 254;
        bool keep_previous_plane_pts = false;
        int track_x_dn_sample = 16 * 8;
        int track_y_dn_sample = 16 * 8;
    };

    rs_sf_planefit(const rs_sf_intrinsics* camera);    
    rs_sf_status process_depth_image(const rs_sf_image* img);
    rs_sf_status track_depth_image(const rs_sf_image* img);

    int num_detected_planes() const;
    rs_sf_status get_plane_index_map(rs_sf_image* map, int hole_filled = -1) const; 

protected:

    typedef Eigen::Matrix<float, 3, 1> v3;
    typedef Eigen::Matrix<int, 2, 1> i2;
    typedef Eigen::Quaternion<float> rotation;
    static const int INVALID_PID = -1;
    static const int MAX_VALID_PID = 254;

    struct plane;
    struct pt3d {
        v3 pos, normal; int px, ppx; plane* best_plane;
        pt3d(v3& _pos, v3& _nor, int _px = 0, int _ppx = 0, plane* _bestplane = nullptr) 
            : pos(_pos), normal(_nor), px(_px), ppx(_ppx), best_plane(_bestplane) {}
    };
    typedef std::vector<pt3d*> vec_pt_ref;
    typedef std::list<plane*> list_plane_ref;
    struct plane {
        v3 normal; float d; pt3d* src; vec_pt_ref pts, best_pts; int pid; const plane* past_plane;
        plane(v3& _nor, float _d, pt3d* _src, int _pid = INVALID_PID, const plane* _past_plane = nullptr)
            : normal(_nor), d(_d), src(_src), pid(_pid), past_plane(_past_plane) {}
    };
    
    typedef std::vector<pt3d> vec_pt3d;
    typedef std::vector<plane> vec_plane;
    typedef std::vector<plane*> vec_plane_ref;
    struct scene {
        vec_pt3d pt_cloud;
        vec_plane planes;
        void clear() { pt_cloud.clear(); planes.clear(); }
        void swap(scene& ref) { pt_cloud.swap(ref.pt_cloud); planes.swap(ref.planes); }
    };

    rs_sf_intrinsics m_intrinsics;
    parameter m_param;
    scene m_view, m_ref_scene;
    vec_pt_ref m_inlier_buf;
    vec_plane_ref m_tracked_pid, m_sorted_plane_ptr;

private:
    
    // per frame detection
    bool is_valid_pt3d_z(const pt3d& pt) const;
    bool is_valid_pt3d_normal(const pt3d& pt) const;
    void image_to_pointcloud(const rs_sf_image* img, vec_pt3d& pt_cloud);
    void img_pointcloud_to_normal(vec_pt3d& img_pt_cloud);
    void img_pointcloud_to_planecandidate(vec_pt3d& img_pt_cloud, vec_plane& img_planes);
    bool is_inlier(const plane& candidate, const pt3d& p);
    void grow_planecandidate(vec_pt3d& img_pt_cloud, vec_plane& plane_candidates);
    void test_planecandidate(vec_pt3d& pt_cloud, vec_plane& plane_candidates);
    void non_max_plane_suppression(vec_pt3d& pt_cloud, vec_plane& plane_candidates);
    void sort_plane_size(vec_plane& planes, vec_plane_ref& sorted_planes);

    // plane tracking
    void save_current_scene_as_reference();
    void find_candidate_plane_from_past(scene& current_view, scene& past_view);
    void combine_planes_from_the_same_past(scene& current_view, scene& past_view);
    void assign_planes_pid(vec_plane_ref& sorted_planes);
    bool is_tracked_pid(int pid);

    // output utility 
    void upsize_pt_cloud_to_plane_map(const vec_pt3d& img_pt_cloud, rs_sf_image* dst) const;

    // debug only
    rs_sf_image ref_img;
};