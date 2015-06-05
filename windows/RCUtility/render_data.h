#ifndef __RENDER_DATA__
#define __RENDER_DATA__

#include "../filter/rc_intel_interface.h"
#include <mutex>
#include <vector>

typedef struct {
    float position[3];
    unsigned char color[4];
} GLData;

class render_data
{

private:
    void build_grid_vertex_data();
    std::vector <GLData> path_history;

public:

    render_data();
    ~render_data();
    void update_data(rc_Timestamp time, rc_Pose pose, rc_Feature * features, size_t feature_count);
    std::mutex data_lock;

    GLData * axis_vertex;
    int axis_vertex_num;
    GLData * pose_vertex;
    int pose_vertex_num;
    GLData * grid_vertex;
    int grid_vertex_num;
    GLData * path_vertex;
    int path_vertex_num;
};

#endif
