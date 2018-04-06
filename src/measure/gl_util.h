#ifndef __VIS_GL_UTIL__
#define __VIS_GL_UTIL__

#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if TARGET_OS_IPHONE
#include <OpenGLES/ES2/gl.h>
#else
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

void build_orthographic_projection_matrix(float * projMatrix, float ratio, float * min, float * max);
void build_projection_matrix(float * projMatrix, float fov, float ratio, float nearP, float farP);
void print_shader_info_log(GLuint obj);
void print_program_info_log(GLuint obj);

#endif
