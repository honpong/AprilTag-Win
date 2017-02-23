#include <opencv2/opencv.hpp>
#include "rs_sf_planefit.h"

rs_sf_planefit::rs_sf_planefit(const rs_sf_intrinsics * camera)
{
    m_intrinsics = *camera;

    m_param.point_cloud_reserve =
        (m_intrinsics.img_h * m_intrinsics.img_w + 1) /
        (m_param.img_x_dn_sample*m_param.img_y_dn_sample);

    m_inlier_buf.reserve(m_param.point_cloud_reserve);
    m_tracked_pid.reserve(m_param.max_num_plane_output);
}

rs_sf_status rs_sf_planefit::process_depth_image(const rs_sf_image * img)
{
    if (!img || img->byte_per_pixel != 2) return RS_SF_INVALID_ARG;

    // for debug
    ref_img = img[1];
   
    //reset
    m_view.clear();
    m_ref_scene.clear();

    //plane fitting
    image_to_pointcloud(img, m_view.pt_cloud);
    img_pointcloud_to_normal(m_view.pt_cloud);
    img_pointcloud_to_planecandidate(m_view.pt_cloud, m_view.planes);
    grow_planecandidate(m_view.pt_cloud, m_view.planes);
    //test_planecandidate(m_view.pt_cloud, m_view.planes);
    non_max_plane_suppression(m_view.pt_cloud, m_view.planes);
    sort_plane_size(m_view.planes);

    //debug drawing
    visualize(m_view.planes);

    return RS_SF_SUCCESS;
}

rs_sf_status rs_sf_planefit::track_depth_image(const rs_sf_image *img)
{
    if (!img || img->byte_per_pixel != 2) return RS_SF_INVALID_ARG;

    // for debug 
    ref_img = img[1];

    // save history
    m_ref_scene.planes.swap(m_view.planes);
    m_ref_scene.pt_cloud.swap(m_view.pt_cloud);
    m_view.clear();

    // preprocessing
    image_to_pointcloud(img, m_view.pt_cloud);
    img_pointcloud_to_normal(m_view.pt_cloud);

    // search for old planes
    find_candidate_plane_from_past(m_view, m_ref_scene);
    //img_pointcloud_to_planecandidate(m_view.pt_cloud, m_view.planes);
    grow_planecandidate(m_view.pt_cloud, m_view.planes);
    non_max_plane_suppression(m_view.pt_cloud, m_view.planes);
    combine_planes_from_the_same_past(m_view, m_ref_scene);
    sort_plane_size(m_view.planes);
    assign_planes_pid(m_view.planes);

    // debug drawing
    visualize(m_view.planes);

    return RS_SF_SUCCESS;
}

int rs_sf_planefit::num_detected_planes() const
{
    int n = 0; 
    for (auto& pl : m_view.planes) 
        if (pl.best_pts.size() > 0) 
            ++n; 
    return n;
}

bool rs_sf_planefit::is_valid_pt3d(const pt3d& pt)
{
    return pt.pos.z() > m_param.min_z_value;
}

void rs_sf_planefit::image_to_pointcloud(const rs_sf_image * img, vec_pt3d& pt_cloud)
{
    const int img_h = m_intrinsics.img_h;
    const int img_w = m_intrinsics.img_w;
    const auto px = m_intrinsics.cam_px;
    const auto py = m_intrinsics.cam_py;
    const auto fx = m_intrinsics.cam_fx;
    const auto fy = m_intrinsics.cam_fy;
    const auto src_depth = (unsigned short*)img->data;

    const int y_step = m_param.img_y_dn_sample;
    const int x_step = m_param.img_x_dn_sample;

    pt_cloud.clear();
    pt_cloud.reserve(m_param.point_cloud_reserve);
    for (int y = 0, p = 0, pp=0; y < img_h; y += y_step, p = y*img_w)
    {
        for (int x = 0; x < img_w; x += x_step, p += x_step, ++pp)
        {
            const auto z = (float)src_depth[p];
            pt_cloud.emplace_back(v3{
                z * (x - px) / fx,
                z * (y - py) / fy,
                z }
            , v3(), p, pp, nullptr);
        }
    }
}

void rs_sf_planefit::img_pointcloud_to_normal(vec_pt3d& img_pt_cloud)
{
    const int img_h = m_intrinsics.img_h / m_param.img_y_dn_sample;
    const int img_w = m_intrinsics.img_w / m_param.img_x_dn_sample;

    auto* src_pt_cloud = img_pt_cloud.data();
    for (int y = 0, ey = img_h - 1, p = 0; y < ey; ++y, p = y*img_w) {
        for (int x = 0, ex = img_w - 1; x < ex; ++x, ++p)
        {
            auto& src_pt_cloud_p = src_pt_cloud[p];
            if (is_valid_pt3d(src_pt_cloud_p))
            {
                const auto p_right = p + 1, p_down = p + img_w;
                auto dx = src_pt_cloud[p_right].pos - src_pt_cloud_p.pos;
                auto dy = src_pt_cloud[p_down].pos - src_pt_cloud_p.pos;
                src_pt_cloud_p.normal = dy.cross(dx).normalized();
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
                img_planes.emplace_back(nor, d, &src);
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

void rs_sf_planefit::test_planecandidate(vec_pt3d& pt_cloud, vec_plane& plane_candidates)
{
    // test each plane candidate
    for (auto& plane : plane_candidates)
    {
        plane.pts.clear();
        m_inlier_buf.clear();

        for (auto& p : pt_cloud)
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
    const int img_h = m_intrinsics.img_h / m_param.img_y_dn_sample;
    const int img_w = m_intrinsics.img_w / m_param.img_x_dn_sample;
    const auto is_within_fov = [h=img_h, w=img_w](const int& x, const int& y)
    {
        return 0 <= x && x < w && 0 <= y && y < h;
    };

    auto* src_img_point = img_pt_cloud.data();
    for (auto& plane : plane_candidates)
    {
        plane.pts.reserve(m_param.point_cloud_reserve >> 1);
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
            const int x = p->ppx % img_w, y = p->ppx / img_w;
            const int xb[] = { x - 1, x + 1, x, x };
            const int yb[] = { y, y, y - 1, y + 1 };
            const int pb[] = { p->ppx - 1, p->ppx + 1, p->ppx - img_w, p->ppx + img_w };
            
            // grow
            for (int b = 0; b < 4; ++b)
            {
                if (is_within_fov(xb[b], yb[b])) //within fov
                {
                    auto& pt = src_img_point[pb[b]];
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

void rs_sf_planefit::non_max_plane_suppression(vec_pt3d& pt_cloud, vec_plane& plane_candidates)
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

        // clear point list for each plane 
        plane.best_pts.clear();
        plane.best_pts.reserve(this_plane_size);
    }

    // rebuild point list for each plane based on the best fitting
    for (auto& pt : pt_cloud)
        if ( pt.best_plane )
            pt.best_plane->best_pts.push_back(&pt);
}

void rs_sf_planefit::sort_plane_size(vec_plane & planes)
{
    std::sort(planes.begin(), planes.end(),
        [](const plane& p0, const plane& p1) { return p0.best_pts.size() > p1.best_pts.size(); });
}

void rs_sf_planefit::find_candidate_plane_from_past(scene & current_view, scene & past_view)
{
    const int src_w = m_intrinsics.img_w;
    const int img_h = m_intrinsics.img_h / m_param.img_y_dn_sample;
    const int img_w = m_intrinsics.img_w / m_param.img_x_dn_sample;

    current_view.planes.clear();
    current_view.planes.reserve(past_view.planes.size());
    auto& pt_cloud = current_view.pt_cloud;
    const int dn_xy = m_param.img_x_dn_sample*m_param.img_y_dn_sample;

    // for each past plane
    for (auto& past_plane : past_view.planes)
    {
        if (past_plane.best_pts.size() < m_param.min_num_plane_pt) continue;

        v3 avg_normal(0, 0, 0), avg_inlier_normal(0, 0, 0), avg_inlier_pos(0, 0, 0);
        int n_valid_current_pt = 0;
        
        // for each past plane points
        for (auto& past_pt : past_plane.pts)
        {
            // check normal in current view           
            auto& current_pt = pt_cloud[past_pt->ppx];
            if (is_valid_pt3d(current_pt))
            {
                avg_normal += current_pt.normal;
                n_valid_current_pt++;
            }
        }

        if (n_valid_current_pt <= 0) continue;
        avg_normal *= (1.0f / n_valid_current_pt);

        // binning
        m_inlier_buf.clear();
        float min_dist_to_past_center = std::numeric_limits<float>::max();
        pt3d* best_inlier_to_past_center = nullptr;

        for (auto& past_pt : past_plane.pts)
        {
            // test point            
            auto& current_pt = pt_cloud[past_pt->ppx];

            if (is_valid_pt3d(current_pt))
            {
                if (std::abs(avg_normal.dot(current_pt.normal)) > m_param.max_normal_thr)                
                {
                    avg_inlier_normal += current_pt.normal;
                    avg_inlier_pos += current_pt.pos;
                    m_inlier_buf.push_back(&current_pt);

                    const auto dist = (past_plane.src->pos - current_pt.pos).squaredNorm();
                    if (dist < min_dist_to_past_center)
                    {
                        min_dist_to_past_center = dist;
                        best_inlier_to_past_center = &current_pt;
                    }

                    if ((past_pt->px % src_w) % (m_param.candidate_x_dn_sample * 4) == 0 &&
                        (past_pt->px / src_w) % (m_param.candidate_y_dn_sample * 4) == 0)
                    {
                        auto d = -current_pt.pos.dot(current_pt.normal);
                        current_view.planes.emplace_back(current_pt.normal, d, &current_pt, INVALID_PID, &past_plane);
                    }
                }
            }
        }

        auto center = best_inlier_to_past_center;
        if (center)
        {
            Eigen::Matrix3f em; em.setZero();
            for (auto& pt : m_inlier_buf)
            {
                const auto dp = (pt->pos - center->pos);
                em += dp * dp.transpose();
            }

            float D[6] = { em(0,0),em(0,1),em(0,2),em(1,1),em(1,2),em(2,2) }, u[3], v[3][3];
            eigen_3x3_real_symmetric(D, u, v);
            v3 normal(v[2][0], v[2][1], v[2][2]);

            current_view.planes.emplace_back(normal, -center->pos.dot(normal), center, INVALID_PID, &past_plane);
        }
    }
#ifdef _DEBUG
    printf("num plane %d -> %d \n", (int)past_view.planes.size(), (int)current_view.planes.size());
#endif
}

void rs_sf_planefit::combine_planes_from_the_same_past(scene & current_view, scene & past_view)
{
    for (auto& past_plane : past_view.planes)
    {
        list_plane_ref child_planes;
        for (auto& new_plane : current_view.planes)
        {
            if (new_plane.past_plane == &past_plane)
            {
                child_planes.push_back(&new_plane);
            }
        }

        plane* best_child = nullptr;
        int max_child_size = 0;
        for (auto& new_plane : child_planes)
        {
            if ((int)new_plane->best_pts.size() > max_child_size)
            {
                best_child = new_plane;
                max_child_size = (int)best_child->best_pts.size();
            }
        }

        if (best_child)
        {
            auto& best_child_pt = best_child->best_pts;
            auto& child_pt = best_child->pts;
            for (auto& new_plane : child_planes)
            {
                if (new_plane == best_child) continue;
                if (is_inlier(*best_child, *new_plane->src))
                {
                    auto& new_best_pt = new_plane->best_pts;
                    if (new_best_pt.size() > 0)
                    {
                        best_child_pt.insert(best_child_pt.end(), new_best_pt.begin(), new_best_pt.end());
                        child_pt.insert(child_pt.end(), new_best_pt.begin(), new_best_pt.end());
                        new_plane->best_pts.clear();
                    }
                    new_plane->pts.clear();
                }
            }
        }
    }
}

void rs_sf_planefit::assign_planes_pid(vec_plane& planes)
{
    m_tracked_pid.resize(MAX_VALID_PID + 1);
    std::fill_n(m_tracked_pid.begin(), MAX_VALID_PID + 1, nullptr);
    
    for (auto& plane : planes)
    {
        if (plane.past_plane && plane.best_pts.size() > 0 && plane.past_plane->pid != INVALID_PID)
        {
            if (!m_tracked_pid[plane.past_plane->pid])
            {
                m_tracked_pid[plane.pid = plane.past_plane->pid] = &plane;
            }
        }
    }

    int next_pid = 0, max_current_pid = std::min(MAX_VALID_PID, (int)planes.size()-1);
    for (int i = 0; i <= max_current_pid; ++i)
    {
        if (planes[i].pid == INVALID_PID)
        {
            while (m_tracked_pid[next_pid]) { if (++next_pid > MAX_VALID_PID) return; }
            m_tracked_pid[planes[i].pid = next_pid] = &planes[i];
        }
    }
}

static int display_time = 0;
void rs_sf_planefit::visualize(vec_plane & planes)
{
    static const cv::Vec3b src_color[] = {
        cv::Vec3b(255,0,0),
        cv::Vec3b(0,255,0),
        cv::Vec3b(0,0,255),
        cv::Vec3b(255,0,255),
        cv::Vec3b(255,255,0),
        cv::Vec3b(0,255,255),
    };
    int nColor = std::min((int)(sizeof(src_color)/sizeof(cv::Vec3b)), (int)planes.size());
    cv::Mat ref(ref_img.img_h, ref_img.img_w, CV_8U, ref_img.data);
    cv::Mat disp, mask = cv::Mat::zeros(ref.rows / m_param.img_x_dn_sample, ref.cols / m_param.img_y_dn_sample, CV_8UC3);
    cv::cvtColor(ref, disp, CV_GRAY2RGB, 3);
    std::string name = "planes_" + std::to_string(display_time++);


    for (int i = nColor - 1; i >= 0; --i)
    {
        auto& plane = planes[i];
        if (0 <= plane.pid && plane.pid < nColor)
        {
            for (auto& pt : plane.best_pts)
            {
                mask.at<cv::Vec3b>(pt->ppx) = src_color[plane.pid];
            }
        }
    }

    //cv::medianBlur(mask, mask, 3);
    //cv::dilate(mask, mask, cv::Mat());
    //cv::erode(mask, mask, cv::Mat());

    cv::Mat disp_clone = disp.clone();
    cv::resize(mask, mask, disp.size(), 0, 0, CV_INTER_NN);
    cv::bitwise_or(disp, mask, disp);
    cv::hconcat(disp_clone, disp, disp);
    cv::imshow("planes", disp);
    cv::waitKey(1);

    std::string path = "c:\\temp\\shapefit\\e\\";
    cv::imwrite(path + name + ".png", disp);
}


