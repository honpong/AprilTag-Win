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
    ref_img = img[1];
   
    //reset
    m_pt_cloud.clear();
    m_plane_candidates.clear();

    //plane fitting
    image_to_pointcloud(img, m_pt_cloud);
    img_pointcloud_to_normal(m_pt_cloud);
    img_pointcloud_to_planecandidate(m_pt_cloud, m_plane_candidates);
    grow_planecandidate(m_pt_cloud, m_plane_candidates);
    //test_planecandidate(m_plane_candidates);
    non_max_plane_suppression(m_plane_candidates);


    //debug drawing
    visualize(m_plane_candidates);

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

void rs_sf_planefit::img_pointcloud_to_normal(vec_pt3d& img_pt_cloud)
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

void rs_sf_planefit::img_pointcloud_to_planecandidate(
    vec_pt3d& img_pt_cloud,
    vec_plane& img_planes)
{
    const int img_h = m_intrinsics.img_h / m_param.img_y_dn_sample;
    const int img_w = m_intrinsics.img_w / m_param.img_x_dn_sample;

    const int y_step = m_param.candidate_y_dn_sample / m_param.img_y_dn_sample;
    const int x_step = m_param.candidate_x_dn_sample / m_param.img_x_dn_sample;

    auto* src_img_point = img_pt_cloud.data();

    for (int y = 0, ey = img_h - 1, p = 0; y < ey; y += y_step, p = y*img_w) {
        for (int x = 0, ex = img_w - 1; x < ex; x += x_step, p += x_step)
        {
            auto& src = src_img_point[p];
            auto& pos = src.pos;
            auto& nor = src.normal;
            if (is_valid_pt3d(src))
            {
                float d = -nor.dot(pos);
                img_planes.push_back({ nor, d, &src, vec_pt_ref(), vec_pt_ref(), 0 });
            }
        }
    }
}

bool rs_sf_planefit::is_inlier(const plane & candidate, const pt3d & p)
{
    if (is_valid_pt3d(p))
    {
        const float nor_fit = std::abs(candidate.normal.dot(p.normal));
        if (nor_fit > m_param.max_normal_thr)
        {
            const float fit_err = std::abs(candidate.normal.dot(p.pos) + candidate.d);
            if (fit_err < m_param.max_fit_err_thr)
                return true;
        }
    }
    return false;
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
           if (is_inlier(plane, p))
                m_inlier_buf.push_back(&p);
        }

        if (m_inlier_buf.size() >= m_param.min_num_plane_pt)
        {
            plane.pts.reserve(m_inlier_buf.size());
            plane.pts = m_inlier_buf;
        }
    }
}


void rs_sf_planefit::grow_planecandidate(vec_pt3d& img_pt_cloud, vec_plane& plane_candidates)
{
    //assume img_pt_cloud is from a 2D grid
    const int src_h = m_intrinsics.img_h;
    const int src_w = m_intrinsics.img_w;
    const int dn_x = m_param.img_x_dn_sample;
    const int dn_y = m_param.img_y_dn_sample;
    const int img_h = src_h / dn_y;
    const int img_w = src_w / dn_x;
    const auto is_within_fov = [h=img_h, w=img_w](const int& x, const int& y)
    {
        return 0 <= x && x < w && 0 <= y && y < h;
    };

    auto* src_img_point = img_pt_cloud.data();
    for (auto& plane : plane_candidates)
    {
        plane.pts.reserve(m_param.point_cloud_reserve);
        plane.pts.clear();
        m_inlier_buf.clear();

        m_inlier_buf.push_back(plane.src);
        plane.src->best_plane = &plane;
        while (!m_inlier_buf.empty())
        {
            // new inlier point
            const auto& p = m_inlier_buf.back();
            m_inlier_buf.pop_back();

            // store this inlier
            plane.pts.push_back(p);

            // neighboring points
            const int x = (p->px % src_w) / dn_x, y = (p->px / src_w) / dn_y;
            const int xb[] = { x - 1, x + 1, x,x };
            const int yb[] = { y,y,y - 1,y + 1 };
            
            // grow
            for (int b = 0; b < 4; ++b)
            {
                if (is_within_fov(xb[b], yb[b])) //within fov
                {
                    auto& pt = src_img_point[yb[b] * img_w + xb[b]];
                    if (!pt.best_plane && is_inlier(plane, pt)) //accept plane point
                    {
                        pt.best_plane = &plane;
                        m_inlier_buf.push_back(&pt);
                    }
                }
            }
        }

        // reset marker for next plane candidate
        for (auto& p : plane.pts)
            p->best_plane = nullptr;

        if (plane.pts.size() < m_param.min_num_plane_pt)
        {
            plane.pts.clear();
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
        plane.best_pts.clear();

    // rebuild point list for each plane based on the best fitting
    for (auto& pt : m_pt_cloud)
        if ( pt.best_plane )
            pt.best_plane->best_pts.push_back(&pt);

    // id each plane
    std::sort(plane_candidates.begin(), plane_candidates.end(), 
        [](const plane& p0, const plane& p1) { return p0.best_pts.size() > p1.best_pts.size(); });
    int pid = 0;
    for (auto& plane : plane_candidates)
    {
        plane.pid = pid++;
        if (pid > m_param.max_num_plane_output)
            plane.pid = pid = m_param.max_num_plane_output;
    }
}

void rs_sf_planefit::visualize(vec_plane & plane_candidates)
{
    cv::Mat ref(ref_img.img_h, ref_img.img_w, CV_8U, ref_img.data);
    cv::Mat disp, mask = cv::Mat::zeros(ref.rows / m_param.img_x_dn_sample, ref.cols / m_param.img_y_dn_sample, CV_8UC3);
    cv::cvtColor(ref, disp, CV_GRAY2RGB, 3);
    std::string name = "display";

    for (int i = 3; i >= 0; --i)
    {
        for (auto& pt : plane_candidates[i].pts)
        {
            auto& pixel = mask.at<cv::Vec3b>((cv::Point((pt->px%ref_img.img_w) / m_param.img_x_dn_sample, (pt->px / ref_img.img_w) / m_param.img_y_dn_sample)));
            if (i < 3) pixel[i] = 255;
            else {
                pixel[i % 3] = 255; pixel[2] = 255;
            }
        }
    }
    //cv::medianBlur(mask, mask, 3);

    cv::Mat disp_clone = disp.clone();
    cv::resize(mask, mask, disp.size(), 0, 0, CV_INTER_NN);
    cv::bitwise_or(disp, mask, disp);
    cv::hconcat(disp_clone, disp, disp);
    cv::imshow(name, disp);
    cv::waitKey(0);
}


