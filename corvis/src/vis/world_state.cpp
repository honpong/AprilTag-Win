#include "world_state.h"

static VertexData axis_data[] = {
    {{0, 0, 0}, {221, 141, 81, 255}},
    {{.5, 0, 0}, {221, 141, 81, 255}},
    {{0, 0, 0}, {0, 201, 89, 255}},
    {{0, .5, 0}, {0, 201, 89, 255}},
    {{0, 0, 0}, {247, 88, 98, 255}},
    {{0, 0, .5}, {247, 88, 98, 255}},
};

world_state::world_state()
{
    path_vertex = (VertexData *)calloc(sizeof(VertexData), path_vertex_alloc);
    feature_vertex = (VertexData *)calloc(sizeof(VertexData), feature_vertex_alloc);
    build_grid_vertex_data();
    axis_vertex = axis_data;
    axis_vertex_num = 6;
}

world_state::~world_state()
{
    if(path_vertex)
        free(path_vertex);
    if(feature_vertex)
        free(feature_vertex);
    if(grid_vertex)
        free(grid_vertex);
}

static inline void set_position(VertexData * vertex, float x, float y, float z)
{
    vertex->position[0] = x;
    vertex->position[1] = y;
    vertex->position[2] = z;
}

static inline void set_color(VertexData * vertex, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha)
{
    vertex->color[0] = r;
    vertex->color[1] = g;
    vertex->color[2] = b;
    vertex->color[3] = alpha;
}

void world_state::update_vertex_arrays(bool show_only_good)
{
    /*
     * Build vertex arrays for feature and path data
     */
    display_lock.lock();
    int idx;

    // reallocate if we now have more data than room for vertices
    if(feature_vertex_alloc < features.size()) {
        feature_vertex = (VertexData *)realloc(feature_vertex, sizeof(VertexData)*features.size()*1.5);
    }
    if(path_vertex_alloc < path.size()) {
        path_vertex = (VertexData *)realloc(path_vertex, sizeof(VertexData)*features.size()*1.5);
    }

    idx = 0;
    for(auto const & item : features) {
        //auto feature_id = item.first;
        auto f = item.second;
        if (f.last_seen == current_feature_timestamp)
            set_color(&feature_vertex[idx], 247, 88, 98, 255);
        else {
            if (show_only_good && !f.good)
                continue;
            set_color(&feature_vertex[idx], 255, 255, 255, 255);
        }
        set_position(&feature_vertex[idx], f.x, f.y, f.z);
        idx++;
    }
    feature_vertex_num = idx;

    idx = 0;
    for(auto p : path)
    {
        if (p.timestamp == current_timestamp)
            set_color(&path_vertex[idx], 0, 255, 0, 255);
        else
            set_color(&path_vertex[idx], 0, 178, 206, 255); // path color
        set_position(&path_vertex[idx], p.g.T.x(), p.g.T.y(), p.g.T.z());
        idx++;
    }
    path_vertex_num = idx;
    display_lock.unlock();
}

void world_state::build_grid_vertex_data()
{
    float scale = 1; /* meter */
    grid_vertex_num = 21*16; /* -10 to 10 with 16 each iteration */
    grid_vertex = (VertexData *)calloc(sizeof(VertexData), grid_vertex_num);
    /* Grid */
    int idx = 0;
    unsigned char gridColor[4] = {122, 126, 146, 255};
    for(float x = -10*scale; x < 11*scale; x += scale)
    {
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], x, -10*scale, 0);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], x, 10*scale, 0);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], -10*scale, x, 0);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], 10*scale, x, 0);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], -0, -10*scale, 0);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], -0, 10*scale, 0);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], -10*scale, -0, 0);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], 10*scale, -0, 0);

        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], 0, -.1*scale, x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], 0, .1*scale, x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], -.1*scale, 0, x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], .1*scale, 0, x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], 0, -.1*scale, -x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], 0, .1*scale, -x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], -.1*scale, 0, -x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], .1*scale, 0, -x);
    }
}

void world_state::observe_feature(uint64_t timestamp, uint64_t feature_id, float x, float y, float z, bool good)
{
    Feature f;
    f.x = x;
    f.y = y;
    f.z = z;
    f.last_seen = timestamp;
    f.good = good;
    display_lock.lock();
    if(timestamp > current_feature_timestamp)
        current_feature_timestamp = timestamp;
    if(timestamp > current_timestamp)
        current_timestamp = timestamp;
    features[feature_id] = f;
    display_lock.unlock();
}

void world_state::observe_position(uint64_t timestamp, float x, float y, float z, float qw, float qx, float qy, float qz)
{
    Position p;
    p.timestamp = timestamp;
    quaternion q(qw, qx, qy, qz);
    p.g = transformation(q, v4(x, y, z, 0));
    display_lock.lock();
    path.push_back(p);
    if(timestamp > current_timestamp)
        current_timestamp = timestamp;
    display_lock.unlock();
}
