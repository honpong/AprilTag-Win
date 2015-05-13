#include "world_state_render.h"

#include <stdio.h>
#include <stdlib.h>

#include "platform_gl.h"
#include "shaders.h"

static GLuint program;
static GLuint vertexLoc, colorLoc;
static GLuint projMatrixLoc, viewMatrixLoc;

typedef struct _draw_item {
    GLuint vertex_buffer_object[2];
} draw_item;

static draw_item grid_di, axis_di, feature_di, path_di, orientation_di;

const char * vs =
"#version 120\n"
""
"uniform mat4 viewMatrix, projMatrix;\n"
" "
"attribute vec3 position;\n"
"attribute vec4 color;\n"
"varying vec4 color_fs;\n"
" "
"void main()\n"
"{\n"
"    color_fs = color;\n"
"    gl_Position = projMatrix * viewMatrix * vec4(position, 1);\n"
//"    gl_Position = vec4(position[0], position[1], 0, 1.0);\n"
"    gl_PointSize = 8.;\n"
"}\n";

const char * fs = ""
"#version 120\n"
" "
"varying vec4 color_fs;\n"
"varying vec4 color_out;\n"
" "
"void main()"
"{\n"
"    gl_FragColor = color_fs;\n"
"}";

const char * vs_es =
"uniform mat4 viewMatrix, projMatrix;\n"
" "
"attribute vec3 position;\n"
"attribute vec4 color;\n"
"varying vec4 color_fs;\n"
" "
"void main()\n"
"{\n"
"    color_fs = color;\n"
"    gl_Position = projMatrix * viewMatrix * vec4(position, 1);\n"
"    gl_PointSize = 8.;\n"
"}\n";

const char * fs_es =
"varying lowp vec4 color_fs;\n"
"void main()\n"
"{\n"
"    gl_FragColor = color_fs;\n"
"}";


GLuint setupShaders() {

    GLuint p,v,f;

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);

#if TARGET_OS_IPHONE
    glShaderSource(v, 1, &vs_es,NULL);
    glShaderSource(f, 1, &fs_es,NULL);
#else
    glShaderSource(v, 1, &vs,NULL);
    glShaderSource(f, 1, &fs,NULL);
#endif

    glCompileShader(v);
    glCompileShader(f);

    printShaderInfoLog(v);
    printShaderInfoLog(f);

    p = glCreateProgram();
    glAttachShader(p,v);
    glAttachShader(p,f);

    glLinkProgram(p);
    printProgramInfoLog(p);

    vertexLoc = glGetAttribLocation(p,"position");
    colorLoc = glGetAttribLocation(p, "color");

    projMatrixLoc = glGetUniformLocation(p, "projMatrix");
    viewMatrixLoc = glGetUniformLocation(p, "viewMatrix");

    return(p);
}

static inline void setup_draw_item(draw_item & item)
{
    glGenBuffers(2, item.vertex_buffer_object);
}

void setup_world()
{
    setup_draw_item(grid_di);
    setup_draw_item(axis_di);
    setup_draw_item(feature_di);
    setup_draw_item(path_di);
    setup_draw_item(orientation_di);
}

static inline void draw_buffer(draw_item & item, VertexData * data, int number, int gl_type)
{
    glBindBuffer(GL_ARRAY_BUFFER, item.vertex_buffer_object[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData)*number, &data[0].position, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(vertexLoc);
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, 0, sizeof(VertexData), 0);

    glBindBuffer(GL_ARRAY_BUFFER, item.vertex_buffer_object[1]);
    //TODO: is this copying too much data?
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData)*number - sizeof(data[0].position), &data[0].color, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(colorLoc);
    glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), 0);

    glDrawArrays(gl_type, 0, number);
}

bool world_state_render_init()
{
    program = setupShaders();
    glEnable(GL_DEPTH_TEST);

    setup_world();
    return true;
}

void world_state_render_teardown()
{
}

void world_state_render(world_state * world, float * viewMatrix, float * projMatrix)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);

    // must be called after glUseProgram
    glUniformMatrix4fv(projMatrixLoc,  1, false, projMatrix);
    glUniformMatrix4fv(viewMatrixLoc,  1, false, viewMatrix);

    world->display_lock.lock();

#if !(TARGET_OS_IPHONE)
    glPointSize(3.0f);
#endif
    glLineWidth(2.0f);
    draw_buffer(grid_di, world->grid_vertex, world->grid_vertex_num, GL_LINES);
    glLineWidth(4.0f);
    draw_buffer(axis_di, world->axis_vertex, world->axis_vertex_num, GL_LINES);
    draw_buffer(orientation_di, world->orientation_vertex, world->orientation_vertex_num, GL_LINES);
    draw_buffer(feature_di, world->feature_vertex, world->feature_vertex_num, GL_POINTS);
    draw_buffer(path_di, world->path_vertex, world->path_vertex_num, GL_POINTS);

    world->display_lock.unlock();
}
