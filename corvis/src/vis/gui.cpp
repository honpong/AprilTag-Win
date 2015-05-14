#include <limits>
#include "gui.h"
gui * gui::static_gui;

#include "world_state_render.h"
#include "../filter/replay.h"
#include "gl_util.h"

const static float initial_scale = 5;

void gui::configure_view()
{
    float aspect = 1.f*width/height;
    float nearclip = 0.1f;
    float farclip = 30.f;
    if(scale > farclip) farclip = scale*1.5;
    if(scale < nearclip) nearclip = scale*0.75;
    build_projection_matrix(_projectionMatrix, 60.0f, aspect, nearclip, farclip);

    m4 R = to_rotation_matrix(arc.get_quaternion());
    R(2, 3) = -scale; // Translate by -scale
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            _modelViewMatrix[j * 4 + i] = (float)R(i, j);
        }
    }
}

void gui::mouse_move(GLFWwindow * window, double x, double y)
{
    if(is_rotating)
        arc.continue_rotation(x, y);
}

void gui::mouse(GLFWwindow * window, int button, int action, int mods)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        arc.start_rotation(x, y);
        is_rotating = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        is_rotating = false;
    }
}

void gui::scroll(GLFWwindow * window, double xoffset, double yoffset)
{
    scale *= (1+yoffset*.05);
}

#ifdef WIN32
void gui::create_plots() {}
#else // !WIN32

#include "lodepng.h"
#define _MSC_VER 1900 // Force mathgl to avoid C99's typeof
#include <mgl2/mgl.h>
#undef _MSC_VER

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
            auto first = sensor_clock::tp_to_micros(p.front().first);

            int j = 0;
            for(auto data : p) {
                float seconds = (sensor_clock::tp_to_micros(data.first) - first)/1e6;
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
}
#endif //WIN32

#include "lodepng.h"
void gui::write_frame()
{
    state->image_lock.lock();
    const int W = 640;
    const int H = 480;
    uint8_t image[W*H*4];
    for(int i = 0; i < W*H; i++) {
        image[i*4 + 0] = state->last_image.image[i];
        image[i*4 + 1] = state->last_image.image[i];
        image[i*4 + 2] = state->last_image.image[i];
        image[i*4 + 3] = 255;
    }

    state->image_lock.unlock();

    //Encode the image
    unsigned error = lodepng::encode("last_frame.png", image, 640, 480);
    if(error)
        fprintf(stderr, "encoder error %d: %s\n", error, lodepng_error_text(error));
}

void gui::keyboard(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if(key == GLFW_KEY_0 && action == GLFW_PRESS)
        scale = initial_scale;
    if(key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
        scale *= 1/1.1;
    if(key == GLFW_KEY_MINUS && action == GLFW_PRESS)
        scale *= 1.1;
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
       replay_control->toggle_pause();
    if(key == GLFW_KEY_S && action == GLFW_PRESS)
       replay_control->step();
    if(key == GLFW_KEY_P && action == GLFW_PRESS)
       create_plots();
    if(key == GLFW_KEY_F && action == GLFW_PRESS)
       write_frame();
}

void gui::render_video()
{
    glViewport(0, 0, 640, 480);
    world_state_render_video(state);
}

void gui::render()
{
    glViewport(0, 0, width, height);
    state->update_vertex_arrays();

    // Draw call
    configure_view();
    world_state_render(state, _modelViewMatrix, _projectionMatrix);
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

void gui::start_glfw()
{
    if (!glfwInit())
        exit(EXIT_FAILURE);

    int pos_x = 50;
    int pos_y = 50;

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_RESIZABLE, true);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* main_window = glfwCreateWindow(width, height, "Replay", NULL, NULL);
    if (!main_window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetWindowPos(main_window, pos_x, pos_y);
    if(show_main)
        glfwShowWindow(main_window);
    glfwMakeContextCurrent(main_window);
    glfwSwapInterval(1);

    glfwSetKeyCallback(main_window, gui::keyboard_callback);
    glfwSetInputMode(main_window, GLFW_STICKY_MOUSE_BUTTONS, 1);
    glfwSetMouseButtonCallback(main_window, gui::mouse_callback);
    glfwSetCursorPosCallback(main_window, gui::move_callback);
    glfwSetScrollCallback(main_window, gui::scroll_callback);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    world_state_render_init();

    glfwWindowHint(GLFW_RESIZABLE, false);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* video_window = glfwCreateWindow(640, 480, "Replay Video", NULL, NULL);
    glfwSetKeyCallback(video_window, gui::keyboard_callback);
    if (!video_window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetWindowPos(video_window, pos_x + width + 20, pos_y);
    if(show_video)
        glfwShowWindow(video_window);
    glfwMakeContextCurrent(video_window);
    glfwSwapInterval(1);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    world_state_render_video_init();

    //fprintf(stderr, "OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);

    while (!glfwWindowShouldClose(main_window))
    {
        if(show_main) {
            glfwMakeContextCurrent(main_window);
            glfwGetFramebufferSize(main_window, &width, &height);
            render();
            glfwSwapBuffers(main_window);
        }
        glfwPollEvents();
 
        if(show_video) {
            glfwMakeContextCurrent(video_window);
            render_video();
            glfwSwapBuffers(video_window);
        }
        glfwPollEvents();
    }
    glfwDestroyWindow(main_window);
    glfwDestroyWindow(video_window);
    glfwDestroyWindow(plots_window);
    glfwTerminate();
}


void gui::start(replay * rp)
{
    replay_control = rp;
    arc.reset();
    start_glfw();
}

gui::gui(world_state * world) : state(world), scale(initial_scale), width(512), height(512)
{
    static_gui = this;
}

gui::~gui()
{
    world_state_render_teardown();
}
