#ifndef __CORVIS_WORLD_STATE_H__
#define __CORVIS_WORLD_STATE_H__

#include <map>
#include <unordered_map>
#include <vector>
#include <list>
#include <mutex>
#include "transformation.h"
#include "rc_tracker.h"
#include "sensor_fusion.h"

typedef struct _VertexData {
    float position[3];
    unsigned char color[4];
} VertexData;

typedef struct _feature {
    rc_Feature feature;
    rc_Timestamp last_seen;
    int times_seen;
    float cx, cy, ctheta;
    bool good;
    bool depth_measured;
    bool not_in_filter;
    bool is_triangulated;
    rc_Sensor camera_id;
} Feature;

typedef struct _position {
    transformation g;
    rc_Timestamp timestamp;
} Position;

typedef struct _ImageData {
    uint8_t * image;
    int width, height;
    rc_Timestamp timestamp;
    bool luminance;
} ImageData;

typedef struct _Neighbor {
    uint64_t id;
    edge_type type = edge_type::new_edge;
    bool in_canonical_path = false;
    _Neighbor(uint64_t id_) : id(id_) {}
} Neighbor;

typedef struct _mapnode {
    uint64_t id;
    bool finished;
    bool unlinked;
    transformation position;
    std::vector<Neighbor> neighbors;
    std::vector<Feature> features;
    std::vector<transformation> virtual_object_links;
} MapNode;

typedef struct _sensor {
    transformation extrinsics;
} Sensor;

typedef struct _polyhedron_coordinates {
    aligned_vector<v3> vertices;  // local coordinates, meters
    std::vector<size_t> vertex_indices;  // order of vertices to draw the polyhedron
} PolyhedronCoordinates;

typedef struct _virtual_object {
    Position pose;  // pose in the world (i.e. G_world_object)
    bool unlinked;  // unlinked to main map, as nodes
    unsigned char rgba[4];  // edge color
    _polyhedron_coordinates coords;
    aligned_vector<v3> bounding_box;  // if not empty, "project" uses it to decide
                                      // faster if the object is in the image

    static _virtual_object make_cube(f_t side_length = 0.4);
    static _virtual_object make_tetrahedron(f_t side_length = 0.4);

    aligned_vector<v2> project(const transformation& G_camera_vo,
                               const state_vision_intrinsics* intrinsics) const;
    aligned_vector<v2> project_axes(const transformation& G_camera_vo,
                                    const state_vision_intrinsics* intrinsics) const;
    aligned_vector<v2> project_axis(
            const v3& vo_p, const v3& vo_q, const transformation& G_camera_world,
            const state_vision_intrinsics* intrinsics) const;
    static aligned_vector<v2> project_points(
            const aligned_vector<v3>& vo_points, const std::vector<size_t>& point_indices,
            const aligned_vector<v3>& vo_bounding_box, const transformation& G_camera_wo,
            const state_vision_intrinsics* intrinsics);
} VirtualObject;

typedef std::pair<rc_Timestamp, float> plot_item;
typedef std::list<plot_item > plot_data;

class replay_output;
class world_state
{
public:
    class overlay_data
    {
    public:
        std::vector<VertexData> feature_ellipse_vertex;
        std::vector<VertexData> feature_projection_vertex;
        std::vector<VertexData> feature_residual_vertex;
        std::vector<VertexData> virtual_objects_vertex;
        ImageData image;
    };

    typedef std::map<std::string, plot_data> plot;
private:
    typedef std::chrono::steady_clock wall_clock;
    typedef wall_clock::time_point wall_time_point;
    std::unordered_map<std::string, size_t> plots_by_name;
    std::map<int, std::map<uint16_t, Sensor, std::less<uint16_t>, Eigen::aligned_allocator<std::pair<const uint16_t, Sensor> > > > sensors;
    std::map<uint16_t, const state_vision_intrinsics*, std::less<uint16_t>, Eigen::aligned_allocator<std::pair<const uint16_t, const state_vision_intrinsics*> > > camera_intrinsics;
    std::map<uint64_t, MapNode> map_nodes;
    std::map<wall_time_point, std::vector<MapNode>> removed_map_nodes;
    std::map<std::pair<rc_Sensor,uint64_t>, Feature> features;
    std::map<std::string, VirtualObject> virtual_objects;
    std::vector<Position, Eigen::aligned_allocator<Position> > path_reloc;
    std::vector<Position, Eigen::aligned_allocator<Position> > path;
    std::vector<Position, Eigen::aligned_allocator<Position> > path_mini;
    std::vector<Position, Eigen::aligned_allocator<Position> > path_gt;
    rc_Timestamp current_feature_timestamp{0};
    rc_Timestamp current_timestamp{0};
    void build_grid_vertex_data();
    void generate_feature_ellipse(const Feature & feat, std::vector<VertexData> & feature_ellipse_vertex, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);
    void generate_innovation_line(const Feature &feat, std::vector<VertexData> &feature_residual_vertex, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);

    void update_current_timestamp(const rc_Timestamp & timestamp);
    void update_plots(rc_Tracker * tracker, const rc_Data * data);
    void update_sensors(rc_Tracker * tracker, const rc_Data * data);
    void update_map(rc_Tracker * tracker, rc_Timestamp timestamp_us);
    void update_relocalization(const replay_output *output);

    size_t get_plot_by_name(std::string plot_name);

    std::vector<plot> plots;
    bool dirty{true};

public:
    std::mutex image_lock;
    std::mutex depth_lock;
    std::mutex display_lock;
    std::mutex plot_lock;
    std::mutex time_lock;
    std::vector<VertexData> grid_vertex;
    std::vector<VertexData> axis_vertex;
    std::vector<VertexData> path_vertex;
    std::vector<VertexData> path_mini_vertex;
    std::vector<VertexData> path_gt_vertex;
    std::vector<VertexData> path_axis_vertex;
    std::vector<VertexData> feature_vertex;
    std::vector<VertexData> orientation_vertex;
    std::vector<VertexData> map_node_vertex;
    std::vector<VertexData> map_edge_vertex;
    std::vector<VertexData> map_feature_vertex;
    std::vector<VertexData> removed_map_node_vertex;
    std::vector<VertexData> removed_map_feature_vertex;
    std::vector<VertexData> reloc_vertex;
    std::vector<VertexData> sensor_vertex;
    std::vector<VertexData> sensor_axis_vertex;
    std::vector<VertexData> virtual_object_vertex;
    std::vector<overlay_data> cameras;
    std::vector<overlay_data> debug_cameras;
    std::vector<ImageData> depths;
    float up[3] = {0,0,1};
    float ate = 0;

    rc_Timestamp max_plot_history_us = 30e6;

    world_state();
    ~world_state();
    bool update_vertex_arrays(bool show_only_good=true);
    void render_plot(size_t plot_index, size_t key_index, std::function<void (plot&, size_t key_index)> render_callback);
    size_t change_plot(size_t plot_index);
    size_t change_plot_key(size_t plot_index, size_t key_index);

    void rc_data_callback(const replay_output *output, const rc_Data * data);
    void observe_sensor(int sensor_type, uint16_t sensor_id, float x, float y, float z, float qw, float qx, float qy, float qz);
    void observe_camera_intrinsics(uint16_t sensor_id, const state_vision_intrinsics& intrinsics);
    void observe_world(float world_up_x, float world_up_y, float world_up_z,
                       float world_forward_x, float world_forward_y, float world_forward_z,
                       float body_forward_x, float body_forward_y, float body_forward_z);
    void observe_feature(rc_Timestamp timestamp, rc_Sensor camera_id, const rc_Feature & feature);
    void observe_position(rc_Timestamp timestamp_us, float x, float y, float z, float qw, float qx, float qy, float qz, bool fast);
    void observe_position_gt(rc_Timestamp timestamp_us, float x, float y, float z, float qw, float qx, float qy, float qz);
    void observe_plot_item(rc_Timestamp timestamp_us, size_t plot_index, std::string plot_name, float value);
    void observe_image(rc_Timestamp timestamp_us, rc_Sensor sensor_id, const rc_ImageData & data, std::vector<overlay_data> &cameras);
    void observe_depth(rc_Timestamp timestamp_us, rc_Sensor sensor_id, const rc_ImageData & data);
    void observe_depth_overlay_image(rc_Timestamp timestamp_us, uint16_t * aligned_depth, int width, int height, int stride);
    void observe_ate(rc_Timestamp timestamp_us, const float absolute_trajectory_error);
    void observe_rpe(rc_Timestamp timestamp_us, const float relative_pose_error);
    void observe_position_reloc(rc_Timestamp timestamp, const rc_Pose* poses, size_t nposes);
    void observe_virtual_object(rc_Timestamp timestamp, const std::string& uuid, const rc_Pose& pose, bool unlinked = false);
    void unobserve_virtual_object(const std::string &name);
    std::string get_feature_stats();
    float get_feature_lifetime();
    int get_feature_depth_measurements();

    void get_bounding_box(float min[3], float max[3]);
    rc_Timestamp get_current_timestamp();

    void reset() {
        display_lock.lock();
        features.clear();
        path.clear();
        current_timestamp = 0;
        current_feature_timestamp = 0;
        display_lock.unlock();

        plot_lock.lock();
        plots.clear();
        plot_lock.unlock();
    };
};

#endif
