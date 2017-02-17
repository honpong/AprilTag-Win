#include "rs_sf_planefit.h"

rs_sf_planefit::rs_sf_planefit(const rs_sf_intrinsics * camera)
{
    m_intrinsics = *camera;

    m_param.point_cloud_reserve =
        (m_intrinsics.img_h * m_intrinsics.img_w + 1) /
        (m_param.img_x_dn_sample*m_param.img_y_dn_sample);
}

rs_sf_status rs_sf_planefit::process_depth_image(const rs_sf_image * img)
{
    if (!img || img->byte_per_pixel != 2) return RS_SF_INVALID_ARG;

    image_to_pointcloud(img, m_pt_cloud);
    pointcloud_to_normal(m_pt_cloud);

    return RS_SF_SUCCESS;
}



void rs_sf_planefit::image_to_pointcloud(const rs_sf_image * img, std::vector<pt3d>& pt_cloud)
{
    const int img_h = m_intrinsics.img_h;
    const int img_w = m_intrinsics.img_w;
    const auto& px = m_intrinsics.cam_px;
    const auto& py = m_intrinsics.cam_py;
    const auto& fx = m_intrinsics.cam_fx;
    const auto& fy = m_intrinsics.cam_fy;
    const auto* src_depth = (unsigned short*)img->data;

    pt_cloud.clear();
    pt_cloud.reserve(m_param.point_cloud_reserve);
    for (int y = 0, p = 0; y < img_h; y += m_param.img_y_dn_sample)
    {
        for (int x = 0; x < img_w; x += m_param.img_x_dn_sample)
        {
            const auto z = (float)src_depth[p];
            pt_cloud.push_back({ v3{ z *(x - px) / fx,z*(y - py) / fy,z },v3() });
        }
    }
}

void rs_sf_planefit::pointcloud_to_normal(std::vector<pt3d>& img_pt_cloud)
{
    const int img_h = m_intrinsics.img_h / m_param.img_y_dn_sample;
    const int img_w = m_intrinsics.img_w / m_param.img_x_dn_sample;

    auto* src_pt_cloud = img_pt_cloud.data();
    for (int y = 0, ey = img_h - 1, p = 0; y < ey; ++y) {
        for (int x = 0, ex = img_w - 1; x < ex; ++x, ++p)
        {
            const auto p_right = p + 1, p_down = p + img_w;
            auto dx = src_pt_cloud[p_right].pos - src_pt_cloud[p].pos;
            auto dy = src_pt_cloud[p_down].pos - src_pt_cloud[p].pos;
            src_pt_cloud[p].normal = dy.cross(dx).normalized();
        }
    }
}
