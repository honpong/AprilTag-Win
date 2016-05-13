#ifndef __VIS_RENDER__
#define __VIS_RENDER__

#include "gl_util.h"
#include "world_state.h" // for VertexData

class render
{
private:
    GLuint program;
    GLuint vertex_loc, color_loc;
    GLuint projection_matrix_loc, view_matrix_loc;

public:
    void gl_init();
    void start_render(float * view_matrix, float * projection_matrix);
    void gl_destroy();
    void draw_array(const VertexData * data, int number, int gl_type);
};

#endif
