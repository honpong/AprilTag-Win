#include "gui.h"
gui * gui::static_gui;

#include "offscreen_render.h"
#include "world_state_render.h"

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GLUT/glut.h>
#endif

void gui::configure_view()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Note miny and maxy and minz and maxz are reversed to get the
    // deisred viewpoint. TODO: set up an actual projection matrix
    // here
    glOrtho(-2, 2, 2, -2, 10, -10);

    glGetFloatv(GL_PROJECTION_MATRIX, _modelViewProjectionMatrix);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void gui::init_gl()
{
    glShadeModel(GL_SMOOTH);                    // shading mathod: GL_SMOOTH or GL_FLAT
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment

    // enable /disable features
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);

     // track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glClearColor(0, 0, 0, 0);                   // background color
    glClearStencil(0);                          // clear stencil buffer
    glClearDepth(1.0f);                         // 0 is near, 1 is far
    glDepthFunc(GL_LEQUAL);
}

void gui::render()
{
    state->update_vertex_arrays();

    // Draw call
    configure_view();
    world_state_render(state, _modelViewProjectionMatrix, _normalMatrix);
    glutSwapBuffers();
    return;
}

void gui::init_glut()
{
    int argc = 0;
    char ** argv = NULL;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);   // display mode
    glutInitWindowSize(512, 512);              // window size
    glutInitWindowPosition(100, 100);                           // window location
    window_id = glutCreateWindow("Replay");     // param is the title of window
    // Configure glut callbacks
    glutDisplayFunc(gui::render_callback);
    glutIdleFunc(gui::render_callback);
}

void gui::start()
{
    init_glut();
    init_gl();
    world_state_render_init();
    glutMainLoop();
}

gui::gui(world_state * world) : state(world)
{
    static_gui = this;
}

gui::~gui()
{
    world_state_render_teardown();
}

void gui::queue_render()
{
    glutSetWindow(window_id);
    glutPostRedisplay();
}
