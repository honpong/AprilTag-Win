#ifndef __VIS_VIDEO_RENDER__
#define __VIS_VIDEO_RENDER__

#include "gl_util.h"

class video_render
{
private:
    GLuint program;
    GLuint vertex_loc, texture_coord_loc;
    GLuint frame_loc, channels_loc;
    GLuint texture;
    int width, height, channels;

public:
    void gl_init(int width, int height, bool luminance);
    void render(uint8_t * frame);
    void gl_destroy();
};

#endif
