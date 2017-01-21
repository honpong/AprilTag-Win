#define NOMINMAX // Tell windows.h to avoid defining min/max
#include "world_state_render.h"

#include <stdio.h>
#include <stdlib.h>

#include <limits>

#include "gl_util.h"
#include "render.h"
#include "video_render.h"

static video_render depth_render;
static video_render frame_render;
static video_render plot_render;
static render render;

bool world_state_render_init()
{
    render.gl_init();

    return true;
}

void world_state_render_teardown()
{
    render.gl_destroy();
}

bool world_state_render_video_init()
{
    frame_render.gl_init();
    return true;
}

void world_state_render_video_teardown()
{
    frame_render.gl_destroy();
}

void world_state_render_video(world_state * world, rc_Sensor id, int viewport_width, int viewport_height)
{
    world->display_lock.lock();
    world->image_lock.lock();
    const auto & c = world->cameras[id];
    frame_render.render(c.image.image, c.image.width, c.image.height, viewport_width, viewport_height, true);
#if TARGET_OS_IPHONE
#else
    glPointSize(3.0f);
#endif
    glLineWidth(2.0f);
    frame_render.draw_overlay(c.feature_ellipse_vertex.data(), c.feature_ellipse_vertex.size(), GL_LINES, c.image.width, c.image.height, viewport_width, viewport_height);
    frame_render.draw_overlay(c.feature_projection_vertex.data(), c.feature_projection_vertex.size(), GL_POINTS, c.image.width, c.image.height, viewport_width, viewport_height);
    frame_render.draw_overlay(c.feature_residual_vertex.data(), c.feature_residual_vertex.size(), GL_LINES, c.image.width, c.image.height, viewport_width, viewport_height);
    world->image_lock.unlock();
    world->display_lock.unlock();
}

bool world_state_render_video_get_size(world_state * world, rc_Sensor id, int *width, int *height)
{
    if(id >= world->cameras.size()) return false;
    world->image_lock.lock();
    *width = world->cameras[id].image.width;
    *height = world->cameras[id].image.height;
    world->image_lock.unlock();
    return *width && *height;
}

bool world_state_render_depth_init()
{
    depth_render.gl_init();
    return true;
}

void world_state_render_depth_teardown()
{
    depth_render.gl_destroy();
}

void world_state_render_depth(world_state * world, rc_Sensor id, int viewport_width, int viewport_height)
{
    world->display_lock.lock();
    world->depth_lock.lock();
    depth_render.render(world->depths[id].image, world->depths[id].width, world->depths[id].height, viewport_width, viewport_height, true);
    world->depth_lock.unlock();
    world->display_lock.unlock();
}

void world_state_render_depth_on_video(world_state * world, rc_Sensor id, int viewport_width, int viewport_height)
{
    world->display_lock.lock();
    world->depth_lock.lock();
    frame_render.render(world->depths[id].image, world->depths[id].width, world->depths[id].height, viewport_width, viewport_height, true);
    world->depth_lock.unlock();
    world->display_lock.unlock();
}

bool world_state_render_depth_get_size(world_state * world, rc_Sensor id, int *width, int *height)
{
    if(id >= world->depths.size()) return false;
    world->depth_lock.lock();
    *width = world->depths[id].width;
    *height =  world->depths[id].height;
    world->depth_lock.unlock();
    return *width && *height;
}

void world_state_render(world_state * world, float * view_matrix, float * projection_matrix)
{
    render.start_render(view_matrix, projection_matrix);

    world->display_lock.lock();

#if !(TARGET_OS_IPHONE)
    glPointSize(3.0f);
#endif
    glLineWidth(2.0f);
    render.draw_array(world->grid_vertex.data(), world->grid_vertex.size(), GL_LINES);
    glLineWidth(4.0f);
    render.draw_array(world->axis_vertex.data(), world->axis_vertex.size(), GL_LINES);
    render.draw_array(world->orientation_vertex.data(), world->orientation_vertex.size(), GL_LINES);
    render.draw_array(world->feature_vertex.data(), world->feature_vertex.size(), GL_POINTS);
    render.draw_array(world->path_vertex.data(), world->path_vertex.size(), GL_POINTS);
    render.draw_array(world->path_gt_vertex.data(), world->path_gt_vertex.size(), GL_POINTS);
    render.draw_array(world->path_mini_vertex.data(), world->path_mini_vertex.size(), GL_POINTS);

    glLineWidth(2.0f);
    render.draw_array(world->sensor_axis_vertex.data(), world->sensor_axis_vertex.size(), GL_LINES);
    glPointSize(8.0f);
    render.draw_array(world->sensor_vertex.data(), world->sensor_vertex.size(), GL_POINTS);

#if !(TARGET_OS_IPHONE)
    glPointSize(10.0f);
#endif
    render.draw_array(world->map_node_vertex.data(), world->map_node_vertex.size(), GL_POINTS);
    glLineWidth(4.0f);
    render.draw_array(world->map_edge_vertex.data(), world->map_edge_vertex.size(), GL_LINES);
#if !(TARGET_OS_IPHONE)
    glPointSize(5.0f);
#endif
    render.draw_array(world->map_feature_vertex.data(), world->map_feature_vertex.size(), GL_POINTS);

    world->display_lock.unlock();
}

bool world_state_render_plot_init()
{
    plot_render.gl_init();
    return true;
}

void world_state_render_plot_teardown()
{
    plot_render.gl_destroy();
}

#ifndef HAVE_MGL
static void create_plot(world_state * state, size_t plot_index, size_t key_index, uint8_t *plot_frame, int plot_width, int plot_height) {}
#else // HAVE_MGL

#include "lodepng.h"
#define _MSC_VER 1900 // Force mathgl to avoid C99's typeof
#include <mgl2/mgl.h>
#undef _MSC_VER

static void create_plot(world_state * state, size_t plot_index, size_t key_index, uint8_t *plot_frame, int plot_width, int plot_height)
{
    mglGraph gr(0,plot_width,plot_height); // 600x400 graph, plotted to an image
    // mglData stores the x and y data to be plotted
    state->render_plot(plot_index, key_index, [&] (world_state::plot &plot, size_t key_index) {
        gr.NewFrame();
        gr.Alpha(false);
        gr.Clf('w');
        gr.SubPlot(1,1,0,"T");
        uint64_t mint = std::numeric_limits<uint64_t>::max(), maxt = std::numeric_limits<uint64_t>::min();
        float miny = std::numeric_limits<float>::max(), maxy = std::numeric_limits<float>::min();
        std::string names;
        const char *colors[] = {"r","g","b"};

        // Iterate over the data once to determine the extents
        int i=0;
        for (auto kvi = plot.begin(); kvi != plot.end(); ++kvi, i++) {
            // Enable these two lines if you want per-plot line
            // scaling when using the arrow keys
            //if (key_index != (size_t)-1 && key_index != i)
            //    continue;

            for(auto data : kvi->second) {
                uint64_t t = data.first;
                if(t < mint) mint = t;
                if(t > maxt) maxt = t;

                float val = data.second;
                if(val < miny) miny = val;
                if(val > maxy) maxy = val;
            }
        }

        i = 0;
        for (auto kvi = plot.begin(); kvi != plot.end(); ++kvi, i++) {
            if (key_index != (size_t)-1 && key_index != i)
                continue;
            const std::string &name = kvi->first; const plot_data &p = kvi->second;
            names += (names.size() ? " " : "") + name;

            mglData data_x(p.size());
            mglData data_y(p.size());

            int j = 0;
            for(auto data : p) {
                float seconds = -((maxt - data.first)/1.0e6);
                data_x.a[j] = seconds;

                float val = data.second;
                data_y.a[j++] = val;
            }

            gr.SetRange('x', -(state->max_plot_history_us/1.0e6), 0);
            gr.SetRange('y', miny, maxy);
            gr.Plot(data_x, data_y, colors[key_index == (size_t)-1 ? i%3 : key_index % 3]);
        }


        gr.Axis();
        gr.Title(names.c_str(),"k",6);
        gr.EndFrame();
        gr.GetRGBA((char *)plot_frame, plot_width*plot_height*4);

        //Encode the image
        //std::string filename = names + ".png";
        //unsigned error = lodepng::encode(filename.c_str(), plot_frame, plot_width, plot_height);
        //if(error)
        //    fprintf(stderr, "encoder error %d: %s\n", error, lodepng_error_text(error));
    });
}
#endif //HAVE_MGL

void world_state_render_plot(world_state * world, size_t plot_index, size_t key_index, int viewport_width, int viewport_height)
{
    static int plot_width = -1, plot_height = -1;
    static uint8_t *plot_frame;
    if (plot_width != viewport_width || plot_height != viewport_height) {
        plot_width = viewport_width;
        plot_height = viewport_height;
        free(plot_frame);
        plot_frame = (uint8_t *)malloc(plot_width*plot_height*4*sizeof(uint8_t));
    }
    if (plot_width && plot_height) {
        create_plot(world, plot_index, key_index, plot_frame, plot_width, plot_height);
        plot_render.render(plot_frame, plot_width, plot_height, viewport_width, viewport_height, false);
    }
}
