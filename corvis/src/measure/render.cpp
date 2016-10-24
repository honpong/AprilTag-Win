#include "render.h"

static const char * vs =
"#version 120\n"
""
"uniform mat4 view, proj;\n"
" "
"attribute vec3 position;\n"
"attribute vec4 color;\n"
"varying vec4 color_fs;\n"
" "
"void main()\n"
"{\n"
"    color_fs = color;\n"
"    gl_Position = proj * view * vec4(position, 1);\n"
//"    gl_Position = vec4(position[0], position[1], 0, 1.0);\n"
"    gl_PointSize = 8.;\n"
"}\n";

static const char * fs = ""
"#version 120\n"
" "
"varying vec4 color_fs;\n"
" "
"void main()"
"{\n"
"    gl_FragColor = color_fs;\n"
"}";

static const char * vs_es =
"uniform mat4 view, proj;\n"
" "
"attribute vec3 position;\n"
"attribute vec4 color;\n"
"varying vec4 color_fs;\n"
" "
"void main()\n"
"{\n"
"    color_fs = color;\n"
"    gl_Position = proj * view * vec4(position, 1);\n"
"    gl_PointSize = 8.;\n"
"}\n";

static const char * fs_es =
"varying lowp vec4 color_fs;\n"
"void main()\n"
"{\n"
"    gl_FragColor = color_fs;\n"
"}";

void render::gl_init()
{
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

    vertex_loc = glGetAttribLocation(p, "position");
    color_loc = glGetAttribLocation(p, "color");

    projection_matrix_loc = glGetUniformLocation(p, "proj");
    view_matrix_loc = glGetUniformLocation(p, "view");
    program = p;
}

void render::draw_array(const VertexData * data, int number, int gl_type)
{
    if (!number) return;

    // Draw the frame
	glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, 0, sizeof(VertexData), &data[0].position);
	glEnableVertexAttribArray(vertex_loc);
	glVertexAttribPointer(color_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), &data[0].color);
	glEnableVertexAttribArray(color_loc);

    glDrawArrays(gl_type, 0, number);
}

void render::start_render(float * view_matrix, float * projection_matrix)
{
    glUseProgram(program);

    glUniformMatrix4fv(projection_matrix_loc,  1, false, projection_matrix);
    glUniformMatrix4fv(view_matrix_loc,  1, false, view_matrix);
}

void render::gl_destroy()
{
    glDeleteProgram(program);
}
