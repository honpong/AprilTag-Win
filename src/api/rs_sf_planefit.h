#pragma once
#include "rs_sf_util.h"
#include <vector>

struct rs_sf_planefit
{
    typedef Eigen::Matrix<float, 3, 1> v3;
    typedef Eigen::Matrix<int, 2, 1> i2;
    struct plane;
    struct pt3d { v3 pos, normal; int px; plane* best_plane; };
    typedef std::vector<pt3d*> vec_pt_ref;
    struct plane { v3 normal; float d; const pt3d* src; vec_pt_ref pts; int pid; };

    struct parameter
    {
        int img_x_dn_sample = 8;
        int img_y_dn_sample = 8;
        int point_cloud_reserve = -1;
        int candidate_x_dn_sample = 16;
        int candidate_y_dn_sample = 16;
        float max_fit_err_thr = 30.0f;
        float max_normal_thr = 0.7f;
        int min_num_plane_pt = 200;
        float min_z_value = 100.0f;
    };

    rs_sf_planefit(const rs_sf_intrinsics* camera);
    
    rs_sf_status process_depth_image(const rs_sf_image* img);

protected:
    rs_sf_intrinsics m_intrinsics;
    parameter m_param;

    typedef std::vector<pt3d> vec_pt3d;
    typedef std::vector<plane> vec_plane;
    vec_pt3d m_pt_cloud;
    vec_plane m_plane_candidates;
    vec_pt_ref m_inlier_buf;

    rs_sf_image ref_img;

private:
    bool is_valid_pt3d(const pt3d& pos);
    void image_to_pointcloud(const rs_sf_image* img, vec_pt3d& pt_cloud);
    void pointcloud_to_normal(vec_pt3d& img_pt_cloud);
    void pointcloud_to_planecandidate(vec_pt3d& img_pt_cloud, vec_plane& img_planes);
    void test_planecandidate(vec_plane& plane_candidates);
    void non_max_plane_suppression(vec_plane& plane_candidates);
};