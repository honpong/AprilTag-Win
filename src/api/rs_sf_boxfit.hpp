/*******************************************************************************
 
 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or nondisclosure
 agreement with Intel Corporation and may not be copied or disclosed except in
 accordance with the terms of that agreement
 Copyright(c) 2017 Intel Corporation. All Rights Reserved.
 
 *******************************************************************************/
//
//  rs_sf_boxfit.hpp
//  algo-core
//
//  Created by Hon Pong (Gary) Ho
//
#pragma once
#ifndef rs_sf_boxfit_hpp
#define rs_sf_boxfit_hpp

#include "rs_sf_planefit.hpp"

#define NUM_BOX_PLANE_BIN 21 //for future enhancement to dynamic bin count

struct rs_sf_boxfit : public rs_sf_planefit
{
    struct parameter
    {
        float plane_pair_angle_thr   = 0.030f; // max dot product of box plane pair normals
        float tracked_pair_angle_thr = 0.050f; // max dot product of tracked box plane pair normals
        float plane_intersect_thr    = 0.030f; // points on 2 box planes touch within 30mm
        float min_box_thickness      = 0.060f; // minimum box thickness in meter
        float max_box_thickness      = 1.500f; // maximum box thickness in meter
        float max_plane_pt_error     = 0.015f; // max point to box plane error
        float box_state_gain         = 0.100f; // fraction of box update allowed per frame
        float box_miss_ms            = 500.0f; // milliseconds allowed for a tracked box get lost
        float fov_margin             = 2.000f; // extend box length outside fov (0 no extension)
        int   max_box_history        = 11;     // length of box history per tracked box
        bool  refine_box_plane       = false;  // flag to refine box edge
        
        parameter get_preset(rs_sf_param_preset s) const {
            //if (s==RS_SF_PRESET_BOX_S) { return {0.03f,0.05f,0.03f,0.03f,1.10f,0.015f,0.1f,500.0f,fov_margin,11,refine_box_plane}; }
            //if (s==RS_SF_PRESET_BOX_L) { return {0.03f,0.05f,0.03f,0.30f,2.00f,0.015f,0.1f,500.0f,fov_margin,31,refine_box_plane}; }
            //return                              {0.03f,0.05f,0.03f,0.06f,1.50f,0.015f,0.1f,500.0f,fov_margin,11,refine_box_plane};

            parameter dst;
            dst.fov_margin = fov_margin;
            dst.refine_box_plane = refine_box_plane;
            if (s == RS_SF_PRESET_BOX_S) { dst.min_box_thickness = 0.03f; dst.max_box_thickness = 1.10f; }
            if (s == RS_SF_PRESET_BOX_L) { dst.min_box_thickness = 0.30f; dst.max_box_thickness = 2.00f; dst.max_box_history = 31; }
            return dst;
        }
    };
    
    struct box
    {
        v3 center;
        v3 dimension;
        m3 axis;
        
        rs_sf_box to_rs_sf_box() const;
        v3 origin() const { return center - axis * dimension * 0.5f; }
        float max_radius() const { return (center - origin()).norm(); }
        float min_dimension() const { return dimension.minCoeff(); }
        float max_dimension() const { return dimension.maxCoeff(); }
    };
    
    rs_sf_boxfit(const rs_sf_intrinsics* camera);
    ~rs_sf_boxfit() override { run_task(-1); }
    rs_sf_status set_option(rs_sf_fit_option option, double value) override;
    rs_sf_status set_locked_outputs() override;
    rs_sf_status process_depth_image() override;
    rs_sf_status track_depth_image() override;
    
    int num_detected_boxes() const { return (int)m_box_ref_scene.tracked_boxes.size(); }
    rs_sf_box get_box(int box_id, float& history_progress) const { return m_box_ref_scene.get_tracked_box(box_id, history_progress, m_param.max_box_history); }
    std::vector<rs_sf_box> get_boxes() const;
    
protected:
    
    parameter m_param;
    
    template<int n>
    struct state_v {
        v<n> state[3], obse[3], pred[3], resi[3];
        state_v(const v<n>& init)
        {
            for (int d = 0; d < n; ++d) {
                for (int i = 0; i < 3; ++i)
                {
                    state[i][d] = 0;
                    obse[i][d] = 0;
                    pred[i][d] = 0;
                    resi[i][d] = 0;
                }
                state[0][d] = obse[0][d] = init[d];
            }
        }
        const v<n>& value() const { return state[0]; }
        const v<n>& update(const v<n>& observation, const float gain)
        {
            for (int d = 0; d < n; ++d)
            {
                // update prediction
                pred[2][d] = state[2][d];
                pred[1][d] = pred[2][d] + state[1][d];
                pred[0][d] = pred[1][d] + state[0][d];
            }
            
            for (int d = 0; d < n; ++d)
            {
                // update observation
                obse[2][d] = observation[d] - obse[0][d] - obse[1][d];
                obse[1][d] = observation[d] - obse[0][d];
                obse[0][d] = observation[d];
            }
            
            for (int d = 0; d < n; ++d)
            {
                // prediction error
                resi[0][d] = obse[0][d] - pred[0][d];
                resi[1][d] = obse[1][d] - pred[1][d];
                resi[2][d] = obse[2][d] - pred[2][d];
            }
            
            for (int d = 0; d < n; ++d)
            {
                // update state
                state[0][d] = pred[0][d] + resi[0][d] * gain;
                state[1][d] = pred[1][d] + resi[1][d] * gain;
                state[2][d] = pred[2][d] + resi[2][d] * gain;
            }
            
            return state[0];
        }
    };
    
    struct tracked_box;
    struct plane_pair {
        plane *p0, *p1, *p2; box *new_box; tracked_box* prev_box;
        plane_pair(plane* _p0, plane* _p1, tracked_box* _pb = nullptr, box *_nb = nullptr)
        : p0(_p0), p1(_p1), p2(nullptr), new_box(_nb), prev_box(_pb) {}
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
    
    struct box_record : public box {
        box_record(const box& ref, const pose_t& pose, const rs_sf_intrinsics& cam, float margin);
        pose_t cam_pose;
        int num_visible_plane = 0;
        bool vis_pl[3][2];    // visible planes[axis][sign] of this box
        bool vis_pt[3][2][4]; // visible corners[axis][sign][corner]
        bool fov_pt[3][2][4]; // is point detected within fov
        bool clear_dir[3][2]; // is direction of this box cleared
        v3 pt[3][2][4];       // box corners[axis][sign][corner]
        v3 half_dir(int a) const { return axis.col(a) * dimension[a] * 0.5f; }
        bool is_visible_line(int axis, int sign, int l) const { return vis_pt[axis][sign][l] && vis_pt[axis][sign][(l+1)%4]; }
        bool is_fov_line(int axis, int sign, int l) const { return fov_pt[axis][sign][l] && fov_pt[axis][sign][(l+1)%4]; }
    };
    
    struct tracked_box : public box {
        int pid[3];
        std::list<box_record> box_history;
        state_v<3> track_pos;
        state_v<4> track_axis;
        std::chrono::time_point<std::chrono::steady_clock> last_appear;
        tracked_box(const plane_pair& pair, const std::chrono::time_point<std::chrono::steady_clock>& now)
        : box(*pair.new_box), track_pos(center), track_axis(qv3(axis).coeffs()), last_appear(now) {
            pid[0] = pair.p0->pid;
            pid[1] = pair.p1->pid;
            pid[2] = (pair.p2 ? pair.p2->pid : 0);
        }
        bool try_update(const plane_pair& pair, const pose_t& pose, const rs_sf_intrinsics& cam, const parameter& param);
    };
    typedef std::deque<tracked_box> queue_tracked_box;
    
    struct box_scene {
        scene* plane_scene;
        std::vector<plane_pair> plane_pairs;
        std::vector<box> boxes;
        queue_tracked_box tracked_boxes;
        
        inline void clear() { plane_pairs.clear(); boxes.clear(); tracked_boxes.clear(); }
        inline void swap(box_scene& ref) {
            plane_pairs.swap(ref.plane_pairs);
            boxes.swap(ref.boxes);
        }
        rs_sf_box get_tracked_box(int id, float& history_progress, int max_hist) const {
            history_progress = (float)tracked_boxes[id].box_history.size() / max_hist;
            return tracked_boxes[id].to_rs_sf_box();
        }
    } m_box_scene, m_box_ref_scene;
    
    virtual rs_sf_status refit_boxes(box_scene& view) { return RS_SF_SUCCESS; };
    
private:
    
    struct box_plane_t;
    
    // per-frame detection
    void detect_new_boxes(box_scene& view);
    bool is_valid_box_plane(const plane& p0) const;
    bool is_valid_box_dimension(const float l) const;
    bool is_valid_box_dimension(const box& b) const;
    bool form_box_from_two_planes(box_scene& view, plane_pair& pair);
    bool refine_box_from_third_plane(box_scene& view, plane_pair& pair, plane& p2);
    
    // manage box list
    void update_tracked_boxes(box_scene& view);
    
    // debug
    void draw_box(const std::string& name, const box& src, const box_plane_t* p0 = nullptr, const box_plane_t* p1 = nullptr) const;
};

#endif // ! rs_sf_boxfit_hpp

