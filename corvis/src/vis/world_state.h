#ifndef __CORVIS_WORLD_H__
#define __CORVIS_WORLD_H__

#include <map>
#include <vector>
#include "../numerics/transformation.h"

typedef struct _VertexData {
    float position[3];
    unsigned char color[4];
} VertexData;

typedef struct _feature {
    float x, y, z;
    uint64_t last_seen;
    bool good;
} Feature;

typedef struct _position {
    transformation g;
    uint64_t timestamp;
} Position;

class world_state
{
private:
    std::map<uint64_t, Feature> features;
    std::vector<Position> path;
    uint64_t current_feature_timestamp;
    uint64_t current_timestamp;
    int path_vertex_alloc = 1000;
    int feature_vertex_alloc = 1000;
    void build_grid_vertex_data();

public:
    std::mutex display_lock;
    VertexData * grid_vertex;
    VertexData * axis_vertex;
    VertexData * path_vertex;
    VertexData * feature_vertex;
    int grid_vertex_num, axis_vertex_num, path_vertex_num, feature_vertex_num;

    world_state();
    ~world_state();
    void update_vertex_arrays(bool show_only_good=true);
    void observe_feature(uint64_t timestamp, uint64_t feature_id, float x, float y, float z, bool good);
    void observe_position(uint64_t timestamp, float x, float y, float z, float qw, float qx, float qy, float qz);
    void reset() {
        display_lock.lock();
        features.clear();
        path.clear();
        current_timestamp = 0;
        current_feature_timestamp = 0;
        display_lock.unlock();
    };
};

#endif
