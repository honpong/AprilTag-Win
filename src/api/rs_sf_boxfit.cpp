#include "rs_sf_boxfit.h"

static rs_sf_boxfit* debug_this = nullptr;
rs_sf_boxfit::rs_sf_boxfit(const rs_sf_intrinsics * camera) : rs_sf_planefit(camera)
{
    rs_sf_planefit::m_param.filter_plane_map = false;
    //rs_sf_planefit::m_param.img_x_dn_sample = 3;
    //rs_sf_planefit::m_param.img_y_dn_sample = 3;
    //parameter_updated();

    m_box_scene.plane_scene = &m_view;
    m_box_ref_scene.plane_scene = &m_ref_view;
    debug_this = this;
}

rs_sf_status rs_sf_boxfit::process_depth_image(const rs_sf_image * img)
{
    cv::waitKey(0);

    auto pf_status = rs_sf_planefit::process_depth_image(img);
    if (pf_status < 0) return pf_status;

    m_tracked_boxes.clear();
    m_box_scene.clear();
    m_box_ref_scene.clear();
    
    form_list_of_plane_pairs(m_box_scene);
    detect_new_boxes(m_box_scene);
    add_new_boxes_for_tracking(m_box_scene);

    return pf_status;
}

rs_sf_status rs_sf_boxfit::track_depth_image(const rs_sf_image * img)
{
    cv::waitKey(0);

    auto pf_status = rs_sf_planefit::track_depth_image(img);
    if (pf_status < 0) return pf_status;

    m_box_scene.swap(m_box_ref_scene);

    form_list_of_plane_pairs(m_box_scene);
    detect_new_boxes(m_box_scene);
    add_new_boxes_for_tracking(m_box_scene);

    return pf_status;
}

std::vector<rs_sf_box> rs_sf_boxfit::get_boxes() const
{
    std::vector<rs_sf_box> dst;
    dst.reserve(num_detected_boxes());
    for (const auto& box : m_tracked_boxes)
        dst.push_back(box.to_rs_sf_box());
    return dst;
}

void rs_sf_boxfit::detect_new_boxes(box_scene& view)
{
    for (auto& pair : view.plane_pairs)
    {
        form_box_from_two_planes(view, pair);
    }
}

bool rs_sf_boxfit::is_valid_box_plane(const plane & p0)
{
    return p0.best_pts.size() >= rs_sf_planefit::m_param.min_num_plane_pt;
}

void rs_sf_boxfit::form_list_of_plane_pairs(box_scene& view)
{
    const int num_plane = (int)m_sorted_plane_ptr.size();
    const int max_pid = (int)m_tracked_pid.size();
    view.plane_pairs.clear();
    view.plane_pairs.reserve(num_plane*(num_plane + 1) + num_detected_boxes());

    // set null box ptr to all tracked planes
    std::vector<std::vector<tracked_box*>> box_plane_table(max_pid);
    for (auto& row : box_plane_table) row.assign(max_pid, nullptr);

    // old box pairs 
    for (auto& box : m_tracked_boxes)
    {
        auto* p0 = get_tracked_plane(box.pid[0]);
        auto* p1 = get_tracked_plane(box.pid[1]);
        if (p0 && p1) {
            box_plane_table[box.pid[0]][box.pid[1]] = &box;
            box_plane_table[box.pid[1]][box.pid[0]] = &box;
        }
    }

    // form all box pairs
    for (int i = 0; i < num_plane; ++i) 
    {
        if (is_valid_box_plane(*m_sorted_plane_ptr[i])) {
            for (int j = i + 1; j < num_plane; ++j) {
                if (is_valid_box_plane(*m_sorted_plane_ptr[j])) {
                    view.plane_pairs.emplace_back(m_sorted_plane_ptr[i], m_sorted_plane_ptr[j],
                        box_plane_table[m_sorted_plane_ptr[i]->pid][m_sorted_plane_ptr[j]->pid]);
                }
            }
        }
    }

    view.boxes.clear();
    view.boxes.reserve(view.plane_pairs.size());
}

#include <array>
// box plane helper
struct rs_sf_boxfit::box_plane_t
{
    typedef std::array<float, 11> box_axis_bin;
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
        upper_bin = n_bins * 3 / 4;
    }

    void project_pts_on_plane(const v3& _normal, const float _d, const float max_plane_pt_error)
    {
        d = _d;
        normal = _normal;
        pts.reserve(src->best_pts.size());
        for (auto& pt : src->best_pts)
        {
            if (pt->valid_pos)
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
        std::sort(width_min.begin(), width_min.end());
        std::sort(width_max.begin(), width_max.end());
        return{ width_min[upper_bin],width_max[upper_bin] };
    }
};

bool rs_sf_boxfit::form_box_from_two_planes(box_scene& view, plane_pair& pair)
{
    auto& plane0 = *pair.p0, &plane1 = *pair.p1;

    // reject plane pair not perpendiculars only if no box history
    if (!pair.prev_box) {
        auto cosine_theta = std::abs(plane0.normal.dot(plane1.normal));
        if (cosine_theta > m_param.plane_angle_thr) return false;
    }

    // box axis
    v3 axis[3] = { plane1.normal, plane0.normal, {} };

    const float cen0_to_plane1_d = plane1.normal.dot(plane0.src->pos) + plane1.d;
    const float cen1_to_plane0_d = plane0.normal.dot(plane1.src->pos) + plane0.d;
    // axis pointing towards plane cetner
    axis[0] = (cen0_to_plane1_d < 0 ? -axis[0] : axis[0]);
    axis[1] = (cen1_to_plane0_d < 0 ? -axis[1] : axis[1]);

    // reject concave plane pair
    const v3 cen0_on_plane1 = plane0.src->pos - cen0_to_plane1_d * plane1.normal;
    const v3 cen1_on_plane0 = plane1.src->pos - cen1_to_plane0_d * plane0.normal;
    if ( axis[0].dot(cen0_on_plane1 - view.plane_scene->cam_pose.translation) < 0 ||
        axis[1].dot(cen1_on_plane0 - view.plane_scene->cam_pose.translation) < 0 )
        return false;

    // third axis is the cross product of two plane normals
    axis[2] = axis[0].cross(axis[1]).normalized();

    // fix non-orthogonal axis
    const v3 avg_normal = (axis[0] + axis[1]).normalized();
    qv3 avg_to_n0(Eigen::AngleAxisf(-M_PI_4, axis[2]));
    qv3 avg_to_n1(Eigen::AngleAxisf(M_PI_4, axis[2]));
    axis[0] = avg_to_n0._transformVector(avg_normal);
    axis[1] = avg_to_n1._transformVector(avg_normal);

    // part of box origin can be found by the plane intersection
    v3 axis_origin;
    axis_origin[0] = axis[0].dot(plane1.src->pos);
    axis_origin[1] = axis[1].dot(plane0.src->pos);
 
    // box plane helper
    box_plane_t box_plane[2] = { box_plane_t(plane0),box_plane_t(plane1) };

    // setup box planes
    box_plane[0].project_pts_on_plane(axis[1], -axis_origin[1], m_param.max_plane_pt_error);
    box_plane[1].project_pts_on_plane(axis[0], -axis_origin[0], m_param.max_plane_pt_error);

    // get approximated common width range
    const auto width0_bin_range = box_plane[0].approx_width_range(axis[2]);
    const auto width1_bin_range = box_plane[1].approx_width_range(axis[2]);
    const v2 width_bin_range(
        std::max(width0_bin_range[0], width1_bin_range[0]),
        std::min(width0_bin_range[1], width1_bin_range[1]));

    // compute good plane height
    const auto axis0_length = box_plane[0].get_height_length(axis[0], width_bin_range, axis_origin[0]);
    const auto axis1_length = box_plane[1].get_height_length(axis[1], width_bin_range, axis_origin[1]);

    // check if two planes touch in 3D
    if (!pair.prev_box) {
        if (std::abs(axis0_length[0]) > m_param.plane_intersect_thr) return false;
        if (std::abs(axis1_length[0]) > m_param.plane_intersect_thr) return false;
    }

    // compute good plane widths
    const auto width0_range = box_plane[0].get_width_range(axis_origin[0]);
    const auto width1_range = box_plane[1].get_width_range(axis_origin[1]);
    axis_origin[2] = std::min(width0_range[0], width1_range[0]);
    const auto axis2_length = std::max(width0_range[1], width1_range[1]) - axis_origin[2];

    // setup box axis
    m3 box_axis;
    box_axis.col(0) = axis[0];
    box_axis.col(1) = axis[1];
    box_axis.col(2) = axis[2];

    // form the box dimension by end point differences
    const v3 box_dimension(axis0_length[1], axis1_length[1], axis2_length);

    // form the box center by box origin + 1/2 box dimension
    const v3 box_center = box_axis * (axis_origin + (box_dimension*0.5f));

    // make new box
    view.boxes.push_back({ box_center, box_dimension, box_axis });
    pair.box = &view.boxes.back();

    return true;
}

void rs_sf_boxfit::add_new_boxes_for_tracking(box_scene & view)
{
    for (auto& box : m_tracked_boxes)
        box.updated = false;

    // each newly detected box
    for (auto& pair : view.plane_pairs) {
        if (pair.box != nullptr) //new box
        {
            for (auto& old_box : m_tracked_boxes) {
                if (old_box.try_update(pair, m_param.max_box_history)) {
                    pair.box = nullptr;
                    old_box.updated = true;
                    break;
                }
            }
        }
    }

    // delete boxes that lost tracking
    queue_tracked_box prev_tracked_boxes;
    prev_tracked_boxes.swap(m_tracked_boxes);
    for (auto& box : prev_tracked_boxes) {
        if (box.updated) m_tracked_boxes.push_back(box);
    }

    // each newly detected box without match
    for (auto& pair : view.plane_pairs) {
        if (pair.box != nullptr) {
            m_tracked_boxes.emplace_back(pair);
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
            (pt3d.x() * cam.cam_fx) / pt3d.z() + cam.cam_px,
            (pt3d.y() * cam.cam_fy) / pt3d.z() + cam.cam_py);
    };

    cv::Mat map(ref_img.img_h, ref_img.img_w, CV_8U, cv::Scalar(0));
    cv::Mat(ref_img.img_h, ref_img.img_w, CV_16U, ref_img.data).convertTo(map, CV_8U, 255.0 / 4000.0);

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

bool rs_sf_boxfit::tracked_box::try_update(const plane_pair& pair, int max_box_history)
{
    // center not overlap
    auto& new_observed_box = *pair.box;

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
    box& new_box = *this;
    new_box.center = new_observed_box.center;
    new_box.axis = new_observed_box.axis * (match_select.cwiseProduct(match_table.cwiseSign()));
    new_box.dimension = new_observed_box.dimension.transpose() * match_select;

    // add to history
    pid[0] = pair.p0->pid;
    pid[1] = pair.p1->pid;
    box_history.push_back(new_box);
    while (box_history.size() > max_box_history) box_history.pop_front();

    const int num_boxes = (int)box_history.size();
    const int half_n_boxes = num_boxes / 2;

    // filter box history
    std::vector<float> sort_center[3], sort_dim[3], sort_q[4];
    for (int d = 0; d < 3; ++d) {
        sort_center[d].reserve(num_boxes);
        sort_dim[d].reserve(num_boxes);
        sort_q[d].reserve(num_boxes);
    }
    for (const auto& bh : box_history) {
        const qv3 rotation(bh.axis);

        for (int d = 0; d < 3; ++d) {
            sort_center[d].emplace_back(bh.center[d]);
            sort_dim[d].emplace_back(bh.dimension[d]);
            sort_q[d].emplace_back(rotation.coeffs()[d]);
        }
        sort_q[3].emplace_back(rotation.w());
    }
    for (int d = 0; d < 3; ++d) {
        std::sort(sort_center[d].begin(), sort_center[d].end());
        std::sort(sort_dim[d].begin(), sort_dim[d].end());
        std::sort(sort_q[d].begin(), sort_q[d].end());
        center[d] = sort_center[d][half_n_boxes];
        dimension[d] = sort_dim[d][half_n_boxes];
    }
    std::sort(sort_q[3].begin(), sort_q[3].end());
    axis = qv3(
        sort_q[3][half_n_boxes],
        sort_q[0][half_n_boxes],
        sort_q[1][half_n_boxes],
        sort_q[2][half_n_boxes]).matrix();

    return true;
}
