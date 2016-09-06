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
    Eigen::Matrix<float,4,4,Eigen::ColMajor> view_matrix;

    void start_glfw();
    void init_gl();
    void configure_view(int view_width, int view_height);
    void mouse(GLFWwindow * window, int button, int action, int mods);
    void mouse_move(GLFWwindow * window, double x, double y);
    void scroll(GLFWwindow * window, double xoffset, double yoffset);
    void keyboard(GLFWwindow * window, int key, int scancode, int action, int mods);
    void render(int main_width, int main_height);
    void render_video(int video_width, int video_height);
    void render_depth(int depth_width, int depth_height);
    void render_plot(int plots_width, int plots_height);

    void calculate_viewports();

    void write_frame();

    world_state * state;

    float scale;
    int width, height;

    float plot_scale = 1;

    std::atomic<size_t> current_plot{0};
    std::atomic<size_t> current_plot_key{(size_t)-1};
    std::atomic<bool> quit{false};

    // Mouse related
    arcball arc;
    bool is_rotating{false};
    v3 translation_m;
    v3 translation_start;
    v3 translation_finish;
    bool is_translating{false};
    v3 get_view_translation();
    std::atomic<bool> dirty{true};

    // Display related
    GLFWwindow * main_window;
    bool show_main, show_video, show_depth, show_plots, show_depth_on_video;
    std::function<bool(double x, double y)> in_main, in_plots, in_video, in_depth;
    bool is_main_selected = true, is_depth_selected = false, is_plot_selected = false, is_video_selected = false;

    replay * replay_control;

public:
    gui(world_state * render_state, bool show_main, bool show_video, bool show_depth, bool show_plots);
    ~gui();
    void queue_render();
    // If replay is NULL, pause and stepping control is not enabled
    void start(replay * rp=NULL);
};

#endif
