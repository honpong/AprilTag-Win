#include "world_state.h"
#include "../filter/filter.h"
#include "../cor/packet.h"

static VertexData axis_data[] = {
    {{0, 0, 0}, {221, 141, 81, 255}},
    {{.5, 0, 0}, {221, 141, 81, 255}},
    {{0, 0, 0}, {0, 201, 89, 255}},
    {{0, .5, 0}, {0, 201, 89, 255}},
    {{0, 0, 0}, {247, 88, 98, 255}},
    {{0, 0, .5}, {247, 88, 98, 255}},
};

static VertexData orientation_data[] = {
    {{0, 0, 0}, {221, 141, 81, 255}},
    {{.5, 0, 0}, {221, 141, 81, 255}},
    {{0, 0, 0}, {0, 201, 89, 255}},
    {{0, .5, 0}, {0, 201, 89, 255}},
    {{0, 0, 0}, {247, 88, 98, 255}},
    {{0, 0, .5}, {247, 88, 98, 255}},
};

static int max_plot_samples = 1000;
void world_state::render_plots(std::function<void (plot&)> render_plot)
{
    plot_lock.lock();
    for(auto &plot : plots)
        render_plot(plot);
    plot_lock.unlock();
}

void world_state::observe_plot_item(sensor_clock::time_point timestamp, int index, std::string name, float value)
{
    plot_lock.lock();
    if (index+1 > plots.size())
        plots.resize(index+1);
    auto &plot = plots[index][name];
    plot.push_back(plot_item(timestamp, value));
    if(plot.size() > max_plot_samples)
        plot.pop_front();
    plots[index][name] = plot;
    plot_lock.unlock();
}

void world_state::receive_packet(const filter * f, sensor_clock::time_point tp, enum packet_type packet_type)
{
    if(packet_type == packet_camera) {
        // Only update position and features on packet camera,
        // matches what we do in other visualizations
        for(auto feat : f->s.features) {
            if(feat->is_valid()) {
                float stdev = (float)feat->v.stdev_meters(sqrt(feat->variance()));
                bool good = stdev / feat->v.depth() < .02;
                observe_feature(tp, feat->id,
                        (float)feat->world[0], (float)feat->world[1], (float)feat->world[2], good);
            }
        }

        v4 T = f->s.T.v;
        quaternion q = to_quaternion(f->s.W.v);
        observe_position(tp, (float)T[0], (float)T[1], (float)T[2], (float)q.w(), (float)q.x(), (float)q.y(), (float)q.z());
    }

    /*
    if(packet_type == packet_camera) {
        // reach into the filter struct and get whatever data you want
        // to plot, and add it to a named plot (in this case "Tx").
        observe_plot_item(tp, 0, "Tx", f->s.T.v[0]);
        observe_plot_item(tp, 0, "Ty", f->s.T.v[1]);
        observe_plot_item(tp, 0, "Tz", f->s.T.v[2]);

        observe_plot_item(tp, 1, "w_bias_x", f->s.w_bias.v[0]);
        observe_plot_item(tp, 1, "w_bias_y", f->s.w_bias.v[1]);
        observe_plot_item(tp, 1, "w_bias_z", f->s.w_bias.v[2]);

        observe_plot_item(tp, 2, "a_bias_x", f->s.a_bias.v[0]);
        observe_plot_item(tp, 2, "a_bias_y", f->s.a_bias.v[1]);
        observe_plot_item(tp, 2, "a_bias_z", f->s.a_bias.v[2]);

        observe_plot_item(tp, 3, "a_inn_mean_x", observation_accelerometer::inn_stdev.mean[0]);
        observe_plot_item(tp, 3, "a_inn_mean_y", observation_accelerometer::inn_stdev.mean[1]);
        observe_plot_item(tp, 3, "a_inn_mean_z", observation_accelerometer::inn_stdev.mean[2]);

        observe_plot_item(tp, 4, "g_inn_mean_x", observation_gyroscope::inn_stdev.mean[0]);
        observe_plot_item(tp, 4, "g_inn_mean_y", observation_gyroscope::inn_stdev.mean[1]);
        observe_plot_item(tp, 4, "g_inn_mean_z", observation_gyroscope::inn_stdev.mean[2]);

        observe_plot_item(tp, 5, "v_inn_mean_x", observation_vision_feature::inn_stdev[0].mean);
        observe_plot_item(tp, 5, "v_inn_mean_y", observation_vision_feature::inn_stdev[1].mean);

    }
    */
}

world_state::world_state()
{
    path_vertex = (VertexData *)calloc(sizeof(VertexData), path_vertex_alloc);
    feature_vertex = (VertexData *)calloc(sizeof(VertexData), feature_vertex_alloc);
    build_grid_vertex_data();
    axis_vertex = axis_data;
    axis_vertex_num = 6;
    orientation_vertex = orientation_data;
    orientation_vertex_num = 6;
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
        if (p.timestamp == current_timestamp) {
            set_color(&path_vertex[idx], 0, 255, 0, 255);
            for(int i = 0; i < 6; i++) {
                v4 vertex(axis_vertex[i].position[0],
                          axis_vertex[i].position[1],
                          axis_vertex[i].position[2],
                          0);
                vertex = transformation_apply(p.g, vertex);
                orientation_data[i].position[0] = vertex[0];
                orientation_data[i].position[1] = vertex[1];
                orientation_data[i].position[2] = vertex[2];
            }
        }
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

void world_state::observe_feature(sensor_clock::time_point timestamp, uint64_t feature_id, float x, float y, float z, bool good)
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

void world_state::observe_position(sensor_clock::time_point timestamp, float x, float y, float z, float qw, float qx, float qy, float qz)
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
