#include <opencv2/opencv.hpp>
#include "rs_sf_planefit.h"

rs_sf_planefit::rs_sf_planefit(const rs_sf_intrinsics * camera)
{
    m_intrinsics = *camera;

    m_param.point_cloud_reserve =
        (m_intrinsics.img_h * m_intrinsics.img_w + 1) /
        (m_param.img_x_dn_sample*m_param.img_y_dn_sample);

    m_inlier_buf.reserve(m_param.point_cloud_reserve);
}

rs_sf_status rs_sf_planefit::process_depth_image(const rs_sf_image * img)
{
    if (!img || img->byte_per_pixel != 2) return RS_SF_INVALID_ARG;

    // for debug
    ref_img = *img;
   
    //reset
    m_pt_cloud.clear();
    m_plane_candidates.clear();

    //plane fitting
    image_to_pointcloud(img, m_pt_cloud);
    pointcloud_to_normal(m_pt_cloud);
    pointcloud_to_planecandidate(m_pt_cloud, m_plane_candidates);
    test_planecandidate(m_plane_candidates);
    non_max_plane_suppression(m_plane_candidates);

    return RS_SF_SUCCESS;
}

bool rs_sf_planefit::is_valid_pt3d(const pt3d& pt)
{
    return pt.pos.z() > m_param.min_z_value;
}

void rs_sf_planefit::image_to_pointcloud(const rs_sf_image * img, vec_pt3d& pt_cloud)
{
    const int img_h = m_intrinsics.img_h;
    const int img_w = m_intrinsics.img_w;
    const auto& px = m_intrinsics.cam_px;
    const auto& py = m_intrinsics.cam_py;
    const auto& fx = m_intrinsics.cam_fx;
    const auto& fy = m_intrinsics.cam_fy;
    const auto* src_depth = (unsigned short*)img->data;

    const int y_step = m_param.img_y_dn_sample;
    const int x_step = m_param.img_x_dn_sample;

    pt_cloud.clear();
    pt_cloud.reserve(m_param.point_cloud_reserve);
    for (int y = 0, p = 0; y < img_h; y += y_step, p = y*img_w)
    {
        for (int x = 0; x < img_w; x += x_step, p += x_step)
        {
            const auto z = (float)src_depth[p];
            pt_cloud.push_back({ v3{
                z * (x - px) / fx,
                z * (y - py) / fy,
                z }
            , v3(), p, nullptr });
        }
    }
}

void rs_sf_planefit::pointcloud_to_normal(vec_pt3d& img_pt_cloud)
{
    const int img_h = m_intrinsics.img_h / m_param.img_y_dn_sample;
    const int img_w = m_intrinsics.img_w / m_param.img_x_dn_sample;

    auto* src_pt_cloud = img_pt_cloud.data();
    for (int y = 0, ey = img_h - 1, p = 0; y < ey; ++y, p=y*img_w) {
        for (int x = 0, ex = img_w - 1; x < ex; ++x, ++p)
        {
            if (is_valid_pt3d(src_pt_cloud[p]))
            {
                const auto p_right = p + 1, p_down = p + img_w;
                auto dx = src_pt_cloud[p_right].pos - src_pt_cloud[p].pos;
                auto dy = src_pt_cloud[p_down].pos - src_pt_cloud[p].pos;
                src_pt_cloud[p].normal = dy.cross(dx).normalized();
            }
        }
    }
}

void rs_sf_planefit::pointcloud_to_planecandidate(
    vec_pt3d& img_pt_cloud,
    vec_plane& img_planes)
{
    const int img_h = m_intrinsics.img_h / m_param.img_y_dn_sample;
    const int img_w = m_intrinsics.img_w / m_param.img_x_dn_sample;

    const int y_step = m_param.candidate_y_dn_sample / m_param.img_y_dn_sample;
    const int x_step = m_param.candidate_x_dn_sample / m_param.img_x_dn_sample;

    auto* src_pt_cloud = img_pt_cloud.data();

    for (int y = 0, ey = img_h - 1, p = 0; y < ey; y += y_step, p = y*img_w) {
        for (int x = 0, ex = img_w - 1; x < ex; x += x_step, p += x_step)
        {
            auto& src = src_pt_cloud[p];
            auto& pos = src.pos;
            auto& nor = src.normal;
            if (is_valid_pt3d(src))
            {
                float d = -nor.dot(pos);
                img_planes.push_back({ nor, d, &src, vec_pt_ref(), 0 });
            }
        }
    }
}

void rs_sf_planefit::test_planecandidate(vec_plane& plane_candidates)
{
    // test each plane candidate
    for (auto& plane : plane_candidates)
    {
        plane.pts.clear();
        m_inlier_buf.clear();

        for (auto& p : m_pt_cloud)
        {
            if (is_valid_pt3d(p))
            {
                const float nor_fit = std::abs(plane.normal.dot(p.normal));
                if (nor_fit > m_param.max_normal_thr)
                {
                    const float fit_err = std::abs(plane.normal.dot(p.pos) + plane.d);
                    if (fit_err < m_param.max_fit_err_thr)
                        m_inlier_buf.push_back(&p);
                }
            }
        }

        if (m_inlier_buf.size() >= m_param.min_num_plane_pt)
        {
            plane.pts.reserve(m_inlier_buf.size());
            plane.pts = m_inlier_buf;
        }
    }
}

void rs_sf_planefit::non_max_plane_suppression(vec_plane& plane_candidates)
{
    for (auto& plane : plane_candidates)
    {
        const int this_plane_size = (int)plane.pts.size();
        for (auto& pt : plane.pts)
        {
            if (!pt->best_plane ||
                (int)pt->best_plane->pts.size() < this_plane_size)
                pt->best_plane = &plane;
        }
    }

    // clear point list for each plane 
    for (auto& plane : plane_candidates)
        plane.pts.clear(); 

    // rebuild point list for each plane based on the best fitting
    for (auto& pt : m_pt_cloud)
        if ( pt.best_plane )
            pt.best_plane->pts.push_back(&pt);

    // id each plane
    std::sort(plane_candidates.begin(), plane_candidates.end(), [](const plane& p0, const plane& p1) { return p0.pts.size() > p1.pts.size(); });
    int pid = 0;
    for (auto& plane : plane_candidates)
    {
        plane.pid = pid++;
        if (pid > 255)
            plane.pid = pid = 255;
    }

    cv::Mat disp;
    cv::Mat(ref_img.img_h, ref_img.img_w, CV_16U, ref_img.data).convertTo(disp, CV_8U, 255 / 4000.0);

    for (int i = 0; i < 3; ++i)
    {
        std::string name = "display" + std::to_string(i);
        cv::Mat plane_image = disp.clone();
        for (auto& pt : plane_candidates[i].pts)
        {
            cv::circle(plane_image, cv::Point(pt->px%ref_img.img_w, pt->px / ref_img.img_w), m_param.img_x_dn_sample / 2 +1,
                255, -1, CV_AA);
        }
        cv::imshow(name, plane_image);
        cv::waitKey(1);
    }

    cv::waitKey(0);
}
