#ifndef __CORVIS_WORLD_STATE_H__
#define __CORVIS_WORLD_STATE_H__

#include <map>
#include <vector>
#include <list>
#include "../numerics/transformation.h"
#include "../cor/platform/sensor_clock.h"
#include "../cor/packet.h"

typedef struct _VertexData {
    float position[3];
    unsigned char color[4];
} VertexData;

typedef struct _feature {
    float x, y, z;
    sensor_clock::time_point last_seen;
    bool good;
} Feature;

typedef struct _position {
    transformation g;
    sensor_clock::time_point timestamp;
} Position;

struct filter;

typedef std::pair<sensor_clock::time_point, float> plot_item;
typedef std::list<plot_item > plot_data;

class world_state
{
private:
    std::map<uint64_t, Feature> features;
    std::vector<Position> path;
    sensor_clock::time_point current_feature_timestamp;
    sensor_clock::time_point current_timestamp;
    int path_vertex_alloc = 1000;
    int feature_vertex_alloc = 1000;
    void build_grid_vertex_data();

    std::map<std::string, plot_data > plot_items;

public:
    std::mutex display_lock;
    std::mutex plot_lock;
    VertexData * grid_vertex;
    VertexData * axis_vertex;
    VertexData * path_vertex;
    VertexData * feature_vertex;
    int grid_vertex_num, axis_vertex_num, path_vertex_num, feature_vertex_num;

    world_state();
    ~world_state();
    void update_vertex_arrays(bool show_only_good=true);
    void render_plots(std::function<void (std::string, plot_data)> render_callback);
    void receive_packet(const filter * f, sensor_clock::time_point timestamp, enum packet_type packet_type);
    void observe_feature(sensor_clock::time_point timestamp, uint64_t feature_id, float x, float y, float z, bool good);
    void observe_position(sensor_clock::time_point timestamp, float x, float y, float z, float qw, float qx, float qy, float qz);
    void observe_plot_item(sensor_clock::time_point timestamp, std::string plot_name, float value);
    void reset() {
        display_lock.lock();
        features.clear();
        path.clear();
        current_timestamp = sensor_clock::time_point(sensor_clock::duration(0));
        current_feature_timestamp = sensor_clock::time_point(sensor_clock::duration(0));
        display_lock.unlock();

        plot_lock.lock();
        plot_items.clear();
        plot_lock.unlock();
    };
};

#endif
