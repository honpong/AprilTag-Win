#include "offscreen_render.h"
#include "world_state_render.h"

#include "lodepng.h"

#include "platform_gl.h"

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


static void build_projection_matrix(float * projMatrix, float fov, float ratio, float nearP, float farP)
{
    float f = 1.0f / tan (fov * (M_PI / 360.0));

    for(int i = 0; i < 16; i++) projMatrix[i] = 0;
    projMatrix[0] = 1;
    projMatrix[5] = 1;
    projMatrix[10] = 1;
    projMatrix[15] = 1;

    projMatrix[0] = f / ratio;
    projMatrix[1 * 4 + 1] = f;
    projMatrix[2 * 4 + 2] = (farP + nearP) / (nearP - farP);
    projMatrix[3 * 4 + 2] = (2.0f * farP * nearP) / (nearP - farP);
    projMatrix[2 * 4 + 3] = -1.0f;
    projMatrix[3 * 4 + 3] = 0.0f;
}

void configure_view()
{
    glViewport(0, 0, width, height);
    float aspect = 1.f*width/height;
    build_projection_matrix(projection_matrix, 60.0f, aspect, 1.0f, 30.0f);

    /* Set the camera position */
    modelview_matrix[12]  = 0.0f;
    modelview_matrix[13]  = 0.0f;
    modelview_matrix[14]  = -5.0f;
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
    GLFWwindow* window = glfwCreateWindow(512, 512, "Offscreen Render", NULL, NULL);
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
    configure_view();
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
        fprintf(stderr, "encoder error %d: %s\n", error, lodepng_error_text(error));

    glfwDestroyWindow(window);
    glfwTerminate();

    return true;
}
