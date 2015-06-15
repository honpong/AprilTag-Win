#include "render_data.h"

#include <stdlib.h>

static GLData axis_data[] = {
    {{0, 0, 0}, {255, 0, 0, 255}},
    {{.5, 0, 0}, {255, 0, 0, 255}},
    {{0, 0, 0}, {0, 255, 0, 255}},
    {{0, .5, 0}, {0, 255, 0, 255}},
    {{0, 0, 0}, {0, 0, 255, 255}},
    {{0, 0, .5}, {0, 0, 255, 255}},
};

static GLData pose_data[] = {
    {{0, 0, 0}, {255, 0, 0, 255}},
    {{.5, 0, 0}, {255, 0, 0, 255}},
    {{0, 0, 0}, {0, 255, 0, 255}},
    {{0, .5, 0}, {0, 255, 0, 255}},
    {{0, 0, 0}, {0, 0, 255, 255}},
    {{0, 0, .5}, {0, 0, 255, 255}},
};

static inline void transform_by_pose(const rc_Pose pose, const GLData & vi, GLData & vo)
{
    vo.position[0] = vi.position[0]*pose[0] + vi.position[1]*pose[1] + vi.position[2]*pose[2]  + pose[3];
    vo.position[1] = vi.position[0]*pose[4] + vi.position[1]*pose[5] + vi.position[2]*pose[6]  + pose[7];
    vo.position[2] = vi.position[0]*pose[8] + vi.position[1]*pose[9] + vi.position[2]*pose[10] + pose[11];
}

static inline void set_position(GLData * vertex, float x, float y, float z)
{
    vertex->position[0] = x;
    vertex->position[1] = y;
    vertex->position[2] = z;
}

static inline void set_color(GLData * vertex, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha)
{
    vertex->color[0] = r;
    vertex->color[1] = g;
    vertex->color[2] = b;
    vertex->color[3] = alpha;
}

void render_data::reset()
{
    data_lock.lock();

    // Position and orientation
    for (int i = 0; i < axis_vertex_num; i++)
        transform_by_pose(rc_POSE_IDENTITY, axis_vertex[i], pose_vertex[i]);

    // Path history
    path_history.clear();

    data_lock.unlock();
}

void render_data::update_data(rc_Timestamp time, rc_Pose pose, rc_Feature * current_features, size_t current_feature_count)
{
    data_lock.lock();

    // Position and orientation
    for(int i = 0; i < axis_vertex_num; i++)
        transform_by_pose(pose, axis_vertex[i], pose_vertex[i]);

    // Path history
    GLData now;
    set_position(&now, pose[3], pose[7], pose[11]);
    set_color(&now, 0, 178, 206, 255);
    path_history.push_back(now);
    path_vertex = &path_history[0];
    path_vertex_num = (int) path_history.size();

    // Current features
    features.clear();
    for(int i = 0; i < current_feature_count; i++) {
        GLData glfeat;
        rc_Feature * f = &current_features[i];
        set_position(&glfeat, f->world.x, f->world.y, f->world.z);
        set_color(&glfeat, 255, 255, 255, 255);
        features.push_back(glfeat);
    }
    if(features.size()) feature_vertex = &features[0];
    feature_vertex_num = (int) features.size();

    data_lock.unlock();
}

void render_data::build_grid_vertex_data()
{
    float scale = 1; /* meter */
    grid_vertex_num = 21*16; /* -10 to 10 with 16 each iteration */
    grid_vertex = (GLData *)calloc(sizeof(GLData), grid_vertex_num);
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
        set_position(&grid_vertex[idx++], 0, -.1f*scale, x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], 0, .1f*scale, x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], -.1f*scale, 0, x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], .1f*scale, 0, x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], 0, -.1f*scale, -x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], 0, .1f*scale, -x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], -.1f*scale, 0, -x);
        set_color(&grid_vertex[idx], gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&grid_vertex[idx++], .1f*scale, 0, -x);
    }
}


render_data::render_data()
{
    axis_vertex = axis_data;
    axis_vertex_num = 6;
    pose_vertex = pose_data;
    pose_vertex_num = 6;

    build_grid_vertex_data();
}

render_data::~render_data()
{
    if(grid_vertex)
        free(grid_vertex);
}
