#include "offscreen_render.h"
#include "world_state_render.h"

#include "lodepng.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
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
    if(!GLAD_GL_VERSION_3_0) {
        fprintf(stderr, "OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);
        fprintf(stderr, "don't support 3.0\n");
        exit(EXIT_FAILURE);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment
    glEnable(GL_DEPTH_TEST);

    GLuint framebuffer, renderbuffer;
    GLenum status;

    //Set up a FBO with one renderbuffer attachment
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenRenderbuffers(1, &renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                     GL_RENDERBUFFER, renderbuffer);
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        // Handle errors
        fprintf(stderr, "status %d\n", status);

    world_state_render_init();

    // Draw call
    configure_view();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0, 0, 0);                   // background color

    world_state_render(world, modelview_matrix, projection_matrix);
    glfwSwapBuffers(window);
    glfwPollEvents();

    // Extract the pixels
    std::vector<std::uint8_t> data(width*height*4);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,&data[0]);

    // Make the window the target
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Delete the renderbuffer attachment
    glDeleteRenderbuffers(1, &renderbuffer);

    //Encode the image
    unsigned error = lodepng::encode(filename, data, width, height);

    // If there's an error, display it
    if(error)
        fprintf(stderr, "encoder error %d: %s\n", error, lodepng_error_text(error));

    glfwDestroyWindow(window);
    glfwTerminate();

    return true;
}
