#include "rs_sf_boxfit.h"

rs_sf_boxfit::rs_sf_boxfit(const rs_sf_intrinsics * camera) : rs_sf_planefit(camera)
{
}

rs_sf_status rs_sf_boxfit::process_depth_image(const rs_sf_image * img)
{
    auto pf_status = rs_sf_planefit::process_depth_image(img);
    if (pf_status < 0) return pf_status;

    run_static_boxfit(img);
    return pf_status;
}

rs_sf_status rs_sf_boxfit::track_depth_image(const rs_sf_image * img)
{
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

bool rs_sf_boxfit::form_box_from_two_planes(const plane& plane0, const plane& plane1, box* &new_box_ptr)
{
    auto to_cam = m_current_pose.invert();
    auto proj = [to_cam = to_cam, cam = m_intrinsics](const v3& pt) {
        const auto pt3d = to_cam.rotation * pt + to_cam.translation;
        return v2{
            (pt3d.x() * cam.cam_fx) / pt3d.z() + cam.cam_px,
            (pt3d.y() * cam.cam_fy) / pt3d.z() + cam.cam_py };
    };

    static const float max_float = std::numeric_limits<float>::max();
    static const float min_float = -max_float;

    auto cosine_theta = std::abs(plane0.normal.dot(plane1.normal));
    if (cosine_theta > m_param.plane_angle_thr) return false;

    m3 box_axis;
    box_axis.col(2) = plane0.normal.cross(plane1.normal).normalized();
    box_axis.col(1) = plane1.normal; // box_axis.col(2).cross(plane0.normal).normalized();
    box_axis.col(0) = plane0.normal; // plane1.normal.cross(box_axis.col(2)).normalized();

    v3 axis_min(max_float, max_float, max_float);
    v3 axis_max(min_float, min_float, min_float);

    vec_pt3d plane_pt[2];
    plane_pt[0].reserve(plane0.best_pts.size());
    plane_pt[1].reserve(plane1.best_pts.size());
    const plane* plane_src[] = { &plane0, &plane1 };
    for (int i = 0; i < 2; ++i) {
        auto normal = box_axis.col(i);
        auto d = plane_src[i]->d;
        for (auto& pt : plane_src[i]->best_pts) {
            auto plpt = *pt;
            auto correct = normal.dot(pt->pos) + d;
            plpt.pos = pt->pos - correct * normal;
            
            if (std::abs(correct) > 10.0f) {
                //printf("%.2f ", correct);
            }
            else {
                plane_pt[i].push_back(plpt);
            }
        }
        //printf("\n");
    }

    for (int i = 0; i < 2; ++i) {
        for (auto& pt : plane_pt[1 - i]) {
            const auto axis_proj_i = box_axis.col(i).dot(pt.pos);
            axis_min[i] = std::min(axis_proj_i, axis_min[i]);
            axis_max[i] = std::max(axis_proj_i, axis_max[i]);

            const auto axis_proj_2 = box_axis.col(2).dot(pt.pos);
            axis_min[2] = std::min(axis_proj_2, axis_min[2]);
            axis_max[2] = std::max(axis_proj_2, axis_max[2]);
        }
    }

    // one end of a box axis can be found by the plane itself
    float known_endpt_on_axis[] = {
        box_axis.col(0).dot(plane0.src->pos),
        box_axis.col(1).dot(plane1.src->pos) };

    // check if two planes touch in 3D
    for (int i = 0; i < 2; ++i) {
        const float end_pt_diff_min = std::abs(axis_min[i] - known_endpt_on_axis[i]);
        const float end_pt_diff_max = std::abs(axis_max[i] - known_endpt_on_axis[i]);

        if (std::min(end_pt_diff_min, end_pt_diff_max) > m_param.plane_intersect_thr)
            return false; //end points too far, two planes are not touching
    }

    // form the box origin by min end points of three axis
    const v3 box_translation = box_axis * axis_min;

    // form the box dimension by end point differences
    const v3 box_dimension = axis_max - axis_min;

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

    for (auto& pl : plane_pt)
        for (auto& pt : pl)
        {
            auto p0 = proj(pt.pos);
            auto p1 = proj((box_axis.col(2).dot(pt.pos) - axis_min[2]) * box_axis.col(2) + box_translation);
            cv::circle(map, cv::Point((int)p0.x(), (int)p0.y()), 1, cv::Scalar(255), -1);
            cv::line(map, cv::Point((int)p0.x(), (int)p0.y()), cv::Point((int)p1.x(), (int)p1.y()), cv::Scalar(128), 1, CV_AA);
        }

    // make new box
    m_boxes.push_back({ box_translation,box_dimension,box_axis });
    new_box_ptr = &m_boxes.back();

    return true;
}

rs_sf_box rs_sf_boxfit::box::to_rs_sf_box() const
{
    rs_sf_box dst;
    v3_map(dst.origin) = translation;
    m3_axis_map((float*)dst.axis) = (axis * dimension.asDiagonal());
    return dst;
}
