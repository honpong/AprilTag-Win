#include "world_state.h"
#include "../filter/filter.h"
#include "../filter/rc_tracker.h"

static const VertexData axis_data[] = {
    {{0, 0, 0}, {255, 0, 0, 255}},
    {{.5, 0, 0}, {255, 0, 0, 255}},
    {{0, 0, 0}, {0, 255, 0, 255}},
    {{0, .5, 0}, {0, 255, 0, 255}},
    {{0, 0, 0}, {0, 0, 255, 255}},
    {{0, 0, .5}, {0, 0, 255, 255}},
};

static const unsigned char indexed_colors[][4] = {
    {255,   0,   0, 255},
    {  0, 255,   0, 255},
    {  0,   0, 255, 255},
    {  0, 255, 255, 255},
    {255,   0, 255, 255},
    {255, 255,   0, 255},
    {255, 255, 255, 255},
};

static const std::size_t feature_ellipse_vertex_size = 30; // 15 segments
static const std::size_t max_plot_samples = 1000;
void world_state::render_plot(size_t plot_index, size_t key_index, std::function<void (plot&, size_t key_index)> render_callback)
{
    std::lock_guard<std::mutex> lock(plot_lock);
    if(plot_index < plots.size() && (key_index == (size_t)-1 || key_index < plots[plot_index].size()))
        render_callback(plots[plot_index], key_index);
}

size_t world_state::change_plot(size_t index)
{
    std::lock_guard<std::mutex> lock(plot_lock);
    if(index == plots.size())
        return 0;
    if(index > plots.size())
        return plots.size() - 1;
    return index;
}

size_t world_state::change_plot_key(size_t plot_index, size_t key_index)
{
    std::lock_guard<std::mutex> lock(plot_lock);
    if (plot_index < plots.size()) {
        if (key_index < plots[plot_index].size())
            return key_index;
        if (key_index == plots[plot_index].size() || key_index == (size_t)-1)
            return (size_t)-1;
        return plots[plot_index].size() - 1;
    }
    return (size_t)-1;
}

void world_state::observe_plot_item(uint64_t timestamp, size_t index, std::string name, float value)
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

void world_state::observe_sensor(int sensor_type, uint16_t sensor_id, float x, float y, float z, float qw, float qx, float qy, float qz)
{
    display_lock.lock();
    Sensor s;
    s.extrinsics = transformation(quaternion(qw,qx,qy,qz), v3(x,y,z));
    sensors[sensor_type][sensor_id] = s;
    display_lock.unlock();
}

void world_state::observe_world(float world_up_x, float world_up_y, float world_up_z,
                   float world_forward_x, float world_forward_y, float world_forward_z,
                   float body_forward_x, float body_forward_y, float body_forward_z)
{
    display_lock.lock();
    if(up[0] != world_up_x || up[1] != world_up_y || up[2] != world_up_z) {
        up[0] = world_up_x;
        up[1] = world_up_y;
        up[2] = world_up_z;
        build_grid_vertex_data();
    }
    display_lock.unlock();
}

void world_state::observe_map_node(uint64_t timestamp, uint64_t node_id, bool finished, bool loop_closed, bool unlinked, const transformation &position, vector<uint64_t> & neighbors, vector<Feature> & features)
{
    display_lock.lock();
    MapNode n;
    n.id = node_id;
    n.finished = finished;
    n.loop_closed = loop_closed;
    n.unlinked = unlinked;
    n.position = position;
    n.neighbors = neighbors;
    n.features = features;
    map_nodes[node_id] = n;
    display_lock.unlock();
}

void world_state::observe_image(uint64_t timestamp, uint8_t * image, int width, int height, int stride)
{
    image_lock.lock();
    if(last_image.image && (width != last_image.width || height != last_image.height))
        last_image.image = (uint8_t *)realloc(last_image.image, sizeof(uint8_t)*width*height);

    if(!last_image.image)
        last_image.image = (uint8_t *)malloc(sizeof(uint8_t)*width*height);

    for(int i=0; i<height; i++)
        memcpy(last_image.image + i*width, image + i*stride, sizeof(uint8_t)*width);

    last_image.width = width;
    last_image.height = height;
    image_lock.unlock();
}

#define MAX_DEPTH 8191

void world_state::observe_depth(uint64_t timestamp, uint16_t *image, int width, int height, int stride)
{
    depth_lock.lock();
    if(last_depth.image && (width != last_depth.width || height != last_depth.height))
        last_depth.image = (uint8_t *)realloc(last_depth.image, sizeof(uint8_t)*width*height);
    
    if(!last_depth.image)
        last_depth.image = (uint8_t *)malloc(sizeof(uint8_t)*width*height);

    for(int i = 0; i < height; ++i)
        for(int j = 0; j < width; ++j)
            last_depth.image[width * i + j] = (image[stride/2 * i + j] == 0 || image[stride/2 * i + j] > MAX_DEPTH) ? 0 : 255 - (image[stride/2 * i + j] / 32);

    last_depth.width = width;
    last_depth.height = height;
    depth_lock.unlock();
}

void world_state::observe_depth_overlay_image(uint64_t timestamp, uint16_t *aligned_depth, int width, int height, int stride)
{
    depth_lock.lock();
    image_lock.lock();

    if (last_depth_overlay_image.image && (width != last_depth_overlay_image.width || height != last_depth_overlay_image.height))
        last_depth_overlay_image.image = (uint8_t *)realloc(last_depth_overlay_image.image, sizeof(uint8_t)*width*height);

    if (!last_depth_overlay_image.image)
        last_depth_overlay_image.image = (uint8_t *)malloc(sizeof(uint8_t)*width*height);

    if (last_image.image) {
        for (int i = 0, p = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j, ++p) {
                const auto z = aligned_depth[stride / 2 * i + j];
                if (0 < z && z <= MAX_DEPTH) {
                    last_depth_overlay_image.image[p] = 255 - (z / 32);
                }
                else {
                    last_depth_overlay_image.image[p] = last_image.image[p];
                }
            }
        }
    }

    last_depth_overlay_image.width = width;
    last_depth_overlay_image.height = height;

    image_lock.unlock();
    depth_lock.unlock();
}

static inline void compute_covariance_ellipse(state_vision_feature * feat, float & cx, float & cy, float & ctheta)
{
    //http://math.stackexchange.com/questions/8672/eigenvalues-and-eigenvectors-of-2-times-2-matrix
    const static float chi_square_95 = 5.991f;

    float x = (float)feat->innovation_variance_x;
    float y = (float)feat->innovation_variance_y;
    float xy = (float)feat->innovation_variance_xy;
    float tau = 0;
    if(xy != 0.f)
        tau = (y - x) / xy / 2.f;
    float t = (tau >= 0.) ? (1.f / (fabs(tau) + sqrt(1.f + tau * tau))) : (-1.f / (fabs(tau) + sqrt(1.f + tau * tau)));
    float c = 1.f / sqrt(1.f + tau * tau);
    float s = c * t;
    float l1 = x - t * xy;
    float l2 = y + t * xy;
    float theta = atan2(-s, c);

    cx = (float)2. * sqrt(l1 * chi_square_95); // ellipse width
    cy = (float)2. * sqrt(l2 * chi_square_95); // ellipse height
    ctheta = (float)theta; // rotate
}

uint64_t world_state::get_current_timestamp()
{
    std::lock_guard<std::mutex> lock(time_lock);
    return current_timestamp;
}

void world_state::update_current_timestamp(const uint64_t & timestamp)
{
    std::lock_guard<std::mutex> lock(time_lock);
    if(timestamp > current_timestamp)
        current_timestamp = timestamp;
}

void world_state::receive_camera(const filter * f, image_gray8 &&d)
{
    uint64_t timestamp_us = sensor_clock::tp_to_micros(d.timestamp);
    update_current_timestamp(timestamp_us);
    current_feature_timestamp = timestamp_us;
    transformation G = f->origin*f->s.get_transformation();

    for(const auto & s : f->accelerometers) {
        observe_sensor(rc_SENSOR_TYPE_ACCELEROMETER, s->id, s->extrinsics.mean.T[0], s->extrinsics.mean.T[1], s->extrinsics.mean.T[2],
                      s->extrinsics.mean.Q.w(), s->extrinsics.mean.Q.x(), s->extrinsics.mean.Q.y(), s->extrinsics.mean.Q.z());
    }
    for(const auto & s : f->gyroscopes) {
        observe_sensor(rc_SENSOR_TYPE_GYROSCOPE, s->id, s->extrinsics.mean.T[0], s->extrinsics.mean.T[1], s->extrinsics.mean.T[2],
                      s->extrinsics.mean.Q.w(), s->extrinsics.mean.Q.x(), s->extrinsics.mean.Q.y(), s->extrinsics.mean.Q.z());
    }
    for(const auto & s : f->cameras) {
        observe_sensor(rc_SENSOR_TYPE_IMAGE, s->id, s->extrinsics.mean.T[0], s->extrinsics.mean.T[1], s->extrinsics.mean.T[2],
                      s->extrinsics.mean.Q.w(), s->extrinsics.mean.Q.x(), s->extrinsics.mean.Q.y(), s->extrinsics.mean.Q.z());
    }
    for(const auto & s : f->depths) {
        observe_sensor(rc_SENSOR_TYPE_DEPTH, s->id, s->extrinsics.mean.T[0], s->extrinsics.mean.T[1], s->extrinsics.mean.T[2],
                      s->extrinsics.mean.Q.w(), s->extrinsics.mean.Q.x(), s->extrinsics.mean.Q.y(), s->extrinsics.mean.Q.z());
    }

    observe_world(f->s.world_up[0], f->s.world_up[1], f->s.world_up[2],
                  f->s.world_initial_forward[0], f->s.world_initial_forward[1], f->s.world_initial_forward[2],
                  f->s.body_forward[0], f->s.body_forward[1], f->s.body_forward[2]);

    for(auto g : f->s.groups.children) {
        for(auto feat : g->features.children) {
            if(feat->is_valid()) {
                float stdev = (float)feat->v.stdev_meters(sqrt(feat->variance()));
                bool good = stdev / feat->v.depth() < .02;
                float cx, cy, ctheta;
                compute_covariance_ellipse(feat, cx, cy, ctheta);

                v3 world = G * feat->body;
                observe_feature(timestamp_us, feat->id,
                                (float)world[0], (float)world[1], (float)world[2],
                                (float)feat->current[0], (float)feat->current[1],
                                (float)feat->prediction[0], (float)feat->prediction[1],
                                cx, cy, ctheta, good, feat->depth_measured);
            }
        }
    }
    observe_image(timestamp_us, d.image, d.width, d.height, d.stride);

    if(f->has_depth) {
        observe_depth(sensor_clock::tp_to_micros(f->recent_depth.timestamp), f->recent_depth.image, f->recent_depth.width, f->recent_depth.height, f->recent_depth.stride);

        if (generate_depth_overlay){
            auto depth_overlay = filter_aligned_depth_overlay(f, f->recent_depth, d);
            observe_depth_overlay_image(sensor_clock::tp_to_micros(depth_overlay->timestamp), depth_overlay->image, depth_overlay->width, depth_overlay->height, depth_overlay->stride);
        }
    }

    if(f->s.map_enabled) {
        for(auto map_node : f->s.map.get_nodes()) {
            bool loop_closed = false;
            vector<uint64_t> neighbors;
            for(auto edge : map_node.edges) {
                neighbors.push_back(edge.neighbor);
                if(edge.loop_closure)
                    loop_closed = true;
            }
            vector<Feature> features;
            for(auto feature : map_node.features) {
                Feature f;
                f.x = feature->position[0];
                f.y = feature->position[1];
                f.z = feature->position[2];
                features.push_back(f);
            }
            bool unlinked = f->s.map.is_unlinked(map_node.id);
            observe_map_node(timestamp_us, map_node.id, map_node.finished, loop_closed, unlinked, map_node.global_transformation.transform, neighbors, features);
        }
    }

    observe_position(timestamp_us, (float)G.T[0], (float)G.T[1], (float)G.T[2], (float)G.Q.w(), (float)G.Q.x(), (float)G.Q.y(), (float)G.Q.z());

    int p = 0;

    if (f->observations.recent_a.get()) {
        observe_plot_item(timestamp_us, p, "a_x", (float)f->observations.recent_a->meas[0]);
        observe_plot_item(timestamp_us, p, "a_y", (float)f->observations.recent_a->meas[1]);
        observe_plot_item(timestamp_us, p, "a_z", (float)f->observations.recent_a->meas[2]);
    }
    p++;

    if (f->observations.recent_g.get()) {
        observe_plot_item(timestamp_us, p, "g_x", (float)f->observations.recent_g->meas[0]);
        observe_plot_item(timestamp_us, p, "g_y", (float)f->observations.recent_g->meas[1]);
        observe_plot_item(timestamp_us, p, "g_z", (float)f->observations.recent_g->meas[2]);
    }
    p++;

    if (f->s.camera.intrinsics.estimate) {
        if (f->s.camera.intrinsics.fisheye)
            observe_plot_item(timestamp_us, p, "kw", (float)f->s.camera.intrinsics.k1.v);
        else {
            observe_plot_item(timestamp_us, p, "k1", (float)f->s.camera.intrinsics.k1.v);
            observe_plot_item(timestamp_us, p, "k2", (float)f->s.camera.intrinsics.k2.v);
            observe_plot_item(timestamp_us, p, "k3", (float)f->s.camera.intrinsics.k3.v);
        }
        p++;

        observe_plot_item(timestamp_us, p, "F", (float)(f->s.camera.intrinsics.focal_length.v * f->s.camera.intrinsics.image_height));
        p++;

        observe_plot_item(timestamp_us, p, "C_x", (float)(f->s.camera.intrinsics.center_x.v * f->s.camera.intrinsics.image_height + f->s.camera.intrinsics.image_width  / 2. - .5));
        observe_plot_item(timestamp_us, p, "C_y", (float)(f->s.camera.intrinsics.center_y.v * f->s.camera.intrinsics.image_height + f->s.camera.intrinsics.image_height / 2. - .5));
        p++;
    }

    if (f->s.camera.extrinsics.estimate) {
        observe_plot_item(timestamp_us, p, "Tc_x", (float)f->s.camera.extrinsics.T.v[0]);
        observe_plot_item(timestamp_us, p, "Tc_y", (float)f->s.camera.extrinsics.T.v[1]);
        observe_plot_item(timestamp_us, p, "Tc_z", (float)f->s.camera.extrinsics.T.v[2]);
        p++;

        observe_plot_item(timestamp_us, p, "Wc_x", (float)to_rotation_vector(f->s.camera.extrinsics.Q.v).raw_vector()[0]);
        observe_plot_item(timestamp_us, p, "Wc_y", (float)to_rotation_vector(f->s.camera.extrinsics.Q.v).raw_vector()[1]);
        observe_plot_item(timestamp_us, p, "Wc_z", (float)to_rotation_vector(f->s.camera.extrinsics.Q.v).raw_vector()[2]);
        p++;
    }

    observe_plot_item(timestamp_us, p, "T_x", (float)f->s.T.v[0]);
    observe_plot_item(timestamp_us, p, "T_y", (float)f->s.T.v[1]);
    observe_plot_item(timestamp_us, p, "T_z", (float)f->s.T.v[2]);
    p++;

    observe_plot_item(timestamp_us, p, "Tvar_x", (float)f->s.T.variance()[0]);
    observe_plot_item(timestamp_us, p, "Tvar_y", (float)f->s.T.variance()[1]);
    observe_plot_item(timestamp_us, p, "Tvar_z", (float)f->s.T.variance()[2]);
    p++;

    observe_plot_item(timestamp_us, p, "wbias_x", (float)f->s.imu.intrinsics.w_bias.v[0]);
    observe_plot_item(timestamp_us, p, "wbias_y", (float)f->s.imu.intrinsics.w_bias.v[1]);
    observe_plot_item(timestamp_us, p, "wbias_z", (float)f->s.imu.intrinsics.w_bias.v[2]);
    p++;

    observe_plot_item(timestamp_us, p, "var-wbias_x", (float)f->s.imu.intrinsics.w_bias.variance()[0]);
    observe_plot_item(timestamp_us, p, "var-wbias_y", (float)f->s.imu.intrinsics.w_bias.variance()[1]);
    observe_plot_item(timestamp_us, p, "var-wbias_z", (float)f->s.imu.intrinsics.w_bias.variance()[2]);
    p++;

    observe_plot_item(timestamp_us, p, "abias_x", (float)f->s.imu.intrinsics.a_bias.v[0]);
    observe_plot_item(timestamp_us, p, "abias_y", (float)f->s.imu.intrinsics.a_bias.v[1]);
    observe_plot_item(timestamp_us, p, "abias_z", (float)f->s.imu.intrinsics.a_bias.v[2]);
    p++;

    observe_plot_item(timestamp_us, p, "var-abias_x", (float)f->s.imu.intrinsics.a_bias.variance()[0]);
    observe_plot_item(timestamp_us, p, "var-abias_y", (float)f->s.imu.intrinsics.a_bias.variance()[1]);
    observe_plot_item(timestamp_us, p, "var-abias_z", (float)f->s.imu.intrinsics.a_bias.variance()[2]);
    p++;

    for (const auto &a : f->accelerometers) {
        observe_plot_item(timestamp_us, p, "a-inn-mean_x", (float)a->inn_stdev.mean[0]);
        observe_plot_item(timestamp_us, p, "a-inn-mean_y", (float)a->inn_stdev.mean[1]);
        observe_plot_item(timestamp_us, p, "a-inn-mean_z", (float)a->inn_stdev.mean[2]);
        p++;
    }

    for (const auto &g : f->gyroscopes) {
        observe_plot_item(timestamp_us, p, "g-inn-mean_x", (float)g->inn_stdev.mean[0]);
        observe_plot_item(timestamp_us, p, "g-inn-mean_y", (float)g->inn_stdev.mean[1]);
        observe_plot_item(timestamp_us, p, "g-inn-mean_z", (float)g->inn_stdev.mean[2]);
        p++;
    }

    for (const auto &c : f->cameras) {
        observe_plot_item(timestamp_us, p, "v-inn-mean_x", (float)c->inn_stdev.mean[0]);
        observe_plot_item(timestamp_us, p, "v-inn-mean_y", (float)c->inn_stdev.mean[1]);
        p++;
    }

    if (f->observations.recent_a.get()) {
        observe_plot_item(timestamp_us, p, "a-inn_x", (float)f->observations.recent_a->innovation(0));
        observe_plot_item(timestamp_us, p, "a-inn_y", (float)f->observations.recent_a->innovation(1));
        observe_plot_item(timestamp_us, p, "a-inn_z", (float)f->observations.recent_a->innovation(2));
    }
    p++;

    if (f->observations.recent_g.get()) {
        observe_plot_item(timestamp_us, p, "g-inn_x", (float)f->observations.recent_g->innovation(0));
        observe_plot_item(timestamp_us, p, "g-inn_y", (float)f->observations.recent_g->innovation(1));
        observe_plot_item(timestamp_us, p, "g-inn_z", (float)f->observations.recent_g->innovation(2));
    }
    p++;

    for (auto &of : f->observations.recent_f_map)
      observe_plot_item(timestamp_us,  p, "v-inn_x " + std::to_string(of.first), (float)of.second->innovation(0));
    p++;

    for (auto &of : f->observations.recent_f_map)
      observe_plot_item(timestamp_us,  p, "v-inn_y " + std::to_string(of.first), (float)of.second->innovation(1));
    p++;

    for (auto &of : f->observations.recent_f_map)
      observe_plot_item(timestamp_us, p, "v-inn_r " + std::to_string(of.first), (float)hypot(of.second->innovation(0), of.second->innovation(1)));
    p++;

    observe_plot_item(timestamp_us, p, "median-depth-var", (float)f->median_depth_variance);
    p++;
    
    observe_plot_item(timestamp_us, p, "accel timer", f->accel_timer.count());
    p++;

    observe_plot_item(timestamp_us, p, "gyro timer", f->gyro_timer.count());
    p++;

    observe_plot_item(timestamp_us, p, "image timer", f->image_timer.count());
    p++;
}

world_state::world_state()
{
    build_grid_vertex_data();
    for(int i = 0; i < 6; i++)
        axis_vertex.push_back(axis_data[i]);
    for(int i = 0; i < 6; i++)
        orientation_vertex.push_back(axis_data[i]);
    last_image.width = 0;
    last_image.height = 0;
    last_image.image = NULL;
    last_depth.width = 0;
    last_depth.height = 0;
    last_depth.image = NULL;
    generate_depth_overlay = false;
    last_depth_overlay_image.width = 0;
    last_depth_overlay_image.height = 0;
    last_depth_overlay_image.image = NULL;
}

world_state::~world_state()
{
}

static inline void set_position(VertexData * vertex, float x, float y, float z)
{
    vertex->position[0] = x;
    vertex->position[1] = y;
    vertex->position[2] = z;
}

static inline void set_position(VertexData * vertex, const v3 & v)
{
    vertex->position[0] = v[0];
    vertex->position[1] = v[1];
    vertex->position[2] = v[2];
}

static inline void set_color(VertexData * vertex, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha)
{
    vertex->color[0] = r;
    vertex->color[1] = g;
    vertex->color[2] = b;
    vertex->color[3] = alpha;
}

static inline void set_indexed_color(VertexData * vertex, int index)
{
    int ci = index % sizeof(indexed_colors);
    set_color(vertex, indexed_colors[ci][0], indexed_colors[ci][1], indexed_colors[ci][2], indexed_colors[ci][3]);
}

static inline void ellipse_segment(VertexData * v, const Feature & feat, float percent)
{
    float theta = (float)(2*M_PI*percent) + feat.ctheta;
    float x = feat.cx/2.f*cos(theta);
    float y = feat.cy/2.f*sin(theta);

    //rotate
    float x_out = x*cos(feat.ctheta) - y*sin(feat.ctheta);
    float y_out = x*sin(feat.ctheta) + y*cos(feat.ctheta);
    x = x_out;
    y = y_out;

    x += feat.projected_x;
    y += feat.projected_y;
    set_position(v, x, y, 0);
}

void world_state::generate_feature_ellipse(const Feature & feat, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha)
{
    int ellipse_segments = feature_ellipse_vertex_size/2;
    for(int i = 0; i < ellipse_segments; i++)
    {
        VertexData v1, v2;
        set_color(&v1, r, g, b, alpha);
        set_color(&v2, r, g, b, alpha);
        ellipse_segment(&v1, feat, (float)i/ellipse_segments);
        ellipse_segment(&v2, feat, (float)(i+1)/ellipse_segments);
        feature_ellipse_vertex.push_back(v1);
        feature_ellipse_vertex.push_back(v2);
    }
}

void world_state::update_vertex_arrays(bool show_only_good)
{
    /*
     * Build vertex arrays for feature and path data
     */
    uint64_t now = get_current_timestamp();
    display_lock.lock();

    feature_vertex.clear();
    feature_projection_vertex.clear();
    feature_ellipse_vertex.clear();
    for(auto const & item : features) {
        //auto feature_id = item.first;
        auto f = item.second;
        VertexData v, vp;
        if (f.last_seen == current_feature_timestamp) {
            if(f.good) {
                generate_feature_ellipse(f, 88, 247, 98, 255);
                set_color(&v, 88, 247, 98, 255);

                set_position(&vp, f.image_x, f.image_y, 0);
                set_color(&vp, 88, 247, 98, 255);
            }
            else {
                generate_feature_ellipse(f, 247, 88, 98, 255);
                set_color(&v, 247, 88, 98, 255);

                set_position(&vp, f.image_x, f.image_y, 0);
                set_color(&vp, 247, 88, 98, 255);
            }
        }
        else {
            if (show_only_good && !f.good)
                continue;
            set_color(&v, 255, 255, 255, 255);
        }
        set_position(&v, f.x, f.y, f.z);
        feature_vertex.push_back(v);
        feature_projection_vertex.push_back(vp);
    }

    path_vertex.clear();
    transformation current_position;
    for(auto p : path)
    {
        VertexData v;
        if (p.timestamp == now) {
            set_color(&v, 0, 255, 0, 255);
            for(int i = 0; i < 6; i++) {
                v3 vertex(axis_vertex[i].position[0],
                          axis_vertex[i].position[1],
                          axis_vertex[i].position[2]);
                current_position = p.g;
                vertex = transformation_apply(p.g, vertex);
                orientation_vertex[i].position[0] = (float)vertex[0];
                orientation_vertex[i].position[1] = (float)vertex[1];
                orientation_vertex[i].position[2] = (float)vertex[2];
            }
        }
        else
            set_color(&v, 0, 178, 206, 255); // path color
        set_position(&v, (float)p.g.T.x(), (float)p.g.T.y(), (float)p.g.T.z());
        path_vertex.push_back(v);
    }

    map_node_vertex.clear();
    map_edge_vertex.clear();
    map_feature_vertex.clear();
    for(auto n : map_nodes) {
        auto id = n.first;
        auto node = n.second;
        int alpha = 255;
        if(node.unlinked)
            alpha = 50;

        VertexData v;
        if(!node.finished)
            set_color(&v, 255, 0, 255, alpha);
        else if(node.loop_closed)
            set_color(&v, 255, 0, 0, alpha);
        else
            set_color(&v, 255, 255, 0, alpha);
        v3 v1(node.position.T);
        set_position(&v, v1[0], v1[1], v1[2]);
        map_node_vertex.push_back(v);
        for(uint64_t neighbor_id : node.neighbors) {
            if(!node.finished || !map_nodes[neighbor_id].finished) continue;

            VertexData ve;
            if(node.loop_closed && map_nodes[neighbor_id].loop_closed)
                set_color(&ve, 255, 0, 0, 255);
            else
                set_color(&ve, 255, 0, 255, alpha*0.2);
            set_position(&ve, v1[0], v1[1], v1[2]);
            map_edge_vertex.push_back(ve);

            auto node2 = map_nodes[neighbor_id];
            if(node.loop_closed && map_nodes[neighbor_id].loop_closed)
                set_color(&ve, 255, 0, 0, 255);
            else
                set_color(&ve, 255, 0, 255, alpha*0.2);
            set_position(&ve, node2.position.T.x(), node2.position.T.y(), node2.position.T.z());
            map_edge_vertex.push_back(ve);
        }
        for(Feature f : node.features) {
            VertexData vf;
            v3 vertex(f.x, f.y, f.z);
            vertex = transformation_apply(node.position, vertex);
            if(node.loop_closed)
                set_color(&vf, 255, 127, 127, 255);
            else
                set_color(&vf, 0, 0, 255, alpha);
            set_position(&vf, vertex[0], vertex[1], vertex[2]);
            map_feature_vertex.push_back(vf);
        }
    }

    for(auto p : path_gt)
    {
        VertexData v;
        set_color(&v, 206, 100, 178, 255); // path color
        set_position(&v, (float)p.g.T.x(), (float)p.g.T.y(), (float)p.g.T.z());
        path_gt_vertex.push_back(v);
    }

    sensor_vertex.clear();
    sensor_axis_vertex.clear();
    for(auto st : sensors) {
        int sensor_type = st.first;
        for(auto s : st.second) {
            uint16_t sensor_id = s.first;
            transformation g = current_position*s.second.extrinsics;
            VertexData v;
            set_indexed_color(&v, sensor_type);
            set_position(&v, (float)g.T.x(), (float)g.T.y(), (float)g.T.z());
            sensor_vertex.push_back(v);

            for(int i = 0; i < 6; i++) {
                VertexData va;
                v3 vertex(0.25*axis_vertex[i].position[0],
                          0.25*axis_vertex[i].position[1],
                          0.25*axis_vertex[i].position[2]);
                vertex = g*vertex;
                set_position(&va, vertex[0], vertex[1], vertex[2]);
                set_color(&va, axis_vertex[i].color[0], axis_vertex[i].color[1], axis_vertex[i].color[2], axis_vertex[i].color[3]);
                sensor_axis_vertex.push_back(va);
            }

        }
    }
    display_lock.unlock();
}

void world_state::build_grid_vertex_data()
{
    quaternion Q = rotation_between_two_vectors(v3(0,0,1), v3(up[0], up[1], up[2]));
    float scale = 1; /* meter */
    grid_vertex.clear();
    /* Grid */
    int idx = 0;
    unsigned char gridColor[4] = {122, 126, 146, 255};
    for(float x = -10*scale; x < 11*scale; x += scale)
    {
        VertexData v;
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(x, -10*scale, 0));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(x, 10*scale, 0));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(-10*scale, x, 0));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(10*scale, x, 0));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(-0, -10*scale, 0));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(-0, 10*scale, 0));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(-10*scale, -0, 0));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(10*scale, -0, 0));
        grid_vertex.push_back(v);

        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(0, -.1f*scale, x));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(0, .1f*scale, x));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(-.1f*scale, 0, x));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(.1f*scale, 0, x));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(0, -.1f*scale, -x));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(0, .1f*scale, -x));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(-.1f*scale, 0, -x));
        grid_vertex.push_back(v);
        set_color(&v, gridColor[0], gridColor[1], gridColor[2], gridColor[3]);
        set_position(&v, Q*v3(.1f*scale, 0, -x));
        grid_vertex.push_back(v);
    }
}

int world_state::get_feature_depth_measurements()
{
    int depth_measurements = 0;
    display_lock.lock();
    for(auto f : features)
        if(f.second.depth_measured)
            depth_measurements++;
    display_lock.unlock();
    return depth_measurements;
}

float world_state::get_feature_lifetime()
{
    float average_times_seen = 0;
    display_lock.lock();
    if(features.size()) {
        for(auto f : features) {
            average_times_seen += f.second.times_seen;
        }
        average_times_seen /= features.size();
    }
    display_lock.unlock();
    return average_times_seen;
}

std::string world_state::get_feature_stats()
{
    std::ostringstream os;
    os.precision(2);
    os << fixed;
    float average_times_seen = get_feature_lifetime();
    int depth_initialized = get_feature_depth_measurements();
    display_lock.lock();
    os << "Features seen: " << features.size() << " ";
    display_lock.unlock();
    os << "with an average life of " << average_times_seen << " frames" << std::endl;
    os << "Features initialized with depth: " << depth_initialized << std::endl;
    return os.str();
}

void world_state::observe_feature(uint64_t timestamp, uint64_t feature_id, float x, float y, float z, float image_x, float image_y, float projected_x, float projected_y, float cx, float cy, float ctheta, bool good, bool depth_measured)
{
    Feature f;
    f.x = x;
    f.y = y;
    f.z = z;
    f.image_x = image_x;
    f.image_y = image_y;
    f.projected_x = projected_x;
    f.projected_y = projected_y;
    f.cx = cx;
    f.cy = cy;
    f.ctheta = ctheta;
    f.last_seen = timestamp;
    f.good = good;
    f.times_seen = 1;
    f.depth_measured = depth_measured;
    display_lock.lock();
    if(timestamp > current_feature_timestamp)
        current_feature_timestamp = timestamp;
    update_current_timestamp(timestamp);
    if(features.count(feature_id))
        f.times_seen = features[feature_id].times_seen+1;
    features[feature_id] = f;
    display_lock.unlock();
}

void world_state::observe_position(uint64_t timestamp, float x, float y, float z, float qw, float qx, float qy, float qz)
{
    Position p;
    p.timestamp = timestamp;
    quaternion q(qw, qx, qy, qz);
    p.g = transformation(q, v3(x, y, z));
    display_lock.lock();
    path.push_back(p);
    update_current_timestamp(timestamp);
    display_lock.unlock();
}

void world_state::get_bounding_box(float min[3], float max[3])
{
    // in meters
    min[0] = -1; min[1] = -1; min[2] = -1;
    max[0] =  1; max[1] =  1; max[2] =  1;
    for(auto p : path) {
        min[0] = std::min(min[0], (float)p.g.T.x());
        min[1] = std::min(min[1], (float)p.g.T.y());
        min[2] = std::min(min[2], (float)p.g.T.z());
        max[0] = std::max(max[0], (float)p.g.T.x());
        max[1] = std::max(max[1], (float)p.g.T.y());
        max[2] = std::max(max[2], (float)p.g.T.z());
    }
    for(auto p : path_gt) {
        min[0] = std::min(min[0], (float)p.g.T.x());
        min[1] = std::min(min[1], (float)p.g.T.y());
        min[2] = std::min(min[2], (float)p.g.T.z());
        max[0] = std::max(max[0], (float)p.g.T.x());
        max[1] = std::max(max[1], (float)p.g.T.y());
        max[2] = std::max(max[2], (float)p.g.T.z());
    }
    for(auto const & item : features) {
        //auto feature_id = item.first;
        auto f = item.second;
        if(!f.good) continue;

        min[0] = std::min(min[0], (float)f.x);
        min[1] = std::min(min[1], (float)f.y);
        min[2] = std::min(min[2], (float)f.z);
        max[0] = std::max(max[0], (float)f.x);
        max[1] = std::max(max[1], (float)f.y);
        max[2] = std::max(max[2], (float)f.z);
    }
}

void world_state::observe_position_gt(uint64_t timestamp, float x, float y, float z, float qw, float qx, float qy, float qz)
{
    Position p;
    p.timestamp = timestamp;
    quaternion q(qw, qx, qy, qz);
    p.g = transformation(q, v3(x, y, z));
    display_lock.lock();
    path_gt.push_back(p);
    display_lock.unlock();
}
