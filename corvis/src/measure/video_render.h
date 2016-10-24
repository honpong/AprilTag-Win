#ifndef __VIS_VIDEO_RENDER__
#define __VIS_VIDEO_RENDER__

#include "gl_util.h"
#include "world_state.h"

class video_render
{
private:
    GLuint program;
    GLuint vertex_loc, texture_coord_loc;
    GLuint frame_loc, channels_loc;
    GLuint texture;
    GLuint width_2_loc, height_2_loc;
    GLuint scale_x_loc, scale_y_loc;

    GLuint overlay_program;
    GLuint overlay_width_2_loc, overlay_height_2_loc;
    GLuint overlay_vertex_loc, overlay_color_loc;
    GLuint overlay_scale_y_loc, overlay_scale_x_loc;

public:
    void gl_init();
    void render(const uint8_t * frame, int width, int height, int viewport_width, int viewport_height, bool luminance);
    void gl_destroy();
    void draw_overlay(const VertexData * data, int number, int gl_type, int width, int height, int viewport_width, int viewport_height);
};

#endif
