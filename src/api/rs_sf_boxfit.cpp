/*******************************************************************************
 
 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or nondisclosure
 agreement with Intel Corporation and may not be copied or disclosed except in
 accordance with the terms of that agreement
 Copyright(c) 2017 Intel Corporation. All Rights Reserved.
 
 *******************************************************************************/
//
//  rs_sf_boxfit.cpp
//  algo-core
//
//  Created by Hon Pong (Gary) Ho
//
#include "rs_sf_boxfit.hpp"

static rs_sf_boxfit* debug_this = nullptr;
rs_sf_boxfit::rs_sf_boxfit(const rs_sf_intrinsics * camera) : rs_sf_planefit(camera)
{
    m_box_scene.plane_scene = &m_view;
    m_box_ref_scene.plane_scene = &m_ref_view;
    debug_this = this;
}

rs_sf_status rs_sf_boxfit::set_option(rs_sf_fit_option option, double value)
{
    auto status = rs_sf_planefit::set_option(option, value);
    switch (option) {
        case RS_SF_OPTION_BOX_PLANE_RES:
            m_param.refine_box_plane = (value > 0); break;
        case RS_SF_OPTION_BOX_SCAN_MODE:
            m_param.fov_margin = (value > 0 ? parameter().fov_margin : 0); break;
        case RS_SF_OPTION_PARAM_PRESET:
            m_param = m_param.get_preset((rs_sf_param_preset)(int)value); break;
        default: break;
    }
    return status;
}

rs_sf_status rs_sf_boxfit::set_locked_outputs()
{
    auto status = rs_sf_planefit::set_locked_outputs();
    if (status == RS_SF_SUCCESS) {
        m_box_scene.swap(m_box_ref_scene);
        m_box_ref_scene.tracked_boxes.assign(m_box_scene.tracked_boxes.begin(), m_box_scene.tracked_boxes.end());
    }
    return status;
}

rs_sf_status rs_sf_boxfit::process_depth_image()
{
    auto pf_status = rs_sf_planefit::process_depth_image();
    if (pf_status < 0) return pf_status;
    
    m_box_scene.clear();
    //m_box_ref_scene.clear();
    
    detect_new_boxes(m_box_scene);
    update_tracked_boxes(m_box_scene);
    
    return pf_status;
}

rs_sf_status rs_sf_boxfit::track_depth_image()
{
    auto pf_status = rs_sf_planefit::track_depth_image();
    if (pf_status < 0) return pf_status;
    
    //m_box_scene.swap(m_box_ref_scene);
    
    detect_new_boxes(m_box_scene);
    update_tracked_boxes(m_box_scene);
    
    return pf_status;
}

std::vector<rs_sf_box> rs_sf_boxfit::get_boxes() const
{
    std::vector<rs_sf_box> dst;
    dst.reserve(num_detected_boxes());
    for (const auto& box : m_box_ref_scene.tracked_boxes)
        dst.push_back(box.to_rs_sf_box());
    return dst;
}

bool rs_sf_boxfit::is_valid_box_plane(const plane & p0) const
{
    return (is_valid_past_plane(p0) || is_valid_plane(p0)) &&
    (p0.pid == 0 || !m_bp_map.plane_used[p0.pid]);
}

bool rs_sf_boxfit::is_valid_box_dimension(const float length) const
{
    return length >= m_param.min_box_thickness &&
    length <= m_param.max_box_thickness;
}

bool rs_sf_boxfit::is_valid_box_dimension(const box& b) const
{
    return is_valid_box_dimension(b.min_dimension()) &&
    is_valid_box_dimension(b.max_dimension());
}

void rs_sf_boxfit::detect_new_boxes(box_scene& view)
{
    const int num_plane = (int)view.plane_scene->sorted_plane_ptr.size();
    view.plane_pairs.clear();
    view.plane_pairs.reserve(num_plane);
    view.boxes.clear();
    view.boxes.reserve(num_plane);
    
    // old box pairs
    m_bp_map.reset();
    for (auto&& box : m_box_scene.tracked_boxes){
        m_bp_map.add(box);
    }
    
    // form all box pairs
    auto* pptr = view.plane_scene->sorted_plane_ptr.data();
    for (int i = 0; i < num_plane; ++i) {
        if (is_valid_box_plane(*pptr[i])) {
            for (int j = i + 1; j < num_plane; ++j) {
                if (is_valid_box_plane(*pptr[j])) {
                    auto pair = m_bp_map.form_pair(pptr[i], pptr[j]);
                    if (form_box_from_two_planes(view, pair)) {
                        for (int k = j + 1; k < num_plane; ++k) {
                            if (is_valid_box_plane(*pptr[k]) &&
                                refine_box_from_third_plane(view, pair, *pptr[k])) {
                                break; //k loop
                            }
                        }
                        view.plane_pairs.emplace_back(pair);
                        m_bp_map.mark(pair);
                        break; //j loop
                    }
                }
            }
        }
    }
    
    // for child class to further enhance the result
    refit_boxes(view);
}

// box plane helper
struct rs_sf_boxfit::box_plane_t
{
    typedef std::array<float, NUM_BOX_PLANE_BIN> box_axis_bin;
    const plane* src;
    vec_pt3d pts;
    v3 normal, width_axis, height_axis;
    float d, height_min;
    box_axis_bin width_min, width_max, height_max;
    int n_bins, upper_bin;
    
    box_plane_t(const plane& src_plane) : src(&src_plane)
    {
        width_min.fill(FLOAT_MAX_VALUE);
        width_max.fill(FLOAT_MIN_VALUE);
        height_min = FLOAT_MAX_VALUE;
        height_max.fill(FLOAT_MIN_VALUE);
        n_bins = sizeof(box_axis_bin) / sizeof(float);
        upper_bin = n_bins / 2 - 1;  // take median length
    }
    
    void project_pts_on_plane(const v3& _normal, const float _d, const float max_plane_pt_error)
    {
        d = _d;
        normal = _normal;
        
        auto& tmp_fine_pts = (src->fine_pts.size() ? src->fine_pts : src->best_pts);
        pts.reserve(tmp_fine_pts.size());
        for (auto* pt : tmp_fine_pts)
        {
            if (pt->is_valid_pos())
            {
                const float plane_pt_err = normal.dot(pt->pos) + d;
                if (std::abs(plane_pt_err) < max_plane_pt_error)
                {
                    pt->pos -= plane_pt_err * normal;
                    pts.emplace_back(*pt);
                }
            }
        }
    }
    
    v2 approx_width_range(const v3& _width_axis)
    {
        width_axis = _width_axis;
        v2 width_range(FLOAT_MAX_VALUE, FLOAT_MIN_VALUE);
        for (const auto& pt : pts)
        {
            const float coeff = width_axis.dot(pt.pos);
            width_range[0] = std::min(width_range[0], coeff);
            width_range[1] = std::max(width_range[1], coeff);
        }
        return width_range;
    }
    
    v2 get_height_length(const v3& _height_axis, const v2& width_bin_range, const float height_origin)
    {
        height_axis = _height_axis;
        const float w_bin_origin = width_bin_range[0];
        const float bin_size = (width_bin_range[1] - w_bin_origin) / n_bins;
        for (const auto& pt : pts)
        {
            const float w_len_coeff = width_axis.dot(pt.pos) - w_bin_origin;
            const int w_bin = std::min(n_bins - 1, std::max(0, (int)(w_len_coeff / bin_size)));
            const float h_coeff = height_axis.dot(pt.pos) - height_origin;
            height_max[w_bin] = std::max(height_max[w_bin], h_coeff);
            height_min = std::min(height_min, h_coeff);
        }
        fill_empty_max_bin(height_max);
        std::sort(height_max.begin(), height_max.end());
        return{ height_min, height_max[upper_bin] };
    }
    
    v2 get_width_range(const float height_origin)
    {
        const float h_bin_origin = height_origin;
        const float bin_size = (height_max[upper_bin] - height_min) / n_bins;
        for (const auto& pt : pts)
        {
            const float h_len_coeff = std::abs(height_axis.dot(pt.pos) - h_bin_origin);
            const int h_bin = std::max(0, (int)(h_len_coeff / bin_size));
            if (h_bin < n_bins)
            {
                const float w_coeff = width_axis.dot(pt.pos);
                width_max[h_bin] = std::max(width_max[h_bin], w_coeff);
                width_min[h_bin] = std::min(width_min[h_bin], w_coeff);
            }
        }
        fill_empty_min_bin(width_min);
        fill_empty_max_bin(width_max);
        std::sort(width_min.begin(), width_min.end());
        std::sort(width_max.begin(), width_max.end());
        return{ width_min[upper_bin],width_max[upper_bin] };
    }
    
    void fill_empty_max_bin(box_axis_bin& bin)
    {
        for (int b = 0; b < n_bins; ++b) {
            for (int n = 0; (bin[b] == FLOAT_MIN_VALUE) && (n < n_bins); ++n) {
                if ((b - n) >= 0) bin[b] = std::max(bin[b], bin[b - n]);
                if ((b + n) < n_bins) bin[b] = std::max(bin[b], bin[b + n]);
            }
        }
    }
    
    void fill_empty_min_bin(box_axis_bin& bin)
    {
        for (int b = 0; b < n_bins; ++b) {
            for (int n = 0; (bin[b] == FLOAT_MAX_VALUE) && (n < n_bins); ++n) {
                if ((b - n) >= 0) bin[b] = std::min(bin[b], bin[b - n]);
                if ((b + n) < n_bins) bin[b] = std::min(bin[b], bin[b + n]);
            }
        }
    }
};

bool rs_sf_boxfit::form_box_from_two_planes(box_scene& view, plane_pair& pair)
{
    auto& plane0 = *pair.p0, &plane1 = *pair.p1;
    
    // reject plane pair not perpendiculars
    auto cosine_theta = std::abs(plane0.normal.dot(plane1.normal));
    if (!pair.prev_box && (cosine_theta > m_param.plane_pair_angle_thr)) return false;
    if (pair.prev_box && (cosine_theta > m_param.tracked_pair_angle_thr)) return false;
    
    // box axis
    v3 axis[3] = { plane1.normal, plane0.normal, {} };
    v3 pt_on_interect(0, 0, 0);
    {
        const auto a0 = plane0.normal[0], b0 = plane0.normal[1], c0 = plane0.normal[2], d0 = plane0.d;
        const auto a1 = plane1.normal[0], b1 = plane1.normal[1], c1 = plane1.normal[2], d1 = plane1.d;
        const float D0 = b0 * c1 - b1 * c0; //assume x=0;
        const float D1 = a0 * c1 - a1 * c0; //assume y=0;
        
        if (std::abs(D0) > 0.0001f)
        {
            pt_on_interect.y() = (-d0*c1 + d1*c0) / D0;
            pt_on_interect.z() = (-d1*b0 + d0*b1) / D0;
        }
        else if (std::abs(D1) > 0.0001f)
        {
            pt_on_interect.x() = (-d0*c1 + d1*c0) / D1;
            pt_on_interect.z() = (-d1*a0 + d0*a1) / D1;
        }
        else
            return false; //bad planes
    }
    
    const float cen0_to_plane1_d = plane1.normal.dot(plane0.src->pos) + plane1.d;
    const float cen1_to_plane0_d = plane0.normal.dot(plane1.src->pos) + plane0.d;
    // axis pointing towards plane center
    axis[0] = (cen0_to_plane1_d < 0 ? -axis[0] : axis[0]);
    axis[1] = (cen1_to_plane0_d < 0 ? -axis[1] : axis[1]);
    
    // reject concave plane pair
    const v3 cen0_on_plane1 = plane0.src->pos - cen0_to_plane1_d * plane1.normal;
    const v3 cen1_on_plane0 = plane1.src->pos - cen1_to_plane0_d * plane0.normal;
    if ( axis[0].dot(cen0_on_plane1 - view.plane_scene->cam_pose.translation) < 0 ||
        axis[1].dot(cen1_on_plane0 - view.plane_scene->cam_pose.translation) < 0 )
        return false;
    
    // enforce orthogonal axis
    const v3 avg_normal = (axis[0] + axis[1])*0.5f; //perpendicular bisector
    const float avg_length = avg_normal.norm();
    axis[0] = ((axis[0] - avg_normal).normalized() * avg_length + avg_normal).normalized();
    axis[1] = ((axis[1] - avg_normal).normalized() * avg_length + avg_normal).normalized();
    
    // box plane helper
    box_plane_t box_plane[2] = { box_plane_t(plane0),box_plane_t(plane1) };
    
    if (m_param.refine_box_plane) {
        refine_plane_boundary(plane0);
        refine_plane_boundary(plane1);
    }
    
    // third axis is the cross product of two plane normals
    axis[2] = axis[0].cross(axis[1]).normalized();
    
    // part of box origin can be found by the plane-plane intersection
    v3 axis_origin;
    const v3 pt_on_intersect0 = (axis[2] * (plane0.src->pos - pt_on_interect).dot(axis[2])) + pt_on_interect;
    const v3 pt_on_intersect1 = (axis[2] * (plane1.src->pos - pt_on_interect).dot(axis[2])) + pt_on_interect;
    axis_origin[0] = axis[0].dot(pt_on_intersect1);
    axis_origin[1] = axis[1].dot(pt_on_intersect0);
    
    // setup box planes
    box_plane[0].project_pts_on_plane(axis[1], -axis_origin[1], m_param.max_plane_pt_error);
    box_plane[1].project_pts_on_plane(axis[0], -axis_origin[0], m_param.max_plane_pt_error);
    
    // get approximated common width range
    const auto width0_bin_range = box_plane[0].approx_width_range(axis[2]);
    const auto width1_bin_range = box_plane[1].approx_width_range(axis[2]);
    const v2 width_bin_range(
                             std::max(width0_bin_range[0], width1_bin_range[0]),
                             std::min(width0_bin_range[1], width1_bin_range[1]));
    
    // check if two planes overlap along axis 2
    if (width_bin_range[0] >= width_bin_range[1]) return false;
    
    // compute good plane height
    const auto axis0_length = box_plane[0].get_height_length(axis[0], width_bin_range, axis_origin[0]);
    const auto axis1_length = box_plane[1].get_height_length(axis[1], width_bin_range, axis_origin[1]);
    
    // check if two planes touch in 3D
    if (std::abs(axis0_length[0]) > m_param.plane_intersect_thr) return false;
    if (std::abs(axis1_length[0]) > m_param.plane_intersect_thr) return false;
    
    // compute good plane widths
    const auto width0_range = box_plane[0].get_width_range(axis_origin[0]);
    const auto width1_range = box_plane[1].get_width_range(axis_origin[1]);
    axis_origin[2] = std::min(width0_range[0], width1_range[0]);
    const auto axis2_length = std::max(width0_range[1], width1_range[1]) - axis_origin[2];
    
    // make new box
    view.boxes.push_back({});
    box* new_box = pair.new_box = &view.boxes.back();
    // copy box axis
    new_box->axis << axis[0], axis[1], axis[2];
    // form the box dimension by end point differences
    new_box->dimension << axis0_length[1], axis1_length[1], axis2_length;
    // form the box center by box origin + 1/2 box dimension
    new_box->center << new_box->axis * (axis_origin + (new_box->dimension*0.5f));
    
    // reject super thin box
    if (!is_valid_box_dimension(*new_box)) {
        view.boxes.pop_back();
        return false;
    }
    return true;
}

bool rs_sf_boxfit::refine_box_from_third_plane(box_scene & view, plane_pair & pair, plane& p2)
{
    if (!pair.new_box) return false;
    if (!p2.src->is_valid_pos()) return false;
    
    auto& new_box = *pair.new_box;
    const v3 axis2 = new_box.axis.col(2);
    
    // plane normal should align with axis2
    if (std::abs(axis2.dot(p2.normal)) < (1.0f-m_param.tracked_pair_angle_thr)) return false;
    
    // two candidiate plane2
    const float half_dim2 = new_box.dimension[2] * 0.5f;
    const v3 p2_cen[] = {
        new_box.center - axis2 * half_dim2,  //negative end of axis 2
        new_box.center + axis2 * half_dim2 };//positive end of axis 2
    
    // check which plane2 is facing the camera
    const float p2_neg_dp = (p2_cen[0] - view.plane_scene->cam_pose.translation).dot(-axis2);
    const float p2_pos_dp = (p2_cen[1] - view.plane_scene->cam_pose.translation).dot(axis2);
    
    // which end of axis 2 be the visible plane 2
    if (p2_pos_dp >= 0 && p2_neg_dp >= 0) return false;    // both planes are invisible
    const int p2_end = (p2_neg_dp < 0 ? 0 : 1); //visible end
    
    // project p2 src onto box plane 2
    const auto src_coeff2 = p2.src->pos.dot(axis2);
    const auto dst_coeff2 = p2_cen[p2_end].dot(axis2);
    if (std::abs(src_coeff2 - dst_coeff2) > m_param.max_plane_pt_error)
        return false; //candidate p2 too far from box plane2
    
    // reject p2 if points out of the box along axis 0 or axis 1
    const v3 axis0 = new_box.axis.col(0);
    const v3 axis1 = new_box.axis.col(1);
    const float half_dim0 = new_box.dimension[0] * 0.5f;
    const float half_dim1 = new_box.dimension[1] * 0.5f;
    const float max_diff_dim0 = half_dim0 + m_param.max_plane_pt_error;
    const float max_diff_dim1 = half_dim1 + m_param.max_plane_pt_error;
    for (auto* p2pt : p2.best_pts) {
        const v3 p2vec = (p2pt->pos - new_box.center); //p2 to center
        if (std::abs(p2vec.dot(axis0)) > max_diff_dim0) return false; //outside box along axis0
        if (std::abs(p2vec.dot(axis1)) > max_diff_dim1) return false; //outside box along axis1
    }
    
    // adjust the box along axis 2
    const float adjust2 = src_coeff2 - dst_coeff2;
    const auto new_box_center = new_box.center + (adjust2 *0.5f)* axis2;
    const float cen_coeff2 = new_box_center.dot(axis2);
    const float new_box_dimension2 = std::abs(src_coeff2 - cen_coeff2) * 2.0f;
    
    if (is_valid_box_dimension(new_box_dimension2))
    {
        new_box.center = new_box_center;
        new_box.dimension[2] = new_box_dimension2;
        pair.p2 = &p2;
        return true;
    }
    return false; //reject poor extension
}

void rs_sf_boxfit::update_tracked_boxes(box_scene & view)
{
    const auto current_time = now();
    
    // each newly detected box
    for (auto& pair : view.plane_pairs) {
        if (pair.new_box != nullptr) //new box
        {
            for (auto& old_box : m_box_scene.tracked_boxes) {
                if (old_box.try_update(pair, view.plane_scene->cam_pose, m_intrinsics, m_param)) {
                    pair.new_box = nullptr;
                    old_box.last_appear = current_time;
                    break;
                }
            }
        }
    }
    
    // delete boxes that lost tracking
    queue_tracked_box prev_tracked_boxes, immatured_tracked_boxes;
    prev_tracked_boxes.swap(m_box_scene.tracked_boxes);
    for (const auto& box : prev_tracked_boxes) {
        if (abs_time_diff_ms(current_time, box.last_appear) < m_param.box_miss_ms) {
            if (is_valid_box_dimension(box)) {
                if ((int)box.box_history.size() < m_param.max_box_history) {
                    immatured_tracked_boxes.emplace_back(box); }
                else {
                    m_box_scene.tracked_boxes.emplace_back(box); }
            }
        }
    }
    
    // put back immatured_tracked_boxes
    for (const auto& imbox : immatured_tracked_boxes)
        m_box_scene.tracked_boxes.emplace_back(imbox);
    
    // each newly detected box without match
    for (const auto& pair : view.plane_pairs) {
        if (pair.new_box != nullptr) {
            m_box_scene.tracked_boxes.emplace_back(pair, current_time);
        }
    }
}

void rs_sf_boxfit::draw_box(const std::string& name, const box & src, const box_plane_t* p0, const box_plane_t* p1) const
{
#if defined(__OPENCV_ALL_HPP__) | defined(OPENCV_ALL_HPP)
    
    auto to_cam = m_box_scene.plane_scene->cam_pose.invert();
    auto proj = [to_cam = to_cam, cam = m_intrinsics](const v3& pt) {
        const auto pt3d = to_cam.rotation * pt + to_cam.translation;
        return cv::Point(
                         (int)((pt3d.x() * cam.fx) / pt3d.z() + cam.ppx + 0.5f),
                         (int)((pt3d.y() * cam.fy) / pt3d.z() + cam.ppy + 0.5f));
    };
    print_box(src.to_rs_sf_box());
    
    auto& src_depth_img = *m_box_scene.plane_scene->src_depth_img;
    cv::Mat map(src_depth_img.img_h, src_depth_img.img_w, CV_8U, cv::Scalar(0));
    cv::Mat(src_depth_img.img_h, src_depth_img.img_w, CV_16U, src_depth_img.data).convertTo(map, CV_8U, 255.0 / 4000.0);
    
    auto pt0 = proj(src.origin());
    for (int a = 0; a < 3; ++a) {
        auto pt1 = proj(src.dimension[a] * src.axis.col(a) + src.origin());
        cv::line(map, pt0, pt1, cv::Scalar(70 * (a + 1)), 2, CV_AA);
    }
    
    if (p0 && p1)
    {
        cv::circle(map, proj(p1->src->src->pos), 2, cv::Scalar(255), -1);
        const v3 src1 = p1->src->src->pos;
        const v3 src1_p0 = src1 - (p0->src->normal.dot(src1) + p0->src->d) * p0->src->normal;
        cv::circle(map, proj(src1_p0), 2, cv::Scalar(128), -1);
        
        
        cv::circle(map, proj(p0->src->src->pos), 2, cv::Scalar(250), -1);
        const v3 src0 = p0->src->src->pos;
        const v3 src0_p1 = src0 - (p1->src->normal.dot(src0) + p1->src->d) * p1->src->normal;
        cv::circle(map, proj(src0_p1), 2, cv::Scalar(255), 1);
        
        const v3 org_p0 = src.origin() - (p0->src->normal.dot(src.origin()) + p0->src->d) * p0->src->normal;
        cv::circle(map, proj(org_p0), 3, cv::Scalar(255), 1);
        
        const v3 org_p1 = src.origin() - (p1->src->normal.dot(src.origin()) + p1->src->d) * p1->src->normal;
        cv::circle(map, proj(org_p1), 3, cv::Scalar(255), 1);
        
        //for (auto& pt : p0->pts) cv::circle(map, proj(pt.pos), 2, cv::Scalar(255), -1);
        //for (auto& pt : p1->pts) cv::circle(map, proj(pt.pos), 2, cv::Scalar(128), -1);
    }
    cv::imshow(name, map);
#endif
}

rs_sf_box rs_sf_boxfit::box::to_rs_sf_box() const
{
    rs_sf_box dst;
    v3_map(dst.center) = center;
    m3_axis_map((float*)dst.axis) = (axis * dimension.asDiagonal());
    return dst;
}

rs_sf_boxfit::box_record::box_record(const box& ref, const pose_t& pose, const rs_sf_intrinsics& cam, float margin)
: box(ref), cam_pose(pose)
{
    auto to_cam = cam_pose.invert();
    // check if box plane orthogonal to a box axis is visible and mark in the record
    auto record_visible_box_dir = [this, &to_cam = to_cam](int a, int sign) {
        const auto plane_cen = center + half_dir(a) * sign;
        const bool visible = vis_pl[a][(1 - sign) / 2] =
        to_cam.transform(plane_cen).normalized().dot(to_cam.rotation * axis.col(a) * sign) < 0;
        return visible ? 1 : 0;
    };
    // check if an image point with fov
    auto out_img_fov = [min_x = margin, max_x = (float)cam.width - margin, min_y = margin, max_y = (float)cam.height - margin](const v2& pt){
        return pt.x() < min_x || max_x < pt.x() || pt.y() < min_y || max_y < pt.y();
    };
    // check if a 3d point in world inside fov of image
    auto in_fov = [&to_cam, &cam, &out_img_fov](const v3& pt) {
        const auto pt3d = to_cam.transform(pt);
        const v2 img_pt = { (pt3d.x() * cam.fx) / pt3d.z() + cam.ppx, (pt3d.y() * cam.fy) / pt3d.z() + cam.ppy };
        return out_img_fov(img_pt) ? 0 : 1;
    };
    // count number of visible box planes
    for (auto a : { 0, 1, 2 }) {
        for (auto sign : { 1, -1 }) {
            num_visible_plane += record_visible_box_dir(a, sign);
        }
    }
    // check clearance of axis towards two ends.
    // an axis direction is cleared when its end point is not touching image margin.
    for (auto a : { 0, 1, 2 }) {
        for (auto s : { 0, 1 }) {
            const int b = (a + 1) % 3, c = (a + 2) % 3; // symbols for other two axis
            const auto pl_cen = center + half_dir(a) * (s == 0 ? 1 : -1); // pos, neg, plane center
            
            // each axis direction has 4 end points defined by the other two axis.
            const auto p0 = pt[a][s][0] = pl_cen + half_dir(b) + half_dir(c); // box plane corner 0 orthogonal to signed axis d
            const auto p1 = pt[a][s][1] = pl_cen + half_dir(b) - half_dir(c); // box plane corner 1 orthogonal to signed axis d
            const auto p2 = pt[a][s][2] = pl_cen - half_dir(b) - half_dir(c); // box plane corner 2 orthogonal to signed axis d
            const auto p3 = pt[a][s][3] = pl_cen - half_dir(b) + half_dir(c); // box plane corner 3 orthogonal to signed axis d
            
            // a box corner is visible when any one of the planes containing that corner is visible.
            vis_pt[a][s][0] = (vis_pl[a][s] || vis_pl[b][0] || vis_pl[c][0] ? 1 : 0); //plane corner 0 visibility
            vis_pt[a][s][1] = (vis_pl[a][s] || vis_pl[b][0] || vis_pl[c][1] ? 1 : 0); //plane corner 1 visibility
            vis_pt[a][s][2] = (vis_pl[a][s] || vis_pl[b][1] || vis_pl[c][1] ? 1 : 0); //plane corner 2 visibility
            vis_pt[a][s][3] = (vis_pl[a][s] || vis_pl[b][1] || vis_pl[c][0] ? 1 : 0); //plane corner 3 visibility
            
            // a box corner inside fov of image
            fov_pt[a][s][0] = in_fov(p0);
            fov_pt[a][s][1] = in_fov(p1);
            fov_pt[a][s][2] = in_fov(p2);
            fov_pt[a][s][3] = in_fov(p3);
            
            // we clear (lock) the direction, i.e. count a 1/2 length toward final measurement, when the
            // number of visible corner not touching margin > 2
            clear_dir[s][s] = ((is_visible_line(a,s,0) && is_fov_line(a,s,0)) ||
                               (is_visible_line(a,s,1) && is_fov_line(a,s,1)) ||
                               (is_visible_line(a,s,2) && is_fov_line(a,s,2)) ||
                               (is_visible_line(a,s,3) && is_fov_line(a,s,3)));
        }
    }
}

bool rs_sf_boxfit::tracked_box::try_update(const plane_pair& pair, const pose_t& pose, const rs_sf_intrinsics& cam, const parameter& param)
{
    // center not overlap
    auto& new_observed_box = *pair.new_box;
    
    if (!pair.prev_box) { // new box
        if ((center - new_observed_box.center).norm() > max_radius())
            return false;
    }
    else if (pair.prev_box != this) // tracked box
        return false;
    
    // match new box axis to old axis
    const m3 match_table = (new_observed_box.axis.transpose() * axis);
    const m3 match_select = (match_table.cwiseAbs2().array().round());
    
    // change axis
    box_record new_box(*this, pose, cam, param.fov_margin);
    new_box.center = new_observed_box.center;
    new_box.axis = new_observed_box.axis * (match_select.cwiseProduct(match_table.cwiseSign()));
    new_box.dimension = new_observed_box.dimension.transpose() * match_select;
    
    // add to history
    pid[0] = pair.p0->pid;
    pid[1] = pair.p1->pid;
    pid[2] = pair.p2 ? pair.p2->pid : 0;
    box_history.push_back(new_box);
    while ((int)box_history.size() > param.max_box_history)
        box_history.pop_front();
    
    // match latest rotation
    v4 in_rotation = qv3(new_box.axis).coeffs();
    auto track_axis_value = track_axis.value();
    if (in_rotation.dot(track_axis_value) < 0) in_rotation = -in_rotation;
    axis = qv3(track_axis.update(in_rotation, param.box_state_gain).data()).matrix();
    
    // filter box states
    if (param.fov_margin == 0) // box fully inside FOV (safe)
    {
        const int num_boxes = (int)box_history.size();
        const int half_n_boxes = num_boxes / 2;
        for (int d = 0; d < 3; ++d) {
            std::vector<float> sort_dim;
            sort_dim.reserve(num_boxes);
            for (const auto& bh : box_history)
                sort_dim.emplace_back(bh.dimension[d]);
            std::sort(sort_dim.begin(), sort_dim.end());
            dimension[d] = sort_dim[half_n_boxes]; // median
        }
        center = track_pos.update(new_box.center, param.box_state_gain);
    }
    else // box extension mode (suffer from optical illusion)
    {
        float new_coeff[3];
        const int num_boxes = (int)box_history.size();
        for (int d = 0; d < 3; ++d)
        {
            v3 end_pt_d[2];                 // two end points of boxes along axis d
            for (const auto s : { 0, 1 }) { // each end point direction
                std::vector<float> sort_half_dim; //sort half dimension
                sort_half_dim.reserve(num_boxes);
                // axis d direction
                v3 dir_d = (s == 0 ? 1 : -1) * axis.col(d);
                // for each box history
                for (const auto& bh : box_history) {
                    // only sort dimension measured within fov
                    if (bh.clear_dir[d][s]) {
                        // 3 box axis in actual length
                        const auto u = bh.half_dir((d + 1) % 3);
                        const auto v = bh.half_dir((d + 2) % 3);
                        const auto w = bh.half_dir(d) * (s == 0 ? 1 : -1);
                        const v3 pl[] = { u + v + w, u - v + w, -u + v + w, -u - v + w };
                        float c_min = std::numeric_limits<float>::max();
                        // rotate old boxes to new axis to prevent historical lengths extended by axis rotation
                        for (const auto& c : pl) {
                            c_min = std::min(c_min, (c + bh.center - center).dot(dir_d));
                        }
                        sort_half_dim.emplace_back(c_min);
                    }
                }
                // sort axis d half ranges
                std::sort(sort_half_dim.begin(), sort_half_dim.end());
                // take stable median
                const int third_quarter = (int)sort_half_dim.size() * 3 / 4;
                // length is the median of lengths within fov
                end_pt_d[s] = center + dir_d * (sort_half_dim.size() ? sort_half_dim[third_quarter] : dimension[d] * 0.5f);
            }
            // reform length from end points along axis d
            dimension[d] = (end_pt_d[0] - end_pt_d[1]).norm();
            new_coeff[d] = (end_pt_d[0] + end_pt_d[1]).dot(axis.col(d)) * 0.5f;
        }
        // new center
        const auto new_center = new_coeff[0] * axis.col(0) + new_coeff[1] * axis.col(1) + new_coeff[2] * axis.col(2);
        center = new_center; // track_pos.update(new_center, param.box_state_gain);
    }
    return true;
}

