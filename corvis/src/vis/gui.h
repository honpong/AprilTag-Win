#ifndef __CORVIS_GUI__
#define __CORVIS_GUI__

#include "world_state.h"
#include "arcball.h"

#include <atomic>

class replay;
struct GLFWwindow;

class gui
{
private:
    static gui * static_gui;
    static void mouse_callback(GLFWwindow * window, int button, int action, int mods) {
        gui::static_gui->mouse(window, button, action, mods);
    };
    static void move_callback(GLFWwindow * window, double x, double y) {
        gui::static_gui->mouse_move(window, x, y);
    };
    static void keyboard_callback(GLFWwindow * window, int key, int scancode, int action, int mods) {
        gui::static_gui->keyboard(window, key, scancode, action, mods);
    };
    static void scroll_callback(GLFWwindow * window, double dx, double dy) {
        gui::static_gui->scroll(window, dx, dy);
    };

    float projection_matrix[16]; // 4x4
    float view_matrix[16]; // 4x4

    void start_glfw();
    void init_gl();
    void configure_view(int view_width, int view_height);
    void mouse(GLFWwindow * window, int button, int action, int mods);
    void mouse_move(GLFWwindow * window, double x, double y);
    void scroll(GLFWwindow * window, double xoffset, double yoffset);
    void keyboard(GLFWwindow * window, int key, int scancode, int action, int mods);
    void render(int main_width, int main_height);
    void render_video(int video_width, int video_height);
    void render_plot(int plots_width, int plots_height);
    void next_plot();

    void calculate_viewports();

    void write_frame();

    world_state * state;

    float scale;
    int width, height;

    std::atomic<int> current_plot{0};
    std::atomic<bool> quit{false};

    // Mouse related
    arcball arc;
    bool is_rotating{false};

    // Display related
    GLFWwindow * main_window;
    bool show_main, show_video, show_plots;

    replay * replay_control;

public:
    gui(world_state * render_state, bool show_main, bool show_video, bool show_plots);
    ~gui();
    void queue_render();
    // If replay is NULL, pause and stepping control is not enabled
    void start(replay * rp=NULL);
};

#endif
