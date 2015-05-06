#include "world_state_render.h"

#if TARGET_OS_IPHONE
#include <OpenGLES/ES2/gl.h>
#elif TARGET_OS_MAC
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

int GLKVertexAttribPosition = 0;
int GLKVertexAttribColor = 2;
int GLKVertexAttribNormal = 4;

GLuint _vertexArray;
GLuint _vertexBuffer;
GLuint _program;

// Uniform index for Shader.vsh
enum
{
    UNIFORM_MODELVIEWPROJECTION_MATRIX,
    UNIFORM_NORMAL_MATRIX,
    NUM_UNIFORMS
};
GLint uniforms[NUM_UNIFORMS];

const char * fragment_shader_source = ""
"#ifndef GL_ES\n"
"#define lowp \n"
"#endif\n"

"varying lowp vec4 colorVarying;\n"
"void main()\n"
"{\n"
"    gl_FragColor = colorVarying;\n"
"}";

const char * vertex_shader_source = ""
"#ifndef GL_ES\n"
"#define lowp \n"
"#endif\n"

"attribute vec4 position;\n"
"attribute vec4 color;\n"
"attribute vec3 normal;\n"

"varying lowp vec4 colorVarying;\n"

"uniform mat4 modelViewProjectionMatrix;\n"
"uniform mat3 normalMatrix;\n"

"void main()\n"
"{\n"
"    vec3 eyeNormal = normalize(normalMatrix * normal);\n"
"    /*\n"
"    vec3 lightPosition = vec3(0.0, 0.0, 1.0);\n"
"    vec4 diffuseColor = vec4(1.0, 1.0, 1.0, 1.0);\n"

"    float nDotVP = max(0.0, dot(eyeNormal, normalize(lightPosition)));\n"

"    colorVarying = diffuseColor * nDotVP;\n"
"    */\n"
"    colorVarying = color;\n"

"    gl_Position = modelViewProjectionMatrix * position;\n"
"    gl_PointSize = 8.;\n"
"}";

bool compile_shader(GLuint * shader, GLenum type, const char * source)
{
    GLint status;

    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        fprintf(stderr, "Shader compile log:\n%s", log);
        free(log);
    }

    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        glDeleteShader(*shader);
        return false;
    }

    return true;
}

bool link_program(GLuint prog)
{
    GLint status;
    glLinkProgram(prog);

    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        fprintf(stderr, "Program link log:\n%s", log);
        free(log);
    }

    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0) {
        return false;
    }

    return true;
}

bool validate_program(GLuint prog)
{
    GLint logLength, status;

    glValidateProgram(prog);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        fprintf(stderr, "Program validate log:\n%s", log);
        free(log);
    }
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0) {
        return false;
    }
    
    return true;
}

bool world_state_render_init()
{
    GLuint vertShader, fragShader;

    // Create shader program.
    _program = glCreateProgram();

    // Create and compile vertex shader.
    if (!compile_shader(&vertShader, GL_VERTEX_SHADER, vertex_shader_source)) {
        fprintf(stderr, "Failed to compile vertex shader\n");
        return false;
    }

    // Create and compile fragment shader.
    if (!compile_shader(&fragShader, GL_FRAGMENT_SHADER, fragment_shader_source)) {
        fprintf(stderr, "Failed to compile fragment shader\n");
        return false;
    }

    // Attach vertex shader to program.
    glAttachShader(_program, vertShader);

    // Attach fragment shader to program.
    glAttachShader(_program, fragShader);

    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation(_program, GLKVertexAttribPosition, "position");
    glBindAttribLocation(_program, GLKVertexAttribColor, "color");
    glBindAttribLocation(_program, GLKVertexAttribNormal, "normal");

    // Link program.
    if (!link_program(_program)) {
        fprintf(stderr, "Failed to link program: %d\n", _program);

        if (vertShader) {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader) {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (_program) {
            glDeleteProgram(_program);
            _program = 0;
        }

        return false;
    }

    // Get uniform locations.
    uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation(_program, "modelViewProjectionMatrix");
    uniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation(_program, "normalMatrix");

    // Release vertex and fragment shaders.
    if (vertShader) {
        glDetachShader(_program, vertShader);
        glDeleteShader(vertShader);
    }
    if (fragShader) {
        glDetachShader(_program, fragShader);
        glDeleteShader(fragShader);
    }

    return true;
}

void world_state_render_teardown()
{
    if (_program) {
        glDeleteProgram(_program);
        _program = 0;
    }
}

void world_state_render(world_state * world, float * _modelViewProjectionMatrix, float * _normalMatrix)
{
    glClearColor(.274f, .286f, .349f, 1.0f); // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(_program);

    glUniformMatrix4fv(uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _modelViewProjectionMatrix);
    glUniformMatrix3fv(uniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _normalMatrix);

    world->display_lock.lock();
    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &world->grid_vertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &world->grid_vertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glLineWidth(4.0f);
    glDrawArrays(GL_LINES, 0, world->grid_vertex_num);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &world->axis_vertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &world->axis_vertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glLineWidth(8.0f);
    glDrawArrays(GL_LINES, 0, world->axis_vertex_num);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &world->feature_vertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &world->feature_vertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

#if TARGET_OS_IPHONE
#else
    glPointSize(3.0f);
#endif

    glDrawArrays(GL_POINTS, 0, world->feature_vertex_num);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &world->path_vertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &world->path_vertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glDrawArrays(GL_POINTS, 0, world->path_vertex_num);

    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &world->orientation_vertex[0].position);
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &world->orientation_vertex[0].color);
    glEnableVertexAttribArray(GLKVertexAttribColor);

    glLineWidth(8.0f);
    glDrawArrays(GL_LINES, 0, world->orientation_vertex_num);

    world->display_lock.unlock();
}

