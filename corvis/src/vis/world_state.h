#ifndef __CORVIS_WORLD_STATE_H__
#define __CORVIS_WORLD_STATE_H__

#include <map>
#include <vector>
#include <list>
#include <mutex>
#include "../numerics/transformation.h"
#include "../cor/platform/sensor_clock.h"
#include "../cor/packet.h"
#include "../cor/sensor_data.h"

typedef struct _VertexData {
    float position[3];
    unsigned char color[4];
} VertexData;

typedef struct _feature {
    float x, y, z;
    sensor_clock::time_point last_seen;
    int times_seen;
    float image_x, image_y;
    float cx, cy, ctheta;
    bool good;
} Feature;

typedef struct _position {
    transformation g;
    sensor_clock::time_point timestamp;
} Position;

typedef struct _ImageData {
    uint8_t * image;
    int width, height;
} ImageData;

typedef struct _mapnode {
    uint64_t id;
    bool finished;
    bool loop_closed;
    transformation position;
    std::vector<uint64_t> neighbors;
    std::vector<Feature> features;
} MapNode;

struct filter;

typedef std::pair<sensor_clock::time_point, float> plot_item;
typedef std::list<plot_item > plot_data;

class world_state
{
public:
    typedef std::map<std::string, plot_data> plot;
private:
    std::map<uint64_t, MapNode> map_nodes;
    std::map<uint64_t, Feature> features;
    std::vector<Position, Eigen::aligned_allocator<Position> > path;
    sensor_clock::time_point current_feature_timestamp;
    sensor_clock::time_point current_timestamp;
    std::size_t path_vertex_alloc = 1000;
    std::size_t feature_vertex_alloc = 1000;
    std::size_t feature_ellipse_vertex_alloc = 1000;
    std::size_t map_node_vertex_alloc = 1000;
    std::size_t map_edge_vertex_alloc = 1000;
    std::size_t map_feature_vertex_alloc = 1000;
    void build_grid_vertex_data();
    void generate_feature_ellipse(const Feature & feat, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);

    std::vector<plot> plots;

public:
    std::mutex image_lock;
    std::mutex depth_lock;
    std::mutex display_lock;
    std::mutex plot_lock;
    VertexData * grid_vertex;
    VertexData * axis_vertex;
    VertexData * path_vertex;
    VertexData * feature_vertex;
    VertexData * orientation_vertex;
    VertexData * feature_ellipse_vertex;
    VertexData * map_node_vertex;
    VertexData * map_edge_vertex;
    VertexData * map_feature_vertex;
    ImageData last_image;
    ImageData last_depth;
    int grid_vertex_num, axis_vertex_num, path_vertex_num, feature_vertex_num, orientation_vertex_num;
    int feature_ellipse_vertex_num;
    int map_node_vertex_num, map_edge_vertex_num, map_feature_vertex_num;

    world_state();
    ~world_state();
    void update_vertex_arrays(bool show_only_good=true);
    void render_plot(size_t plot_index, size_t key_index, std::function<void (plot&, size_t key_index)> render_callback);
    size_t change_plot(size_t plot_index);
    size_t change_plot_key(size_t plot_index, size_t key_index);

    void receive_camera(const filter * f, image_gray8 &&data);
    void observe_feature(sensor_clock::time_point timestamp, uint64_t feature_id, float x, float y, float z, float image_x, float image_y, float cx, float cy, float cxy, bool good);
    void observe_position(sensor_clock::time_point timestamp, float x, float y, float z, float qw, float qx, float qy, float qz);
    void observe_plot_item(sensor_clock::time_point timestamp, size_t plot_index, std::string plot_name, float value);
    void observe_image(sensor_clock::time_point timestamp, uint8_t * image, int width, int height);
    void observe_depth(sensor_clock::time_point timestamp, uint16_t * image, int width, int height);
    void observe_map_node(sensor_clock::time_point timestamp, uint64_t id, bool finished, bool loop_closed, const transformation &T, std::vector<uint64_t> & neighbors, std::vector<Feature> & features);
    std::string get_feature_stats();
    float get_feature_lifetime();
    void reset() {
        display_lock.lock();
        features.clear();
        path.clear();
        current_timestamp = sensor_clock::time_point(sensor_clock::duration(0));
        current_feature_timestamp = sensor_clock::time_point(sensor_clock::duration(0));
        display_lock.unlock();

        plot_lock.lock();
        plots.clear();
        plot_lock.unlock();
    };
};

#endif
