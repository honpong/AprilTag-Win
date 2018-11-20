#include "world_state.h"
#include "sensor_fusion.h"
#include "rc_compat.h"
#include "rc_internal.h"
#include "bstream.h"
#include <array>

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
static const float chi_square_95_2df = 5.99146f; //http://math.stackexchange.com/questions/8672/eigenvalues-and-eigenvectors-of-2-times-2-matrix

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

size_t world_state::get_plot_by_name(std::string plot_name)
{
    std::lock_guard<std::mutex> lock(plot_lock);
    auto inserted = plots_by_name.emplace(plot_name, plots_by_name.size());

    return inserted.first->second;
}

void world_state::observe_plot_item(rc_Timestamp timestamp, size_t index, std::string name, float value)
{
    plot_lock.lock();
    if (index+1 > plots.size())
        plots.resize(index+1);
    auto &plot = plots[index][name];
    plot.push_back(plot_item(timestamp, value));
    while(plot.front().first + max_plot_history_us < timestamp)
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

void world_state::observe_camera_intrinsics(uint16_t sensor_id, const state_vision_intrinsics& intrinsics)
{
    display_lock.lock();
    camera_intrinsics[sensor_id] = &intrinsics;
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

static void update_image_size(const rc_ImageData & src, ImageData & dst)
{
    bool src_luminance = src.format == rc_FORMAT_GRAY8 || src.format == rc_FORMAT_DEPTH16; // both rendered as grey
    if(!dst.image)
        dst.image = (uint8_t *)malloc(            sizeof(uint8_t)*src.width*src.height*(src_luminance?1:4));
    else if(src.width != dst.width || src.height != dst.height || src_luminance != dst.luminance)
        dst.image = (uint8_t *)realloc(dst.image, sizeof(uint8_t)*src.width*src.height*(src_luminance?1:4));
    dst.width = src.width;
    dst.height = src.height;
    dst.luminance = src_luminance;
}

void world_state::observe_image(rc_Timestamp timestamp, rc_Sensor camera_id, const rc_ImageData &data,
                                std::vector<overlay_data> &cameras)
{
    image_lock.lock();
    if(cameras.size() < (size_t)camera_id+1)
        cameras.resize(camera_id+1);

    ImageData & image = cameras[camera_id].image;
    image.timestamp = timestamp;
    update_image_size(data, image);

    for(int i=0; i<data.height; i++)
        memcpy(image.image + i*image.width*(image.luminance?1:4), (uint8_t *)data.image + i*data.stride, sizeof(uint8_t)*(data.format == rc_FORMAT_GRAY8?1:4)*data.width);

    image_lock.unlock();
}

#define MAX_DEPTH 8191

void world_state::observe_depth(rc_Timestamp timestamp, rc_Sensor camera_id, const rc_ImageData & data)
{
    depth_lock.lock();

    if(depths.size() < (size_t)camera_id+1)
        depths.resize(camera_id+1);

    ImageData & image = depths[camera_id];
    update_image_size(data, image);

    const uint16_t * src = (const uint16_t *) data.image;
    for(int i = 0; i < data.height; ++i)
        for(int j = 0; j < data.width; ++j)
            image.image[image.width * i + j] = (src[data.stride/2 * i + j] == 0 || src[data.stride/2 * i + j] > MAX_DEPTH) ? 0 : 255 - (src[data.stride/2 * i + j] / 32);

    depth_lock.unlock();
}

void world_state::observe_depth_overlay_image(rc_Timestamp timestamp, uint16_t *aligned_depth, int width, int height, int stride)
{
    /*
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
    */
}

static inline void compute_covariance_ellipse(float x, float y, float xy, float & cx, float & cy, float & ctheta)
{
    if(x == 0) x = 1;
    if(y == 0) y = 1;
    float tau = 0;
    if(xy != 0.f)
        tau = (y - x) / xy / 2.f;
    float t = (tau >= 0.) ? (1.f / (fabs(tau) + sqrt(1.f + tau * tau))) : (-1.f / (fabs(tau) + sqrt(1.f + tau * tau)));
    float c = 1.f / sqrt(1.f + tau * tau);
    float s = c * t;
    float l1 = x - t * xy;
    float l2 = y + t * xy;
    float theta = atan2(-s, c);

    cx = (float)2. * sqrt(l1 * chi_square_95_2df); // ellipse width
    cy = (float)2. * sqrt(l2 * chi_square_95_2df); // ellipse height
    ctheta = (float)theta; // rotate
}

rc_Timestamp world_state::get_current_timestamp()
{
    std::lock_guard<std::mutex> lock(time_lock);
    return current_timestamp;
}

void world_state::update_current_timestamp(const rc_Timestamp &timestamp)
{
    std::lock_guard<std::mutex> lock(time_lock);
    if(timestamp > current_timestamp)
        current_timestamp = timestamp;
}

void world_state::update_plots(rc_Tracker * tracker, const rc_Data * data)
{
    const struct filter * f = &((sensor_fusion *)tracker)->sfm;
    rc_Timestamp timestamp_us = data->time_us;

    int p;

    if (f->observations.recent_a.get()) {
        auto id = std::to_string(f->observations.recent_a->source.id);
        p = get_plot_by_name("accel obs " + id);
        observe_plot_item(timestamp_us, p, "a_x" + id, (float)f->observations.recent_a->meas[0]);
        observe_plot_item(timestamp_us, p, "a_y" + id, (float)f->observations.recent_a->meas[1]);
        observe_plot_item(timestamp_us, p, "a_z" + id, (float)f->observations.recent_a->meas[2]);
    }

    if (f->observations.recent_g.get()) {
        auto id = std::to_string(f->observations.recent_g->source.id);
        p = get_plot_by_name("gyro obs " + id);
        observe_plot_item(timestamp_us, p, "g_x" + id, (float)f->observations.recent_g->meas[0]);
        observe_plot_item(timestamp_us, p, "g_y" + id, (float)f->observations.recent_g->meas[1]);
        observe_plot_item(timestamp_us, p, "g_z" + id, (float)f->observations.recent_g->meas[2]);
    }

    if (f->observations.recent_v.get()) {
        auto id = std::to_string(f->observations.recent_v->source.id);
        p = get_plot_by_name("velo obs " + id);
        observe_plot_item(timestamp_us, p, "v_x" + id, (float)f->observations.recent_v->meas[0]);
        observe_plot_item(timestamp_us, p, "v_y" + id, (float)f->observations.recent_v->meas[1]);
        observe_plot_item(timestamp_us, p, "v_z" + id, (float)f->observations.recent_v->meas[2]);
    }

    p = get_plot_by_name("state velocity");
    observe_plot_item(timestamp_us, p, "sv_x", (float)f->s.V.v[0]);
    observe_plot_item(timestamp_us, p, "sv_y", (float)f->s.V.v[1]);
    observe_plot_item(timestamp_us, p, "sv_z", (float)f->s.V.v[2]);

    p = get_plot_by_name("state acceleration");
    observe_plot_item(timestamp_us, p, "sa_x", (float)f->s.a.v[0]);
    observe_plot_item(timestamp_us, p, "sa_y", (float)f->s.a.v[1]);
    observe_plot_item(timestamp_us, p, "sa_z", (float)f->s.a.v[2]);

    p = get_plot_by_name("state rotational velocity");
    observe_plot_item(timestamp_us, p, "sw_x", (float)f->s.w.v[0]);
    observe_plot_item(timestamp_us, p, "sw_y", (float)f->s.w.v[1]);
    observe_plot_item(timestamp_us, p, "sw_z", (float)f->s.w.v[2]);

    p = get_plot_by_name("state rotational acceleration");
    observe_plot_item(timestamp_us, p, "sdw_x", (float)f->s.dw.v[0]);
    observe_plot_item(timestamp_us, p, "sdw_y", (float)f->s.dw.v[1]);
    observe_plot_item(timestamp_us, p, "sdw_z", (float)f->s.dw.v[2]);

    if (data->path == rc_DATA_PATH_SLOW) {
        rc_StorageStats storage = rc_getStorageStats(tracker);
        p = get_plot_by_name("map size");
        observe_plot_item(timestamp_us, p, "nodes", storage.nodes);
        observe_plot_item(timestamp_us, p, "features", storage.features);
        observe_plot_item(timestamp_us, p, "edges", storage.edges);
    }

    for (size_t i=0; i<f->s.cameras.children.size(); i++) {
        const auto &camera = *f->s.cameras.children[i];
        if (!camera.intrinsics.estimate) continue;
        p = get_plot_by_name("distortion");
        if (camera.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE)
            observe_plot_item(timestamp_us, p, "w" + std::to_string(i), (float)camera.intrinsics.k.v[0]);
        else if (camera.intrinsics.type == rc_CALIBRATION_TYPE_POLYNOMIAL3 || camera.intrinsics.type == rc_CALIBRATION_TYPE_KANNALA_BRANDT4) {
            observe_plot_item(timestamp_us, p, "k1-" + std::to_string(i), (float)camera.intrinsics.k.v[0]);
            observe_plot_item(timestamp_us, p, "k2-" + std::to_string(i), (float)camera.intrinsics.k.v[1]);
            observe_plot_item(timestamp_us, p, "k3-" + std::to_string(i), (float)camera.intrinsics.k.v[2]);
            if (camera.intrinsics.type == rc_CALIBRATION_TYPE_KANNALA_BRANDT4)
                observe_plot_item(timestamp_us, p, "k4-" + std::to_string(i), (float)camera.intrinsics.k.v[3]);
        }

        p = get_plot_by_name("focal");
        observe_plot_item(timestamp_us, p, "F" + std::to_string(i), (float)(camera.intrinsics.focal_length.v * camera.intrinsics.image_height));

        p = get_plot_by_name("center");
        observe_plot_item(timestamp_us, p, "C_x" + std::to_string(i), (float)(camera.intrinsics.center.v.x() * camera.intrinsics.image_height + camera.intrinsics.image_width  / 2. - .5));
        observe_plot_item(timestamp_us, p, "C_y" + std::to_string(i), (float)(camera.intrinsics.center.v.y() * camera.intrinsics.image_height + camera.intrinsics.image_height / 2. - .5));
    }

    for (size_t i=0; i<f->s.cameras.children.size(); i++) {
        const auto &extrinsics = f->s.cameras.children[i]->extrinsics;
        if (!extrinsics.estimate) continue;
        p = get_plot_by_name("extrinsics_Tc" + std::to_string(i));
        observe_plot_item(timestamp_us, p, "Tc" + std::to_string(i) + "_x", (float)extrinsics.T.v[0]);
        observe_plot_item(timestamp_us, p, "Tc" + std::to_string(i) + "_y", (float)extrinsics.T.v[1]);
        observe_plot_item(timestamp_us, p, "Tc" + std::to_string(i) + "_z", (float)extrinsics.T.v[2]);

        p = get_plot_by_name("extrinsics_Wc" + std::to_string(i));
        observe_plot_item(timestamp_us, p, "Wc" + std::to_string(i) + "_x", (float)to_rotation_vector(extrinsics.Q.v).raw_vector()[0]);
        observe_plot_item(timestamp_us, p, "Wc" + std::to_string(i) + "_y", (float)to_rotation_vector(extrinsics.Q.v).raw_vector()[1]);
        observe_plot_item(timestamp_us, p, "Wc" + std::to_string(i) + "_z", (float)to_rotation_vector(extrinsics.Q.v).raw_vector()[2]);
    }

    for (size_t i=0; i<f->s.imus.children.size(); i++) {
        const auto &extrinsics = f->s.imus.children[i]->extrinsics;
        if (!extrinsics.estimate) continue;
        p = get_plot_by_name("extrinsics_Ta" + std::to_string(i));
        observe_plot_item(timestamp_us, p, "Ta" + std::to_string(i) + "_x", (float)extrinsics.T.v[0]);
        observe_plot_item(timestamp_us, p, "Ta" + std::to_string(i) + "_y", (float)extrinsics.T.v[1]);
        observe_plot_item(timestamp_us, p, "Ta" + std::to_string(i) + "_z", (float)extrinsics.T.v[2]);

        p = get_plot_by_name("extrinsics_Wa" + std::to_string(i));
        observe_plot_item(timestamp_us, p, "Wa" + std::to_string(i) + "_x", (float)to_rotation_vector(extrinsics.Q.v).raw_vector()[0]);
        observe_plot_item(timestamp_us, p, "Wa" + std::to_string(i) + "_y", (float)to_rotation_vector(extrinsics.Q.v).raw_vector()[1]);
        observe_plot_item(timestamp_us, p, "Wa" + std::to_string(i) + "_z", (float)to_rotation_vector(extrinsics.Q.v).raw_vector()[2]);
    }

    for (size_t i=0; i<f->s.velocimeters.children.size(); i++) {
        const auto &extrinsics = f->s.velocimeters.children[i]->extrinsics;
        if (!extrinsics.estimate) continue;
        p = get_plot_by_name("extrinsics_Tv" + std::to_string(i));
        observe_plot_item(timestamp_us, p, "Tv" + std::to_string(i) + "_x", (float)extrinsics.T.v[0]);
        observe_plot_item(timestamp_us, p, "Tv" + std::to_string(i) + "_y", (float)extrinsics.T.v[1]);
        observe_plot_item(timestamp_us, p, "Tv" + std::to_string(i) + "_z", (float)extrinsics.T.v[2]);

        p = get_plot_by_name("extrinsics_Wv" + std::to_string(i));
        observe_plot_item(timestamp_us, p, "Wv" + std::to_string(i) + "_x", (float)to_rotation_vector(extrinsics.Q.v).raw_vector()[0]);
        observe_plot_item(timestamp_us, p, "Wv" + std::to_string(i) + "_y", (float)to_rotation_vector(extrinsics.Q.v).raw_vector()[1]);
        observe_plot_item(timestamp_us, p, "Wv" + std::to_string(i) + "_z", (float)to_rotation_vector(extrinsics.Q.v).raw_vector()[2]);
    }

    p = get_plot_by_name("translation");
    observe_plot_item(timestamp_us, p, "T_x", (float)f->s.T.v[0]);
    observe_plot_item(timestamp_us, p, "T_y", (float)f->s.T.v[1]);
    observe_plot_item(timestamp_us, p, "T_z", (float)f->s.T.v[2]);

   if (path_gt.size() && path_gt.back().timestamp == data->time_us) {
       transformation ref = path_gt.back().g;
       rotation_vector v_current = to_rotation_vector(f->s.Q.v);
       rotation_vector v_ref = to_rotation_vector(ref.Q);
       f_t a_current = v_current.raw_vector().norm();
       f_t a_ref = v_ref.raw_vector().norm();
       rotation_vector v_error = to_rotation_vector(f->s.Q.v*ref.Q.conjugate());
       f_t orientation_error = v_error.raw_vector().norm();
       f_t error_Tx = f->s.T.v[0] - ref.T[0];
       f_t error_Ty = f->s.T.v[1] - ref.T[1];
       f_t error_Tz = f->s.T.v[2] - ref.T[2];
       p = get_plot_by_name("angle");
       observe_plot_item(timestamp_us, p, "angle current", a_current);
       observe_plot_item(timestamp_us, p, "angle ref", a_ref);
       p = get_plot_by_name("angle error");
       observe_plot_item(timestamp_us, p, "angle error", orientation_error);
       observe_plot_item(timestamp_us, p, "x angle error", v_error.x());
       observe_plot_item(timestamp_us, p, "y angle error", v_error.y());
       observe_plot_item(timestamp_us, p, "z angle error", v_error.z());
       p = get_plot_by_name("translation error");
       observe_plot_item(timestamp_us, p, "x error", error_Tx);
       observe_plot_item(timestamp_us, p, "y error", error_Ty);
       observe_plot_item(timestamp_us, p, "z error", error_Tz);
       p = get_plot_by_name("ate");
       observe_plot_item(timestamp_us, p, "ate", ate);
    }

    p = get_plot_by_name("translation var");
    observe_plot_item(timestamp_us, p, "Tvar_x", (float)f->s.T.variance()[0]);
    observe_plot_item(timestamp_us, p, "Tvar_y", (float)f->s.T.variance()[1]);
    observe_plot_item(timestamp_us, p, "Tvar_z", (float)f->s.T.variance()[2]);

    for (size_t i=0; i<f->s.imus.children.size(); i++) { const auto &imu = f->s.imus.children[i];
        p = get_plot_by_name("gyro bias" + std::to_string(i));
        observe_plot_item(timestamp_us, p, "wbias_x" + std::to_string(i), (float)imu->intrinsics.w_bias.v[0]);
        observe_plot_item(timestamp_us, p, "wbias_y" + std::to_string(i), (float)imu->intrinsics.w_bias.v[1]);
        observe_plot_item(timestamp_us, p, "wbias_z" + std::to_string(i), (float)imu->intrinsics.w_bias.v[2]);
    }
    for (size_t i=0; i<f->s.imus.children.size(); i++) { const auto &imu = f->s.imus.children[i];
        p = get_plot_by_name("gyro bias var" + std::to_string(i));
        observe_plot_item(timestamp_us, p, "var-wbias_x" + std::to_string(i), (float)imu->intrinsics.w_bias.variance()[0]);
        observe_plot_item(timestamp_us, p, "var-wbias_y" + std::to_string(i), (float)imu->intrinsics.w_bias.variance()[1]);
        observe_plot_item(timestamp_us, p, "var-wbias_z" + std::to_string(i), (float)imu->intrinsics.w_bias.variance()[2]);
    }
    for (size_t i=0; i<f->s.imus.children.size(); i++) { const auto &imu = f->s.imus.children[i];
        p = get_plot_by_name("accel bias" + std::to_string(i));
        observe_plot_item(timestamp_us, p, "abias_x" + std::to_string(i), (float)imu->intrinsics.a_bias.v[0]);
        observe_plot_item(timestamp_us, p, "abias_y" + std::to_string(i), (float)imu->intrinsics.a_bias.v[1]);
        observe_plot_item(timestamp_us, p, "abias_z" + std::to_string(i), (float)imu->intrinsics.a_bias.v[2]);
    }
    for (size_t i=0; i<f->s.imus.children.size(); i++) { const auto &imu = f->s.imus.children[i];
        p = get_plot_by_name("accel bias var" + std::to_string(i));
        observe_plot_item(timestamp_us, p, "var-abias_x" + std::to_string(i), (float)imu->intrinsics.a_bias.variance()[0]);
        observe_plot_item(timestamp_us, p, "var-abias_y" + std::to_string(i), (float)imu->intrinsics.a_bias.variance()[1]);
        observe_plot_item(timestamp_us, p, "var-abias_z" + std::to_string(i), (float)imu->intrinsics.a_bias.variance()[2]);
    }

    for (const auto &a : f->accelerometers) {
        p = get_plot_by_name("accel inn" + std::to_string(a->id));
        observe_plot_item(timestamp_us, p, "a-inn-mean_x", (float)a->inn_stdev.mean[0]);
        observe_plot_item(timestamp_us, p, "a-inn-mean_y", (float)a->inn_stdev.mean[1]);
        observe_plot_item(timestamp_us, p, "a-inn-mean_z", (float)a->inn_stdev.mean[2]);
    }

    for (const auto &g : f->gyroscopes) {
        p = get_plot_by_name("gyro inn" + std::to_string(g->id));
        observe_plot_item(timestamp_us, p, "g-inn-mean_x", (float)g->inn_stdev.mean[0]);
        observe_plot_item(timestamp_us, p, "g-inn-mean_y", (float)g->inn_stdev.mean[1]);
        observe_plot_item(timestamp_us, p, "g-inn-mean_z", (float)g->inn_stdev.mean[2]);
    }

    for (const auto &c : f->cameras) {
        p = get_plot_by_name("vinn" + std::to_string(c->id));
        observe_plot_item(timestamp_us, p, "v-inn-mean_x", (float)c->inn_stdev.mean[0]);
        observe_plot_item(timestamp_us, p, "v-inn-mean_y", (float)c->inn_stdev.mean[1]);
    }

    if (f->observations.recent_a.get()) {
        auto id = std::to_string(f->observations.recent_a->source.id);
        p = get_plot_by_name("a obs inn" + id);
        observe_plot_item(timestamp_us, p, "a-inn_x" + id, (float)f->observations.recent_a->innovation(0));
        observe_plot_item(timestamp_us, p, "a-inn_y" + id, (float)f->observations.recent_a->innovation(1));
        observe_plot_item(timestamp_us, p, "a-inn_z" + id, (float)f->observations.recent_a->innovation(2));
    }

    if (f->observations.recent_g.get()) {
        auto id = std::to_string(f->observations.recent_g->source.id);
        p = get_plot_by_name("g obs inn" + id);
        observe_plot_item(timestamp_us, p, "g-inn_x" + id, (float)f->observations.recent_g->innovation(0));
        observe_plot_item(timestamp_us, p, "g-inn_y" + id, (float)f->observations.recent_g->innovation(1));
        observe_plot_item(timestamp_us, p, "g-inn_z" + id, (float)f->observations.recent_g->innovation(2));
    }

    p = get_plot_by_name("fmap x");
    for (auto &of : f->observations.recent_f_map)
      observe_plot_item(timestamp_us,  p, "v-inn_x " + std::to_string(of.first), (float)of.second->innovation(0));

    p = get_plot_by_name("fmap y");
    for (auto &of : f->observations.recent_f_map)
      observe_plot_item(timestamp_us,  p, "v-inn_y " + std::to_string(of.first), (float)of.second->innovation(1));

    p = get_plot_by_name("fmap r");
    for (auto &of : f->observations.recent_f_map)
      observe_plot_item(timestamp_us, p, "v-inn_r " + std::to_string(of.first), (float)hypot(of.second->innovation(0), of.second->innovation(1)));

    p = get_plot_by_name("median depth var");
    observe_plot_item(timestamp_us, p, "median-depth-var", (float)f->median_depth_variance);

    p = get_plot_by_name("state-size");
    observe_plot_item(timestamp_us, p, "state-size", (float)f->s.statesize);
    int group_storage = f->s.groups.children.size() * 6;;
    int feature_storage = 0;
    for (const auto &g : f->s.groups.children)
        feature_storage += g->features.children.size();
    observe_plot_item(timestamp_us, p, "groups", (float)group_storage);
    observe_plot_item(timestamp_us, p, "feats", (float)feature_storage);

    p = get_plot_by_name("track counts");
    for (size_t i=0; i<f->s.cameras.children.size(); i++) {
        const auto &camera = *f->s.cameras.children[i];
        int lost = 0, found = 0; for (const auto &t : camera.tracks) { t.track.found() ? found++ : lost++; }
        observe_plot_item(timestamp_us, p, "lost" + std::to_string(i), lost);
        observe_plot_item(timestamp_us, p, "found" + std::to_string(i), found);
    }

    p = get_plot_by_name("acc timer");
    for (size_t i=0; i<f->accelerometers.size(); i++) {
        observe_plot_item(timestamp_us, p, "accel timer " + std::to_string(i), f->accelerometers[i]->measure_time_stats.last[0]);
        observe_plot_item(timestamp_us, p, "mini accel timer " + std::to_string(i), f->accelerometers[i]->fast_path_time_stats.last[0]);
    }

    p = get_plot_by_name("gyro timer");
    for (size_t i=0; i<f->gyroscopes.size(); i++) {
        observe_plot_item(timestamp_us, p, "gyro timer " + std::to_string(i), f->gyroscopes[i]->measure_time_stats.last[0]);
        observe_plot_item(timestamp_us, p, "mini gyro timer " + std::to_string(i), f->gyroscopes[i]->fast_path_time_stats.last[0]);
    }

    p = get_plot_by_name("velo timer");
    for (size_t i=0; i<f->velocimeters.size(); i++) {
        observe_plot_item(timestamp_us, p, "velo timer " + std::to_string(i), f->velocimeters[i]->measure_time_stats.last[0]);
        //observe_plot_item(timestamp_us, p, "mini velo timer " + std::to_string(i), f->velocimeters[i]->other_time_stats.last[0]);
    }

    p = get_plot_by_name("image timer");
    for (size_t i=0; i<f->cameras.size(); i++)
        observe_plot_item(timestamp_us, p, "image timer " + std::to_string(i), f->cameras[i]->measure_time_stats.last[0]);

    p = get_plot_by_name("detect timer");
    for (size_t i=0; i<f->cameras.size(); i++)
        observe_plot_item(timestamp_us, p, "detect timer " + std::to_string(i), f->cameras[i]->detect_time_stats.last[0]);
}

void world_state::update_sensors(rc_Tracker * tracker, const rc_Data * data)
{
    const struct filter * f = &((sensor_fusion *)tracker)->sfm;

    for(const auto & s : f->accelerometers) {
        observe_sensor(rc_SENSOR_TYPE_ACCELEROMETER, s->id, s->extrinsics.mean.T[0], s->extrinsics.mean.T[1], s->extrinsics.mean.T[2],
                      s->extrinsics.mean.Q.w(), s->extrinsics.mean.Q.x(), s->extrinsics.mean.Q.y(), s->extrinsics.mean.Q.z());
    }
    for(const auto & s : f->gyroscopes) {
        observe_sensor(rc_SENSOR_TYPE_GYROSCOPE, s->id, s->extrinsics.mean.T[0], s->extrinsics.mean.T[1], s->extrinsics.mean.T[2],
                      s->extrinsics.mean.Q.w(), s->extrinsics.mean.Q.x(), s->extrinsics.mean.Q.y(), s->extrinsics.mean.Q.z());
    }
    for(const auto & s : f->velocimeters) {
        observe_sensor(rc_SENSOR_TYPE_VELOCIMETER, s->id, s->extrinsics.mean.T[0], s->extrinsics.mean.T[1], s->extrinsics.mean.T[2],
                      s->extrinsics.mean.Q.w(), s->extrinsics.mean.Q.x(), s->extrinsics.mean.Q.y(), s->extrinsics.mean.Q.z());
    }
    for(const auto & s : f->cameras) {
        observe_sensor(rc_SENSOR_TYPE_IMAGE, s->id, s->extrinsics.mean.T[0], s->extrinsics.mean.T[1], s->extrinsics.mean.T[2],
                      s->extrinsics.mean.Q.w(), s->extrinsics.mean.Q.x(), s->extrinsics.mean.Q.y(), s->extrinsics.mean.Q.z());
    }
    for(const auto & s : f->s.cameras.children) {
        observe_camera_intrinsics(s->id, s->intrinsics);
    }
    for(const auto & s : f->depths) {
        observe_sensor(rc_SENSOR_TYPE_DEPTH, s->id, s->extrinsics.mean.T[0], s->extrinsics.mean.T[1], s->extrinsics.mean.T[2],
                      s->extrinsics.mean.Q.w(), s->extrinsics.mean.Q.x(), s->extrinsics.mean.Q.y(), s->extrinsics.mean.Q.z());
    }

    observe_world(f->s.world.up[0], f->s.world.up[1], f->s.world.up[2],
                  f->s.world.initial_forward[0], f->s.world.initial_forward[1], f->s.world.initial_forward[2],
                  f->s.body_forward[0], f->s.body_forward[1], f->s.body_forward[2]);
}

void world_state::update_map(rc_Tracker * tracker, rc_Timestamp timestamp_us)
{
    const struct filter * f = &((sensor_fusion *)tracker)->sfm;

    if(f->map) {
        std::map<uint64_t, MapNode> current_map_nodes;
        std::vector<MapNode> current_removed_map_nodes;
        wall_time_point wall_time = wall_clock::now();

        const auto& nodes = f->map->get_nodes();
        for(const auto& it : nodes) {
            auto& map_node = it.second;
            std::vector<Neighbor> neighbors;
            for(auto& edge : map_node.edges) {
                Neighbor neighbor(edge.first);
                neighbor.type = edge.second.type;
                neighbors.push_back(neighbor);
            }
            for(auto& edge : map_node.relocalization_edges) {
                Neighbor neighbor(edge.first);
                neighbor.type = edge.second.type;
                neighbors.push_back(neighbor);
            }

            std::vector<Feature> features;
            for(auto &feat : map_node.features) {
                Feature fw;
                v3 G_node_feature = f->map->get_feature3D(map_node.id, feat.second.feature->id);
                v3 G_world_feature = transformation_apply(map_node.global_transformation, G_node_feature);
                fw.feature.id = feat.second.feature->id;
                fw.feature.world.x = G_world_feature[0];
                fw.feature.world.y = G_world_feature[1];
                fw.feature.world.z = G_world_feature[2];
                fw.is_triangulated = feat.second.type == feature_type::triangulated;
                features.push_back(fw);
            }

            MapNode n;
            n.id = map_node.id;
            n.finished = map_node.status == node_status::finished;
            n.unlinked = f->map->is_unlinked(map_node.id);
            n.position = map_node.global_transformation;
            n.neighbors = std::move(neighbors);
            n.features = std::move(features);
            current_map_nodes.emplace(map_node.id, std::move(n));
        }

        // find removed nodes and features
        MapNode virtual_removed_node;  // fake node that contains the features that are removed from live nodes
        virtual_removed_node.id = std::numeric_limits<decltype(virtual_removed_node.id)>::max();
        for(auto it = map_nodes.begin(); it != map_nodes.end(); ++it) {
            auto node_it = nodes.find(it->first);
            if(node_it == nodes.end()) {
                current_removed_map_nodes.emplace_back(std::move(it->second));
            } else {
                auto& map_node = node_it->second;
                std::vector<Feature>& features = it->second.features;
                features.erase(
                    std::remove_if(features.begin(), features.end(), [&map_node, &virtual_removed_node](Feature& fw) {
                        if(map_node.features.find(fw.feature.id) == map_node.features.end()) {
                            virtual_removed_node.features.emplace_back(std::move(fw));
                            return true;
                        }
                        return false;
                    }), features.end());
            }
        }
        if (!virtual_removed_node.features.empty())
            current_removed_map_nodes.emplace_back(std::move(virtual_removed_node));

        for (const auto& it : *f->map->stages) {
            const auto& name = it.first;
            const auto& stage = it.second;
            auto node_it = current_map_nodes.find(stage.closest_id);
            if (node_it != current_map_nodes.end()) {
                transformation G_world_stage = node_it->second.position * stage.Gr_closest_stage;
                node_it->second.virtual_object_links.emplace_back(G_world_stage);
                observe_virtual_object(timestamp_us, name, to_rc_Pose(G_world_stage), node_it->second.unlinked);
            } else
                unobserve_virtual_object(name);
        }

        std::lock_guard<std::mutex> lock(display_lock);
        map_nodes = std::move(current_map_nodes);
        if (!current_removed_map_nodes.empty())
            removed_map_nodes[wall_time] = std::move(current_removed_map_nodes);
    }
}

void world_state::update_relocalization(const replay_output *output) {
    rc_Pose* poses;
    size_t n = output->rc_getRelocPoses(&poses);
    observe_position_reloc(output->sensor_time_us, poses, (n > 0 ? 1 : 0));
}

template<size_t idx>
static rc_ImageData get_rc_image(const rc_StereoData &data) {
    return rc_ImageData{
        data.shutter_time_us, data.width, data.height,
        idx ? data.stride2 : data.stride1, data.format,
        idx ? data.image2 : data.image1, nullptr, nullptr };
}

void world_state::rc_data_callback(const replay_output *output, const rc_Data * data)
{
    rc_Tracker *tracker = output->tracker;
    auto &pt = output->rc_getPose(data->path);
    rc_Timestamp timestamp_us = pt.time_us;
    transformation G = to_transformation(pt.pose_m);
    observe_position(timestamp_us, (float)G.T[0], (float)G.T[1], (float)G.T[2], (float)G.Q.w(), (float)G.Q.x(), (float)G.Q.y(), (float)G.Q.z(), data->path == rc_DATA_PATH_FAST);
    if(tracker) update_sensors(tracker, data);

    if(data->path == rc_DATA_PATH_FAST) return;

    if (data->type == rc_SENSOR_TYPE_IMAGE || data->type == rc_SENSOR_TYPE_STEREO) {
        auto output_type = output->get_output_type();
        if (output_type == replay_output::output_mode::POSE_FEATURE || output_type == replay_output::output_mode::POSE_FEATURE_MAP) {
            rc_Feature * img_feats;
            int nfeatures = output->rc_getFeatures(&img_feats);
            for (int i = 0; i < nfeatures; i++)
                observe_feature(timestamp_us, data->id, img_feats[i]);
        }
        if (output_type == replay_output::output_mode::POSE_MAP || output_type == replay_output::output_mode::POSE_FEATURE_MAP) {
            update_relocalization(output);
        }
        if (data->type == rc_SENSOR_TYPE_IMAGE)
            observe_image(timestamp_us, data->id, data->image, cameras);
        else {
            observe_image(timestamp_us, data->id + 0, get_rc_image<0>(data->stereo), cameras);
            ///observe_image(timestamp_us, data->id + 1, get_rc_image<1>(data->stereo), cameras); //TODO support second image
        }
    }

    switch(data->type) {
        case rc_SENSOR_TYPE_DEBUG:
            {
            if (data->debug.message)
                std::cout << "debug(" << data->id << "): " << data->debug.message << "\n";
            if (data->debug.erase_previous_debug_images)
                debug_cameras.clear();
            observe_image(timestamp_us, data->id, data->debug.image, debug_cameras);
            }
            break;
        case rc_SENSOR_TYPE_IMAGE:
        case rc_SENSOR_TYPE_STEREO:
            {
            if (!tracker) break;

            const struct filter * f = &((sensor_fusion *)tracker)->sfm;
            for(const auto &t: f->s.cameras.children[data->id]->standby_tracks)
            {
                rc_Feature rcf;
                rcf.id = t.feature->id;
                rcf.camera_id = 0; // no depth, so no camera
                rcf.image_x = t.x;
                rcf.image_y = t.y;
                rcf.image_prediction_x = t.x;
                rcf.image_prediction_y = t.y;
                rcf.initialized = false;
                rcf.stdev = 100;
                rcf.depth = 100;
                rcf.depth_measured = false;
                rcf.recovered = false;
                rcf.innovation_variance_x = rcf.innovation_variance_y = rcf.innovation_variance_xy = 0;
                observe_feature(timestamp_us, data->id, rcf);
            }

            // Map update is slow and loop closure checks only happen
            // on images, so only update on image updates
            update_map(tracker, data->time_us);
            }
            break;

        case rc_SENSOR_TYPE_DEPTH:
            {
            if (!tracker) break;
            const struct filter * f = &((sensor_fusion *)tracker)->sfm;
            if (f->has_depth) {
                observe_depth(timestamp_us, data->id, data->depth);

#if 0
                if (generate_depth_overlay){
                    auto depth_overlay = filter_aligned_depth_overlay(f, f->recent_depth, d);
                    observe_depth_overlay_image(sensor_clock::tp_to_micros(depth_overlay->timestamp), depth_overlay->image, depth_overlay->width, depth_overlay->height, depth_overlay->stride);
                }
#endif
            }
            }
            break;

        case rc_SENSOR_TYPE_ACCELEROMETER:
            {
            int p = get_plot_by_name("ameas" + std::to_string(data->id));
            observe_plot_item(timestamp_us, p, "ameas_x", (float)data->acceleration_m__s2.x);
            observe_plot_item(timestamp_us, p, "ameas_y", (float)data->acceleration_m__s2.y);
            observe_plot_item(timestamp_us, p, "ameas_z", (float)data->acceleration_m__s2.z);
            }
            break;


        case rc_SENSOR_TYPE_GYROSCOPE:
            {
            int p = get_plot_by_name("wmeas" + std::to_string(data->id));
            observe_plot_item(timestamp_us, p, "wmeas_x", (float)data->angular_velocity_rad__s.x);
            observe_plot_item(timestamp_us, p, "wmeas_y", (float)data->angular_velocity_rad__s.y);
            observe_plot_item(timestamp_us, p, "wmeas_z", (float)data->angular_velocity_rad__s.z);
            }
            break;

        case rc_SENSOR_TYPE_VELOCIMETER:
            {
            int p = get_plot_by_name("vmeas");
            if (data->id == 0)
                observe_plot_item(timestamp_us, p, "vmeas_x"+std::to_string(data->id), (float)data->translational_velocity_m__s.x);
            else if (data->id == 1)
                observe_plot_item(timestamp_us, p, "vmeas_x"+std::to_string(data->id), (float)data->translational_velocity_m__s.x);
            }
            break;

        default:
            break;
    }

    if(tracker) update_plots(tracker, data);
}

world_state::world_state()
{
    build_grid_vertex_data();
    for(int i = 0; i < 6; i++)
        axis_vertex.push_back(axis_data[i]);
    for(int i = 0; i < 6; i++)
        orientation_vertex.push_back(axis_data[i]);
    /*
    generate_depth_overlay = false;
    last_depth_overlay_image.width = 0;
    last_depth_overlay_image.height = 0;
    last_depth_overlay_image.image = NULL;
    */
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
    int ci = index % (sizeof(indexed_colors) / sizeof(indexed_colors[0]));
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

    x += feat.feature.image_prediction_x;
    y += feat.feature.image_prediction_y;
    set_position(v, x, y, 0);
}

static PolyhedronCoordinates generate_cuboid_coordinates(const v3& L) {
    return {
        {
            {-L[0], -L[1], -L[2]}, {L[0], -L[1], -L[2]}, { L[0],  L[1], -L[2]}, {-L[0], L[1], -L[2]},
            {-L[0], -L[1],  L[2]}, {L[0], -L[1],  L[2]}, { L[0],  L[1],  L[2]}, {-L[0], L[1],  L[2]}
        },
        {0, 1, 2, 3, 0, 4, 5, 6, 7, 4, 5, 1, 2, 6, 7, 3}};
}

static PolyhedronCoordinates generate_tetrahedron_coordinates(f_t L) {
    return {
        { {-L, -L, 0}, {L, -L, 0}, { L, L, 0}, {-L, L, 0}, { 0, 0, L } },
        {0, 1, 2, 3, 0, 4, 1, 4, 2, 4, 3}};
}

static PolyhedronCoordinates generate_arrow_coordinates(f_t L) {
    return {
        { {0, 0, 0}, {0, 0, L}, {L/4, 0, L/4*3}, {-L/4, 0, L/4*3} },
        {0, 1, 2, 1, 3}};
}

template<typename It>
static void generate_vertices(const transformation& G_world_object, const PolyhedronCoordinates& coordinates, const unsigned char rgba[4], It out_it) {
    VertexData v;
    set_color(&v, rgba[0], rgba[1], rgba[2], rgba[3]);
    for (size_t i = 0; i < coordinates.vertex_indices.size(); ++i) {
        const v3& obj_vertex = coordinates.vertices[coordinates.vertex_indices[i]];
        set_position(&v, G_world_object * obj_vertex);
        *out_it++ = v;
        if (i > 0 && i + 1 < coordinates.vertex_indices.size())  // replicate last point
            *out_it++ = v;
    }
}

void world_state::generate_feature_ellipse(const Feature & feat, std::vector<VertexData> & feature_ellipse_vertex, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha)
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

void world_state::generate_innovation_line(const Feature & feat, std::vector<VertexData> & feature_residual_vertex, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha)
{
     /* Add line between predicted feature and their corresponding matched observations (tracked features)
      * when the innovations are very large. Calculate NEES: Test based on Normalized Mahalanobis Distance.
      */
    Eigen::Matrix2f S = {
        { feat.feature.innovation_variance_x, feat.feature.innovation_variance_xy },
        { feat.feature.innovation_variance_xy,feat.feature.innovation_variance_y },
    };
    Eigen::Vector2f v_inn = {
        feat.feature.image_prediction_x - feat.feature.image_x,
        feat.feature.image_prediction_y - feat.feature.image_y
    };
    float nees = v_inn.dot(S.llt().solve(v_inn)) / chi_square_95_2df;
    if (nees > 1.f || !(feat.feature.innovation_variance_x  && feat.feature.innovation_variance_y)) {
        r = 0; g = 191; b = 255; alpha = 255;
    }

    VertexData v_meas, v_pred;
    set_position(&v_meas, feat.feature.image_x, feat.feature.image_y, 0);
    set_color(&v_meas, r, g, b, alpha);
    set_position(&v_pred, feat.feature.image_prediction_x, feat.feature.image_prediction_y, 0);
    set_color(&v_pred, r, g, b, alpha);
    feature_residual_vertex.push_back(v_meas);
    feature_residual_vertex.push_back(v_pred);
}

bool world_state::update_vertex_arrays(bool show_only_good)
{
    /*
     * Build vertex arrays for feature and path data
     */
    rc_Timestamp now = get_current_timestamp();
    std::lock_guard<std::mutex> lock(display_lock);
    if(!dirty)
        return false;

    feature_vertex.clear();
    for(auto & c : cameras) {
        c.feature_projection_vertex.clear();
        c.feature_ellipse_vertex.clear();
        c.feature_residual_vertex.clear();
        c.virtual_objects_vertex.clear();
    }

    for(auto const & item : features) {
        //auto feature_id = item.first;
        auto f = item.second;
        VertexData v, vp;
        if (cameras.size() && f.last_seen == cameras[f.camera_id].image.timestamp) {
            std::array<unsigned char,4> feature_color;
            if     (f.depth_measured)      feature_color = {{247, 247,  98, 255}}; // yellow
            else if(f.feature.recovered)   feature_color = {{255, 140,   0, 255}}; // orange
            else if(f.good)                feature_color = {{88,  247,  98, 255}}; // green
            else if(f.not_in_filter)       feature_color = {{88,   98, 247, 255}}; // blue
            else                           feature_color = {{247,  88,  98, 255}}; // red

            generate_feature_ellipse(f, cameras[f.camera_id].feature_ellipse_vertex, feature_color[0], feature_color[1], feature_color[2], feature_color[3]);
            set_color(&v, feature_color[0], feature_color[1], feature_color[2], feature_color[3]);

            set_position(&vp, f.feature.image_x, f.feature.image_y, 0);
            set_color(&vp, feature_color[0], feature_color[1], feature_color[2], feature_color[3]);

            generate_innovation_line(f,cameras[f.camera_id].feature_residual_vertex, feature_color[0], feature_color[1], feature_color[2], feature_color[3]);
            cameras[f.camera_id].feature_projection_vertex.push_back(vp);
        }
        else {
            if (show_only_good && !f.good)
                continue;
            set_color(&v, 255, 255, 255, 255);
        }
        set_position(&v, f.feature.world.x, f.feature.world.y, f.feature.world.z);
        feature_vertex.push_back(v);
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
        auto node = n.second;
        int alpha = 255;
        if(node.unlinked)
            alpha = 50;

        VertexData v;
        if(!node.finished)
            set_color(&v, 255, 0, 255, alpha);
        else
            set_color(&v, 255, 255, 0, alpha);
        v3 v1(node.position.T);
        set_position(&v, v1[0], v1[1], v1[2]);
        map_node_vertex.push_back(v);
        for(Neighbor neighbor : node.neighbors) {
            VertexData ve;
            if(neighbor.type == edge_type::filter)
                set_color(&ve, 255, 0, 255, alpha*0.2);
            else if(neighbor.type == edge_type::map)
                set_color(&ve, 0, 255, 0, alpha*0.2);
            else if(neighbor.type == edge_type::composition)
                set_color(&ve, 255, 140, 0, alpha);
            else if(neighbor.type == edge_type::dead_reckoning)
                set_color(&ve, 255, 0, 0, alpha);
            else if(neighbor.type == edge_type::relocalization)
                set_color(&ve, 0, 255, 0, alpha);

            set_position(&ve, v1[0], v1[1], v1[2]);
            map_edge_vertex.push_back(ve);

            auto node2 = map_nodes[neighbor.id];
            if(neighbor.type == edge_type::filter)
                set_color(&ve, 255, 0, 255, alpha*0.2);
            else if(neighbor.type == edge_type::map)
                set_color(&ve, 0, 255, 0, alpha*0.2);
            else if(neighbor.type == edge_type::composition)
                set_color(&ve, 255, 140, 0, alpha*0.2);
            else if(neighbor.type == edge_type::dead_reckoning)
                set_color(&ve, 255, 0, 0, alpha);
            else if(neighbor.type == edge_type::relocalization)
                set_color(&ve, 0, 255, 0, alpha);

            set_position(&ve, node2.position.T.x(), node2.position.T.y(), node2.position.T.z());
            map_edge_vertex.push_back(ve);
        }
        for(Feature f : node.features) {
            VertexData vf;
            if (f.is_triangulated) {
                set_color(&vf, 255, 153, 0, alpha);
            } else {
                set_color(&vf, 0, 0, 255, alpha);
            }
            set_position(&vf, f.feature.world.x, f.feature.world.y, f.feature.world.z);
            map_feature_vertex.push_back(vf);
        }
        for(transformation& t : node.virtual_object_links) {
            VertexData ve;
            set_color(&ve, 255, 165, 0, alpha);
            set_position(&ve, v1[0], v1[1], v1[2]);
            map_edge_vertex.push_back(ve);
            set_position(&ve, t.T[0], t.T[1], t.T[2]);
            map_edge_vertex.push_back(ve);
        }
    }

    constexpr float decay_seconds = 3.f;
    wall_time_point wall_time = std::chrono::steady_clock::now();
    wall_time_point old_time = wall_time -  std::chrono::duration_cast<wall_clock::duration>(std::chrono::duration<float>(decay_seconds));
    removed_map_nodes.erase(removed_map_nodes.begin(), removed_map_nodes.lower_bound(old_time));

    removed_map_node_vertex.clear();
    removed_map_feature_vertex.clear();
    for (auto& it : removed_map_nodes) {
        float elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<float>>(wall_time - it.first).count();
        const unsigned char alpha = (1.f - elapsed_seconds / decay_seconds) * 255.f;
        constexpr unsigned char color[3] = {255, 0, 0};  // red

        removed_map_node_vertex.reserve(removed_map_node_vertex.size() + it.second.size());
        for (MapNode& n : it.second) {
            if (n.id != std::numeric_limits<decltype(n.id)>::max()) {  // special value we have to ignore
                removed_map_node_vertex.emplace_back();
                set_position(&removed_map_node_vertex.back(), n.position.T[0], n.position.T[1], n.position.T[2]);
                set_color(&removed_map_node_vertex.back(), color[0], color[1], color[2], alpha);
            }
            removed_map_feature_vertex.reserve(removed_map_feature_vertex.size() + n.features.size());
            for (Feature& f : n.features) {
                removed_map_feature_vertex.emplace_back();
                set_position(&removed_map_feature_vertex.back(), f.feature.world.x, f.feature.world.y, f.feature.world.z);
                set_color(&removed_map_feature_vertex.back(), color[0], color[1], color[2], alpha);
            }
        }
    }

    path_mini_vertex.clear();
    for(auto p : path_mini)
    {
        VertexData v;
        set_color(&v, 255, 206, 100, 255); // path color
        set_position(&v, (float)p.g.T.x(), (float)p.g.T.y(), (float)p.g.T.z());
        path_mini_vertex.push_back(v);
    }
    
    path_gt_vertex.clear();
    path_axis_vertex.clear();
    for(Position p : path_gt)
    {
        VertexData v;
        set_color(&v, 206, 100, 178, 255); // path color
        set_position(&v, (float)p.g.T.x(), (float)p.g.T.y(), (float)p.g.T.z());
        path_gt_vertex.push_back(v);
    }

    if (path_gt.size()) {
        Position p = path_gt[path_gt.size()-1];
        for(int i = 0; i < 6; i++) {
            VertexData vagt;
            v3 vertex(axis_vertex[i].position[0],
                      axis_vertex[i].position[1],
                      axis_vertex[i].position[2]);
            vertex = p.g*vertex;
            set_position(&vagt, vertex[0], vertex[1], vertex[2]);
            set_color(&vagt, axis_vertex[i].color[0], axis_vertex[i].color[1], axis_vertex[i].color[2], axis_vertex[i].color[3]);
            path_axis_vertex.push_back(vagt);
        }
    }

    sensor_vertex.clear();
    sensor_axis_vertex.clear();
    for(auto st : sensors) {
        int sensor_type = st.first;
        for(auto s : st.second) {
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

    reloc_vertex.clear();
    for(Position p : path_reloc)
    {
        VertexData v;
        set_color(&v, 10, 255, 10, 255); // path color
        set_position(&v, (float)p.g.T.x(), (float)p.g.T.y(), (float)p.g.T.z());
        reloc_vertex.push_back(v);
    }

    virtual_object_vertex.clear();
    for(auto& it : virtual_objects) {
        const VirtualObject& vo = it.second;
        auto clip_alpha = [&vo](unsigned char alpha) {
            return std::min<unsigned char>((vo.unlinked ? 50 : 255), alpha);
        };
        for(int i = 0; i < 6; i++) {
            VertexData v;
            v3 vertex(0.5*axis_vertex[i].position[0],
                      0.5*axis_vertex[i].position[1],
                      0.5*axis_vertex[i].position[2]);
            vertex = vo.pose.g*vertex;
            set_position(&v, vertex[0], vertex[1], vertex[2]);
            set_color(&v, axis_vertex[i].color[0], axis_vertex[i].color[1], axis_vertex[i].color[2], clip_alpha(axis_vertex[i].color[3]));
            virtual_object_vertex.emplace_back(v);
        }
        generate_vertices(vo.pose.g, vo.coords, vo.rgba, std::back_inserter(virtual_object_vertex));
    }

    if(!path_mini.empty() || !path.empty()) {
        const transformation& G_world_body = (path.empty() ? path_mini.back().g : path.back().g);
        for(auto& cit : camera_intrinsics) {
            uint16_t sensor_id = cit.first;
            const state_vision_intrinsics* intrinsics = cit.second;
            const transformation& G_body_camera = sensors.at(rc_SENSOR_TYPE_IMAGE).at(sensor_id).extrinsics;
            transformation G_camera_world = invert(G_world_body * G_body_camera);
            auto& vertices = cameras[sensor_id].virtual_objects_vertex;
            for(auto& vit : virtual_objects) {
                const VirtualObject& vo = vit.second;
                if (vo.unlinked) continue;
                aligned_vector<v2> projection = vo.project(G_camera_world, intrinsics);
                for(v2& vertex : projection) {
                    VertexData v;
                    set_position(&v, vertex[0], vertex[1], 1);
                    set_color(&v, vo.rgba[0], vo.rgba[1], vo.rgba[2], vo.rgba[3]);
                    vertices.emplace_back(v);
                }
                for(size_t i = 0; i < axis_vertex.size(); i += 2) {
                    auto& a = axis_vertex[i];
                    auto& b = axis_vertex[i+1];
                    projection = vo.project_axis({a.position[0], a.position[1], a.position[2]},
                                                 {b.position[0], b.position[1], b.position[2]},
                                                 G_camera_world, intrinsics);
                    for(auto& vertex : projection) {
                        VertexData v;
                        set_position(&v, vertex[0], vertex[1], 1);
                        set_color(&v, axis_vertex[i].color[0], axis_vertex[i].color[1], axis_vertex[i].color[2], axis_vertex[i].color[3]);
                        vertices.emplace_back(v);
                    }
                }
            }
        }
    }

    dirty = false;
    return true;
}

void world_state::build_grid_vertex_data()
{
    quaternion Q = rotation_between_two_vectors(v3(0,0,1), v3(up[0], up[1], up[2]));
    float scale = 1; /* meter */
    grid_vertex.clear();
    /* Grid */
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
        if(f.second.feature.depth_measured)
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
    os << std::fixed;
    float average_times_seen = get_feature_lifetime();
    int depth_initialized = get_feature_depth_measurements();
    display_lock.lock();
    os << "Features seen: " << features.size() << " ";
    display_lock.unlock();
    os << "with an average life of " << average_times_seen << " frames" << std::endl;
    os << "Features initialized with depth: " << depth_initialized << std::endl;
    return os.str();
}

void world_state::observe_feature(rc_Timestamp timestamp, rc_Sensor camera_id, const rc_Feature & feature)
{
    float cx, cy, ctheta;
    compute_covariance_ellipse(feature.innovation_variance_x, feature.innovation_variance_y, feature.innovation_variance_xy, cx, cy, ctheta);
    Feature f;
    f.feature = feature;
    f.cx = cx;
    f.cy = cy;
    f.ctheta = ctheta;
    f.last_seen = timestamp;
    f.times_seen = 1;
    f.good = feature.stdev / feature.depth < 0.02;;
    f.depth_measured = feature.depth_measured;
    f.not_in_filter = feature.innovation_variance_x == 0;
    f.camera_id = camera_id;

    display_lock.lock();
    if(timestamp > current_feature_timestamp)
        current_feature_timestamp = timestamp;
    if(features.count(std::make_pair(camera_id,feature.id)))
        f.times_seen = features[std::make_pair(camera_id,feature.id)].times_seen+1;
    features[std::make_pair(camera_id,feature.id)] = f;
    display_lock.unlock();
}

void world_state::observe_position(rc_Timestamp timestamp, float x, float y, float z, float qw, float qx, float qy, float qz, bool fast)
{
    Position p;
    p.timestamp = timestamp;
    quaternion q(qw, qx, qy, qz);
    p.g = transformation(q, v3(x, y, z));
    display_lock.lock();
    if (fast)
        path_mini.push_back(p);
    else {
        path.push_back(p);
        update_current_timestamp(timestamp);
    }
    dirty = true;
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
    for(auto p : path_mini) {
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

        min[0] = std::min(min[0], (float)f.feature.world.x);
        min[1] = std::min(min[1], (float)f.feature.world.y);
        min[2] = std::min(min[2], (float)f.feature.world.z);
        max[0] = std::max(max[0], (float)f.feature.world.x);
        max[1] = std::max(max[1], (float)f.feature.world.y);
        max[2] = std::max(max[2], (float)f.feature.world.z);
    }
}

void world_state::observe_position_gt(rc_Timestamp timestamp, float x, float y, float z, float qw, float qx, float qy, float qz)
{
    Position p;
    p.timestamp = timestamp;
    quaternion q(qw, qx, qy, qz);
    p.g = transformation(q, v3(x, y, z));
    display_lock.lock();
    path_gt.push_back(p);
    display_lock.unlock();
}

void world_state::observe_ate(rc_Timestamp timestamp_us, const float absolute_trajectory_error)
{
    ate = absolute_trajectory_error;
}

void world_state::observe_rpe(rc_Timestamp timestamp_us, const float relative_pose_error)
{
    int p = get_plot_by_name("last rpe_T");
    observe_plot_item(timestamp_us, p, "last 1s rpe T", (float)relative_pose_error);
}

void world_state::observe_position_reloc(rc_Timestamp timestamp, const rc_Pose* poses, size_t nposes) {
    Position p;
    p.timestamp = timestamp;
    decltype(path_reloc) additional_reloc;
    additional_reloc.reserve(nposes);
    for (size_t i = 0; i < nposes; ++i) {
        const rc_Pose& G_world_body = poses[i];
        p.g = to_transformation(G_world_body);
        additional_reloc.push_back(p);
    }
    display_lock.lock();
    path_reloc.insert(path_reloc.end(),
                      std::make_move_iterator(additional_reloc.begin()),
                      std::make_move_iterator(additional_reloc.end()));
    display_lock.unlock();
}

void world_state::observe_virtual_object(rc_Timestamp timestamp, const std::string &name, const rc_Pose& pose, bool unlinked) {
    display_lock.lock();
    auto it = virtual_objects.lower_bound(name);
    if (it == virtual_objects.end() || it->first != name) {
        it = virtual_objects.emplace_hint(it, name, VirtualObject::make_cube());
        it->second.rgba[0] = 255;
        it->second.rgba[1] = 165;
        it->second.rgba[2] = 0;
        it->second.rgba[3] = 255;
    }
    it->second.unlinked = unlinked;
    it->second.pose.timestamp = timestamp;
    it->second.pose.g = to_transformation(pose);
    display_lock.unlock();
}

void world_state::unobserve_virtual_object(const std::string &name) {
    display_lock.lock();
    virtual_objects.erase(name);
    display_lock.unlock();
}

_virtual_object _virtual_object::make_cube(f_t side_length) {
    struct _virtual_object vo;
    vo.coords = generate_cuboid_coordinates(v3{1, 1, 1} * (side_length * 0.5f));
    return vo;
}

_virtual_object _virtual_object::make_tetrahedron(f_t side_length) {
    struct _virtual_object vo;
    vo.coords = generate_tetrahedron_coordinates(side_length * 0.5f);
    return vo;
}

aligned_vector<v2> _virtual_object::project(const transformation& G_camera_world,
                                            const state_vision_intrinsics* intrinsics) const {
    return project_points(this->coords.vertices, this->coords.vertex_indices, this->bounding_box, G_camera_world * this->pose.g, intrinsics);
}

aligned_vector<v2> _virtual_object::project_axes(const transformation& G_camera_world,
                                                 const state_vision_intrinsics* intrinsics) const {
    aligned_vector<v3> p3d;
    p3d.reserve(6);
    for (int i = 0; i < 6; ++i)
        p3d.emplace_back(axis_data[i].position[0], axis_data[i].position[1], axis_data[i].position[2]);
    return project_points(p3d, {0, 1, 2, 3, 4, 5}, {}, G_camera_world * this->pose.g, intrinsics);
}

aligned_vector<v2> _virtual_object::project_axis(
        const v3& vo_p, const v3& vo_q, const transformation& G_camera_world,
        const state_vision_intrinsics* intrinsics) const {
    aligned_vector<v3> p3d = {vo_p, vo_q};
    return project_points(p3d, {0, 1}, {}, G_camera_world * this->pose.g, intrinsics);
}

aligned_vector<v2> _virtual_object::project_points(
        const aligned_vector<v3>& vo_points, const std::vector<size_t>& point_indices,
        const aligned_vector<v3>& vo_bounding_box, const transformation& G_camera_vo,
        const state_vision_intrinsics* intrinsics) {
    auto project_point = [intrinsics](const v3& p3d, v2& p2d) {
        if (p3d.z() > 0) {
            p2d = intrinsics->project_feature(p3d);
            return true;
        }
        return false;
    };
    auto in_image = [intrinsics](const v2& p) {
        int x = static_cast<int>(p.x());
        int y = static_cast<int>(p.y());
        return (x >= 0 && x < intrinsics->image_width && y >= 0 && y < intrinsics->image_height);
    };
    auto discretize_line = [](const v3& from, const v3& to, aligned_vector<v3>& ps) {
        constexpr f_t step = 0.05;
        const int N = static_cast<int>((to - from).norm() / step) + 1;
        const v3 vstep = (to - from) / (N - 1);
        v3 p = from;
        for (int i = 0; i < N; ++i, p += vstep) ps.emplace_back(p);
        if (!p.isApprox(to)) ps.emplace_back(to);
    };

    bool maybe_visible = [&]() {
        if (vo_bounding_box.empty()) return true;
        v2 p;
        for (const v3& v : vo_bounding_box)
            if (project_point(G_camera_vo * v, p) && in_image(p))
                return true;
        return false;
    }();

    aligned_vector<v2> projection;
    if (maybe_visible) {
        aligned_vector<v3> discretized_lines;
        for (size_t i = 1; i < point_indices.size(); ++i) {
            auto& from = vo_points[point_indices[i-1]];
            auto& to = vo_points[point_indices[i]];
            discretize_line(from, to, discretized_lines);
        }

        if (!discretized_lines.empty()) {
            v2 from, to;
            bool valid_from = false;
            for (const v3& v : discretized_lines) {
                if (project_point(G_camera_vo * v, to) && in_image(to)) {
                    if (!valid_from) {
                        from = to;
                        valid_from = true;
                    } else {
                        projection.emplace_back(from);
                        projection.emplace_back(to);
                        from = to;
                    }
                } else {
                    valid_from = false;
                }
            }
        }
    }
    return projection;
}
