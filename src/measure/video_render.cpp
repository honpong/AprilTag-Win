#include <cstdint>

#include "video_render.h"
#include "gl_util.h"

static const char * overlay_vs =
"#version 120\n"
"attribute vec3 videoPosition;\n"
"attribute vec4 color;\n"
"varying vec4 color_fs;\n"
"uniform float width_2;\n"
"uniform float height_2;\n"
"uniform float scale_x;\n"
"uniform float scale_y;\n"

"varying vec2 textureCoordinate;\n"

"void main()\n"
"{\n"
"    gl_Position = vec4((videoPosition.x - width_2)*scale_x, -(videoPosition.y - height_2)*scale_y, 0, 1);\n"
"    color_fs = color;\n"
"}\n";

static const char * overlay_fs =
"#version 120\n"
"varying vec4 color_fs;\n"
""
"void main()\n"
"{\n"
"    gl_FragColor = color_fs;\n"
"}";

static const char * video_vs =
"#version 120\n"
"attribute vec3 videoPosition;\n"
"attribute vec2 inputTextureCoordinate;\n"
"uniform float width_2;\n"
"uniform float height_2;\n"
"uniform float scale_x;\n"
"uniform float scale_y;\n"

"varying vec2 textureCoordinate;\n"

"void main()\n"
"{\n"
"    gl_Position = vec4((videoPosition.x - width_2)*scale_x, -(videoPosition.y - height_2)*scale_y, .1, 1);\n"
"    textureCoordinate = inputTextureCoordinate.xy;\n"
"}\n";

static const char * video_fs =
"#version 120\n"
"varying vec2 textureCoordinate;\n"
"float color_r;\n"
"uniform sampler2D videoFrame;\n"
"uniform int channels;\n"
""
"void main()\n"
"{\n"
"    if(channels == 1) {\n"
"      color_r = texture2D(videoFrame, textureCoordinate)[0];\n"
"       gl_FragColor = vec4(color_r, color_r, color_r, 1);\n"
"    } else {\n"
"      gl_FragColor = texture2D(videoFrame, textureCoordinate);\n"
"    }\n"
"}";

static GLuint setup_video_shaders() {
    GLuint p,v,f;

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);

#if TARGET_OS_IPHONE
#else
    glShaderSource(v, 1, &video_vs,NULL);
    glShaderSource(f, 1, &video_fs,NULL);
#endif

    glCompileShader(v);
    glCompileShader(f);

    print_shader_info_log(v);
    print_shader_info_log(f);

    p = glCreateProgram();
    glAttachShader(p,v);
    glAttachShader(p,f);

    glLinkProgram(p);
    print_program_info_log(p);

    glDetachShader(p,v);
    glDetachShader(p,f);
    glDeleteShader(v);
    glDeleteShader(f);

    return(p);
}

static GLuint setup_overlay_shaders() {
    GLuint p,v,f;

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);

#if TARGET_OS_IPHONE
#else
    glShaderSource(v, 1, &overlay_vs,NULL);
    glShaderSource(f, 1, &overlay_fs,NULL);
#endif

    glCompileShader(v);
    glCompileShader(f);

    print_shader_info_log(v);
    print_shader_info_log(f);

    p = glCreateProgram();
    glAttachShader(p,v);
    glAttachShader(p,f);

    glLinkProgram(p);
    print_program_info_log(p);

    glDetachShader(p,v);
    glDetachShader(p,f);
    glDeleteShader(v);
    glDeleteShader(f);

    return(p);
}

void video_render::gl_init()
{
    program = setup_video_shaders();
    overlay_program = setup_overlay_shaders();

    vertex_loc = glGetAttribLocation(program, "videoPosition");
    texture_coord_loc = glGetAttribLocation(program, "inputTextureCoordinate");

    frame_loc = glGetUniformLocation(program, "videoFrame");
    channels_loc = glGetUniformLocation(program, "channels");

    width_2_loc = glGetUniformLocation(program, "width_2");
    height_2_loc = glGetUniformLocation(program, "height_2");
    scale_x_loc = glGetUniformLocation(program, "scale_x");
    scale_y_loc = glGetUniformLocation(program, "scale_y");

    overlay_color_loc = glGetAttribLocation(overlay_program, "color");
    overlay_vertex_loc = glGetAttribLocation(overlay_program, "videoPosition");
    overlay_width_2_loc = glGetUniformLocation(overlay_program, "width_2");
    overlay_height_2_loc = glGetUniformLocation(overlay_program, "height_2");
    overlay_scale_x_loc = glGetUniformLocation(overlay_program, "scale_x");
    overlay_scale_y_loc = glGetUniformLocation(overlay_program, "scale_y");

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void video_render::gl_destroy()
{
    glDeleteTextures(1, &texture);
}

static void calculate_scale(int width, int height, int viewport_width, int viewport_height, float & scale_x, float & scale_y)
{
    scale_x = 1.f / (width/2.f);
    scale_y = 1.f / (height/2.f);
    if((float)width/viewport_width > (float)height/viewport_height)
        scale_y *= ((float)height/width)*viewport_width/viewport_height;
    else
        scale_x *= ((float)width/height)*viewport_height/viewport_width;
}

void video_render::render(const uint8_t * image, int width, int height, int viewport_width, int viewport_height, bool luminance)
{
    float scale_x, scale_y;
    calculate_scale(width, height, viewport_width, viewport_height, scale_x, scale_y);

    float width_2 = width/2.f;
    float height_2 = height/2.f;
    int channels = 4; // RGBA
    if(luminance)
        channels = 1; // Luminance only

    glUseProgram(program);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    if(channels == 1)
#if TARGET_OS_IPHONE
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, image);
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image);
#endif
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glUniform1i(frame_loc, 0);
    glUniform1i(channels_loc, channels);

    glUniform1f(width_2_loc, width_2);
    glUniform1f(height_2_loc, height_2);
    glUniform1f(scale_x_loc, scale_x);
    glUniform1f(scale_y_loc, scale_y);

    GLfloat video_vertex[8];
    // Draw the frame
    video_vertex[0] = 0;            video_vertex[1] = 0; // upper left
    video_vertex[2] = (float)width; video_vertex[3] = 0; // upper right
    video_vertex[4] = 0;            video_vertex[5] = (float)height; // lower left
    video_vertex[6] = (float)width; video_vertex[7] = (float)height; // lower right
    glVertexAttribPointer(vertex_loc, 2, GL_FLOAT, 0, 0, video_vertex);
    glEnableVertexAttribArray(vertex_loc);

    const GLfloat texture_coord[] = {
        0.0f,  0.0f,
        1.0f,  0.0f,
        0.0f,  1.0f,
        1.0f,  1.0f,
    };
    glVertexAttribPointer(texture_coord_loc, 2, GL_FLOAT, 0, 0, texture_coord);
    glEnableVertexAttribArray(texture_coord_loc);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void video_render::draw_overlay(const VertexData * data, int number, int gl_type, int width, int height, int viewport_width, int viewport_height)
{
    float scale_x, scale_y;
    calculate_scale(width, height, viewport_width, viewport_height, scale_x, scale_y);


    float width_2 = width/2.f;
    float height_2 = height/2.f;

    glUseProgram(overlay_program);
    glUniform1f(overlay_width_2_loc, width_2);
    glUniform1f(overlay_height_2_loc, height_2);
    glUniform1f(overlay_scale_x_loc, scale_x);
    glUniform1f(overlay_scale_y_loc, scale_y);

    // Draw the frame
    glVertexAttribPointer(overlay_vertex_loc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), &data[0].position);
    glEnableVertexAttribArray(overlay_vertex_loc);
    glVertexAttribPointer(overlay_color_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &data[0].color);
    glEnableVertexAttribArray(overlay_color_loc);

    glDrawArrays(gl_type, 0, number);
}
