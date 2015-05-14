#include "world_state_render.h"

#include <stdio.h>
#include <stdlib.h>

#include "gl_util.h"
#include "render.h"
#include "video_render.h"

static video_render frame_render;
static render render;

static draw_item grid_di, axis_di, feature_di, path_di, orientation_di;

static inline void setup_draw_item(draw_item & item)
{
    glGenBuffers(2, item.vertex_buffer_object);
}

void setup_world()
{
    setup_draw_item(grid_di);
    setup_draw_item(axis_di);
    setup_draw_item(feature_di);
    setup_draw_item(path_di);
    setup_draw_item(orientation_di);
}

bool world_state_render_init()
{
    render.gl_init();
    glEnable(GL_DEPTH_TEST);

    setup_world();
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

void world_state_render_video(world_state * world)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    world->image_lock.lock();
    frame_render.render(world->last_image.image, world->last_image.width, world->last_image.height, true);
    world->image_lock.unlock();
}

void world_state_render(world_state * world, float * viewMatrix, float * projMatrix)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    render.start_render(viewMatrix, projMatrix);

    world->display_lock.lock();

#if !(TARGET_OS_IPHONE)
    glPointSize(3.0f);
#endif
    glLineWidth(2.0f);
    render.draw_buffer(grid_di, world->grid_vertex, world->grid_vertex_num, GL_LINES);
    glLineWidth(4.0f);
    render.draw_buffer(axis_di, world->axis_vertex, world->axis_vertex_num, GL_LINES);
    render.draw_buffer(orientation_di, world->orientation_vertex, world->orientation_vertex_num, GL_LINES);
    render.draw_buffer(feature_di, world->feature_vertex, world->feature_vertex_num, GL_POINTS);
    render.draw_buffer(path_di, world->path_vertex, world->path_vertex_num, GL_POINTS);

    world->display_lock.unlock();
}
