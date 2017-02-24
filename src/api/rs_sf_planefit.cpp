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
    sort_plane_size(m_view.planes, m_sorted_plane_ptr);

    //debug drawing
    //visualize(m_view.planes);

    return RS_SF_SUCCESS;
}

rs_sf_status rs_sf_planefit::track_depth_image(const rs_sf_image *img)
{
    if (!img || img->byte_per_pixel != 2) return RS_SF_INVALID_ARG;

    // for debug 
    ref_img = img[1];

    // save previous scene
    save_current_scene_as_reference();

    // preprocessing
    image_to_pointcloud(img, m_view.pt_cloud);
    img_pointcloud_to_normal(m_view.pt_cloud);

    // search for old planes
    find_candidate_plane_from_past(m_view, m_ref_scene);
    //img_pointcloud_to_planecandidate(m_view.pt_cloud, m_view.planes);
    grow_planecandidate(m_view.pt_cloud, m_view.planes);
    non_max_plane_suppression(m_view.pt_cloud, m_view.planes);
    combine_planes_from_the_same_past(m_view, m_ref_scene);
    sort_plane_size(m_view.planes, m_sorted_plane_ptr);
    assign_planes_pid(m_sorted_plane_ptr);

    // debug drawing
    //visualize(m_view.planes);

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

rs_sf_status rs_sf_planefit::get_plane_index_map(rs_sf_image * map, int hole_filled) const
{
    if (!map || !map->data || map->byte_per_pixel != 1) return RS_SF_INVALID_ARG;
    upsize_pt_cloud_to_plane_map(m_view.pt_cloud, map);
    return RS_SF_SUCCESS;
}

bool rs_sf_planefit::is_valid_pt3d(const pt3d& pt)
{
    return pt.pos.z() > m_param.min_z_value && pt.pos.z() < m_param.max_z_value;
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
    }

    // reserve memory
    for (auto& plane : plane_candidates)
    {
        plane.best_pts.clear();
        plane.best_pts.reserve(plane.pts.size());
    }

    // rebuild point list for each plane based on the best fitting
    for (auto& pt : pt_cloud)
        if (pt.best_plane)
            pt.best_plane->best_pts.push_back(&pt);
}

void rs_sf_planefit::sort_plane_size(vec_plane & planes, vec_plane_ref& sorted_planes)
{
    sorted_planes.reserve(planes.size());
    sorted_planes.clear();

    for (auto& plane : planes)
        sorted_planes.push_back(&plane);

    std::sort(sorted_planes.begin(), sorted_planes.end(),
        [](const plane* p0, const plane* p1) { return p0->best_pts.size() > p1->best_pts.size(); });
}

void rs_sf_planefit::save_current_scene_as_reference()
{
    // remove tails
    if (m_param.keep_previous_plane_pts && !m_ref_scene.pt_cloud.empty()) {
        for (auto& plane : m_view.planes) {
            while (!plane.best_pts.empty()) {
                if (m_ref_scene.pt_cloud[plane.best_pts.back()->ppx].best_plane == &plane) {
                    plane.best_pts.back()->best_plane = nullptr;
                    plane.best_pts.pop_back();
                }
                else break;
            }
        }
    }

    // save history
    m_view.swap(m_ref_scene);
    m_view.clear();
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
                        best_child_pt.reserve(best_child_pt.size() + new_best_pt.size());
                        child_pt.reserve(child_pt.size() + new_best_pt.size());
                        for (auto& transfer_pt : new_best_pt)
                        {
                            best_child_pt.push_back(transfer_pt);
                            child_pt.push_back(transfer_pt);
                            transfer_pt->best_plane = best_child;
                        }
                    }
                    new_plane->best_pts.clear();
                    new_plane->pts.clear();
                }
            }

            if (m_param.keep_previous_plane_pts)
            {
                auto& new_pt = m_view.pt_cloud;
                for (auto& past_pt : past_plane.best_pts)
                {
                    auto& overlay = new_pt[past_pt->ppx];
                    if (overlay.best_plane == nullptr) //a potential hole
                    {
                        past_pt->best_plane = best_child;
                        overlay.best_plane = best_child;
                        best_child_pt.push_back(&overlay);
                    }
                }
            }
        }
    }
}

void rs_sf_planefit::assign_planes_pid(vec_plane_ref& sorted_planes)
{
    m_tracked_pid.resize(m_param.max_num_plane_output);
    std::fill_n(m_tracked_pid.begin(), m_param.max_num_plane_output, nullptr);

   for ( auto plane : sorted_planes)
    {
        if (plane->past_plane && plane->best_pts.size() > 0 && is_tracked_pid(plane->past_plane->pid))
        {
            if (!m_tracked_pid[plane->past_plane->pid]) //assign old pid to new plane
            {
                m_tracked_pid[plane->pid = plane->past_plane->pid] = plane;
            }
        }
    }

    int next_pid = 0;
    for (auto& plane : sorted_planes)
    {
        if (!is_tracked_pid(plane->pid)) //no previous plane detected
        {
            while (m_tracked_pid[next_pid]) { //find next unused pid
                if (++next_pid >= m_param.max_num_plane_output)
                    return;
            }
            m_tracked_pid[plane->pid = next_pid] = plane; //assign new pid
        }
    }
}

bool rs_sf_planefit::is_tracked_pid(int pid)
{
    return pid != INVALID_PID && pid <= MAX_VALID_PID;
}

void rs_sf_planefit::upsize_pt_cloud_to_plane_map(const vec_pt3d & img_pt_cloud, rs_sf_image * dst) const
{
    int dst_w = dst->img_w, dst_h = dst->img_h;
    int src_w = m_intrinsics.img_w / m_param.img_x_dn_sample;
    int src_h = m_intrinsics.img_h / m_param.img_y_dn_sample;
    
    auto src_pt_cloud = img_pt_cloud.data();
    for (int y = 0, p = 0; y < dst_h; ++y) {
        for (int x = 0; x < dst_w; ++x, ++p)
        {
            auto x_src = x * src_w / dst_w;
            auto y_src = y * src_h / dst_h;
            const auto best_plane = src_pt_cloud[y_src*src_w + x_src].best_plane;
            if (best_plane != nullptr)
                dst->data[p] = best_plane->pid + 1;
            else
                dst->data[p] = 0;
        }
    }
}