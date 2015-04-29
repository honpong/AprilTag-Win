#ifndef __CORVIS_GUI__
#define __CORVIS_GUI__

#include "world_state.h"
#include "arcball.h"

class gui
{
private:
    static gui * static_gui;
    static void render_callback() { gui::static_gui->render(); };
    static void reshape_callback(int w, int h) { gui::static_gui->reshape(w, h); };
    static void mouse_callback(int button, int state, int x, int y) {
        gui::static_gui->mouse(button, state, x, y);
    };
    static void move_callback(int x, int y) {
        gui::static_gui->mouse_move(x, y);
    };
    static void keyboard_callback(unsigned char key, int x, int y) {
        gui::static_gui->keyboard(key, x, y);
    };

    float _modelViewProjectionMatrix[16]; // 4x4
    float _normalMatrix[9]; // 3x3
    int window_id;

    void init_glut();
    void init_gl();
    void configure_view();
    void reshape(int width, int height);
    void mouse(int button, int state, int x, int y);
    void mouse_move(int x, int y);
    void keyboard(unsigned char key, int x, int y);
    void render();
    world_state * state;

    float scale;
    int width, height;

    // Mouse related
    arcball arc;
    bool is_rotating;

public:
    gui(world_state * render_state);
    ~gui();
    void queue_render();
    void start();
};

#endif
