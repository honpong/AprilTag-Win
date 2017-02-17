#pragma once
#include "rs_sf_util.h"
#include <vector>

struct rs_sf_planefit
{
    typedef Eigen::Matrix<float, 3, 1> v3;
    typedef Eigen::Matrix<int, 2, 1> i2;
    struct pt3d { v3 pos, normal; };

    struct parameter
    {
        int img_x_dn_sample = 2;
        int img_y_dn_sample = 2;
        int point_cloud_reserve = -1;
    };

    rs_sf_planefit(const rs_sf_intrinsics* camera);
    
    rs_sf_status process_depth_image(const rs_sf_image* img);

protected:
    rs_sf_intrinsics m_intrinsics;
    parameter m_param;

    std::vector<pt3d> m_pt_cloud;

private:
    void image_to_pointcloud(const rs_sf_image* img, std::vector<pt3d>& pt_cloud);
    void pointcloud_to_normal(std::vector<pt3d>& img_pt_cloud);
};