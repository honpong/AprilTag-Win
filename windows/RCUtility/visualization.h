#ifndef __VISUALIZATION__
#define __VISUALIZATION__

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "render_data.h"

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
    static void keyboard_callback(GLFWwindow * window, int key, int scancode, int action, int mods) {
        visualization::static_gui->keyboard(window, key, scancode, action, mods);
    };
    static void scroll_callback(GLFWwindow * window, double dx, double dy) {
        visualization::static_gui->scroll(window, dx, dy);
    };

    float projection_matrix[16]; // 4x4
    float view_matrix[16]; // 4x4

    void configure_view(int view_width, int view_height);
    void scroll(GLFWwindow * window, double xoffset, double yoffset);
    void keyboard(GLFWwindow * window, int key, int scancode, int action, int mods);

    float scale;
    int width, height;
    visualization_render r;
    render_data * data;

    // Display related
    GLFWwindow * main_window;

public:

    visualization(render_data * data);
    ~visualization();
    void start();
};

#endif
