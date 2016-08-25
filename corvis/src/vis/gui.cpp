#ifndef WIN32
#include <alloca.h>
#endif
#include <limits>
#include <thread>
#include <chrono>
#include "gui.h"
gui * gui::static_gui;

#include "world_state_render.h"
#include "../filter/replay.h"
#include "gl_util.h"

const static float initial_scale = 5;

v3 gui::get_view_translation()
{
    v3 d_view = (scale/600.)*(translation_finish - translation_start);
    d_view[1] *= -1; // switch from image coordinates to opengl coordinates
    return d_view;
}

void gui::configure_view(int view_width, int view_height)
{
    float aspect = 1.f*view_width/view_height;
    float nearclip = 0.1f;
    float farclip = 30.f;
    if(scale > farclip) farclip = scale*1.5f;
    if(scale < nearclip) nearclip = scale*0.75f;
    build_projection_matrix(projection_matrix, 60.0f, aspect, nearclip, farclip);

    view_matrix.block<3,3>(0,0) = arc.get_quaternion().cast<float>().toRotationMatrix();
    view_matrix.block<3,1>(0,3) = v3(0,0,-scale) + translation_m;
    if(is_translating)
        view_matrix.block<3,1>(0,3) += get_view_translation();
    view_matrix.block<1,3>(3,0) = v3::Zero();
    view_matrix(3,3) = 1;
}

void gui::mouse_move(GLFWwindow * window, double x, double y)
{
    if(is_rotating)
        arc.continue_rotation((float)x, (float)y);
    if(is_translating)
        translation_finish = v3((float)x, (float)y, 0);
    if(is_rotating || is_translating)
        dirty = true;
}

void gui::mouse(GLFWwindow * window, int button, int action, int mods)
{
    dirty = true;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        arc.start_rotation((float)x, (float)y);
        is_rotating = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        is_rotating = false;
    }

    if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        translation_start = v3((float)x, (float)y, 0);
        translation_finish = translation_start;
        is_translating = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        translation_finish = v3((float)x, (float)y, 0);
        translation_m += get_view_translation();
        is_translating = false;
    }
}

void gui::scroll(GLFWwindow * window, double xoffset, double yoffset)
{
    dirty = true;
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
    dirty = true;
    if (action == GLFW_PRESS) {
        switch (key) {
             break; case GLFW_KEY_0: case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3:
                    case GLFW_KEY_4: case GLFW_KEY_5: case GLFW_KEY_6: case GLFW_KEY_7:
                    case GLFW_KEY_8:
                    case GLFW_KEY_9:        current_plot_key = (size_t)-1; current_plot = state->change_plot(key - GLFW_KEY_0);
             break; case GLFW_KEY_N:        current_plot_key = (size_t)-1; current_plot = state->change_plot(current_plot + ((mods & GLFW_MOD_SHIFT) ? -1 : 1));
             break; case GLFW_KEY_LEFT:     current_plot_key = state->change_plot_key(current_plot, current_plot_key - 1);
             break; case GLFW_KEY_RIGHT:    current_plot_key = state->change_plot_key(current_plot, current_plot_key + 1);
             break; case GLFW_KEY_ESCAPE:   scale = initial_scale; current_plot_key = (size_t)-1;
             break; case GLFW_KEY_EQUAL:    scale *= 1/1.1f;
             break; case GLFW_KEY_MINUS:    scale *= 1.1f;
             break; case GLFW_KEY_SPACE:    if (replay_control) replay_control->toggle_pause();
             break; case GLFW_KEY_S:        if (replay_control) replay_control->step();
             break; case GLFW_KEY_Q:        if (replay_control) replay_control->stop(); quit = true;
             break; case GLFW_KEY_F:        write_frame();
             break; case GLFW_KEY_V:        show_video = !show_video;
             break; case GLFW_KEY_D:        show_depth = !show_depth;
             break; case GLFW_KEY_M:        show_main = !show_main;
             break; case GLFW_KEY_P:        show_plots = !show_plots;
             break; case GLFW_KEY_O:        show_depth_on_video = !show_depth_on_video;
             break; case GLFW_KEY_T:        fprintf(stderr, "Current time: %" PRIu64 "\n", state->get_current_timestamp());
             break; case GLFW_KEY_SLASH:    fprintf(stderr, R"(
0-9   Switch Plots
n     Next Plot
N     Prev Plot
->    Next (or all) individual plot lines
<-    Prev (or all) individual plot lines
ESC   Show all plot lines

+     Zoom in
=     Zoom out
SPC   Pause
s     Step

v     Toggle Video
D     Toggle Depth
m     Toggle (Main) 3D path view
p     Toggle Plots
o     Toggle Depth to Video overlay

t     Print current timestamp

q     Quit
?     Help (this)
)");
        }
    }
}

void gui::render(int view_width, int view_height)
{
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
    glfwWindowHint(GLFW_VISIBLE, true);
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
    world_state_render_video_init();
    world_state_render_depth_init();
    world_state_render_plot_init();

    //fprintf(stderr, "OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);

    int last_width = width;
    int last_height = height;
    while (!glfwWindowShouldClose(main_window) && !quit)
    {
        glfwGetFramebufferSize(main_window, &width, &height);
        if(width != last_width || height != last_height)
            dirty = true;
        last_width = width;
        last_height = height;

        // Calculate layout
        int main_width = width, main_height = height;
        int video_width = 0, video_height = 0;
        int depth_width = 0, depth_height = 0;
        int plots_width = 0, plots_height = 0;
        int video_frame_width = 0, video_frame_height = 0;
        int depth_frame_width = 0, depth_frame_height = 0;

        bool show_video = this->show_video && world_state_render_video_get_size(state, &video_frame_width, &video_frame_height);
        bool show_depth = this->show_depth && world_state_render_depth_get_size(state, &depth_frame_width, &depth_frame_height);

        float right_column_percent = show_main ? .5f : 1.f;
        float video_ratio = show_video ? 1.f : 0.f;
        float plots_ratio = show_plots ? 1.f : 0.f;
        float depth_ratio = show_depth ? 1.f : 0.f;

        float total = video_ratio + plots_ratio + depth_ratio + FLT_EPSILON;
        float video_height_percent = video_ratio / total;
        float depth_height_percent = depth_ratio / total;
        float plots_height_percent = 1.f - video_height_percent - depth_height_percent;

        if(show_video) {
            video_width = lroundf(width*right_column_percent);
            video_height = lroundf(height*video_height_percent);
            if(1.*video_height/video_width > 1.f*video_frame_height/video_frame_width)
                video_height = lroundf(video_width *  1.f*video_frame_height/video_frame_width);
            else
                video_width = lroundf(video_height * 1.f*video_frame_width/video_frame_height);
        }
        if(show_depth) {
            depth_width = lroundf(width*right_column_percent);
            depth_height = lroundf(height*depth_height_percent);
            if(1.*depth_height/depth_width > 1.f*depth_frame_height/depth_frame_width)
                depth_height = lroundf(depth_width *  1.f*depth_frame_height/depth_frame_width);
            else
                depth_width = lroundf(depth_height * 1.f*depth_frame_width/depth_frame_height);
        }

        if(show_plots) {
            plots_width = lroundf(width*right_column_percent);
            plots_height = height - video_height - depth_height;
            if(show_video)
                plots_width = video_width;
            if(!show_video && show_main)
                plots_height = lroundf(height*plots_height_percent);
        }
        if(show_main && (show_video || show_plots || show_depth))
            main_width = width - max(max(video_width, plots_width), depth_width);

        state->generate_depth_overlay = show_depth_on_video;

        // Update data
        bool updated = state->update_vertex_arrays();

        // Draw
        if(updated || dirty) {
            glViewport(0, 0, width, height);
            glEnable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            if(show_main) {
                glViewport(0, 0, main_width, main_height);
                configure_view(main_width, main_height);
                world_state_render(state, view_matrix.data(), projection_matrix);
            }
            if(show_video) {
                // y coordinate is 0 = bottom, height = top (opengl)
                glViewport(width - video_width, 0, video_width, video_height);
                world_state_render_video(state, video_width, video_height);
            }
            if(show_depth) {
                // y coordinate is 0 = bottom, height = top (opengl)
                glViewport(width - depth_width, video_height, depth_width, depth_height);

                if (show_depth_on_video)
                    world_state_render_depth_on_video(state, depth_width, depth_height);
                else
                    world_state_render_depth(state, depth_width, depth_height);
            }
            if(show_plots) {
                // y coordinate is 0 = bottom, height = top (opengl)
                glViewport(width - plots_width, video_height + depth_height, plots_width, plots_height);
                world_state_render_plot(state, current_plot, current_plot_key, plots_width, plots_height);
            }
            glfwSwapBuffers(main_window);
            dirty = false;
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        glfwPollEvents();
    }
    glfwDestroyWindow(main_window);
    glfwTerminate();
}


void gui::start(replay * rp)
{
    replay_control = rp;
    arc.reset();
    start_glfw();
}

gui::gui(world_state * world, bool show_main_, bool show_video_, bool show_depth_, bool show_plots_)
    : state(world), scale(initial_scale), width(640), height(480),
      show_main(show_main_), show_video(show_video_), show_depth(show_depth_), show_plots(show_plots_),
      show_depth_on_video(false)
{
    static_gui = this;
}

gui::~gui()
{
}
