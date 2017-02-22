#pragma once
#include "rs_sf_util.h"
#include <vector>

struct rs_sf_planefit
{
    typedef Eigen::Matrix<float, 3, 1> v3;
    typedef Eigen::Matrix<int, 2, 1> i2;
    typedef Eigen::Quaternion<float> rotation;
    struct plane;
    struct pt3d { v3 pos, normal; int px, ppx; plane* best_plane; };
    typedef std::vector<pt3d*> vec_pt_ref;
    struct plane { v3 normal; float d; pt3d* src; vec_pt_ref pts, best_pts; int pid; const plane* past_plane; };
    typedef std::vector<pt3d> vec_pt3d;
    typedef std::vector<plane> vec_plane;

    struct parameter
    {
#ifdef _DEBUG
        int img_x_dn_sample = 8;
        int img_y_dn_sample = 8;
#else
        int img_x_dn_sample = 4;
        int img_y_dn_sample = 4;
#endif
        int point_cloud_reserve = -1;
        int candidate_x_dn_sample = 16;
        int candidate_y_dn_sample = 16;
        float max_fit_err_thr = 30.0f;
        float max_normal_thr = 0.7f;
        int min_num_plane_pt = 200;
        float min_z_value = 100.0f;
        int max_num_plane_output = 255;
    };

    struct scene {
        vec_pt3d pt_cloud;
        vec_plane planes;
        void clear() { pt_cloud.clear(); planes.clear(); }
    };

    rs_sf_planefit(const rs_sf_intrinsics* camera);
    
    rs_sf_status process_depth_image(const rs_sf_image* img);
    rs_sf_status track_depth_image(const rs_sf_image* img);
    int num_detected_planes() const { return (int)m_view.planes.size(); }

protected:
    rs_sf_intrinsics m_intrinsics;
    parameter m_param;
    scene m_view, m_ref_scene;
    vec_pt_ref m_inlier_buf;

private:
    
    // per frame detection
    bool is_valid_pt3d(const pt3d& pos);
    void image_to_pointcloud(const rs_sf_image* img, vec_pt3d& pt_cloud);
    void img_pointcloud_to_normal(vec_pt3d& img_pt_cloud);
    void img_pointcloud_to_planecandidate(vec_pt3d& img_pt_cloud, vec_plane& img_planes);
    bool is_inlier(const plane& candidate, const pt3d& p);
    void grow_planecandidate(vec_pt3d& img_pt_cloud, vec_plane& plane_candidates);
    void test_planecandidate(vec_pt3d& pt_cloud, vec_plane& plane_candidates);
    void non_max_plane_suppression(vec_pt3d& pt_cloud, vec_plane& plane_candidates);

    // plane tracking
    void find_candidate_plane_from_past(scene& current_view, const scene& past_view);
    
    // debug only
    rs_sf_image ref_img;
    void visualize(vec_plane& plane_candidates);
};