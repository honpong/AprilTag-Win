#include "rs_sf_boxfit.h"

rs_sf_boxfit::rs_sf_boxfit(const rs_sf_intrinsics * camera) : rs_sf_planefit(camera)
{
}

rs_sf_status rs_sf_boxfit::process_depth_image(const rs_sf_image * img)
{
    cv::waitKey(0);
    auto pf_status = rs_sf_planefit::process_depth_image(img);
    if (pf_status < 0) return pf_status;

    run_static_boxfit(img);
    return pf_status;
}

rs_sf_status rs_sf_boxfit::track_depth_image(const rs_sf_image * img)
{
    cv::waitKey(0);
    auto pf_status = rs_sf_planefit::track_depth_image(img);
    if (pf_status < 0) return pf_status;

    run_static_boxfit(img);
    return pf_status;
}

std::vector<rs_sf_box> rs_sf_boxfit::get_boxes() const
{
    std::vector<rs_sf_box> dst;
    dst.reserve(num_detected_boxes());
    for (const auto& box : m_boxes)
        dst.push_back(box.to_rs_sf_box());
    return dst;
}

void rs_sf_boxfit::run_static_boxfit(const rs_sf_image * img)
{
    ir_img = img[1];

    m_current_pose.set_pose(img->cam_pose);

    m_boxes.clear();
    m_boxes.reserve(MAX_VALID_PID);

    form_list_of_plane_pairs(m_plane_pairs);
    for (auto& pairs : m_plane_pairs)
        form_box_from_two_planes(*pairs.p0, *pairs.p1, pairs.box);
}

bool rs_sf_boxfit::is_valid_box_plane(const plane & p0)
{
    return p0.best_pts.size() >= rs_sf_planefit::m_param.min_num_plane_pt;
}

void rs_sf_boxfit::form_list_of_plane_pairs(std::vector<plane_pair>& pairs)
{
    const int num_plane = (int)m_sorted_plane_ptr.size();
    pairs.clear();
    pairs.reserve(num_plane*(num_plane + 1));
    for (int i = 0; i < num_plane; ++i) {
        if (is_valid_box_plane(*m_sorted_plane_ptr[i])) {
            for (int j = i + 1; j < num_plane; ++j) {
                if (is_valid_box_plane(*m_sorted_plane_ptr[j])) {
                    pairs.emplace_back(m_sorted_plane_ptr[i], m_sorted_plane_ptr[j]);
                }
            }
        }
    }
}

#include <array>
bool rs_sf_boxfit::form_box_from_two_planes(const plane& plane0, const plane& plane1, box* &new_box_ptr)
{
    // reject plane pair not perpendicular
    auto cosine_theta = std::abs(plane0.normal.dot(plane1.normal));
    if (cosine_theta > m_param.plane_angle_thr) return false;
    

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
        
        box_plane_t(const plane* _src) : src(_src) {
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

    // box axis
    v3 axis[3] = { plane1.normal, plane0.normal, {} };

    // part of box origin can be found by the plane intersection
    v3 axis_origin;
    axis_origin[0] = -plane1.d;
    axis_origin[1] = -plane0.d;

    if (plane1.normal.dot(box_plane[0].src->src->pos) < axis_origin[0]) {
        axis[0] = -axis[0];
        axis_origin[0] = -axis_origin[0];
    }
    if (plane0.normal.dot(box_plane[1].src->src->pos) < axis_origin[1]) {
        axis[1] = -axis[1];
        axis_origin[1] = -axis_origin[1];
    }
    axis[2] = axis[0].cross(axis[1]).normalized();

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
    if (std::abs(axis0_length[0]) > m_param.plane_intersect_thr) return false;
    if (std::abs(axis1_length[0]) > m_param.plane_intersect_thr) return false;

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

    // form the box origin by min end points of three axis
    const v3 box_translation = box_axis * axis_origin;

    // form the box dimension by end point differences
    const v3 box_dimension(axis0_length[1], axis1_length[1], axis2_length);

    // make new box
    m_boxes.push_back({ box_translation,box_dimension,box_axis });
    new_box_ptr = &m_boxes.back();

#if defined(__OPENCV_ALL_HPP__) | defined(OPENCV_ALL_HPP)

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

    auto pt0 = proj(box_translation);
    auto cv0 = cv::Point((int)pt0.x(), (int)pt0.y());
    for (int a = 0; a < 3; ++a) {
        auto pt1 = proj(box_dimension[a] * box_axis.col(a) + box_translation);
        cv::line(map, cv0, cv::Point((int)pt1.x(), (int)pt1.y()), cv::Scalar(70 * (a + 1)), 2, CV_AA);
    }

    for ( auto& pl : box_plane)
        for (auto& pt : pl.pts)
        {
            auto p0 = proj(pt.pos);
            auto p1 = proj((box_axis.col(2).dot(pt.pos) - axis_origin[2]) * box_axis.col(2) + box_translation);
            cv::circle(map, cv::Point((int)p0.x(), (int)p0.y()), 1, cv::Scalar(255), -1);
            cv::line(map, cv::Point((int)p0.x(), (int)p0.y()), cv::Point((int)p1.x(), (int)p1.y()), cv::Scalar(128), 1, CV_AA);
        }
    cv::imshow("test", map);
#endif

    return true;
}

rs_sf_box rs_sf_boxfit::box::to_rs_sf_box() const
{
    rs_sf_box dst;
    v3_map(dst.origin) = translation;
    m3_axis_map((float*)dst.axis) = (axis * dimension.asDiagonal());
    return dst;
}
