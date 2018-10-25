/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
//
//  rs_sf_planefit.cpp
//  algo-core
//
//  Created by Hon Pong (Gary) Ho
//
#include "rs_sf_planefit.hpp"

rs_sf_planefit::rs_sf_planefit(const rs_sf_intrinsics * camera) 
{
    m_intrinsics = *camera;
    parameter_updated();
}

rs_sf_status rs_sf_planefit::set_option(rs_sf_fit_option option, double value)
{
    auto status = rs_shapefit::set_option(option, value);
    switch (option) {
    case RS_SF_OPTION_PLANE_RES:
        m_param.hole_fill_plane_map = (value > 0);
        m_param.refine_plane_map = (value > 1); break;
    case RS_SF_OPTION_PLANE_NOISE:
        m_param.search_around_missing_z = (value > 0);
        m_param.filter_plane_map = (value > 1); break;
    case RS_SF_OPTION_DEPTH_UNIT:
        m_depth_img_to_meter = (float)value; break;
    case RS_SF_OPTION_PARAM_PRESET:
        m_param.min_num_plane_pt = ((rs_sf_param_preset)(int)value == RS_SF_PRESET_BOX_S) ? 150 : parameter().min_num_plane_pt; break;
    default: break;
    }
    return status;
}

rs_sf_status rs_sf_planefit::set_locked_inputs(const rs_sf_image* img)
{
    const auto status = rs_shapefit::set_locked_inputs(img);
    if (status == RS_SF_SUCCESS) rs_sf_util_copy_depth_image(*m_view.src_depth_img, img);
    return status;
}

rs_sf_status rs_sf_planefit::set_locked_outputs()
{
    const auto status = rs_shapefit::set_locked_outputs();
    if (status == RS_SF_SUCCESS) save_current_scene_as_reference();
    return status;
}

rs_sf_status rs_sf_planefit::process_depth_image()
{
    // reset
    m_view.reset();
    //m_ref_view.reset(); //never touch reference view here
 
    // preprocessing
    image_to_pointcloud(m_view);
    img_pt_group_to_normal(m_view.pt_grp);

    // new plane fitting
    img_pointcloud_to_planecandidate(m_view.pt_grp, m_view.planes);
    grow_planecandidate(m_view.pt_grp, m_view.planes);
    non_max_plane_suppression(m_view.pt_grp, m_view.planes);
    sort_plane_size(m_view.planes, m_view.sorted_plane_ptr);
    assign_planes_pid(m_view.tracked_pid, m_view.sorted_plane_ptr);
    pt_groups_planes_to_full_img(m_view.pt_img, m_view.sorted_plane_ptr);

    return RS_SF_SUCCESS;
}

rs_sf_status rs_sf_planefit::track_depth_image()
{
    // save previous scene
    //save_current_scene_as_reference(); //done by sdk

    // preprocessing
    image_to_pointcloud(m_view);
    img_pt_group_to_normal(m_view.pt_grp);

    // search for old planes
    map_candidate_plane_from_past(m_view, m_ref_view);
    grow_planecandidate(m_view.pt_grp, m_view.planes);
    non_max_plane_suppression(m_view.pt_grp, m_view.planes);
    combine_planes_from_the_same_past(m_view);
    sort_plane_size(m_view.planes, m_view.sorted_plane_ptr);
    assign_planes_pid(m_view.tracked_pid, m_view.sorted_plane_ptr);
    pt_groups_planes_to_full_img(m_view.pt_img, m_view.sorted_plane_ptr);

    return RS_SF_SUCCESS;
}

int rs_sf_planefit::num_detected_planes() const
{
    int n = 0;
    for (auto&& pl : m_ref_view.planes)
        if (is_valid_plane(pl))
            ++n;
    return n;
}

int rs_sf_planefit::max_detected_pid() const
{
    int max_pid = 0;
    for (auto&& pl : m_ref_view.planes)
        if (max_pid <= pl.pid) max_pid = pl.pid;
    return max_pid;
}

rs_sf_status rs_sf_planefit::get_plane_index_map(rs_sf_image * map, int hole_filled) const
{
    if (!map || !map->data || map->byte_per_pixel != 1) return RS_SF_INVALID_ARG;
    upsize_pt_cloud_to_plane_map(m_ref_view, map);
    //mark_plane_src_on_map(map);

    if (hole_filled > 0 || (hole_filled == -1 && m_param.hole_fill_plane_map))
        hole_fill_custom_plane_map(map);

    return RS_SF_SUCCESS;
}

rs_sf_status rs_sf_planefit::mark_plane_src_on_map(rs_sf_image * map) const
{
    if (!map || !map->data || map->byte_per_pixel != 1) return RS_SF_INVALID_ARG;

    const int dst_w = map->img_w;
    const float up_x = (float)dst_w / src_w(), up_y = (float)map->img_h / src_h();
    for (const auto& plane : m_ref_view.planes)
    {
        if (is_valid_new_plane(plane))
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

rs_sf_status rs_sf_planefit::get_planes(rs_sf_plane dst[RS_SF_MAX_PLANE_COUNT], float * point_buffer) const
{
    static const int size_v3 = sizeof(float) * 3;
    std::vector<std::vector<int>> contours;
    if (point_buffer != nullptr) {
        // get contours if point_buffer available 
        std::vector<short> pid_map(num_pixels());
        auto* pid_data = pid_map.data();
        for (auto&& pt : m_ref_view.pt_img)
            pid_data[pt.p] = pt.best_plane ? pt.best_plane->pid : 0;
        contours = find_contours_in_map_uchar(pid_data, src_w(), src_h(), (m_param.img_x_dn_sample + m_param.img_y_dn_sample) * 2);
    }
    else {
        // empty list of contours, just mark available pid for output
        for (auto* plane : m_ref_view.tracked_pid)
            if (plane) { contours.push_back({ plane->src->p }); }
    }

    const auto* pt3d = m_ref_view.pt_img.data();
    int pl = 0, cp0 = 0, pid_contour[RS_SF_MAX_PLANE_COUNT] = { 0 };
    for (const auto& contour : contours)
    {
        auto& dst_plane = dst[pl++] = {};
        auto& src_plane = pt3d[contour[0]].best_plane;
        dst_plane.plane_id = src_plane->pid;
        dst_plane.contour_id = pid_contour[dst_plane.plane_id]++;
        dst_plane.contour_p0 = cp0;
        dst_plane.equation[3] = src_plane->d;
        rs_sf_memcpy(dst_plane.equation, src_plane->normal.data(), size_v3);
        dst_plane.is_new_plane = src_plane->past_plane ? 0 : 255;

        if (point_buffer)
        {
            const int img_w = src_w();
            const v3& normal = src_plane->normal;
            const v3& translate = m_ref_view.cam_pose.translation;
            const m3& rotate = m_ref_view.cam_pose.rotation;
            const float ms_ndott_d = -normal.dot(translate) - src_plane->d;
            auto dst_pos = dst_plane.pos = (float(*)[3])point_buffer;
            for (auto&& px : contour)
            {
                const float xdz = ((px % img_w) - cam_px) * inv_cam_fx;
                const float ydz = ((px / img_w) - cam_py) * inv_cam_fy;
                const v3 Rx = rotate * v3(xdz, ydz, 1);
                const float normal_dot_Rx = normal.dot(Rx);
                if (std::abs(normal_dot_Rx) > 0.001f) {
                    const v3 pos = Rx * (ms_ndott_d / normal_dot_Rx) + translate;
                    rs_sf_memcpy(*(dst_pos++), pos.data(), size_v3);
                }
            }

            // move to next chunk of buffer memory
            dst_plane.num_points = (int)(dst_pos - dst_plane.pos);
            point_buffer += (dst_plane.num_points * 3);
        }
        
        // move to next first point position of buffer memory
        cp0 += (dst_plane.num_points * 3);

        if (pl >= RS_SF_MAX_PLANE_COUNT) return RS_SF_INDEX_OUT_OF_BOUND;
    }
    dst[pl] = {};
    return RS_SF_SUCCESS;
}

void rs_sf_planefit::parameter_updated()
{
    m_grid_h = (int)(std::ceil((float)src_h() / m_param.img_y_dn_sample));
    m_grid_w = (int)(std::ceil((float)src_w() / m_param.img_x_dn_sample));
    m_grid_neighbor[0] = -1;
    m_grid_neighbor[1] = 1;
    m_grid_neighbor[2] = -m_grid_w;
    m_grid_neighbor[3] = m_grid_w;
    m_grid_neighbor[4] = -m_grid_w - 1;
    m_grid_neighbor[5] = -m_grid_w + 1;
    m_grid_neighbor[6] = m_grid_w - 1;
    m_grid_neighbor[7] = m_grid_w + 1;
    m_grid_neighbor[8] = 0;

    inv_cam_fx = 1.0f / cam_fx;
    inv_cam_fy = 1.0f / cam_fy;

    m_plane_pt_reserve = (m_grid_h)*(m_grid_w);
    m_track_plane_reserve = m_param.max_num_plane_output + m_plane_pt_reserve;
    m_inlier_buf.reserve(m_plane_pt_reserve);

    init_img_pt_groups(m_view);
    init_img_pt_groups(m_ref_view);
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
    view.src_depth_img = std::make_unique<rs_sf_image_depth>(src_w(), src_h());
    view.tracked_pid.reserve(m_param.max_num_plane_output);

    // setup sub-image grid
    for (int g = num_pixel_groups() - 1; g >= 0; --g)
    {
        view.pt_grp[g].gp = g;
        view.pt_grp[g].pt.resize(pt_per_group);
    }

    // grid pixel pick up order
    const i2 cen_grp(dwn_x / 2, dwn_y / 2);
    std::vector<std::pair<int, int>> grp_order;
    for (int grp_pt = 0; grp_pt < pt_per_group; ++grp_pt)
        grp_order.emplace_back(std::make_pair((i2(grp_pt % dwn_x, grp_pt / dwn_y) - cen_grp).squaredNorm(), grp_pt));
    std::sort(grp_order.begin(), grp_order.end());

    // setup full image vector
    for (int p = 0, ep = num_pixels(); p < ep; ++p)
    {
        auto& pt = view.pt_img[p];
        pt.pos = pt.normal = v3(0, 0, 0);
        pt.best_plane = nullptr;
        pt.pix = i2((pt.p = p) % img_w, p / img_w);
        pt.grp = &view.pt_grp[(pt.pix[1] / dwn_y) * m_grid_w + (pt.pix[0] / dwn_x)];
        pt.grp->pt[(pt.pix.y() % dwn_y) * dwn_x + (pt.pix.x() % dwn_x)] = &pt;
    }

    // re-insert non-null grid pixel according to distance from grid center
    for (auto&& grp : view.pt_grp)
    {
        vec_pt_ref pt_buf; pt_buf.swap(grp.pt);
        for (auto&& order : grp_order)
            if (pt_buf[order.second] != nullptr)
                grp.pt.emplace_back(pt_buf[order.second]);

        // group center
        grp.pt0 = grp.pt[0];
    }
}

// this is a protected function, use m_view instead of m_ref_view
rs_sf_planefit::plane * rs_sf_planefit::get_tracked_plane(int pid) const 
{
    if (pid <= INVALID_PID || (int)m_view.tracked_pid.size() <= pid) return nullptr; //okay to use m_view here
    return m_view.tracked_pid[pid];
}

void rs_sf_planefit::refine_plane_boundary(plane& dst)
{
    if (dst.fine_pts.size() > 0) return;
    if (!dst.non_empty()) return;

    // buffers
    const int max_pt_per_group = m_param.img_x_dn_sample*m_param.img_y_dn_sample;
    dst.fine_pts.reserve((dst.edge_grp[0].size() + dst.edge_grp[1].size())*max_pt_per_group);
    list_pt_ref next_fine_pt;
    auto* this_plane_ptr = &dst;

    //remove points of boundary
    for (auto&& edge_list : dst.edge_grp) {
        for (auto* edge_grp : edge_list) {
            for (auto* pl_pt : edge_grp->grp->pt) {
                pl_pt->set_boundary();
                if (pl_pt->best_plane == this_plane_ptr)
                    pl_pt->best_plane = nullptr;
            }
        }
    }

    //find inner neighbor groups of inner boundary as seeds
    auto* pt_group = m_view.pt_grp.data();
    for (auto* inner_edge_grp : dst.edge_grp[1])
    {
        const auto gp = inner_edge_grp->grp->gp;
        for (int b = 0; b < 4; ++b)
        {
            auto& neighbor = pt_group[gp + m_grid_neighbor[b]];
            if (!neighbor.pt0->is_boundary() && neighbor.pt0->best_plane == this_plane_ptr) //inner neighbor group found
            {
                for (auto* pl_pt : neighbor.pt)
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
        const auto* fine_pt = next_fine_pt.front();
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

    // release unused memory
    dst.fine_pts.shrink_to_fit();

    //clear unreachable boundary
    for (auto&& edge_list : dst.edge_grp) {
        for (auto* edge_grp : edge_list) {
            for (auto* pl_pt : edge_grp->grp->pt) {
                pl_pt->clear_boundary_flag();
            }
        }
    }
}

i2 rs_sf_planefit::project_grid_i(const v3 & cam_pt) const
{
    return i2(
        (int)(0.5f + (cam_pt.x() * cam_fx) / cam_pt.z() + cam_px) / m_param.img_x_dn_sample,
        (int)(0.5f + (cam_pt.y() * cam_fx) / cam_pt.z() + cam_py) / m_param.img_y_dn_sample);
}

v3 rs_sf_planefit::unproject(const float u, const float v, const float z) const
{
    return v3(z * (u - cam_px) * inv_cam_fx, z * (v - cam_py) * inv_cam_fy, z);
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

float rs_sf_planefit::get_z_in_meter(const pt3d & pt) const
{
    return ((unsigned short*)m_view.src_depth_img->data)[pt.p] * m_depth_img_to_meter;
}

#if 0
/** \brief Distortion model: defines how pixel coordinates should be mapped to sensor coordinates. */
typedef enum rs2_distortion
{
    RS2_DISTORTION_NONE, /**< Rectilinear images. No distortion compensation required. */
    RS2_DISTORTION_MODIFIED_BROWN_CONRADY, /**< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
    RS2_DISTORTION_INVERSE_BROWN_CONRADY, /**< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */
    RS2_DISTORTION_FTHETA, /**< F-Theta fish-eye distortion model */
    RS2_DISTORTION_BROWN_CONRADY, /**< Unmodified Brown-Conrady distortion model */
    RS2_DISTORTION_COUNT, /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_distortion;

static void rs2_deproject_pixel_to_point(float point[3], const struct rs_sf_intrinsics * intrin, const float pixel[2], float depth)
{
    assert(intrin->model != RS2_DISTORTION_MODIFIED_BROWN_CONRADY); // Cannot deproject from a forward-distorted image
    assert(intrin->model != RS2_DISTORTION_FTHETA); // Cannot deproject to an ftheta image
                                                    //assert(intrin->model != RS2_DISTORTION_BROWN_CONRADY); // Cannot deproject to an brown conrady model

    float x = (pixel[0] - intrin->ppx) / intrin->fx;
    float y = (pixel[1] - intrin->ppy) / intrin->fy;
    if (intrin->model == RS2_DISTORTION_INVERSE_BROWN_CONRADY)
    {
        float r2 = x*x + y*y;
        float f = 1 + intrin->coeffs[0] * r2 + intrin->coeffs[1] * r2*r2 + intrin->coeffs[4] * r2*r2*r2;
        float ux = x*f + 2 * intrin->coeffs[2] * x*y + intrin->coeffs[3] * (r2 + 2 * x*x);
        float uy = y*f + 2 * intrin->coeffs[3] * x*y + intrin->coeffs[2] * (r2 + 2 * y*y);
        x = ux;
        y = uy;
    }
    point[0] = depth * x;
    point[1] = depth * y;
    point[2] = depth;
}
#endif

void rs_sf_planefit::compute_pt3d(pt3d & pt, bool search_around) const
{
    //pt.pos = pose.transform(unproject((float)pt.pix.x(), (float)pt.pix.y(), is_valid_raw_z(z) ? z : 0)); 
    float z;
    for (pt3d* grp_pt : ( search_around ? pt.grp->pt : vec_pt_ref{&pt} )) {
        if (is_valid_raw_z(z = get_z_in_meter(*grp_pt)))
        {
            const float x = z * (pt.pix[0] - cam_px) * inv_cam_fx;
            const float y = z * (pt.pix[1] - cam_py) * inv_cam_fy;
            pt.pos[0] = r00 * x + r01 * y + r02 * z + t0;
            pt.pos[1] = r10 * x + r11 * y + r12 * z + t1;
            pt.pos[2] = r20 * x + r21 * y + r22 * z + t2;
            pt.set_valid_pos();
            return;
        }
    }
    pt.set_invalid_pos();
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

void rs_sf_planefit::image_to_pointcloud(scene& current_view, bool force_full_pt_cloud)
{
    // camera pose
    auto& pose = current_view.cam_pose;
    pose.set_pose(current_view.src_depth_img->cam_pose);

    r00 = pose.rotation(0, 0); r01 = pose.rotation(0, 1); r02 = pose.rotation(0, 2);
    r10 = pose.rotation(1, 0); r11 = pose.rotation(1, 1); r12 = pose.rotation(1, 2);
    r20 = pose.rotation(2, 0); r21 = pose.rotation(2, 1); r22 = pose.rotation(2, 2);
    t0 = pose.translation[0], t1 = pose.translation[1], t2 = pose.translation[2];

	const auto search_around = m_param.search_around_missing_z;
    if (force_full_pt_cloud || m_param.compute_full_pt_cloud) {

        // compute full point cloud
        for (auto&& pt : current_view.pt_img)
        {
            pt.clear_all_state();
            compute_pt3d(pt, search_around);
        }

        current_view.is_full_pt_cloud = true;
    }
    else {

        // compute point cloud
        for (auto&& grp : current_view.pt_grp) {
            for (auto* pt : grp.pt)
                pt->clear_all_state();
            compute_pt3d(*grp.pt0, search_around);
        }

        current_view.is_full_pt_cloud = false;
    }
}

void rs_sf_planefit::img_pt_group_to_normal(vec_pt3d_group & pt_groups)
{
    auto* img_pt_group = pt_groups.data();
    for (int y = 1, ey = m_grid_h - 1, ex = m_grid_w - 1, gp = m_grid_w + 1; y < ey; ++y, gp += 2)
    {
        for (int x = 1; x < ex; ++x)
        {
            auto& pt_below = *img_pt_group[gp + m_grid_w].pt0;
            auto& pt_query = *img_pt_group[gp++].pt0;
            auto& pt_right = *img_pt_group[gp].pt0;
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
            auto* src = src_point_group[p].pt0;
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
    for (auto&& plane : plane_candidates)
    {
        if (plane.past_plane) continue;
        if (!plane.src || plane.src->best_plane) continue;

        grow_inlier_buffer(pt_groups.data(), plane, vec_pt_ref{ plane.src });

        if (!is_valid_new_plane(plane)) {
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
    edge_grp[0].reserve(m_plane_pt_reserve >> 2);
    edge_grp[1].clear();
    edge_grp[1].reserve(m_plane_pt_reserve >> 2);

    m_inlier_buf.clear();
    m_inlier_buf.assign(seeds.begin(), seeds.end());
    for (auto* p : m_inlier_buf) {
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
        for (int b = 0; b < 4; ++b)
        {
            //if (is_within_pt_group_fov(xb[b], yb[b])) //within fov
            {
                const int gp_next = gp + m_grid_neighbor[b];
                auto* pt_next = pt_group[gp_next].pt0;

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
    for (auto* outer_edge : edge_grp[0]) {
        outer_edge->clear_boundary_flag();
        outer_edge->clear_check_flag();
    }
    for (auto* inner_edge : edge_grp[1])
        inner_edge->clear_boundary_flag();

    if (reset_best_plane_ptr) {
        // reset marker for next plane candidate
        for (auto* p : plane_pts) {
            p->best_plane = nullptr;
            p->clear_check_flag();
        }
    }
}

void rs_sf_planefit::non_max_plane_suppression(vec_pt3d_group& pt_groups, vec_plane& plane_candidates)
{
    for (auto&& plane : plane_candidates)
    {
        const int this_plane_size = (int)plane.pts.size();
        const bool is_past_plane = (plane.past_plane != nullptr);
        for (auto* pt : plane.pts)
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
    for (auto&& plane : plane_candidates)
    {
        plane.best_pts.clear();
        plane.best_pts.reserve(plane.pts.size());
    }

    // filter best plane ptr map
    if (m_param.filter_plane_map)
        filter_plane_ptr_to_plane_img(pt_groups);

    // rebuild best_pts list
    for (auto&& grp : pt_groups)
    {
        if (grp.pt0->best_plane)
            grp.pt0->best_plane->best_pts.emplace_back(grp.pt0);
    }

    // make sure the plane src is valid
    for (auto&& plane : plane_candidates) {
        if (plane.non_empty() && plane.src->best_plane != &plane)
            plane.src = plane.best_pts[0];
    }
}

void rs_sf_planefit::filter_plane_ptr_to_plane_img(vec_pt3d_group & pt_groups)
{
    // setup filtering buffer
    vec_plane_ref src_plane_ptr;
    src_plane_ptr.reserve(pt_groups.size());
    for (auto&& grp : pt_groups)
        src_plane_ptr.emplace_back(grp.pt0->best_plane);

    auto* src_plane = src_plane_ptr.data();
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

    for (auto&& plane : planes) {
        if ((plane.non_empty()) && (plane.past_plane || is_valid_new_plane(plane)))
            sorted_planes.emplace_back(&plane);
    }

    std::sort(sorted_planes.begin(), sorted_planes.end(),
        [](const plane* p0, const plane* p1) { return p0->best_pts.size() > p1->best_pts.size(); });
}

void rs_sf_planefit::save_current_scene_as_reference()
{
    // save history
    m_view.swap(m_ref_view);
    m_view.reset();
}

void rs_sf_planefit::map_candidate_plane_from_past(scene & current_view, const scene & past_view)
{
    const auto map_to_past = past_view.cam_pose.invert();
    std::vector<plane*> past_to_current_planes(MAX_VALID_PID + 1);
    std::fill_n(past_to_current_planes.begin(), past_to_current_planes.size(), nullptr);
    current_view.planes.clear();
    current_view.planes.reserve(past_view.planes.size() + m_track_plane_reserve);

    // create same amount of current planes as in the past
    auto valid_past_plane = (const plane**)m_inlier_buf.data();
    memset(valid_past_plane, 0, num_pixel_groups() * sizeof(plane*));
    for (auto&& past_plane : past_view.planes)
    {
        if (is_valid_past_plane(past_plane))
        {
            current_view.planes.emplace_back(past_plane.normal, past_plane.d, past_plane.src, &past_plane);
            past_to_current_planes[past_plane.pid] = &current_view.planes.back();
            past_to_current_planes[past_plane.pid]->best_pts.reserve(past_plane.pts.size());

            // setup valid past plane map
            for (auto&& ppt : past_plane.best_pts)
                valid_past_plane[ppt->grp->gp] = &past_plane;
            // remove boundary of the plane
            for (auto&& edge : past_plane.edge_grp[1])
                valid_past_plane[edge->grp->gp] = nullptr;
        }
    }

    // mark current points which belong to some past planes
    for (auto&& grp : current_view.pt_grp)
    {
        auto* pt = grp.pt0;
        if (!pt->is_valid_pos()) continue;
        const auto past_cam_pix = project_grid_i(map_to_past.transform(pt->pos));
        if (is_within_pt_group_fov(past_cam_pix[0], past_cam_pix[1]))
        {
            // best plane in the past
            const auto past_plane = valid_past_plane[past_cam_pix[1] * m_grid_w + past_cam_pix[0]];
            if (past_plane != nullptr && is_inlier(*past_plane, *pt)) {
                past_to_current_planes[past_plane->pid]->best_pts.emplace_back(pt);
            }
        }
    }

    // find a better normal
    for (auto&& plane : current_view.planes)
    {
        if (plane.non_empty())
        {
            v3 center(0, 0, 0);
            for (auto* pt : plane.best_pts)
                center += pt->pos;
            center *= (1.0f / (float)plane.best_pts.size());

            float min_dist_to_cen = std::numeric_limits<float>::max();
            pt3d* src = nullptr;
            for (auto* pt : plane.best_pts) {
                const auto dist = (pt->pos - center).squaredNorm();
                if (dist < min_dist_to_cen) { min_dist_to_cen = dist; src = pt; }
            }

            Eigen::Matrix3f em; em.setZero();
            for (auto* pt : plane.best_pts)
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

            if (normal.dot(plane.normal) > m_param.max_normal_thr) {
                plane.normal = normal;
            }
            plane.d = -center.dot(plane.normal);
            plane.src = src;
            src->pos -= (plane.normal.dot(src->pos) + plane.d) * plane.normal;
        }
    }

    // grow current tracked planes
    for (auto&& current_plane : current_view.planes)
    {
        grow_inlier_buffer(current_view.pt_grp.data(), current_plane, current_plane.best_pts, false); //do not grow the same point twice
    }

    // coarse grid candidate sampling
    img_pointcloud_to_planecandidate(
        current_view.pt_grp, current_view.planes,
        m_param.track_gx_dn_sample, m_param.track_gy_dn_sample);

    // delete candidate if inside a tracked plane
    for (auto&& plane : current_view.planes)
    {
        if (plane.past_plane == nullptr) // coarse grid candidate
        {
            plane.past_plane = plane.src->best_plane; //best_plane is one of the current tracked plane
        }
    }
}

void rs_sf_planefit::combine_planes_from_the_same_past(scene & current_view)
{
    for (auto&& new_plane : current_view.planes)
    {
        if (new_plane.past_plane != nullptr) continue; //tracked plane, okay

        int num_new_point = 0;
        for (auto* pt : new_plane.pts)
        {
            if (pt->best_plane == &new_plane)
                ++num_new_point;
        }

        // delete this plane if it largely overlaps with tracked planes
        if (num_new_point < (int)new_plane.pts.size() / 2)
        {
            for (auto* pt : new_plane.pts)
                if (pt->best_plane == &new_plane)
                    pt->best_plane = nullptr;
            new_plane.best_pts.clear();
            new_plane.pts.clear();
            new_plane.src = nullptr;
        }
    }
}

void rs_sf_planefit::assign_planes_pid(vec_plane_ref& tracked_pid, const vec_plane_ref& sorted_planes)
{
    tracked_pid.resize(m_param.max_num_plane_output + 1);
    std::fill_n(tracked_pid.begin(), m_param.max_num_plane_output + 1, nullptr);

    // sorted_plane must be nonempty tracked plane or bigger than min size
    for (auto* plane : sorted_planes) 
    {
        if (plane->past_plane && is_tracked_pid(plane->past_plane->pid))
        {
            if (!tracked_pid[plane->past_plane->pid]) //assign old pid to new plane
            {
                tracked_pid[plane->pid = plane->past_plane->pid] = plane;
            }
        }
    }

    int next_pid = 1;
    for (auto* plane : sorted_planes)
    {
        //if (plane->best_pts.size() < m_param.min_num_plane_pt) return;

        if (!is_tracked_pid(plane->pid)) //no previous plane detected
        {
            while (tracked_pid[next_pid]) { //find next unused pid
                if (++next_pid >= m_param.max_num_plane_output) {
                    return;
                }
            }
            tracked_pid[plane->pid = next_pid] = plane; //assign new pid
        }
    }
}

void rs_sf_planefit::pt_groups_planes_to_full_img(vec_pt3d & pt_img, vec_plane_ref& sorted_planes)
{
    for (auto&& pt : pt_img) { pt.best_plane = pt.grp->pt0->best_plane; }

    if (m_param.refine_plane_map)
        for (auto pl = sorted_planes.rbegin(); pl != sorted_planes.rend();)
            refine_plane_boundary(**pl++);
}

void rs_sf_planefit::upsize_pt_cloud_to_plane_map(const scene& ref_view, rs_sf_image * dst) const
{
    const int dst_w = dst->img_w, dst_h = dst->img_h, img_w = src_w(), img_h = src_h();
    auto* dst_data = dst->data;

    int dn_x = 1, dn_y = 1;
    if (dst_w > img_w || dst_h > img_h) {
        dn_x = std::max(1, (int)std::round((float)dst_w / img_w));
        dn_y = std::max(1, (int)std::round((float)dst_h / img_h));
    }

    if (dst->cam_pose) {
        rs_sf_util_set_to_zeros(dst);

        pose_t to_cam = pose_t().set_pose(dst->cam_pose).invert();
        const auto rotation = to_cam.rotation * ref_view.cam_pose.rotation;
        const auto translation = to_cam.rotation * ref_view.cam_pose.translation + to_cam.translation;
        const float tcr00 = rotation(0, 0), tcr01 = rotation(0, 1), tcr02 = rotation(0, 2);
        const float tcr10 = rotation(1, 0), tcr11 = rotation(1, 1), tcr12 = rotation(1, 2);
        const float tcr20 = rotation(2, 0), tcr21 = rotation(2, 1), tcr22 = rotation(2, 2);
        const float tct0 = translation[0], tct1 = translation[1], tct2 = translation[2];
        const auto* src_z = (unsigned short*)ref_view.src_depth_img->data;
        const auto* src_p = ref_view.pt_img.data();
        const auto dcam = rs_sf_util_match_intrinsics(dst, m_intrinsics);
        const auto dcam_fx = dcam.fx, dcam_fy = dcam.fy, dcam_ppx = dcam.ppx, dcam_ppy = dcam.ppy;

        auto map_fcn = [&](const int sp, const int ep) {
            for (int p = sp, ex = (dst_w - 1) / dn_x, ey = (dst_h - 1) / dn_y; p < ep; ++p) {
                float z; plane* pl;
                if ((pl = src_p[p].best_plane) && (is_valid_raw_z(z = (src_z[p] * m_depth_img_to_meter)))) {
                    const float x = z * ((p % img_w) - cam_px) * inv_cam_fx;
                    const float y = z * ((p / img_w) - cam_py) * inv_cam_fy;
                    const float xd = tcr00 * x + tcr01 * y + tcr02 * z + tct0;
                    const float yd = tcr10 * x + tcr11 * y + tcr12 * z + tct1;
                    const float iz = 1.0f / (tcr20 * x + tcr21 * y + tcr22 * z + tct2);
                    const int ud = (int)(((xd * dcam_fx) * iz + dcam_ppx) + 0.5f) / dn_x;
                    const int vd = (int)(((yd * dcam_fy) * iz + dcam_ppy) + 0.5f) / dn_y;
                    if (0 < ud && ud < ex && 0 < vd && vd < ey) {
                        for (int j = 0; j < dn_y; ++j)
                            for (int i = 0; i < dn_x; ++i)
                                dst_data[(vd*dn_y + j)*dst_w + (ud*dn_x + i)] = pl->pid;
                    }
                }
            }
        };

        if (get_option_async_process_wait() < 0)
            map_fcn(0, num_pixels()); //deterministics, not threading
        else {
            const int split_p = num_pixels() / 2;
            auto q0 = std::async(std::launch::async, map_fcn, 0, split_p);
            map_fcn(split_p, num_pixels());
            q0.wait();
        }
    }
    else {
        const auto* src_pt = ref_view.pt_img.data();
        for (int y = 0, p = 0; y < dst_h; ++y) {
            for (int x = 0; x < dst_w; ++x) {
                const auto x_src = ((x * img_w) / dst_w);
                const auto y_src = ((y * img_h) / dst_h);
                const auto best_plane = src_pt[y_src * img_w + x_src].best_plane;
                dst_data[p++] = (best_plane ? best_plane->pid : 0);
            }
        }
    }
}


void rs_sf_planefit::hole_fill_custom_plane_map(rs_sf_image * map) const
{
    auto* idx = map->data;
    const int img_w = map->img_w;
    const int num_pixel = map->num_pixel();
    const unsigned char VISITED = 255, NO_PLANE = 0;

    std::vector<unsigned char> hole_map_src;
    std::vector<i2> next_list;
    std::vector<int> holes;
    hole_map_src.reserve(num_pixel);
    hole_map_src.assign(idx, idx + num_pixel);
    next_list.reserve(num_pixel);
    holes.reserve(num_pixel / 4);
    auto* hole_map = hole_map_src.data();

    // frame the hole map
    memset(hole_map, VISITED, sizeof(unsigned char)*img_w);
    memset(hole_map + num_pixel - img_w, VISITED, sizeof(unsigned char)*img_w);
    for (int x0 = 0, ex = num_pixel, img_w_1 = img_w - 1; x0 < ex; x0 += img_w) {
        hole_map[x0] = hole_map[x0 + img_w_1] = VISITED;
    }

    // vertical image frame
    for (int x0 = img_w + 1, xn = 2 * img_w - 2, ex = num_pixel - img_w; x0 < ex; x0 += img_w, xn += img_w) {
        if (idx[x0] == NO_PLANE) next_list.emplace_back(x0, NO_PLANE);
        if (idx[xn] == NO_PLANE) next_list.emplace_back(xn, NO_PLANE);
    }
    // horizontal image frame
    for (int y0 = img_w + 2, yn = num_pixel - 2 * img_w + 2, ey = img_w * 2 - 1; y0 < ey; ++y0, ++yn) {
        if (idx[y0] == NO_PLANE) next_list.emplace_back(y0, NO_PLANE);
        if (idx[yn] == NO_PLANE) next_list.emplace_back(yn, NO_PLANE);
    }
    for (auto&& pt : next_list)
        hole_map[pt[1]] = VISITED;

    // fill background
    const int pb[] = { -1,1,-img_w,img_w };
    while (!next_list.empty())
    {
        const auto pt = next_list.back();
        next_list.pop_back();

        for (auto&& pbb : pb) {
            const auto p = pt[0] + pbb;
            if (hole_map[p] == NO_PLANE) {
                hole_map[p] = VISITED;
                next_list.emplace_back(p, NO_PLANE);
            }
        }
    }

    //pick up planes
    for (int p = num_pixel - 1, pid; p >= 0; --p) {
        if (((pid = hole_map[p]) != NO_PLANE) && pid != VISITED)
            next_list.emplace_back(p, pid);
    }

    //grow planes
    while (!next_list.empty())
    {
        auto pt = next_list.back();
        next_list.pop_back();

        for (auto&& pbb : pb)
        {
            const auto p = pt[0] + pbb;
            auto& hole_p = hole_map[p];
            if (idx[p] == NO_PLANE && hole_p != VISITED)
            {
                if (hole_p == NO_PLANE)
                {
                    hole_p = (unsigned char)pt[1];
                    next_list.emplace_back(p, pt[1]);
                    holes.emplace_back(p);
                }
                else if (hole_p != pt[1])
                {
                    hole_p = VISITED; //not hole, clear this point
                    next_list.emplace_back(p, pt[1]);
                }
            }
        }
    }

    //fill holes
    for (const auto hole_p : holes)
        if (hole_map[hole_p] != VISITED)
            idx[hole_p] = hole_map[hole_p];
}
