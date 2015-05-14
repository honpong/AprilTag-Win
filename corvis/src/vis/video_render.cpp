#include <cstdint>

#include "video_render.h"
#include "gl_util.h"

static const char * video_vs =
"#version 120\n"
"attribute vec4 position;\n"
"attribute vec4 inputTextureCoordinate;\n"

"varying vec2 textureCoordinate;\n"

"void main()\n"
"{\n"
"	gl_Position = position;\n"
"	textureCoordinate = inputTextureCoordinate.xy;\n"
"}\n";

static const char * video_fs =
"#version 120\n"
"varying vec2 textureCoordinate;\n"
"float color;\n"
"uniform sampler2D videoFrame;\n"
"uniform int channels;\n"
""
"void main()\n"
"{\n"
"  if(channels == 1) {\n"
"    color = texture2D(videoFrame, textureCoordinate)[0];\n"
"	 gl_FragColor = vec4(color, color, color, 1);\n"
"  } else {\n"
"    gl_FragColor = texture2D(videoFrame, textureCoordinate);\n"
"  }\n"
"}";

static const GLfloat video_vertex[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    -1.0f,  1.0f,
    1.0f,  1.0f,
};

static const GLfloat texture_coord[] = {
    0.0f,  1.0f,
    1.0f, 1.0f,
    0.0f,  0.0f,
    1.0f, 0.0f,
};

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

    return(p);
}

void video_render::gl_init()
{
    program = setup_video_shaders();

    vertex_loc = glGetAttribLocation(program, "position");
    texture_coord_loc = glGetAttribLocation(program, "inputTextureCoordinate");

    frame_loc = glGetUniformLocation(program, "videoFrame");
    channels_loc = glGetUniformLocation(program, "channels");

    glEnable(GL_DEPTH_TEST);
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

void video_render::render(uint8_t * image, int width, int height, bool luminance)
{
    int channels = 4; // RGBA
    if(luminance)
        channels = 1; // Luminance only

    glUseProgram(program);
    glBindTexture(GL_TEXTURE_2D, texture);

    if(channels == 1)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glUniform1i(frame_loc, 0);
	glUniform1i(channels_loc, channels);

    // Draw the frame
	glVertexAttribPointer(vertex_loc, 2, GL_FLOAT, 0, 0, video_vertex);
	glEnableVertexAttribArray(vertex_loc);
	glVertexAttribPointer(texture_coord_loc, 2, GL_FLOAT, 0, 0, texture_coord);
	glEnableVertexAttribArray(texture_coord_loc);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
