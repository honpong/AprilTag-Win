#include "rs_sf_planefit.h"

rs_sf_planefit::rs_sf_planefit(const rs_sf_intrinsics * camera)
{
    m_intrinsics = *camera;

    int pt_cloud_img_w = m_intrinsics.img_w / m_param.img_x_dn_sample;
    int pt_cloud_img_h = m_intrinsics.img_h / m_param.img_y_dn_sample;
    m_plane_pt_reserve = (pt_cloud_img_w + 1) * (pt_cloud_img_h + 1);
    m_track_plane_reserve = num_pixels() / (m_param.track_x_dn_sample * m_param.track_y_dn_sample);

    m_inlier_buf.reserve(m_plane_pt_reserve);
    m_tracked_pid.reserve(m_param.max_num_plane_output);
}

rs_sf_status rs_sf_planefit::process_depth_image(const rs_sf_image * img)
{
    if (!img || !img->data || img->byte_per_pixel != 2 ) return RS_SF_INVALID_ARG;
    
    // for debug
    ref_img = img[0];
   
    //reset
    m_view.clear();
    m_ref_scene.clear();

    //plane fitting
    image_to_pointcloud(img, m_view.pt_img, m_view.pt_cloud, m_view.cam_pose);
    img_pointcloud_to_normal(m_view.pt_img);
    img_pointcloud_to_planecandidate(m_view.pt_img, m_view.planes);
    grow_planecandidate(m_view.pt_img, m_view.planes);
    //test_planecandidate(m_view.pt_cloud, m_view.planes);
    non_max_plane_suppression(m_view.pt_cloud, m_view.planes);
    upsample_best_plane_ptr(m_view);
    sort_plane_size(m_view.planes, m_sorted_plane_ptr);
    assign_planes_pid(m_sorted_plane_ptr);

    return RS_SF_SUCCESS;
}

rs_sf_status rs_sf_planefit::track_depth_image(const rs_sf_image *img)
{
    if (!img || !img->data || img->byte_per_pixel != 2) return RS_SF_INVALID_ARG;

    // for debug 
    ref_img = img[0];

    // save previous scene
    save_current_scene_as_reference();

    // preprocessing
    image_to_pointcloud(img, m_view.pt_img, m_view.pt_cloud, m_view.cam_pose);
    img_pointcloud_to_normal(m_view.pt_img);

    // search for old planes
    map_candidate_plane_from_past(m_view, m_ref_scene);
    grow_planecandidate(m_view.pt_img, m_view.planes);
    non_max_plane_suppression(m_view.pt_cloud, m_view.planes);
    upsample_best_plane_ptr(m_view);
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

int rs_sf_planefit::max_detected_pid() const
{
    int max_pid = 0;
    for (auto& pl : m_view.planes)
        if (max_pid <= pl.pid) max_pid = pl.pid;
    return max_pid;
}

rs_sf_status rs_sf_planefit::get_plane_index_map(rs_sf_image * map, int hole_filled) const
{
    if (!map || !map->data || map->byte_per_pixel != 1) return RS_SF_INVALID_ARG;
    upsize_pt_cloud_to_plane_map(m_view.pt_img, map);

    if (hole_filled > 0 || (hole_filled == -1 && m_param.hole_fill_plane_map))
    {
        auto* idx = map->data;
        int img_w = map->img_w, img_h = map->img_h;
        const unsigned char VISITED = 255, NO_PLANE = 0;

        std::vector<unsigned char> hole_map;
        std::vector<Eigen::Matrix<int, 4, 1>> next_list;
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

rs_sf_status rs_sf_planefit::mark_plane_src_on_map(rs_sf_image * map) const
{
    if (!map || !map->data || map->byte_per_pixel != 1) return RS_SF_INVALID_ARG;

    const int dst_w = map->img_w;
    const float up_x = (float)dst_w / src_w(), up_y = (float)map->img_h / src_h();
    for (auto& plane : m_view.planes)
    {
        if (plane.best_pts.size() >= m_param.min_num_plane_pt)
        {
            const auto x = (plane.src->pix.x()) * up_x;
            const auto y = (plane.src->pix.y()) * up_y;
            const int x0 = (int)std::floor(x), x1 = (int)std::ceil(x);
            const int y0 = (int)std::floor(y), y1 = (int)std::ceil(y);
            map->data[y0*dst_w + x0] = PLANE_SRC_PID;
            map->data[y0*dst_w + x1] = PLANE_SRC_PID;
            map->data[y1*dst_w + x0] = PLANE_SRC_PID;
            map->data[y1*dst_w + x1] = PLANE_SRC_PID;
        }
    }

    return RS_SF_SUCCESS;
}

rs_sf_status rs_sf_planefit::get_plane_equation(int pid, float equ[4]) const
{
    if (pid <= 0 || pid >= m_tracked_pid.size()) return RS_SF_INVALID_ARG;
    if (!m_tracked_pid[pid]) return RS_SF_INVALID_ARG;

    equ[0] = m_tracked_pid[pid]->normal[0];
    equ[1] = m_tracked_pid[pid]->normal[1];
    equ[2] = m_tracked_pid[pid]->normal[2];
    equ[3] = m_tracked_pid[pid]->d;

    return RS_SF_SUCCESS;
}

i2 rs_sf_planefit::project_i(const v3 & cam_pt) const
{
    return i2{
        (int)(0.5f + (cam_pt.x() * m_intrinsics.cam_fx) / cam_pt.z() + m_intrinsics.cam_px),
        (int)(0.5f + (cam_pt.y() * m_intrinsics.cam_fy) / cam_pt.z() + m_intrinsics.cam_py)
    };
}

v3 rs_sf_planefit::unproject(const float u, const float v, const float z) const
{
    return v3{
        z * (u - m_intrinsics.cam_px) / m_intrinsics.cam_fx,
        z * (v - m_intrinsics.cam_py) / m_intrinsics.cam_fy,
        z };
}

bool rs_sf_planefit::is_within_pt_cloud_fov(const int x, const int y) const
{
    return 0 <= x && x < src_w() && 0 <= y && y < src_h();
}

bool rs_sf_planefit::is_valid_raw_z(const float z) const
{
    return z > m_param.min_z_value && z < m_param.max_z_value;
}

bool rs_sf_planefit::is_valid_pt3d_pos(const pt3d & pt) const
{
    return pt.pos[0] != 0 || pt.pos[1] != 0 || pt.pos[2] != 0;
}

bool rs_sf_planefit::is_valid_pt3d_normal(const pt3d& pt) const
{
    return pt.normal[0] != 0 || pt.normal[1] != 0 || pt.normal[2] != 0;
}

void rs_sf_planefit::image_to_pointcloud(const rs_sf_image * img, vec_pt3d& pt_img, vec_pt_ref& pt_cloud, pose_t& pose)
{
    // define coarse grid
    const int img_w = src_w();
    const int y_step = m_param.img_y_dn_sample;
    const int x_step = m_param.img_x_dn_sample;
    const int dn_step = y_step * src_w();
    const int ey = src_h() - y_step;
    const int ex = src_w() - x_step;

    pt_cloud.reserve(m_plane_pt_reserve);
    pt_img.resize(num_pixels(), pt3d(v3{}, v3{}));
    auto* src_pt_grid = pt_img.data();
    for (int y = 0, p = 0; y <= ey; y += y_step, p = y*img_w)
    {
        for (int x = 0; x <= ex; x += x_step, p += x_step)
        {
            src_pt_grid[p].p = p;
            pt_cloud.emplace_back(src_pt_grid + p);
        }
    }

    // compute point cloud
    const auto* src_depth = (unsigned short*)img->data;
    pose.set_pose(img->cam_pose);
    const float
        r00 = pose.rotation(0, 0), r01 = pose.rotation(0, 1), r02 = pose.rotation(0, 2),
        r10 = pose.rotation(1, 0), r11 = pose.rotation(1, 1), r12 = pose.rotation(1, 2),
        r20 = pose.rotation(2, 0), r21 = pose.rotation(2, 1), r22 = pose.rotation(2, 2),
        t0 = pose.translation[0], t1 = pose.translation[1], t2 = pose.translation[2],
        cam_px = m_intrinsics.cam_px, cam_py = m_intrinsics.cam_py,
        cam_fx = m_intrinsics.cam_fx, cam_fy = m_intrinsics.cam_fy;

    //pt.pos = pose.transform(unproject((float)pt.pix.x(), (float)pt.pix.y(), is_valid_raw_z(z) ? z : 0));
    auto compute_pt3d = [&](pt3d& pt) {
        const float u = (float)(pt.pix[0] = (pt.p % img_w));
        const float v = (float)(pt.pix[1] = (pt.p / img_w));
        const float z = (float)src_depth[pt.p];
        if (is_valid_raw_z(z)) {
            const float x = z * (u - cam_px) / cam_fx;
            const float y = z * (v - cam_py) / cam_fy;
            pt.pos[0] = r00 * x + r01 * y + r02 * z + t0;
            pt.pos[1] = r10 * x + r11 * y + r12 * z + t1;
            pt.pos[2] = r20 * x + r21 * y + r22 * z + t2;
        }
    };

    if (m_param.compute_full_pt_cloud)
        for (int p = num_pixels() - 1; p >= 0; --p) { compute_pt3d(src_pt_grid[p]); }
    else
        for (auto pt : pt_cloud) { compute_pt3d(*pt); }
}

void rs_sf_planefit::img_pointcloud_to_normal(vec_pt3d& img_pt_cloud)
{
    auto* src_img_pt = img_pt_cloud.data();
    const int y_step = m_param.img_y_dn_sample;
    const int x_step = m_param.img_x_dn_sample;
    const int dn_step = y_step * src_w();
    const int ey = src_h() - y_step;
    const int ex = src_w() - x_step;

    for (int y = 0, p = 0; y < ey; y += y_step, p = y*src_w())
    {
        for (int x = 0; x < ex; x += x_step, p += x_step)
        {
            auto& src_pt = src_img_pt[p];
            const auto& right_pt = src_img_pt[p + x_step];
            const auto& down_pt = src_img_pt[p + dn_step];
            if (is_valid_pt3d_pos(src_pt) &&
                is_valid_pt3d_pos(right_pt) &&
                is_valid_pt3d_pos(down_pt)) {
                const auto dx = right_pt.pos - src_pt.pos;
                const auto dy = down_pt.pos - src_pt.pos;
                src_pt.normal = dy.cross(dx).normalized();
            }
        }
    }
}

void rs_sf_planefit::img_pointcloud_to_planecandidate(
    vec_pt3d& img_pt_cloud,
    vec_plane& img_planes,
    int candidate_y_dn_sample,
    int candidate_x_dn_sample)
{
    if (candidate_x_dn_sample == -1) candidate_x_dn_sample = m_param.candidate_x_dn_sample;
    if (candidate_y_dn_sample == -1) candidate_y_dn_sample = m_param.candidate_y_dn_sample;

    img_planes.reserve(MAX_VALID_PID + 1);

    auto* src_img_point = img_pt_cloud.data();
    const int y_step = candidate_y_dn_sample;
    const int x_step = candidate_x_dn_sample;
    const int dn_step = y_step * src_w();
    const int ey = src_h() - y_step * 2;
    const int ex = src_w() - x_step * 2;

    for (int y = y_step; y < ey; y += y_step)
    {
        for (int x = x_step, p = y * src_w() + x; x < ex; x += x_step, p += x_step)
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

void rs_sf_planefit::test_planecandidate(vec_pt_ref& pt_cloud, vec_plane& plane_candidates)
{
    // test each plane candidate
    for (auto& plane : plane_candidates)
    {
        plane.pts.clear();
        m_inlier_buf.clear();

        for (auto p : pt_cloud)
        {
           if (is_inlier(plane, *p))
                m_inlier_buf.push_back(p);
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
    auto* src_img_point = img_pt_cloud.data();
    for (auto& plane : plane_candidates)
    {
        if (plane.past_plane) continue;
        if (!plane.src || plane.src->best_plane) continue;

        grow_inlier_buffer(src_img_point, plane, std::vector<pt3d*>{ plane.src });

        if (plane.pts.size() < m_param.min_num_plane_pt)
            plane.pts.clear();
    }
}

void rs_sf_planefit::grow_inlier_buffer(pt3d src_img_point[], plane & plane_candidate, std::vector<pt3d*>& seeds, bool reset_best_plane_ptr)
{
    m_inlier_buf.clear();
    m_inlier_buf.assign(seeds.begin(), seeds.end());
    for (auto p : m_inlier_buf) { p->best_plane = &plane_candidate; }
    plane_candidate.pts.clear();
    plane_candidate.pts.reserve(m_plane_pt_reserve >> 1);

    const int dx = m_param.img_x_dn_sample;
    const int dy = m_param.img_y_dn_sample;
    const int dpx = dx, dpy = dy * src_w();

    while (!m_inlier_buf.empty())
    {
        // new inlier point
        const auto pt = m_inlier_buf.back();
        m_inlier_buf.pop_back();

        // store this inlier
        plane_candidate.pts.push_back(pt);

        // neighboring points
        const int x = pt->pix.x(), y = pt->pix.y(), p = pt->p;
        const int xb[] = { x - dx, x + dx, x, x };
        const int yb[] = { y, y, y - dy, y + dy };
        const int pb[] = { p - dpx, p + dpx, p - dpy, p + dpy};

        // grow
        for (int b = 0; b < 4; ++b)
        {
            if (is_within_pt_cloud_fov(xb[b], yb[b])) //within fov
            {
                auto& pt_next = src_img_point[pb[b]];
                if (!pt_next.best_plane && is_inlier(plane_candidate, pt_next)) //accept plane point
                {
                    pt_next.best_plane = &plane_candidate;
                    m_inlier_buf.push_back(&pt_next);
                }
            }
        }
    }

    if (reset_best_plane_ptr) {
        // reset marker for next plane candidate
        for (auto p : plane_candidate.pts)
            p->best_plane = nullptr;
    }
}

void rs_sf_planefit::non_max_plane_suppression(vec_pt_ref& pt_cloud, vec_plane& plane_candidates)
{
    for (auto& plane : plane_candidates)
    {
        const int this_plane_size = (int)plane.pts.size();
        const bool is_past_plane = (plane.past_plane != nullptr);
        for (auto& pt : plane.pts)
        {
            if (!pt->best_plane)         //no assignment yet 
                pt->best_plane = &plane; //new assignment
            else if (is_past_plane) {    //is a tracked plane
                if (!pt->best_plane->past_plane || //overwrite a new plane
                    (int)pt->best_plane->pts.size() < this_plane_size) //overwrite a smaller tracked plane
                    pt->best_plane = &plane;
            }
            else if (!pt->best_plane->past_plane &&  //both are new planes
                (int)pt->best_plane->pts.size() < this_plane_size) //overwrite a smaller new plane
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
    for (auto pt : pt_cloud)
        if (pt->best_plane)
            pt->best_plane->best_pts.push_back(pt);

    // make sure the plane src is valid
    for (auto& plane : plane_candidates)
        if (plane.best_pts.size() > 0 && plane.src->best_plane != &plane)
            plane.src = plane.best_pts[0];
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

bool rs_sf_planefit::is_valid_past_plane(const plane & past_plane) const
{
    return past_plane.pid > INVALID_PID && past_plane.best_pts.size() > 0;
}

void rs_sf_planefit::save_current_scene_as_reference()
{
    // save history
    m_view.swap(m_ref_scene);
    m_view.clear();
}

void rs_sf_planefit::map_candidate_plane_from_past(scene & current_view, scene & past_view)
{
    const auto map_to_past = past_view.cam_pose.invert();
    std::vector<plane*> past_to_current_planes(MAX_VALID_PID + 1);
    std::fill_n(past_to_current_planes.begin(), past_to_current_planes.size(), nullptr);
    current_view.planes.clear();
    current_view.planes.reserve(past_view.planes.size() + m_track_plane_reserve);

    // create same amount of current planes as in the past
    for (auto& past_plane : past_view.planes)
    {
        if (is_valid_past_plane(past_plane))
        {
            current_view.planes.emplace_back(past_plane.normal, past_plane.d, past_plane.src, INVALID_PID, &past_plane);
            past_to_current_planes[past_plane.pid] = &current_view.planes.back();
            past_to_current_planes[past_plane.pid]->best_pts.reserve(past_plane.pts.size());
        }
    }
    
    // mark current points which belongs to some past planes
    auto past_pt_img = past_view.pt_img.data();
    const int dn_x = m_param.img_x_dn_sample, dn_y = m_param.img_y_dn_sample;
    const int img_w = src_w(), dn_y_img_w = dn_y * img_w;
    for (auto pt : current_view.pt_cloud)
    {
        auto past_cam_pix = project_i(map_to_past.transform(pt->pos));
        if (is_within_pt_cloud_fov(past_cam_pix[0], past_cam_pix[1]))
        {
            //plane pixel in the past
            auto& past_pt3d = past_pt_img[(past_cam_pix[1] / dn_y) * dn_y_img_w + (past_cam_pix[0] / dn_x)*dn_x];
            if (past_pt3d.best_plane && is_valid_past_plane(*past_pt3d.best_plane)) {
                if (is_inlier(*past_pt3d.best_plane, *pt))
                {
                    auto current_plane = past_to_current_planes[past_pt3d.best_plane->pid];
                    current_plane->best_pts.push_back(pt);
                }
            }
        }
    }

    // find a better normal
    for (auto& plane : current_view.planes)
    {
        if (plane.best_pts.size() > 0)
        {
            v3 center(0, 0, 0);
            for (auto& pt : plane.best_pts)
                center += pt->pos;
            center *= (1.0f / (float)plane.best_pts.size());

            float min_dist_to_cen = std::numeric_limits<float>::max();
            pt3d* src = nullptr;
            for (auto& pt : plane.best_pts) {
                const auto dist = (pt->pos - center).squaredNorm();
                if (dist < min_dist_to_cen) { min_dist_to_cen = dist; src = pt; }
            }

            Eigen::Matrix3f em; em.setZero();
            for (auto& pt : plane.best_pts)
            {
                const auto dp = (pt->pos - src->pos);
                em += dp* dp.transpose();
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

            plane.normal = normal;
            plane.d = -center.dot(normal);
            plane.src = src;
        }
    }

    // grow current 
    auto src_img_point = current_view.pt_img.data();
    for (auto& current_plane : current_view.planes)
    {
        grow_inlier_buffer(src_img_point, current_plane, current_plane.best_pts, false); //do not grow the same point twice
    }

    // coarse grid candidate sampling
    img_pointcloud_to_planecandidate(
        current_view.pt_img, current_view.planes,
        m_param.track_x_dn_sample, m_param.track_y_dn_sample);
}

void rs_sf_planefit::combine_planes_from_the_same_past(scene & current_view, scene & past_view)
{
    for (auto& new_plane : current_view.planes)
    {
        if (new_plane.past_plane != nullptr) continue; //tracked plane, okay

        int num_new_point = 0;
        for (auto& pt : new_plane.pts)
        {
            if (pt->best_plane == &new_plane)
                ++num_new_point;
        }

        // delete this plane if it largely overlaps with tracked planes
        if (num_new_point < (int)new_plane.pts.size() / 2)
        {
            for (auto& pt : new_plane.pts)
                if (pt->best_plane == &new_plane)
                    pt->best_plane = nullptr;
            new_plane.best_pts.clear();
            new_plane.pts.clear();
            new_plane.src = nullptr;
        }
    }
}

void rs_sf_planefit::assign_planes_pid(vec_plane_ref& sorted_planes)
{
    m_tracked_pid.resize(m_param.max_num_plane_output + 1);
    std::fill_n(m_tracked_pid.begin(), m_param.max_num_plane_output + 1, nullptr);

    for (auto* plane : sorted_planes)
    {
        if (plane->past_plane && plane->best_pts.size() > 0 && is_tracked_pid(plane->past_plane->pid))
        {
            if (!m_tracked_pid[plane->past_plane->pid]) //assign old pid to new plane
            {
                m_tracked_pid[plane->pid = plane->past_plane->pid] = plane;
            }
        }
    }

    int next_pid = 1;
    for (auto& plane : sorted_planes)
    {
        if (plane->best_pts.size() < m_param.min_num_plane_pt) return;

        if (!is_tracked_pid(plane->pid)) //no previous plane detected
        {
            while (m_tracked_pid[next_pid]) { //find next unused pid
                if (++next_pid >= m_param.max_num_plane_output) {
                    return;
                }
            }
            m_tracked_pid[plane->pid = next_pid] = plane; //assign new pid
        }
    }
}

bool rs_sf_planefit::is_tracked_pid(int pid)
{
    return INVALID_PID < pid && pid <= MAX_VALID_PID;
}

void rs_sf_planefit::upsample_best_plane_ptr(scene & view)
{
    return; 

    auto src_pt_img = view.pt_img.data();
    auto src_pt_cloud = view.pt_cloud.data();
    const int img_w = src_w();
    const int dn_x = m_param.img_x_dn_sample;
    const int dn_y = m_param.img_y_dn_sample;
    const int cloud_w = src_w() / dn_x;

    for (int p = num_pixels() - 1; p >= 0; --p)
    {
        const int x = p % img_w, y = p / img_w;
        src_pt_img[p].best_plane = src_pt_cloud[(y / dn_y) * cloud_w + (x / dn_x)]->best_plane;
    }
}

void rs_sf_planefit::upsize_pt_cloud_to_plane_map(const vec_pt3d & img_pt_cloud, rs_sf_image * dst) const
{
    const int dst_w = dst->img_w, dst_h = dst->img_h;
    const int dn_x = m_param.img_x_dn_sample;
    const int dn_y = m_param.img_y_dn_sample;

    const auto src_pt_cloud = img_pt_cloud.data();
    for (int y = 0, p = 0; y < dst_h; ++y) {
        for (int x = 0; x < dst_w; ++x, ++p)
        {
            const auto x_src = ((x * src_w()) / dst_w) / dn_x * dn_x;
            const auto y_src = ((y * src_h()) / dst_h) / dn_y * dn_y;
            const auto best_plane = src_pt_cloud[y_src * src_w() + x_src].best_plane;
            dst->data[p] = (best_plane ? best_plane->pid : 0);
        }
    }
}