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

const static float initial_scale = 5;

void gui::configure_view()
{
    float aspect = 1.f*width/height;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluPerspective(65, aspect, 0.1f, 100.0f);
    glTranslatef(0, 0, -scale);
    rotation_vector r = to_rotation_vector(arc.get_quaternion());
    //glScalef(scale, scale, scale);
    v4 axis = v4(r.x(), r.y(), r.z(), 0);
    float angle = 0;
    if(axis.norm() != 0) {
        angle = axis.norm()*180/M_PI;
        axis = axis / axis.norm();
    }
    glRotatef(angle, axis[0], axis[1], axis[2]);
    glGetFloatv(GL_MODELVIEW, _modelViewProjectionMatrix);
}

void gui::reshape(int w, int h)
{
    width = w;
    height = h;
    glViewport(0, 0, width, height);
}

void gui::mouse_move(int x, int y)
{
    if(is_rotating)
        arc.continue_rotation(x, y);
}

void gui::mouse(int button, int state, int x, int y)
{
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        arc.start_rotation(x, y);
        is_rotating = true;
    }
    else if(button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        is_rotating = false;
    }
}
void gui::keyboard(unsigned char key, int x, int y)
{
    if(key == '0')
        scale = initial_scale;
    if(key == '+' || key == '=' || key == 'w')
        scale *= 1/1.1;
    if(key == '-' || key == 's')
        scale *= 1.1;
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
    glutInitWindowSize(width, height);              // window size
    window_id = glutCreateWindow("Replay");     // param is the title of window
    // Configure glut callbacks
    glutDisplayFunc(gui::render_callback);
    glutIdleFunc(gui::render_callback);
    glutReshapeFunc(gui::reshape_callback);
    glutMouseFunc(gui::mouse_callback);
    glutMotionFunc(gui::move_callback);
    glutKeyboardFunc(gui::keyboard_callback);
}

void gui::start()
{
    arc.reset();
    init_glut();
    init_gl();
    world_state_render_init();
    glutMainLoop();
}

gui::gui(world_state * world) : state(world), scale(initial_scale), width(512), height(512)
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
