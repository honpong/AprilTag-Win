#ifndef WIN32
#include <alloca.h>
#endif
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
    if(scale > farclip) farclip = scale*1.5f;
    if(scale < nearclip) nearclip = scale*0.75f;
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
        arc.continue_rotation((float)x, (float)y);
}

void gui::mouse(GLFWwindow * window, int button, int action, int mods)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        arc.start_rotation((float)x, (float)y);
        is_rotating = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        is_rotating = false;
    }
}

void gui::scroll(GLFWwindow * window, double xoffset, double yoffset)
{
    scale *= (1 + (float)yoffset*.05f);
}

#include "lodepng.h"
void gui::write_frame()
{
    state->image_lock.lock();
    const int W = state->last_image.width;
    const int H = state->last_image.height;
    auto image = (uint8_t*)alloca(W*H*4);
    for(int i = 0; i < W*H; i++) {
        image[i*4 + 0] = state->last_image.image[i];
        image[i*4 + 1] = state->last_image.image[i];
        image[i*4 + 2] = state->last_image.image[i];
        image[i*4 + 3] = 255;
    }

    state->image_lock.unlock();

    //Encode the image
    unsigned error = lodepng::encode("last_frame.png", image, W, H);
    if(error)
        fprintf(stderr, "encoder error %d: %s\n", error, lodepng_error_text(error));
}

void gui::keyboard(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if(key == GLFW_KEY_0 && action == GLFW_PRESS)
        scale = initial_scale;
    if(key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
        scale *= 1/1.1f;
    if(key == GLFW_KEY_MINUS && action == GLFW_PRESS)
        scale *= 1.1f;
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS && replay_control)
       replay_control->toggle_pause();
    if(key == GLFW_KEY_S && action == GLFW_PRESS && replay_control)
       replay_control->step();
    if(key == GLFW_KEY_F && action == GLFW_PRESS)
       write_frame();
    if(key == GLFW_KEY_N && action == GLFW_PRESS)
       current_plot = state->next_plot(current_plot);
}

void gui::render_plot(int plots_width, int plots_height)
{
    glViewport(0, 0, plots_width, plots_height);
    world_state_render_plot(state, current_plot, plots_width, plots_height);
}

void gui::render_video(int video_width, int video_height)
{
    glViewport(0, 0, video_width, video_height);
    world_state_render_video(state, video_width, video_height);
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

    glfwWindowHint(GLFW_RESIZABLE, true);
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

    glfwWindowHint(GLFW_RESIZABLE, true);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* plots_window = glfwCreateWindow(600, 400, "Replay Plots", NULL, NULL);
    glfwSetKeyCallback(plots_window, gui::keyboard_callback);
    if (!plots_window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetWindowPos(plots_window, pos_x + width + 20, pos_y + height + 20);
    if(show_plots)
        glfwShowWindow(plots_window);
    glfwMakeContextCurrent(plots_window);
    glfwSwapInterval(1);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    world_state_render_plot_init();

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
            int video_width, video_height;
            glfwGetFramebufferSize(video_window, &video_width, &video_height);
            render_video(video_width, video_height);
            glfwSwapBuffers(video_window);
        }
        glfwPollEvents();

        if(show_plots) {
            glfwMakeContextCurrent(plots_window);
            int plots_width, plots_height;
            glfwGetFramebufferSize(plots_window, &plots_width, &plots_height);
            render_plot(plots_width, plots_height);
            glfwSwapBuffers(plots_window);
        }
        glfwPollEvents();
    }
    glfwDestroyWindow(main_window);
    glfwDestroyWindow(video_window);
    glfwDestroyWindow(plots_window);
    glfwTerminate();
    std::cerr << "GUI closed, replay still running in realtime (press Ctrl+c to quit)\n";
}


void gui::start(replay * rp)
{
    replay_control = rp;
    arc.reset();
    start_glfw();
}

gui::gui(world_state * world, bool show_main_, bool show_video_, bool show_plots_)
    : state(world), scale(initial_scale), width(512), height(512),
      show_main(show_main_), show_video(show_video_), show_plots(show_plots_)
{
    static_gui = this;
}

gui::~gui()
{
}
