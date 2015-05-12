#ifndef __CORVIS_GUI__
#define __CORVIS_GUI__

#include "world_state.h"
#include "arcball.h"

class replay;
struct GLFWwindow;

class gui
{
private:
    static gui * static_gui;
    static void render_callback() { gui::static_gui->render(); };
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

    float _projectionMatrix[16]; // 4x4
    float _modelViewMatrix[16]; // 4x4
    int window_id;

    void create_plots();

    void start_glfw();
    void init_gl();
    void configure_view();
    void reshape(int width, int height);
    void mouse(GLFWwindow * window, int button, int action, int mods);
    void mouse_move(GLFWwindow * window, double x, double y);
    void scroll(GLFWwindow * window, double xoffset, double yoffset);
    void keyboard(GLFWwindow * window, int key, int scancode, int action, int mods);
    void render();
    world_state * state;

    float scale;
    int width, height;

    // Mouse related
    arcball arc;
    bool is_rotating{false};

    replay * replay_control;

public:
    gui(world_state * render_state);
    ~gui();
    void queue_render();
    void start(replay * rp);
};

#endif
