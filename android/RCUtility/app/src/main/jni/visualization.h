#ifndef __VISUALIZATION__
#define __VISUALIZATION__

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "render_data.h"
#include "arcball.h"

class visualization_render
{
private:
    GLuint program;
    GLuint vertex_loc, color_loc;
    GLuint projection_matrix_loc, view_matrix_loc;

public:
    void gl_init();
    void gl_render(float * view_matrix, float * projection_matrix, render_data * data);
    void gl_destroy();
};

class visualization
{
private:
    static visualization * static_gui;
#if 0
    static void mouse_callback(GLFWwindow * window, int button, int action, int mods) {
        visualization::static_gui->mouse(window, button, action, mods);
    };
    static void move_callback(GLFWwindow * window, double x, double y) {
        visualization::static_gui->mouse_move(window, x, y);
    };
    static void keyboard_callback(GLFWwindow * window, int key, int scancode, int action, int mods) {
        visualization::static_gui->keyboard(window, key, scancode, action, mods);
    };
    static void scroll_callback(GLFWwindow * window, double dx, double dy) {
        visualization::static_gui->scroll(window, dx, dy);
    };
#endif

    float projection_matrix[16]; // 4x4
    float view_matrix[16]; // 4x4

    void configure_view(int view_width, int view_height);
#if 0
    void keyboard(GLFWwindow * window, int key, int scancode, int action, int mods);
#endif

    float scale;
    int width, height;
    visualization_render r;
    render_data * data;

    arcball arc;
    bool is_rotating;

    double start_scale;
    double start_offset;
    bool is_scrolling;

public:

    visualization(render_data * data);
    ~visualization();
    void setup(int width, int height);
    void render(int width, int height);
    void teardown();

    void mouse_move(double x, double y);
    void mouse_up();
    void scroll(double offset);
    void scroll_done();
};

#endif
