#include "rs_sf_boxfit.h"

rs_sf_boxfit::rs_sf_boxfit(const rs_sf_intrinsics * camera) : rs_sf_planefit(camera)
{
    m_box_scene.plane_scene = &m_view;
    m_box_ref_scene.plane_scene = &m_ref_view;
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

    // part of box origin can be found by the plane intersection
    v3 axis_origin;
    axis_origin[0] = -plane1.d;
    axis_origin[1] = -plane0.d;

    const float cen0_to_plane1_d = plane1.normal.dot(plane0.src->pos) + plane1.d;
    const float cen1_to_plane0_d = plane0.normal.dot(plane1.src->pos) + plane0.d;
    if (cen0_to_plane1_d < 0) {
        axis[0] = -axis[0];   //axis pointing towards plane center
        axis_origin[0] = -axis_origin[0];
    }
    if (cen1_to_plane0_d < 0) {
        axis[1] = -axis[1];   //axis pointing towards plane center
        axis_origin[1] = -axis_origin[1];
    }
    axis[2] = axis[0].cross(axis[1]).normalized();

    // reject concave plane pair
    const v3 cen0_on_plane1 = plane0.src->pos - cen0_to_plane1_d * plane1.normal;
    const v3 cen1_on_plane0 = plane1.src->pos - cen1_to_plane0_d * plane0.normal;
    if ( axis[0].dot(cen0_on_plane1 - view.plane_scene->cam_pose.translation) < 0 ||
        axis[1].dot(cen1_on_plane0 - view.plane_scene->cam_pose.translation) < 0 )
        return false;
    
    // box plane helper
    struct box_plane_t
    {
        typedef std::array<float, 11> box_axis_bin;
        const plane* src;
        vec_pt3d pts;
        const v3* width_axis, *height_axis;
        float height_min;
        box_axis_bin width_min, width_max, height_max;
        int n_bins, upper_bin;

        box_plane_t(const plane* _src) : src(_src)
        {
            width_min.fill(FLOAT_MAX_VALUE);
            width_max.fill(FLOAT_MIN_VALUE);
            height_min = FLOAT_MAX_VALUE;
            height_max.fill(FLOAT_MIN_VALUE);
            n_bins = sizeof(box_axis_bin) / sizeof(float);
            upper_bin = n_bins * 3 / 4;
        }

        void project_pts_on_plane(const float max_plane_pt_error)
        {
            const float d = src->d;
            const v3& normal = src->normal;
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

        v2 approx_width_range(const v3* _width_axis)
        {
            width_axis = _width_axis;
            v2 width_range(FLOAT_MAX_VALUE, FLOAT_MIN_VALUE);
            for (const auto& pt : pts)
            {
                const float coeff = width_axis->dot(pt.pos);
                width_range[0] = std::min(width_range[0], coeff);
                width_range[1] = std::max(width_range[1], coeff);
            }
            return width_range;
        }

        v2 get_height_length(const v3* _height_axis, const v2& width_bin_range, const float height_origin)
        {
            height_axis = _height_axis;
            const float w_bin_origin = width_bin_range[0];
            const float bin_size = (width_bin_range[1] - w_bin_origin) / n_bins;
            for (const auto& pt : pts)
            {
                const float w_len_coeff = width_axis->dot(pt.pos) - w_bin_origin;
                const int w_bin = std::min(n_bins - 1, std::max(0, (int)(w_len_coeff / bin_size)));
                const float h_coeff = height_axis->dot(pt.pos) - height_origin;
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
                const float h_len_coeff = std::abs(height_axis->dot(pt.pos) - h_bin_origin);
                const int h_bin = std::max(0, (int)(h_len_coeff / bin_size));
                if (h_bin < n_bins)
                {
                    const float w_coeff = width_axis->dot(pt.pos);
                    width_max[h_bin] = std::max(width_max[h_bin], w_coeff);
                    width_min[h_bin] = std::min(width_min[h_bin], w_coeff);
                }
            }
            std::sort(width_min.begin(), width_min.end());
            std::sort(width_max.begin(), width_max.end());
            return{ width_min[upper_bin],width_max[upper_bin] };
        }

    } box_plane[2] = { box_plane_t(&plane0),box_plane_t(&plane1) };

    // setup box planes
    box_plane[0].project_pts_on_plane(m_param.max_plane_pt_error);
    box_plane[1].project_pts_on_plane(m_param.max_plane_pt_error);

    // get approximated common width range
    const auto width0_bin_range = box_plane[0].approx_width_range(&axis[2]);
    const auto width1_bin_range = box_plane[1].approx_width_range(&axis[2]);
    const v2 width_bin_range(
        std::max(width0_bin_range[0], width1_bin_range[0]),
        std::min(width0_bin_range[1], width1_bin_range[1]));

    // compute good plane height
    const auto axis0_length = box_plane[0].get_height_length(&axis[0], width_bin_range, axis_origin[0]);
    const auto axis1_length = box_plane[1].get_height_length(&axis[1], width_bin_range, axis_origin[1]);

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
    const v3 box_center = box_axis * (axis_origin + box_dimension*0.5f);

    // make new box
    view.boxes.push_back({ box_center, box_dimension, box_axis });
    pair.box = &view.boxes.back();

#if 0 & defined(__OPENCV_ALL_HPP__) | defined(OPENCV_ALL_HPP)
    
    auto to_cam = m_current_pose.invert();
    auto proj = [to_cam = to_cam, cam = m_intrinsics](const v3& pt) {
        const auto pt3d = to_cam.rotation * pt + to_cam.translation;
        return v2{
            (pt3d.x() * cam.cam_fx) / pt3d.z() + cam.cam_px,
            (pt3d.y() * cam.cam_fy) / pt3d.z() + cam.cam_py };
    };

    rs_sf_image_mono index(&ref_img);
    rs_sf_planefit_draw_plane_ids(this, &index);
    cv::Mat imap(index.img_h, index.img_w, CV_8U, index.data);

    cv::Mat map(ref_img.img_h, ref_img.img_w, CV_8U, cv::Scalar(0));
    cv::Mat src(ref_img.img_h, ref_img.img_w, CV_16U, ref_img.data);
    src.convertTo(map, CV_8U, 255.0 / 4000.0);

    auto pt0 = proj(new_box_ptr->origin());
    auto cv0 = cv::Point((int)pt0.x(), (int)pt0.y());
    for (int a = 0; a < 3; ++a) {
        auto pt1 = proj(box_dimension[a] * box_axis.col(a) + new_box_ptr->origin());
        cv::line(map, cv0, cv::Point((int)pt1.x(), (int)pt1.y()), cv::Scalar(70 * (a + 1)), 2, CV_AA);
    }

    for ( auto& pl : box_plane)
        for (auto& pt : pl.pts)
        {
            auto p0 = proj(pt.pos);
            auto p1 = proj((box_axis.col(2).dot(pt.pos) - axis_origin[2]) * box_axis.col(2) + new_box_ptr->origin());
            cv::circle(map, cv::Point((int)p0.x(), (int)p0.y()), 1, cv::Scalar(255), -1);
            cv::line(map, cv::Point((int)p0.x(), (int)p0.y()), cv::Point((int)p1.x(), (int)p1.y()), cv::Scalar(128), 1, CV_AA);
        }
    cv::imshow("test", map);
#endif

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
    int match_a[3] = { -1,-1,-1 };
    auto match_table = (new_observed_box.axis.transpose() * axis).cwiseAbs();
    const auto match_a0 = match_table.col(0);
    const auto match_a1 = match_table.col(1);
    const auto match_a2 = match_table.col(2);
    if (match_a0(0) > match_a0(1) && match_a0(0) > match_a0(2))
    {
        match_a[0] = 0; match_a[1] = 1; match_a[2] = 2;
        if (match_a1(2) > match_a1(1)) { match_a[1] = 2; match_a[2] = 1; }
    }
    else if (match_a0(1) > match_a0(0) && match_a0(1) > match_a0(2))
    {
        match_a[0] = 1; match_a[1] = 2; match_a[2] = 0;
        if (match_a1(0) > match_a1(2)) { match_a[1] = 0; match_a[2] = 2; }
    }
    else if (match_a0(2) > match_a0(0) && match_a0(2) > match_a0(1))
    {
        match_a[0] = 2; match_a[1] = 0; match_a[2] = 1;
        if (match_a1(1) > match_a1(0)) { match_a[1] = 1; match_a[2] = 0; }
    }

    // change axis
    box& new_box = *this;
    new_box.center = new_observed_box.center;
    new_box.axis.col(0) = new_observed_box.axis.col(match_a[0]);
    new_box.axis.col(1) = new_observed_box.axis.col(match_a[1]);
    new_box.axis.col(2) = new_observed_box.axis.col(match_a[2]);
    new_box.dimension[0] = new_observed_box.dimension[match_a[0]];
    new_box.dimension[1] = new_observed_box.dimension[match_a[1]];
    new_box.dimension[2] = new_observed_box.dimension[match_a[2]];

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
        const Eigen::Quaternionf rotation(bh.axis);

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
    axis = Eigen::Quaternionf(
        sort_q[3][half_n_boxes],
        sort_q[0][half_n_boxes], 
        sort_q[1][half_n_boxes], 
        sort_q[2][half_n_boxes]).matrix();

    return true;
}
