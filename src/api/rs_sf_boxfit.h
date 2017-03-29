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
        float box_state_gain = 0.3f;
        int max_box_history = 11;
        int max_box_miss_frame = 5;
        bool refine_box_plane = false;
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
    rs_sf_status set_option(rs_sf_fit_option option, double value) override;
    rs_sf_status process_depth_image(const rs_sf_image* img) override;
    rs_sf_status track_depth_image(const rs_sf_image* img) override;

    int num_detected_boxes() const { return (int)m_tracked_boxes.size(); }
    rs_sf_box get_box(int box_id) const { return m_tracked_boxes[box_id].to_rs_sf_box(); }
    std::vector<rs_sf_box> get_boxes() const;

protected:

    parameter m_param;

    template<typename vn>
    struct state_vn {
        vn state[3], obse[3], pred[3], resi[3];
        state_vn(const vn& init) {
            for (int i = 0; i < sizeof(state) / sizeof(vn); ++i)
                state[i] = obse[i] = pred[i] = resi[i].setZero();
            state[0] = obse[0] = init;
        }
        const vn& value() const { return state[0]; }
        const vn& update(const vn& observation, const float gain) {
            // update prediction
            pred[2] = state[2];
            pred[1] = pred[2] + state[1];
            pred[0] = pred[1] + state[0];
            // update observation
            obse[2] = observation - obse[0] - obse[1];
            obse[1] = observation - obse[0];
            obse[0] = observation;
            // prediction error
            resi[0] = obse[0] - pred[0];
            resi[1] = obse[1] - pred[1];
            resi[2] = obse[2] - pred[2];
            // update state
            state[0] = pred[0] + resi[0] * gain;
            state[1] = pred[1] + resi[1] * gain;
            state[2] = pred[2] + resi[2] * gain;
            return state[0];
        }
    };

    struct tracked_box;
    struct plane_pair {
        plane *p0, *p1, *p2; box *new_box; tracked_box* prev_box;
        plane_pair(plane* _p0, plane* _p1, tracked_box* _pb = nullptr, box *_nb = nullptr)
            : p0(_p0), p1(_p1), p2(nullptr), prev_box(_pb), new_box(_nb) {}
    };

    struct box_plane_map
    {
        static const int MVP2 = MAX_VALID_PID + 2;
        bool plane_used[MVP2];
        tracked_box* plane_to_box[MVP2];
        void reset() {
            memset(plane_used, 0, sizeof(plane_used));
            memset(plane_to_box, 0, sizeof(plane_to_box));
        }
        void add(tracked_box& box) {
            plane_to_box[box.pid[0]] = &box;
            plane_to_box[box.pid[1]] = &box;
            plane_to_box[box.pid[2]] = &box;
        }
        tracked_box* get(int pid0, int pid1) {
            if (plane_to_box[pid0]) return plane_to_box[pid0];
            if (plane_to_box[pid1]) return plane_to_box[pid1];
            return nullptr;
        }
        void mark(plane_pair& pair) {
            plane_used[pair.p0->pid] = plane_used[pair.p1->pid] = true;
            if (pair.p2) plane_used[pair.p2->pid] = true;
        }
        plane_pair form_pair(plane* p0, plane* p1) {
            return plane_pair(p0, p1, get(p0->pid, p1->pid));
        }
    } m_bp_map;

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
        std::list<box> box_history;
        state_vn<v3> track_pos;
        state_vn<v4> track_axis;
        int count_miss;
        tracked_box(const plane_pair& pair) : box(*pair.new_box),
            track_pos(center), track_axis(qv3(axis).coeffs()), count_miss(0) {
            pid[0] = pair.p0->pid;
            pid[1] = pair.p1->pid;
            pid[2] = (pair.p2 ? pair.p2->pid : 0);
        }
        bool try_update(const plane_pair& pair, const parameter& param);
    };
    typedef std::deque<tracked_box> queue_tracked_box;
    queue_tracked_box m_tracked_boxes;

private:

    struct box_plane_t;

    // per-frame detection
    void detect_new_boxes(box_scene& view);
    bool is_valid_box_plane(const plane& p0);
    bool form_box_from_two_planes(box_scene& view, plane_pair& pair);
    bool refine_box_from_third_plane(box_scene& view, plane_pair& pair, plane& p2);

    // manage box list
    void update_tracked_boxes(box_scene& view);

    // debug
    void draw_box(const std::string& name, const box& src, const box_plane_t* p0 = nullptr, const box_plane_t* p1 = nullptr) const;
};

#endif // ! rs_sf_boxfit_h