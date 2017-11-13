/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
//
//  rs-box-app.hpp
//  boxsdk
//
#pragma once

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <string>
#include <sstream>
#include <iostream>

#ifndef GL_BGR
#define GL_BGR GL_BGR_EXT
#endif

//////////////////////////////
// Basic Data Types         //
//////////////////////////////

struct float3 { float x, y, z; };
struct float2 { float x, y; };

struct rect
{
    float x, y;
    float w, h;

    // Create new rect within original boundaries with give aspect ration
    rect adjust_ratio(float2 size) const
    {
        auto H = static_cast<float>(h), W = static_cast<float>(h) * size.x / size.y;
        if (W > w)
        {
            auto scale = w / W;
            W *= scale;
            H *= scale;
        }

        return{ x + (w - W) / 2, y + (h - H) / 2, W, H };
    }
};

//////////////////////////////
// Simple font loading code //
//////////////////////////////

#include "../third-party/stb_easy_font.h"

inline void draw_text(int x, int y, const char * text)
{
    char buffer[60000]; // ~300 chars
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, 4 * stb_easy_font_print((float)x, (float)(y - 7), (char *)text, nullptr, buffer, sizeof(buffer)));
    glDisableClientState(GL_VERTEX_ARRAY);
}

//////////////////////
// Box display code //
//////////////////////

inline void draw_box_wire(const int width, const int height, const float box_wire_endpt[12][2][2], const rect& r, int line_width)
{
    if (width <= 0 || height <= 0 || !box_wire_endpt || r.w <= 0 || r.h <= 0) return;
    const auto sx = r.w / width, sy = r.h / height;

    float currentColor[4];
    glGetFloatv(GL_CURRENT_COLOR, currentColor);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glLineWidth(line_width);
    glColor3f(1.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    for (int l = 0; l < 12; ++l)
    {
        glVertex2f(r.x + box_wire_endpt[l][0][0] * sx, r.y + box_wire_endpt[l][0][1] * sy);
        glVertex2f(r.x + box_wire_endpt[l][1][0] * sx, r.y + box_wire_endpt[l][1][1] * sy);
    }
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
    glColor4fv(currentColor);
}

////////////////////////
// Image display code //
////////////////////////

class texture
{
public:

    //template<typename ICON>
    //void render(ICON icon, const rect& r)
    //{
    //int width, height;
    //render(icon(width, height), width, height, { r.x + r.w / 4, r.y, r.w / 2, r.h }, nullptr, RS2_FORMAT_BGR8);

    //GLfloat a = r.x, b = r.y, c = r.x + r.w, d = r.y + r.h;
    //printf("%.2f %.2f %.2f %.2f \n", a, b, c, d);
    //glRectf(a, b, c, d);
    //}

    void render(const rs2::video_frame& frame, const rect& r, const char* text = nullptr)
    {
        if (!frame) return;
        render(frame.get_data(), frame.get_width(), frame.get_height(), r, text, frame.get_profile().format());
    }

    void render(const void* data, const int width, const int height, const rect& r, const char* text = nullptr, const rs2_format& format = RS2_FORMAT_RGB8)
    {
        upload(data, width, height, format);
        show(_r = r.adjust_ratio({ float(width), float(height) }), text);
        //show(_r = r, text);
    }

    void upload(const void* data, const int width, const int height, const rs2_format& format = RS2_FORMAT_RGB8)
    {
        if (!data) return;

        if (!gl_handle)
            glGenTextures(1, &gl_handle);
        GLenum err = glGetError();

        //stream = frame.get_profile().stream_type();

        glBindTexture(GL_TEXTURE_2D, gl_handle);

        switch (format)
        {
        case RS2_FORMAT_RGB8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width = width, _height = height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            break;
        case RS2_FORMAT_BGR8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width = width, _height = height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
            break;
        default:
            throw std::runtime_error("The requested format is not suported by this demo!");
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLuint get_gl_handle() { return gl_handle; }

    void show(const rect& r, const char* text = nullptr) const
    {
        if (!gl_handle)
            return;

        glBindTexture(GL_TEXTURE_2D, gl_handle);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUAD_STRIP);
        glTexCoord2f(0.f, 1.f); glVertex2f(r.x, r.y + r.h);
        glTexCoord2f(0.f, 0.f); glVertex2f(r.x, r.y);
        glTexCoord2f(1.f, 1.f); glVertex2f(r.x + r.w, r.y + r.h);
        glTexCoord2f(1.f, 0.f); glVertex2f(r.x + r.w, r.y);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        draw_text((int)(r.x) + 15, (int)(r.y) + 20, text ? text : rs2_stream_to_string(stream));
    }

    void draw_box(const float box_wire_endpt[12][2][2], int line_width = 2)
    {
        draw_box_wire(_width, _height, box_wire_endpt, _r, line_width);
    }

private:
    GLuint gl_handle = 0;
    int _width = 0;
    int _height = 0;
    rect _r = {};
    rs2_stream stream = RS2_STREAM_ANY;
};

class window
{
public:
    std::function<void(bool)>           on_left_mouse = [](bool) {};
    std::function<void(double, double)> on_mouse_scroll = [](double, double) {};
    std::function<void(double, double)> on_mouse_move = [](double, double) {};
    std::function<void(int)>            on_key_release = [](int) {};

    window(int width, int height, const char* title)
        : _width(width), _height(height)
    {
        glfwInit();
        win = glfwCreateWindow(width, height, title, nullptr, nullptr);
        glfwMakeContextCurrent(win);

        glfwSetWindowUserPointer(win, this);
        glfwSetMouseButtonCallback(win, [](GLFWwindow * win, int button, int action, int mods)
        {
            auto s = (window*)glfwGetWindowUserPointer(win);
            if (button == 0) s->on_left_mouse(action == GLFW_PRESS);
        });

        glfwSetScrollCallback(win, [](GLFWwindow * win, double xoffset, double yoffset)
        {
            auto s = (window*)glfwGetWindowUserPointer(win);
            s->on_mouse_scroll(xoffset, yoffset);
        });

        glfwSetCursorPosCallback(win, [](GLFWwindow * win, double x, double y)
        {
            auto s = (window*)glfwGetWindowUserPointer(win);
            s->on_mouse_move(x, y);
        });

        glfwSetKeyCallback(win, [](GLFWwindow * win, int key, int scancode, int action, int mods)
        {
            auto s = (window*)glfwGetWindowUserPointer(win);
            if (0 == action) // on key release
            {
                s->on_key_release(key);
            }
        });
    }

    float width() const { return float(_width); }
    float height() const { return float(_height); }

    operator bool()
    {
        glPopMatrix();
        glfwSwapBuffers(win);

        auto res = !glfwWindowShouldClose(win);

        glfwPollEvents();
        glfwGetFramebufferSize(win, &_width, &_height);

        // Clear the framebuffer
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, _width, _height);

        // Draw the images
        glPushMatrix();
        glfwGetWindowSize(win, &_width, &_height);
        glOrtho(0, _width, _height, 0, -1, +1);

        return res;
    }

    ~window()
    {
        glfwDestroyWindow(win);
        glfwTerminate();
    }

    operator GLFWwindow*() { return win; }

private:
    GLFWwindow* win;
    int _width, _height;
};
