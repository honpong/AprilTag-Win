#include <limits>
#include "gui.h"
gui * gui::static_gui;

#include "offscreen_render.h"
#include "world_state_render.h"
#include "../filter/replay.h"

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

/*#include "lodepng.h"
#include <mgl2/mgl.h>
void gui::create_plots()
{
    mglGraph gr(0,600,400); // 600x400 graph, plotted to an image
    int width = gr.GetWidth();
    int height = gr.GetHeight();
    unsigned char buffer[width*height*4];
    unsigned char * buf = buffer;

    // mglData stores the x and y data to be plotted
    state->render_plots([&] (world_state::plot &plot) {
        gr.NewFrame();
        gr.Alpha(false);
        gr.Clf('w');
        gr.Box();
        float minx = std::numeric_limits<float>::max(), maxx = std::numeric_limits<float>::min();
        float miny = std::numeric_limits<float>::max(), maxy = std::numeric_limits<float>::min();
        std::string names;
        const char *colors[] = {"r","g","b"}; int i=0;

        for (auto &kv : plot) {
            const std::string &name = kv.first; const plot_data &p = kv.second;
            names += (names.size() ? "-" : "") + name;

            mglData data_x(p.size());
            mglData data_y(p.size());
            auto last = sensor_clock::tp_to_micros(p.back().first);

            int j = 0;
            for(auto data : p) {
                float seconds = -1.*(last - sensor_clock::tp_to_micros(data.first))/1e6;
                if(seconds < minx) minx = seconds;
                if(seconds > maxx) maxx = seconds;
                data_x.a[j] = seconds;

                float val = data.second;
                if(val < miny) miny = val;
                if(val > maxy) maxy = val;
                data_y.a[j++] = val;
            }

            gr.SetRange('x', minx, maxx);
            gr.SetRange('y', miny, maxy);
            gr.Plot(data_x, data_y, colors[i++%3]);
        }


        gr.Axis();
        gr.EndFrame();
        std::string filename = names + ".png";
        gr.GetRGBA((char *)buf, width*height*4);

        //Encode the image
        unsigned error = lodepng::encode(filename.c_str(), buf, width, height);
        if(error)
            fprintf(stderr, "encoder error %d: %s\n", error, lodepng_error_text(error));
    });
}*/

void gui::keyboard(unsigned char key, int x, int y)
{
    if(key == '0')
        scale = initial_scale;
    if(key == '+' || key == '=')
        scale *= 1/1.1;
    if(key == '-')
        scale *= 1.1;
    if(key == ' ')
       replay_control->toggle_pause();
    if(key == 's')
       replay_control->step();
    //if(key == 'p')
     //   create_plots();
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

void gui::start(replay * rp)
{
    replay_control = rp;
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
