#ifndef __CORVIS_WORLD_STATE_H__
#define __CORVIS_WORLD_STATE_H__

#include <map>
#include <vector>
#include <list>
#include <mutex>
#include "../numerics/transformation.h"
#include "../cor/sensor_data.h"

typedef struct _VertexData {
    float position[3];
    unsigned char color[4];
} VertexData;

typedef struct _feature {
    float x, y, z;
    uint64_t last_seen;
    int times_seen;
    float image_x, image_y;
    float projected_x, projected_y;
    float cx, cy, ctheta;
    bool good;
    bool depth_measured;
} Feature;

typedef struct _position {
    transformation g;
    uint64_t timestamp;
} Position;

typedef struct _ImageData {
    uint8_t * image;
    int width, height;
} ImageData;

typedef struct _mapnode {
    uint64_t id;
    bool finished;
    bool loop_closed;
    bool unlinked;
    transformation position;
    std::vector<uint64_t> neighbors;
    std::vector<Feature> features;
} MapNode;

typedef struct _sensor {
    transformation extrinsics;
} Sensor;

struct filter;

typedef std::pair<uint64_t, float> plot_item;
typedef std::list<plot_item > plot_data;

class world_state
{
public:
    typedef std::map<std::string, plot_data> plot;
private:
    std::map<int, std::map<uint16_t, Sensor, std::less<uint16_t>, Eigen::aligned_allocator<std::pair<const uint16_t, Sensor> > > > sensors;
    std::map<uint64_t, MapNode> map_nodes;
    std::map<uint64_t, Feature> features;
    std::vector<Position, Eigen::aligned_allocator<Position> > path;
    std::vector<Position, Eigen::aligned_allocator<Position> > path_gt;
    uint64_t current_feature_timestamp;
    uint64_t current_timestamp;
    void build_grid_vertex_data();
    void generate_feature_ellipse(const Feature & feat, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);
    void update_current_timestamp(const uint64_t & timestamp);

    std::vector<plot> plots;

public:
    std::mutex image_lock;
    std::mutex depth_lock;
    std::mutex display_lock;
    std::mutex plot_lock;
    std::mutex time_lock;
    std::vector<VertexData> grid_vertex;
    std::vector<VertexData> axis_vertex;
    std::vector<VertexData> path_vertex;
    std::vector<VertexData> path_gt_vertex;
    std::vector<VertexData> feature_vertex;
    std::vector<VertexData> orientation_vertex;
    std::vector<VertexData> feature_ellipse_vertex;
    std::vector<VertexData> feature_projection_vertex;
    std::vector<VertexData> map_node_vertex;
    std::vector<VertexData> map_edge_vertex;
    std::vector<VertexData> map_feature_vertex;
    std::vector<VertexData> sensor_vertex;
    std::vector<VertexData> sensor_axis_vertex;
    ImageData last_image;
    ImageData last_depth;
    ImageData last_depth_overlay_image;
    bool generate_depth_overlay;
    float up[3] = {0,0,1};

    world_state();
    ~world_state();
    void update_vertex_arrays(bool show_only_good=true);
    void render_plot(size_t plot_index, size_t key_index, std::function<void (plot&, size_t key_index)> render_callback);
    size_t change_plot(size_t plot_index);
    size_t change_plot_key(size_t plot_index, size_t key_index);

    void receive_camera(const filter * f, image_gray8 &&data);
    void observe_sensor(int sensor_type, uint16_t sensor_id, float x, float y, float z, float qw, float qx, float qy, float qz);
    void observe_feature(uint64_t timestamp_us, uint64_t feature_id, float x, float y, float z, float image_x, float image_y, float projected_x, float projected_y, float cx, float cy, float cxy, bool good, bool depth_measured);
    void observe_position(uint64_t timestamp_us, float x, float y, float z, float qw, float qx, float qy, float qz);
    void observe_position_gt(uint64_t timestamp_us, float x, float y, float z, float qw, float qx, float qy, float qz);
    void observe_plot_item(uint64_t timestamp_us, size_t plot_index, std::string plot_name, float value);
    void observe_image(uint64_t timestamp_us, uint8_t * image, int width, int height, int stride);
    void observe_depth(uint64_t timestamp_us, uint16_t * image, int width, int height, int stride);
    void observe_depth_overlay_image(uint64_t timestamp_us, uint16_t * aligned_depth, int width, int height, int stride);
    void observe_map_node(uint64_t timestamp_us, uint64_t id, bool finished, bool loop_closed, bool is_unlinked, const transformation &T, std::vector<uint64_t> & neighbors, std::vector<Feature> & features);
    std::string get_feature_stats();
    float get_feature_lifetime();
    int get_feature_depth_measurements();

    void get_bounding_box(float min[3], float max[3]);
    uint64_t get_current_timestamp();

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
