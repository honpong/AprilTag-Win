#pragma once
#ifndef rs_sf_boxfit_h
#define rs_sf_boxfit_h

#include "rs_sf_planefit.h"

struct rs_sf_boxfit : public rs_sf_planefit
{
    struct parameter
    {
        float plane_angle_thr = 1.0f - 0.95f; //absolute threshold for plane angles dot product
        float plane_intersect_thr = 30.0f;    //points on 2 box planes touch within 5mm
        float max_plane_pt_error = 20.0f; 
        int max_box_history = 11;
    };

    struct box
    {
        v3 center;
        v3 dimension;
        m3 axis;

        rs_sf_box to_rs_sf_box() const;
        v3 origin() const { return center - axis * dimension * 0.5f; }
        float max_radius() const { return (center - origin()).norm(); }

        void print() const {
            std::cout <<
                "center : " << center.transpose() << std::endl <<
                "dimension : " << dimension.transpose() << std::endl <<
                "axis :" << std::endl << axis << std::endl;
        }
    };

    rs_sf_boxfit(const rs_sf_intrinsics* camera);
    rs_sf_status process_depth_image(const rs_sf_image* img) override;
    rs_sf_status track_depth_image(const rs_sf_image* img) override;

    int num_detected_boxes() const { return (int)m_tracked_boxes.size(); }
    std::vector<rs_sf_box> get_boxes() const;

protected:

    parameter m_param;

    struct tracked_box;
    struct plane_pair {
        plane *p0, *p1; box *box; tracked_box* prev_box;
        plane_pair(plane* _p0, plane* _p1, tracked_box* _pb = nullptr)
            : p0(_p0), p1(_p1), prev_box(_pb), box(nullptr) {}
    };

    struct box_scene {
        std::vector<plane_pair> plane_pairs;
        std::vector<box> boxes;
        scene* plane_scene;

        inline void clear() { plane_pairs.clear(); boxes.clear(); }
        inline void swap(box_scene& ref) {
            plane_pairs.swap(ref.plane_pairs);
            boxes.swap(ref.boxes);
        }
    } m_box_scene, m_box_ref_scene;

    struct tracked_box : public box {
        int pid[3];
        std::deque<box> box_history;
        bool updated;

        tracked_box(const plane_pair& pair) : box(*pair.box), updated(false) {
            pid[0] = pair.p0->pid;
            pid[1] = pair.p1->pid;
            pid[2] = 0;
        }
        bool try_update(const plane_pair& pair, int max_box_history);
    };
    typedef std::deque<tracked_box> queue_tracked_box;
    queue_tracked_box m_tracked_boxes;

private:

    struct box_plane_t;

    // per-frame detection
    void form_list_of_plane_pairs(box_scene& view);
    void detect_new_boxes(box_scene& view);
    bool is_valid_box_plane(const plane& p0);
    bool form_box_from_two_planes(box_scene& view, plane_pair& pair);

    // manage box list
    void add_new_boxes_for_tracking(box_scene& view);

    // debug
    void draw_box(const std::string& name, const box& src, const box_plane_t* p0 = nullptr, const box_plane_t* p1 = nullptr) const;
};

#endif // ! rs_sf_boxfit_h