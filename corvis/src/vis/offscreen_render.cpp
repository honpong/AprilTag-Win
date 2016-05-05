#include "offscreen_render.h"
#include "world_state_render.h"

#include "lodepng.h"

#include "gl_util.h"

static int width = 512;
static int height = 512;

/* Projection matrix */
static GLfloat projection_matrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

/* Model view matrix */
static GLfloat modelview_matrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

void configure_view(float min[3], float max[3])
{
    glViewport(0, 0, width, height);
    float aspect = 1.f*width/height;
    build_orthographic_projection_matrix(projection_matrix, aspect, min, max);
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

bool offscreen_render_to_file(const char * filename, world_state * world)
{
    world->update_vertex_arrays();
    fprintf(stderr, "Rendering %d path nodes and %d features\n", world->path_vertex_num, world->feature_vertex_num);


    if (!glfwInit()) {
        fprintf(stderr, "init failed\n");
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

    glfwSetErrorCallback(error_callback);
    GLFWwindow* window = glfwCreateWindow(width, height, "Offscreen Render", NULL, NULL);
    if (!window)
    {
        fprintf(stderr, "Failed to create a window\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    //fprintf(stderr, "OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment
    glEnable(GL_DEPTH_TEST);

    world_state_render_init();

    // Draw call
    float min[3], max[3];
    float border_factor = 1.1;
    world->get_bounding_box(min, max);
    for(int i = 0; i < 3; i++) {
        min[i] *= border_factor;
        max[i] *= border_factor;
    }
    configure_view(min, max);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0, 0, 0);                   // background color

    world_state_render(world, modelview_matrix, projection_matrix);
    //Don't swap the buffers since we want to read from the buffer
    //glfwSwapBuffers(window);
    glfwPollEvents();

    // Extract the pixels
    std::vector<std::uint8_t> data(width*height*4);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,&data[0]);

    // The buffer is y-flipped because the origin of glReadPixels is
    // the lower left corner of the scene
    for(int y = 0; y < height/2; y++) {
        for(int x = 0; x < width; x++) {
            for(int c = 0; c < 4; c++) {
                unsigned p1 = y * width * 4 + x * 4 + c;
                unsigned p2 = (height-y-1) * width * 4 + x * 4 + c;
                uint8_t temp = data[p1];
                data[p1] = data[p2];
                data[p2] = temp;
            }
        }
    }

    //Encode the image
    unsigned error = lodepng::encode(filename, data, width, height);

    // If there's an error, display it
    if(error)
        fprintf(stderr, "%s: encoder error %d: %s\n", filename, error, lodepng_error_text(error));

    glfwDestroyWindow(window);
    glfwTerminate();

    return !error;
}
