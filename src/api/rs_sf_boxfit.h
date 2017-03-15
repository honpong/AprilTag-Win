#pragma once
#ifndef rs_sf_boxfit_h
#define rs_sf_boxfit_h

#include "rs_sf_planefit.h"

struct rs_sf_boxfit : public rs_sf_planefit
{
    struct parameter
    {
        float plane_angle_thr = 1.0f - 0.97f; //absolute threshold for plane angles dot product
        float plane_intersect_thr = 15.0f;     //points on 2 box planes touch within 5mm
        float max_plane_pt_error = 15.0f; 
    };

    struct box
    {
        v3 translation;
        v3 dimension;
        m3 axis;

        rs_sf_box to_rs_sf_box() const;
    };

    rs_sf_boxfit(const rs_sf_intrinsics* camera);
    rs_sf_status process_depth_image(const rs_sf_image* img) override;
    rs_sf_status track_depth_image(const rs_sf_image* img) override;

    int num_detected_boxes() const { return (int)m_boxes.size(); }
    std::vector<rs_sf_box> get_boxes() const;

protected:

    parameter m_param;

    struct plane_pair {
        const plane *p0, *p1; box *box;
        plane_pair(const plane* _p0, const plane* _p1) 
            : p0(_p0), p1(_p1), box(nullptr) {}
    };
    std::vector<plane_pair> m_plane_pairs;
    std::vector<box> m_boxes;

private:

    void run_static_boxfit(const rs_sf_image* img);

    bool is_valid_box_plane(const plane& p0);
    void form_list_of_plane_pairs(std::vector<plane_pair>& pairs);
    bool form_box_from_two_planes(const plane& plane0, const plane& plane1, box*& new_box_ptr);

    // for debug
    pose_t m_current_pose;
};

#endif // ! rs_sf_boxfit_h