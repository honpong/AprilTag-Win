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
    if (!img || !img->data || img->byte_per_pixel != 2 ) return RS_SF_INVALID_ARG;

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
    assign_planes_pid(m_sorted_plane_ptr);

    return RS_SF_SUCCESS;
}

rs_sf_status rs_sf_planefit::track_depth_image(const rs_sf_image *img)
{
    if (!img || !img->data || img->byte_per_pixel != 2) return RS_SF_INVALID_ARG;

    // for debug 
    ref_img = img[1];

    // save previous scene
    save_current_scene_as_reference();

    // preprocessing
    image_to_pointcloud(img, m_view.pt_cloud);
    img_pointcloud_to_normal(m_view.pt_cloud);

    // search for old planes
    find_candidate_plane_from_past(m_view, m_ref_scene);
    grow_planecandidate(m_view.pt_cloud, m_view.planes);
    non_max_plane_suppression(m_view.pt_cloud, m_view.planes);
    combine_planes_from_the_same_past(m_view, m_ref_scene);
    sort_plane_size(m_view.planes, m_sorted_plane_ptr);
    assign_planes_pid(m_sorted_plane_ptr);

    return RS_SF_SUCCESS;
}

int rs_sf_planefit::num_detected_planes() const
{
    int n = 0; 
    for (auto& pl : m_view.planes) 
        if (pl.best_pts.size() >= m_param.min_num_plane_pt) 
            ++n; 
    return n;
}

rs_sf_status rs_sf_planefit::get_plane_index_map(rs_sf_image * map, int hole_filled) const
{
    if (!map || !map->data || map->byte_per_pixel != 1) return RS_SF_INVALID_ARG;
    upsize_pt_cloud_to_plane_map(m_view.pt_cloud, map);

    if (hole_filled>0 || m_param.hole_fill_plane_map)
    {
        auto* idx = map->data;
        int img_w = map->img_w, img_h = map->img_h;
        const unsigned char VISITED = 255, NO_PLANE = 0;

        std::vector<unsigned char> hole_map;
        std::vector<Eigen::Matrix<int,4,1>> next_list;
        hole_map.assign(idx, idx + map->num_pixel());
        next_list.reserve(map->num_pixel());

        // vertical image frame 
        for (int y = 0, x0 = 0, xn = img_w - 1, p = 0; y < img_h; ++y) {
            if (idx[p = (y*img_w + x0)] == NO_PLANE) next_list.emplace_back(x0, y, p, NO_PLANE);
            if (idx[p = (y*img_w + xn)] == NO_PLANE) next_list.emplace_back(xn, y, p, NO_PLANE);
        }
        // horizontal image frame
        for (int x = 0, y0 = 0, yn = img_h - 1, p = 0; x < img_w; ++x) {
            if (idx[p = (y0*img_w + x)] == NO_PLANE) next_list.emplace_back(x, y0, p, NO_PLANE);
            if (idx[p = (yn*img_w + x)] == NO_PLANE) next_list.emplace_back(x, yn, p, NO_PLANE);
        }
        for (auto& pt : next_list)
            hole_map[pt[2]] = VISITED;

        // fill background
        const int xb[] = { -1,1,0,0 };
        const int yb[] = { 0,0,-1,1 };
        const int pb[] = { -1,1,-img_w,img_w };
        while (!next_list.empty())
        {
            const auto pt = next_list.back();
            next_list.pop_back();

            for (int b = 0; b < 4; ++b)
            {
                const int x = pt[0] + xb[b], y = pt[1] + yb[b];
                if (0 <= x && x < img_w && 0 <= y && y < img_h)
                {
                    const auto p = pt[2] + pb[b];
                    if (hole_map[p] == NO_PLANE) {
                        hole_map[p] = VISITED; 
                        next_list.emplace_back(x, y, p, NO_PLANE);
                    }
                }
            }
        }

        //pick up planes
        for (int y = 0, p = 0, pid; y < img_h; ++y)
            for (int x = 0; x < img_w; ++x, ++p)
                if (((pid = hole_map[p]) != NO_PLANE) && pid != VISITED)
                    next_list.emplace_back(x, y, p, pid);

        //grow planes
        while (!next_list.empty())
        {
            auto pt = next_list.back();
            next_list.pop_back();

            for (int b = 0; b < 4; ++b)
            {
                const int x = pt[0] + xb[b], y = pt[1] + yb[b];
                if (0 <= x && x < img_w && 0 <= y && y < img_h)
                {
                    const auto p = pt[2] + pb[b];
                    auto& hole_p = hole_map[p];
                    if (idx[p] == NO_PLANE && hole_p != VISITED && (hole_p == NO_PLANE || hole_p > pt[3]))
                    {
                        hole_p = (unsigned char)pt[3];
                        next_list.emplace_back(x, y, p, pt[3]);
                    }
                }
            }
        }

        //fill holes
        for (int p = map->num_pixel() - 1; p >= 0; --p)
            if (idx[p] == NO_PLANE && hole_map[p] != VISITED)
                idx[p] = hole_map[p];

    }
    return RS_SF_SUCCESS;
}

bool rs_sf_planefit::is_valid_pt3d_z(const pt3d& pt) const 
{
    return pt.pos.z() > m_param.min_z_value && pt.pos.z() < m_param.max_z_value;
}

bool rs_sf_planefit::is_valid_pt3d_normal(const pt3d& pt) const
{
    return pt.normal[0] != 0 || pt.normal[1] != 0 || pt.normal[2] != 0;
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
            , v3(0,0,0), p, pp, nullptr);
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
            auto& src_pt = src_pt_cloud[p];
            auto& right_pt = src_pt_cloud[p + 1];
            auto& down_pt = src_pt_cloud[p + img_w];
            if (is_valid_pt3d_z(src_pt) && is_valid_pt3d_z(right_pt) && is_valid_pt3d_z(down_pt))
            {
                auto dx = right_pt.pos - src_pt.pos;
                auto dy = down_pt.pos - src_pt.pos;
                src_pt.normal = dy.cross(dx).normalized();
            }
            else
                src_pt.normal = v3{ 0,0,0 };
        }
    }
}

void rs_sf_planefit::img_pointcloud_to_planecandidate(
    vec_pt3d& img_pt_cloud,
    vec_plane& img_planes,
    int candidate_y_dn_sample,
    int candidate_x_dn_sample)
{
    const int img_h = m_intrinsics.img_h / m_param.img_y_dn_sample;
    const int img_w = m_intrinsics.img_w / m_param.img_x_dn_sample;

    if (candidate_x_dn_sample == -1) candidate_x_dn_sample = m_param.candidate_x_dn_sample;
    if (candidate_y_dn_sample == -1) candidate_y_dn_sample = m_param.candidate_y_dn_sample;
       
    const int y_step = candidate_y_dn_sample / m_param.img_y_dn_sample;
    const int x_step = candidate_x_dn_sample / m_param.img_x_dn_sample;

    auto* src_img_point = img_pt_cloud.data();

    for (int y = 0, ey = img_h - 1, p = 0; y < ey; y += y_step, p = y*img_w) {
        for (int x = 0, ex = img_w - 1; x < ex; x += x_step, p += x_step)
        {
            auto& src = src_img_point[p];
            auto& pos = src.pos;
            auto& nor = src.normal;
            if (is_valid_pt3d_normal(src))
            {
                float d = -nor.dot(pos);
                img_planes.emplace_back(nor, d, &src);
            }
        }
    }
}

bool rs_sf_planefit::is_inlier(const plane & candidate, const pt3d & p)
{
    if (is_valid_pt3d_normal(p) )
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

    // coarse grid candidate sampling
    img_pointcloud_to_planecandidate(
        current_view.pt_cloud, current_view.planes,
        m_param.track_x_dn_sample, m_param.track_y_dn_sample);

    // assign past plane reference if available
    for (auto& plane_candidate : current_view.planes)
        plane_candidate.past_plane = past_view.pt_cloud[plane_candidate.src->ppx].best_plane;

    // for each past plane
    for (auto& past_plane : past_view.planes)
    {
        if (past_plane.best_pts.size() < m_param.min_num_plane_pt) continue;

        v3 avg_normal(0, 0, 0), avg_inlier_normal(0, 0, 0), avg_inlier_pos(0, 0, 0);
        int n_valid_current_pt = 0;
        
        // for each past plane points
        for (auto& past_pt : past_plane.best_pts)
        {
            // check normal in current view           
            auto& current_pt = pt_cloud[past_pt->ppx];
            if (is_valid_pt3d_normal(current_pt))
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

        for (auto& past_pt : past_plane.best_pts)
        {
            // test point            
            auto& current_pt = pt_cloud[past_pt->ppx];

            if (is_valid_pt3d_normal(current_pt))
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

            //float D[6] = { em(0,0),em(0,1),em(0,2),em(1,1),em(1,2),em(2,2) }, ui[3], vi[3][3];
            //eigen_3x3_real_symmetric(D, ui, vi);
            //v3 normal(vi[0][0], vi[0][1], vi[0][2]);
            //if (std::abs(ui[2]) < std::min(std::abs(ui[1]), std::abs(ui[0]))) normal = v3(vi[1][0], vi[1][1], vi[1][2]);
            //else if (std::abs(ui[1]) < std::abs(ui[0])) normal = v3(vi[2][0], vi[2][1], vi[2][2]);

            Eigen::EigenSolver<Eigen::Matrix3f> solver(em);
            auto vi = solver.eigenvectors().real();
            auto ui = solver.eigenvalues().real();
            v3 normal = vi.col(0);
            if (std::abs(ui[2]) < std::min(std::abs(ui[1]), std::abs(ui[0]))) normal = vi.col(2);
            else if (std::abs(ui[1]) < std::abs(ui[0])) normal = vi.col(1);

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
        if (plane->past_plane && plane->best_pts.size() >= m_param.min_num_plane_pt && is_tracked_pid(plane->past_plane->pid))
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
        if (plane->best_pts.size() < m_param.min_num_plane_pt) return;

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