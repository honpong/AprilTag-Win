#ifndef __CORVIS_GUI__
#define __CORVIS_GUI__

#include "world_state.h"

class gui
{
private:
    static gui * static_gui;
    static void render_callback() { gui::static_gui->render(); };

    float _modelViewProjectionMatrix[16]; // 4x4
    float _normalMatrix[9]; // 3x3
    int window_id;

    void init_glut();
    void init_gl();
    void configure_view();
    void render();
    world_state * state;

public:
    gui(world_state * render_state);
    ~gui();
    void queue_render();
    void start();
};

#endif
