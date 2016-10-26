#define _USE_MATH_DEFINES
#include "gl_util.h"

#include <cmath>

void build_orthographic_projection_matrix(float * projMatrix, float ratio, float * min, float * max)
{
    for(int i = 0; i < 16; i++) projMatrix[i] = 0;

    // ratio is width / height
    float dx = (max[0] - min[0]);
    float dy = (max[1] - min[1]);
    float dz = max[2] - min[2];
    float scale = fmax(dx, dy*ratio);

    projMatrix[0] = 2/scale;
    projMatrix[5] = 2*ratio/scale;
    projMatrix[10] = 2/dz;
    projMatrix[12] = -(max[0] + min[0]) / scale;
    projMatrix[13] = -(max[1] + min[1]) * ratio / scale;
    projMatrix[14] = -(max[2] + min[2]) / dz;
    projMatrix[15] = 1;

}

void build_projection_matrix(float * projMatrix, float fov, float ratio, float nearP, float farP)
{
    float f = 1.0f / tan (fov * ((float)M_PI / 360.0f));

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

void print_shader_info_log(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        fprintf(stderr, "%s\n",infoLog);
        free(infoLog);
    }
}

void print_program_info_log(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
        fprintf(stderr, "%s\n",infoLog);
        free(infoLog);
    }
}
