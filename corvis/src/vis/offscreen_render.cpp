#include "offscreen_render.h"
#include "world_state_render.h"

#include "lodepng.h"

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GLUT/glut.h>
#endif

float _modelViewProjectionMatrix[16]; // 4x4
float _normalMatrix[9]; // 3x3

void configure_view()
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

void init_gl()
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

void init_glut()
{
    // TODO: Replace glut with whatever fundamentals actually are
    // needed for offscreen rendering
    int argc = 0;
    char ** argv = NULL;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);   // display mode
    glutInitWindowSize(512, 512);              // window size
    glutInitWindowPosition(100, 100);                           // window location
    glutCreateWindow("Title");     // param is the title of window
}

bool offscreen_render_to_file(const char * filename, world_state * world)
{
    world->update_vertex_arrays();
    fprintf(stderr, "Rendering %d path nodes and %d features\n", world->path_vertex_num, world->feature_vertex_num);

    int width = 512;
    int height = 512;

    init_glut();
    init_gl();

    GLuint framebuffer, renderbuffer;
    GLenum status;

    //Set up a FBO with one renderbuffer attachment
    glGenFramebuffersEXT(1, &framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
    glGenRenderbuffersEXT(1, &renderbuffer);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, width, height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                     GL_RENDERBUFFER_EXT, renderbuffer);
    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        // Handle errors
        fprintf(stderr, "status %d\n", status);

    world_state_render_init();

    // Draw call
    configure_view();
    world_state_render(world, _modelViewProjectionMatrix, _normalMatrix);

    // Extract the pixels
    std::vector<std::uint8_t> data(width*height*4);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,&data[0]);

    // Make the window the target
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    // Delete the renderbuffer attachment
    glDeleteRenderbuffersEXT(1, &renderbuffer);

    //Encode the image
    unsigned error = lodepng::encode(filename, data, width, height);

    // If there's an error, display it
    if(error)
        fprintf(stderr, "encoder error %d: %s\n", error, lodepng_error_text(error));

    return true;
}
