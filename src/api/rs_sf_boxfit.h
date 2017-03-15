#pragma once
#ifndef rs_sf_boxfit_h
#define rs_sf_boxfit_h

#include "rs_sf_planefit.h"

struct rs_sf_boxfit : public rs_sf_planefit
{
    struct parameter
    {
        float plane_angle_thr = 1.0f - 0.97f; //absolute threshold for plane angles dot product
        float plane_intersect_thr = 30.0f;    //points on 2 box planes touch within 5mm
        float max_plane_pt_error = 15.0f; 
    };

    struct box
    {
        v3 center;
        v3 dimension;
        m3 axis;

        rs_sf_box to_rs_sf_box() const;
        v3 origin() const { return center - axis * dimension * 0.5f; }
    };

    rs_sf_boxfit(const rs_sf_intrinsics* camera);
    rs_sf_status process_depth_image(const rs_sf_image* img) override;
    rs_sf_status track_depth_image(const rs_sf_image* img) override;

    int num_detected_boxes() const { return (int)m_tracked_boxes.size(); }
    std::vector<rs_sf_box> get_boxes() const;

protected:

    parameter m_param;

    struct plane_pair {
        const plane *p0, *p1; box *box;
        plane_pair(const plane* _p0, const plane* _p1) 
            : p0(_p0), p1(_p1), box(nullptr) {}
    };

    struct box_scene {
        std::vector<plane_pair> plane_pairs;
        std::vector<box> boxes;

        inline void clear() { plane_pairs.clear(); boxes.clear(); }
        inline void swap(box_scene& ref) {
            plane_pairs.swap(ref.plane_pairs);
            boxes.swap(ref.boxes);
        }
    } m_box_scene, m_box_scene_ref;

    struct tracked_box : public box {
        plane_pair* prev_plane_pair;
        std::deque<box> box_history;
    };
    std::deque<tracked_box> m_tracked_boxes;

private:

    void run_static_boxfit(const rs_sf_image* img);

    // per-frame detection
    bool is_valid_box_plane(const plane& p0);
    void form_list_of_plane_pairs(std::vector<plane_pair>& pairs);
    bool form_box_from_two_planes(const plane& plane0, const plane& plane1, box*& new_box_ptr);

    // manage box list
    void add_new_boxes_for_tracking(box_scene& current_view);

    // for debug
    pose_t m_current_pose;
};

#endif // ! rs_sf_boxfit_h