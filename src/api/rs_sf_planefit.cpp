#include "rs_sf_planefit.h"

rs_sf_planefit::rs_sf_planefit(const rs_sf_intrinsics * camera) 
    : src_depth_img(camera->img_w,camera->img_h)
{
    m_intrinsics = *camera;
    parameter_updated();
}

rs_sf_status rs_sf_planefit::process_depth_image(const rs_sf_image * img)
{
    if (!img || !img->data || img->byte_per_pixel != 2 ) return RS_SF_INVALID_ARG;
    
    // reset
    m_view.reset();
    m_ref_view.reset();
 
    // preprocessing
    memcpy(src_depth_img.data, img->data, src_depth_img.num_char());
    image_to_pointcloud(&src_depth_img, m_view);
    img_pt_group_to_normal(m_view.pt_grp);

    // new plane fitting
    img_pointcloud_to_planecandidate(m_view.pt_grp, m_view.planes);
    grow_planecandidate(m_view.pt_grp, m_view.planes);
    non_max_plane_suppression(m_view.pt_grp, m_view.planes);
    sort_plane_size(m_view.planes, m_sorted_plane_ptr);
    assign_planes_pid(m_sorted_plane_ptr);
    pt_groups_planes_to_full_img(m_view.pt_img, m_sorted_plane_ptr);
    end_of_process();

    return RS_SF_SUCCESS;
}

rs_sf_status rs_sf_planefit::track_depth_image(const rs_sf_image *img)
{
    if (!img || !img->data || img->byte_per_pixel != 2) return RS_SF_INVALID_ARG;

    if (img->cam_pose) m_param.filter_plane_map = false;

    // save previous scene
    save_current_scene_as_reference();

    // preprocessing
    memcpy(src_depth_img.data, img->data, src_depth_img.num_char());
    image_to_pointcloud(&src_depth_img, m_view);
    img_pt_group_to_normal(m_view.pt_grp);

    // search for old planes
    map_candidate_plane_from_past(m_view, m_ref_view);
    grow_planecandidate(m_view.pt_grp, m_view.planes);
    non_max_plane_suppression(m_view.pt_grp, m_view.planes);
    combine_planes_from_the_same_past(m_view, m_ref_view);
    sort_plane_size(m_view.planes, m_sorted_plane_ptr);
    assign_planes_pid(m_sorted_plane_ptr);
    pt_groups_planes_to_full_img(m_view.pt_img, m_sorted_plane_ptr);
    end_of_process();

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
    //mark_plane_src_on_map(map);

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

void rs_sf_planefit::parameter_updated()
{
    m_grid_h = (int)(std::ceilf((float)src_h() / m_param.img_y_dn_sample));
    m_grid_w = (int)(std::ceilf((float)src_w() / m_param.img_x_dn_sample));
    m_grid_neighbor[0] = -1;
    m_grid_neighbor[1] = 1;
    m_grid_neighbor[2] = -m_grid_w;
    m_grid_neighbor[3] = m_grid_w;
    m_grid_neighbor[4] = -m_grid_w - 1;
    m_grid_neighbor[5] = -m_grid_w + 1;
    m_grid_neighbor[6] = m_grid_w - 1;
    m_grid_neighbor[7] = m_grid_w + 1;
    m_grid_neighbor[8] = 0;

    m_plane_pt_reserve = (m_grid_h)*(m_grid_w);
    m_track_plane_reserve = m_param.max_num_plane_output + m_plane_pt_reserve;

    m_inlier_buf.reserve(m_plane_pt_reserve);
    m_tracked_pid.reserve(m_param.max_num_plane_output);

    init_img_pt_groups(m_view);
    init_img_pt_groups(m_ref_view);
}

rs_sf_planefit::plane * rs_sf_planefit::get_tracked_plane(int pid) const
{
    if (pid <= 0 || m_tracked_pid.size() <= pid) return nullptr;
    return m_tracked_pid[pid];
}

void rs_sf_planefit::refine_plane_boundary(plane& dst)
{
    if (dst.fine_pts.size() > 0) return;
    if (dst.best_pts.size() <= 0) return;

    // buffers
    std::list<pt3d*> next_fine_pt;
    auto* this_plane_ptr = &dst;

    //remove points of boundary
    for (auto& edge_list : dst.edge_grp) {
        for (auto edge_grp : edge_list) {
            for (auto pl_pt : edge_grp->grp->pt) {
                pl_pt->set_boundary();
                if (pl_pt->best_plane == this_plane_ptr)
                    pl_pt->best_plane = nullptr;
            }
        }
    }

    //find inner neighbor groups of inner boundary as seeds
    auto* pt_group = m_view.pt_grp.data();
    for (auto& inner_edge_grp : dst.edge_grp[1])
    {
        const auto gp = inner_edge_grp->grp->gp;
        for (int b = 0; b < 4; ++b)
        {
            auto& neighbor = pt_group[gp + m_grid_neighbor[b]];
            if (!neighbor.pt0->is_boundary() && neighbor.pt0->best_plane == this_plane_ptr) //inner neighbor group found
            {
                for (auto pl_pt : neighbor.pt)
                {
                    if (pl_pt->best_plane == this_plane_ptr)
                    {
                        next_fine_pt.emplace_back(pl_pt); //assign seeds
                    }
                }
            }
        }
    }

    // grow
    auto* plane_img = m_view.pt_img.data();
    const int xb[] = { -1,1,0,0 };
    const int yb[] = { 0,0,-1,1 };
    const int pb[] = { -1,1,-src_w(),src_w() };
    const int x_step = m_param.img_x_dn_sample;
    const int y_step = m_param.img_y_dn_sample;
    const int y_down = y_step * src_w();
    while (!next_fine_pt.empty())
    {
        const auto fine_pt = next_fine_pt.front();
        next_fine_pt.pop_front();

        for (int b = 0; b < 4; ++b)
        {
            const int bx = fine_pt->pix.x() + xb[b];
            const int by = fine_pt->pix.y() + yb[b];
            if ( 0 <= bx - x_step && bx + x_step < src_w() &&
                 0 <= by - y_step && by + y_step < src_h() )
            {
                auto& bpt = plane_img[fine_pt->p + pb[b]];
                if (!bpt.is_boundary()) continue; //visited 
                bpt.clear_boundary_flag(); //mark visited;

                if (!bpt.is_valid_normal()) //new normal
                {
                    if (!bpt.is_known_pos()) compute_pt3d(bpt); //new pos

                    auto& bpt_right = plane_img[bpt.p + x_step];
                    auto& bpt_below = plane_img[bpt.p + y_down];
                    compute_pt3d_normal(bpt, bpt_right, bpt_below);

                    if (!bpt.is_valid_normal()) //try get normal from another points
                    {
                        auto& bpt_left = plane_img[bpt.p - x_step];
                        compute_pt3d_normal(bpt, bpt_below, bpt_left);

                        if (!bpt.is_valid_normal()) //try get normal from another points
                        {
                            auto& bpt_above = plane_img[bpt.p - y_down];
                            compute_pt3d_normal(bpt, bpt_left, bpt_above);

                            if (!bpt.is_valid_normal()) //try get normal from another points
                            {
                                compute_pt3d_normal(bpt, bpt_above, bpt_right);
                            }
                        }
                    }
                }

                if (bpt.is_valid_normal() && is_inlier(dst, bpt))
                {
                    bpt.best_plane = this_plane_ptr; //accept this plane point
                    next_fine_pt.emplace_back(&bpt); //grow further
                    dst.fine_pts.emplace_back(&bpt); //store this plane point
                }
            }
        }
    }

    //clear unreachable boundary
    for (auto& edge_list : dst.edge_grp) {
        for (auto edge_grp : edge_list) {
            for (auto pl_pt : edge_grp->grp->pt) {
                pl_pt->clear_boundary_flag();
            }
        }
    }
}

void rs_sf_planefit::init_img_pt_groups(scene& view)
{
    const int img_w = src_w();
    const int dwn_x = m_param.img_x_dn_sample;
    const int dwn_y = m_param.img_y_dn_sample;
    const int pt_per_group = dwn_x * dwn_y;

    view.reset();
    view.pt_img.resize(num_pixels());
    view.pt_grp.resize(num_pixel_groups());

    for (int g = num_pixel_groups() - 1; g >= 0; --g)
    {
        auto& grp = view.pt_grp[g];
        grp.gp = g;
        grp.gpix[0] = g % m_grid_w;
        grp.gpix[1] = g / m_grid_w;
        grp.pt.reserve(pt_per_group);
    }

    for (int p = 0, ep = num_pixels(); p < ep; ++p)
    {
        auto& pt = view.pt_img[p];
        pt.pos = pt.normal = v3(0, 0, 0);
        pt.best_plane = nullptr;
        pt.pix = i2((pt.p = p) % img_w, p / img_w);
        pt.grp = &view.pt_grp[(pt.pix[1] / dwn_y) * m_grid_w + (pt.pix[0] / dwn_x)];
        pt.grp->pt.emplace_back(&pt);
    }

    for (auto& grp : view.pt_grp) {
        const auto gp_cen = std::min((int)grp.pt.size() - 1, pt_per_group / 2);
        grp.pt0 = grp.pt[gp_cen];
    }
}

i2 rs_sf_planefit::project_grid_i(const v3 & cam_pt) const
{
    return i2{
        (int)(0.5f + (cam_pt.x() * m_intrinsics.cam_fx) / cam_pt.z() + m_intrinsics.cam_px) / m_param.img_x_dn_sample,
        (int)(0.5f + (cam_pt.y() * m_intrinsics.cam_fy) / cam_pt.z() + m_intrinsics.cam_py) / m_param.img_y_dn_sample
    };
}

v3 rs_sf_planefit::unproject(const float u, const float v, const float z) const
{
    return v3{
        z * (u - m_intrinsics.cam_px) / m_intrinsics.cam_fx,
        z * (v - m_intrinsics.cam_py) / m_intrinsics.cam_fy,
        z };
}

bool rs_sf_planefit::is_within_pt_group_fov(const int x, const int y) const
{
    return 0 <= x && x < m_grid_w && 0 <= y && y < m_grid_h;
}

bool rs_sf_planefit::is_within_pt_img_fov(const int x, const int y) const
{
    return 0 <= x && x < src_w() && 0 <= y && y < src_h();
}

bool rs_sf_planefit::is_valid_raw_z(const float z) const
{
    return z > m_param.min_z_value && z < m_param.max_z_value;
}

unsigned short & rs_sf_planefit::get_raw_z_at(const pt3d & pt) const
{
    return ((unsigned short*)src_depth_img.data)[pt.p];
}

void rs_sf_planefit::compute_pt3d(pt3d & pt) const
{
    //pt.pos = pose.transform(unproject((float)pt.pix.x(), (float)pt.pix.y(), is_valid_raw_z(z) ? z : 0)); 
    float z;
    if (is_valid_raw_z(z = (float)get_raw_z_at(pt))) {
        const float x = z * (pt.pix[0] - cam_px) / cam_fx;
        const float y = z * (pt.pix[1] - cam_py) / cam_fy;
        pt.pos[0] = r00 * x + r01 * y + r02 * z + t0;
        pt.pos[1] = r10 * x + r11 * y + r12 * z + t1;
        pt.pos[2] = r20 * x + r21 * y + r22 * z + t2;
        pt.set_valid_pos();
    }
    else {
        pt.set_invalid_pos();
    }
}

void rs_sf_planefit::compute_pt3d_normal(pt3d& pt_query, pt3d& pt_right, pt3d& pt_below) const
{
    if (!pt_right.is_known_pos()) compute_pt3d(pt_right);
    if (!pt_below.is_known_pos()) compute_pt3d(pt_below);

    if (pt_query.is_valid_pos() &&
        pt_right.is_valid_pos() &&
        pt_below.is_valid_pos()) {
        const v3 dx = pt_right.pos - pt_query.pos;
        const v3 dy = pt_below.pos - pt_query.pos;
        pt_query.normal = dy.cross(dx).normalized();
        pt_query.set_valid_normal();
    }
    else
        pt_query.set_invalid_normal();
}

void rs_sf_planefit::image_to_pointcloud(const rs_sf_image * img, scene& current_view, bool force_full_pt_cloud)
{
    // camera pose
    auto& pose = current_view.cam_pose;
    pose.set_pose(img->cam_pose);

    r00 = pose.rotation(0, 0); r01 = pose.rotation(0, 1); r02 = pose.rotation(0, 2);
    r10 = pose.rotation(1, 0); r11 = pose.rotation(1, 1); r12 = pose.rotation(1, 2);
    r20 = pose.rotation(2, 0); r21 = pose.rotation(2, 1); r22 = pose.rotation(2, 2);
    t0 = pose.translation[0], t1 = pose.translation[1], t2 = pose.translation[2];

    const auto* src_depth = (unsigned short*)img->data;
    if (force_full_pt_cloud || m_param.compute_full_pt_cloud) {

        // compute full point cloud
        for (auto& pt : current_view.pt_img)
        {
            pt.reset();
            compute_pt3d(pt);
        }

        current_view.is_full_pt_cloud = true;
    }
    else {

        // compute point cloud
        for (auto& grp : current_view.pt_grp) {
            for (auto pt : grp.pt) pt->reset();
            compute_pt3d(*grp.pt0);
        }

        current_view.is_full_pt_cloud = false;
    }
}


void rs_sf_planefit::img_pt_group_to_normal(vec_pt3d_group & pt_groups)
{
    auto* img_pt_group = pt_groups.data();
    for (int y = 1, ey = m_grid_h - 1, ex = m_grid_w - 1, gp = m_grid_w + 1; y < ey; ++y, gp += 2)
    {
        for (int x = 1; x < ex; ++x, ++gp)
        {
            auto& pt_query = *img_pt_group[gp].pt0;
            auto& pt_right = *img_pt_group[gp + 1].pt0;
            auto& pt_below = *img_pt_group[gp + m_grid_w].pt0;
            compute_pt3d_normal(pt_query, pt_right, pt_below);
        }
    }
}

void rs_sf_planefit::img_pointcloud_to_planecandidate(
    const vec_pt3d_group& pt_groups,
    vec_plane& img_planes,
    int candidate_gy_dn_sample,
    int candidate_gx_dn_sample)
{
    const int y_step = candidate_gy_dn_sample > 0 ? candidate_gy_dn_sample : m_param.candidate_gy_dn_sample;
    const int x_step = candidate_gx_dn_sample > 0 ? candidate_gx_dn_sample : m_param.candidate_gx_dn_sample;
   
    img_planes.reserve(num_pixel_groups());
    auto* src_point_group = pt_groups.data();

    for (int y = 1, ey = m_grid_h - y_step, ex = m_grid_w - x_step; y <= ey; y += y_step) {
        for (int x = 1, p = y*m_grid_w + x; x <= ex; x += x_step, p += x_step)
        {
            auto src = src_point_group[p].pt0;
            if (src->is_valid_normal()) {
                const float d = -src->normal.dot(src->pos);
                img_planes.emplace_back(src->normal, d, src);
            }
        }
    }
}

bool rs_sf_planefit::is_inlier(const plane & candidate, const pt3d & p)
{
    const float nor_fit = std::abs(candidate.normal.dot(p.normal));
    if (nor_fit > m_param.max_normal_thr)
    {
        const float fit_err = std::abs(candidate.normal.dot(p.pos) + candidate.d);
        if (fit_err < m_param.max_fit_err_thr)
            return true;
    }
    return false;
}

void rs_sf_planefit::grow_planecandidate(vec_pt3d_group& pt_groups, vec_plane& plane_candidates)
{
    //assume pt_groups is from a 2D grid
    for (auto& plane : plane_candidates)
    {
        if (plane.past_plane) continue;
        if (!plane.src || plane.src->best_plane) continue;

        grow_inlier_buffer(pt_groups.data(), plane, vec_pt_ref{ plane.src });

        if (plane.pts.size() < m_param.min_num_plane_pt) {
            plane.pts.clear();
            plane.pts.shrink_to_fit();
        }
    }
}

void rs_sf_planefit::grow_inlier_buffer(pt3d_group pt_group[], plane & plane_candidate, const vec_pt_ref& seeds, bool reset_best_plane_ptr)
{
    auto* this_plane_ptr = &plane_candidate;
    auto& plane_pts = plane_candidate.pts;
    auto& edge_grp = plane_candidate.edge_grp;
    plane_pts.clear();
    plane_pts.reserve(m_plane_pt_reserve);
    edge_grp[0].clear();
    edge_grp[1].clear();

    m_inlier_buf.clear();
    m_inlier_buf.assign(seeds.begin(), seeds.end());
    for (auto p : m_inlier_buf) {
        p->best_plane = this_plane_ptr; //mark seed
        p->set_checked();               //mark inlier
    }

    while (!m_inlier_buf.empty())
    {
        // new inlier point
        const auto pt = m_inlier_buf.back();
        m_inlier_buf.pop_back();

        // store this inlier
        plane_pts.emplace_back(pt);

        // grid point
        const int gp = pt->grp->gp;

        // grow
        for (int b = 0; b < 8; ++b)
        {
            //if (is_within_pt_group_fov(xb[b], yb[b])) //within fov
            {
                const int gp_next = gp + m_grid_neighbor[b];
                auto pt_next = pt_group[gp_next].pt0;

                if (!pt_next->is_checked() && //not yet checked
                    pt_next->best_plane == nullptr && //not assigned a plane
                    pt_next->is_valid_normal() ) //possible plane point
                {
                    if (is_inlier(plane_candidate, *pt_next)) // check this point
                    {
                        pt_next->best_plane = this_plane_ptr; //assign this plane
                        pt_next->set_checked();
                        m_inlier_buf.emplace_back(pt_next);  //go further
                        continue;
                    }
                }

                if (pt_next->best_plane != this_plane_ptr) //still an edge point
                {
                    pt_next->set_checked(); //mark visited
                    if (!pt_next->is_boundary()) {
                        pt_next->set_boundary();           //mark edge point
                        edge_grp[0].emplace_back(pt_next); //save outer edge point
                    }

                    if (!pt->is_boundary()) {
                        pt->set_boundary();           //mark edge point
                        edge_grp[1].emplace_back(pt); //save inner edge point
                    }
                }
            }
        }
    }

    // reset plane boundary flags
    for (auto outer_edge : edge_grp[0]) {
        outer_edge->clear_boundary_flag();
        outer_edge->clear_check_flag();
    }
    for (auto inner_edge : edge_grp[1])
        inner_edge->clear_boundary_flag();

    if (reset_best_plane_ptr) {
        // reset marker for next plane candidate
        for (auto p : plane_pts) {
            p->best_plane = nullptr;
            p->clear_check_flag();
        }
    }
}

void rs_sf_planefit::non_max_plane_suppression(vec_pt3d_group& pt_groups, vec_plane& plane_candidates)
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

    // filter best plane ptr map
    if (m_param.filter_plane_map)
        filter_plane_ptr_to_plane_img(pt_groups);

    // rebuild best_pts list
    for (auto& grp : pt_groups)
    {
        if (grp.pt0->best_plane)
            grp.pt0->best_plane->best_pts.push_back(grp.pt0);
    }

    // make sure the plane src is valid
    for (auto& plane : plane_candidates) {
        if (plane.best_pts.size() > 0 && plane.src->best_plane != &plane)
            plane.src = plane.best_pts[0];
    }
}

void rs_sf_planefit::filter_plane_ptr_to_plane_img(vec_pt3d_group & pt_groups)
{
    // setup filtering buffer
    m_sorted_plane_ptr.clear();
    m_sorted_plane_ptr.reserve(pt_groups.size());
    for (auto& grp : pt_groups)
        m_sorted_plane_ptr.emplace_back(grp.pt0->best_plane);

    auto* src_plane = m_sorted_plane_ptr.data();
    auto* pt_group = pt_groups.data();

    // median filter
    plane* sorter[9], **sorter_end = sorter + 9;
    for (int y = 1, g = m_grid_w + 1, ex = m_grid_w - 1, ey = m_grid_h - 1; y < ey; ++y, g += 2) {
        for (int x = 1; x < ex; ++x, ++g)
        {
            sorter[0] = src_plane[g + m_grid_neighbor[0]];
            sorter[1] = src_plane[g + m_grid_neighbor[1]];
            sorter[2] = src_plane[g + m_grid_neighbor[2]];
            sorter[3] = src_plane[g + m_grid_neighbor[3]];
            sorter[4] = src_plane[g + m_grid_neighbor[4]];
            sorter[5] = src_plane[g + m_grid_neighbor[5]];
            sorter[6] = src_plane[g + m_grid_neighbor[6]];
            sorter[7] = src_plane[g + m_grid_neighbor[7]];
            sorter[8] = src_plane[g + m_grid_neighbor[8]];
            std::sort(sorter, sorter_end);
            pt_group[g].pt0->best_plane = sorter[4];
        }
    }
}

void rs_sf_planefit::sort_plane_size(vec_plane & planes, vec_plane_ref& sorted_planes)
{
    sorted_planes.reserve(planes.size());
    sorted_planes.clear();

    for (auto& plane : planes)
        sorted_planes.emplace_back(&plane);

    std::sort(sorted_planes.begin(), sorted_planes.end(),
        [](const plane* p0, const plane* p1) { return p0->best_pts.size() > p1->best_pts.size(); });
}

bool rs_sf_planefit::is_valid_past_plane(const plane & past_plane) const
{
    return past_plane.pid > INVALID_PID && past_plane.best_pts.size() > 0;
}

bool rs_sf_planefit::is_tracked_pid(int pid) const
{
    return INVALID_PID < pid && pid <= MAX_VALID_PID;
}

void rs_sf_planefit::save_current_scene_as_reference()
{
    // save history
    m_view.swap(m_ref_view);
    m_view.reset();
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
            current_view.planes.emplace_back(past_plane.normal, past_plane.d, past_plane.src, &past_plane);
            past_to_current_planes[past_plane.pid] = &current_view.planes.back();
            past_to_current_planes[past_plane.pid]->best_pts.reserve(past_plane.pts.size());

            for (auto& edge_list : past_plane.edge_grp)
                for (auto edge_grp : edge_list)
                    edge_grp->best_plane = nullptr; //delete past boundary point
        }
    }

    // mark current points which belongs to some past planes
    auto past_pt_grp = past_view.pt_grp.data();
    for (auto& grp : current_view.pt_grp)
    {
        auto& pt = grp.pt0;
        if (!pt->is_valid_pos()) continue;
        auto past_cam_pix = project_grid_i(map_to_past.transform(pt->pos));
        if (is_within_pt_group_fov(past_cam_pix[0], past_cam_pix[1]))
        {
            //plane pixel in the past
            auto& past_pt3d = *past_pt_grp[past_cam_pix[1] * m_grid_w + past_cam_pix[0]].pt0;
            if (past_pt3d.best_plane && is_valid_past_plane(*past_pt3d.best_plane)) {
                if (is_inlier(*past_pt3d.best_plane, *pt))
                {
                    auto current_plane = past_to_current_planes[past_pt3d.best_plane->pid];
                    current_plane->best_pts.emplace_back(pt);
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
            src->pos -= -(plane.normal.dot(src->pos) + plane.d) *plane.normal;
        }
    }

    // grow current tracked planes
    for (auto& current_plane : current_view.planes)
    {
        grow_inlier_buffer(current_view.pt_grp.data(), current_plane, current_plane.best_pts, false); //do not grow the same point twice
    }

    // coarse grid candidate sampling
    img_pointcloud_to_planecandidate(
        current_view.pt_grp, current_view.planes,
        m_param.track_gx_dn_sample, m_param.track_gy_dn_sample);

    // delete candidate if inside a tracked plane
    for (auto& plane : current_view.planes)
    {
        if (plane.past_plane == nullptr) // coarse grid candidate
        {
            plane.past_plane = plane.src->best_plane; //best_plane is one of the current tracked plane
        }
    }
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

void rs_sf_planefit::pt_groups_planes_to_full_img(vec_pt3d & pt_img, vec_plane_ref& sorted_planes)
{
    for (auto& pt : pt_img)
    {
        pt.best_plane = pt.grp->pt0->best_plane;
    }

    if (m_param.refine_plane_map)
    {
        for (auto pl = sorted_planes.rbegin(); pl != sorted_planes.rend(); ++pl)
        {
            refine_plane_boundary(**pl);
        }
    }
}

void rs_sf_planefit::upsize_pt_cloud_to_plane_map(const vec_pt3d& plane_img, rs_sf_image * dst) const
{
    const int dst_w = dst->img_w, dst_h = dst->img_h;
    const int img_w = src_w();
    const int img_h = src_h();
    const int dn_x = m_param.img_x_dn_sample;
    const int dn_y = m_param.img_y_dn_sample;

    const auto src_pt = plane_img.data();
    for (int y = 0, p = 0; y < dst_h; ++y) {
        for (int x = 0; x < dst_w; ++x, ++p)
        {
            const auto x_src = ((x * img_w) / dst_w);
            const auto y_src = ((y * img_h) / dst_h);
            const auto best_plane = src_pt[y_src * img_w + x_src].best_plane;
            dst->data[p] = (best_plane ? best_plane->pid : 0);
        }
    }
}

void rs_sf_planefit::end_of_process()
{
    return;
}
