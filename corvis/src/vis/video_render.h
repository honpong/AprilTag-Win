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
    float width_2, height_2;

    GLuint overlay_program;
    GLuint overlay_width_2_loc, overlay_height_2_loc;
    GLuint overlay_vertex_loc, overlay_color_loc;

public:
    void gl_init();
    void render(uint8_t * frame, int width, int height, bool luminance);
    void gl_destroy();
    void draw_overlay(VertexData * data, int number, int gl_type);
};

#endif
