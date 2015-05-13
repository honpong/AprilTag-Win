#ifndef __VIS_PLATFORM_GL__
#define __VIS_PLATFORM_GL__

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if TARGET_OS_IPHONE
#include <OpenGLES/ES2/gl.h>
#else
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

#endif
