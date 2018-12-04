/*******************************************************************************
 
 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or nondisclosure
 agreement with Intel Corporation and may not be copied or disclosed except in
 accordance with the terms of that agreement
 Copyright(c) 2018 Intel Corporation. All Rights Reserved.
 
 *******************************************************************************/
//
//  rs_sf_demo_d435i.hpp
//  boxsdk
//
#pragma once
#ifndef rs_sf_demo_d435i_hpp
#define rs_sf_demo_d435i_hpp

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <chrono>
#include <thread>
#include <string>
#include <sstream>
#include <iostream>

#ifndef GL_BGR
#define GL_BGR GL_BGR_EXT
#endif
#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE GL_CLAMP
#endif

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define PATH_SEPARATER '\\'
#else
#define PATH_SEPARATER '/'
#endif

#include "rs_icon.h"

namespace d435i {
    
//////////////////////////////
// Basic Data Types         //
//////////////////////////////

struct float3 { float x, y, z; };
struct float2 { float x, y; };
struct int2 { int w, h; };

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
    
    inline float ex() const { return x + w; }
    inline float ey() const { return y + h; }
    inline rect middle() const { return{ x + w / 2 - h * 6 / 14, y + h / 2 - h * 6 / 14, h * 6 / 7, h * 6 / 7 }; }
};

//////////////////////////////
// Simple font loading code //
//////////////////////////////

#include "../third-party/stb_easy_font.h"

inline void draw_text(float x, float y, const char * text)
{
    char buffer[60000]; // ~300 chars
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, 4 * stb_easy_font_print(x, (y - 7), (char *)text, nullptr, buffer, sizeof(buffer)));
    glDisableClientState(GL_VERTEX_ARRAY);
}

//////////////////
// Button Icons //
//////////////////

struct color_icon
{
    color_icon(const char* data, int width, int height, const unsigned char bkg_color[3]) : _width(width), _height(height)
    {
        for (int j = 0; j < 2; ++j) {
            _rgb[j].reserve(_width*_height * 4);
            for (int i = 0, n = _width*_height; i < n; ++i) {
                _rgb[j].push_back((j ? 0 : data[i]) | bkg_color[0]);
                _rgb[j].push_back((j ? 0 : data[i]) | bkg_color[1]);
                _rgb[j].push_back((j ? 0 : data[i]) | bkg_color[2]);
                _rgb[j].push_back((j ? 0 : (data[i] ? 255 : 0)));
            }
        }
    }
    inline const char* data(bool toogle = false) const { return (const char*)_rgb[toogle ? 1 : 0].data(); }
    inline rs2_format format() const { return RS2_FORMAT_RGBA8; /*RS2_FORMAT_BGRA8*/ }
    const int _width, _height;
    std::vector<unsigned char> _rgb[2];
    
};

struct number_icons : public std::vector<unsigned char>
{
    number_icons(const unsigned char bkg_color[3]) : _mm3_icon(get_icon(mm3), bkg_color)
    {
        auto data = get_icon_data(number);
        auto palettew = get_icon_width(number);
        _width = palettew / 11, _height = get_icon_height(number);
        for (int digit = 0; digit < 12; ++digit) {
            _data[digit].reserve(_height*_width * 4);
            for (int y = 0, digit_width = digit*_width; y < _height; ++y)
                for (int x = 0, y_offset = y*palettew + digit_width; x < _width; ++x) {
                    const unsigned char v = ((digit == 11) ? 0 : data[y_offset + x]);
                    _data[digit].push_back(v | bkg_color[0]);
                    _data[digit].push_back(v | bkg_color[1]);
                    _data[digit].push_back(v | bkg_color[2]);
                    _data[digit].push_back(v ? 255 : 0);
                }
        }
    }
    
    const unsigned char* print(const std::string& text)
    {
        _num_char = (int)text.length();
        std::vector<const unsigned char*> icon; icon.reserve(_num_char);
        for (auto& c : text) {
            if (c == 'x') icon.push_back(_data[10].data());
            else if (c - '0' >= 0 && c - '0' <= 9) icon.push_back(_data[c - '0'].data());
            else icon.push_back(_data[11].data());
        }
        
        const int fullw = width(), _num_char_width = _num_char * _width;
        resize(fullw * _height * 4);
        auto _img = data(), _mm3_icon_data = (unsigned char*)_mm3_icon.data();
        for (int y = 0; y < _height; ++y)
            for (int c = 0, y_width = y*_width, y_fullw = y*fullw; c < 4; ++c) {
                for (int d = 0; d < _num_char; ++d) {
                    for (int x = 0, y_offset = (y_fullw + d*_width) * 4 + c; x < _width; ++x)
                        _img[y_offset + x * 4] = icon[d][(y_width + x) * 4 + c];
                }
                for (int x = 0, y_offset = (y_fullw + _num_char_width) * 4 + c, y_mm3_width = y*_mm3_icon._width; x < _mm3_icon._width; ++x)
                    _img[y_offset + x * 4] = _mm3_icon_data[(y_mm3_width + x) * 4 + c];
            }
        
        return _img;
    }
    
    inline int width() const { return _width * _num_char + _mm3_icon._width; }
    inline int height() const { return _height; }
    inline rs2_format format() const {return RS2_FORMAT_RGBA8; }
    
private:
    int _width, _height, _num_char;
    std::vector<unsigned char> _data[12];
    color_icon _mm3_icon;
};

//////////////////////
// Box display code //
//////////////////////

inline void draw_box_wire(const int width, const int height, const float box_wire_endpt[12][2][2], const rect& r, const int app_height, float line_width)
{
    if (width <= 0 || height <= 0 || !box_wire_endpt || r.w <= 0 || r.h <= 0) return;
    const auto sx = r.w / width, sy = r.h / height;
    
    float original_color[4], original_line_width;
    glGetFloatv(GL_CURRENT_COLOR, original_color);
    glGetFloatv(GL_LINE_WIDTH, &original_line_width);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
#ifndef __APPLE__
    glEnable(GL_SCISSOR_TEST);
#endif
    glScissor((int)r.x, (int)(app_height - r.h - r.y), (int)r.w, (int)r.h);
    glLineWidth(line_width);
    glColor3f(1.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    for (int l = 0; l < 12; ++l)
    {
        glVertex2f(r.x + box_wire_endpt[l][0][0] * sx, r.y + box_wire_endpt[l][0][1] * sy);
        glVertex2f(r.x + box_wire_endpt[l][1][0] * sx, r.y + box_wire_endpt[l][1][1] * sy);
    }
    glEnd();
#ifndef __APPLE__
    glDisable(GL_SCISSOR_TEST);
#endif
    glColor4fv(original_color);
    glLineWidth(original_line_width);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}

////////////////////////
// Image display code //
////////////////////////

class texture
{
public:
    
    static const bool set_adjust_ratio = false;
    
    static const char* get_realsense_icon(int* icon_width, int* icon_height, rs2_format* format)
    {
        *icon_width = get_icon_width(realsense);
        *icon_height = get_icon_height(realsense);
        *format = RS2_FORMAT_BGRA8;
        return get_icon_data(realsense);
    }
    
    template<typename ICON>
    void render_middle(ICON icon, const rect& r, const rs2_format& format = RS2_FORMAT_RGB8)
    {
        glRectf(r.x, r.y, r.x + r.w, r.y + r.h);
        
        static int2 dim; rs2_format fm = format;
        render(get_realsense_icon(&dim.w, &dim.h, &fm), dim.w, dim.h, rect{ r.x + r.w / 8, r.y, r.w * 3 / 4, r.h }.adjust_ratio({ float(dim.w),float(dim.h) }), "", fm);
    }
    
    void render(const rs_sf_image* frame, const rect& r, const char* text = nullptr)
    {
        if (!frame) return;
        
        rs2_format format[] = {RS2_FORMAT_ANY,RS2_FORMAT_Y8, RS2_FORMAT_Z16, RS2_FORMAT_RGB8, RS2_FORMAT_RGBA8};
        render(frame->data, frame->img_w, frame->img_h, r, text, format[frame->byte_per_pixel]);
    }
    
    void render(const void* data, const int width, const int height, const rect& r, const char* text = nullptr, const rs2_format& format = RS2_FORMAT_RGB8)
    {
        upload(data, width, height, format);
        show(_r = set_adjust_ratio ? r.adjust_ratio({ float(width), float(height) }) : r, text);
    }
    
    std::vector<uint8_t> _rgb;
    inline void make_depth_histogram(uint8_t rgb_image[], const uint16_t depth_image[], int width, int height)
    {
        static uint32_t histogram[0x10000];
        memset(histogram, 0, sizeof(histogram));
        
        for (auto i = 0; i < width*height; ++i) ++histogram[depth_image[i]];
        for (auto i = 2; i < 0x10000; ++i) histogram[i] += histogram[i - 1]; // Build a cumulative histogram for the indices in [1,0xFFFF]
        for (auto i = 0; i < width*height; ++i)
        {
            if (auto d = depth_image[i])
            {
                int f = histogram[d] * 255 / histogram[0xFFFF]; // 0-255 based on histogram location
                rgb_image[i * 3 + 0] = 255 - f;
                rgb_image[i * 3 + 1] = 0;
                rgb_image[i * 3 + 2] = f;
            }
            else
            {
                rgb_image[i * 3 + 0] = 20;
                rgb_image[i * 3 + 1] = 5;
                rgb_image[i * 3 + 2] = 0;
            }
        }
    }

    void upload(const void* data, const int width, const int height, const rs2_format& format = RS2_FORMAT_RGB8)
    {
        if (!data) return;
        
        if (!gl_handle)
            glGenTextures(1, &gl_handle);
        if (GLenum err = glGetError()){ return; };
        
        glBindTexture(GL_TEXTURE_2D, gl_handle);
        
        switch (format)
        {
            case RS2_FORMAT_Z16:
            case RS2_FORMAT_DISPARITY16:
                _rgb.resize(width*height*4);
                make_depth_histogram(_rgb.data(), reinterpret_cast<const uint16_t *>(data), width, height);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, _rgb.data());
                break;
            case RS2_FORMAT_RGB8:
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width = width, _height = height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                break;
            case RS2_FORMAT_BGRA8:
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width = width, _height = height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
                break;
            case RS2_FORMAT_RGBA8:
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width = width, _height = height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                break;
            case RS2_FORMAT_Y8:
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width = width, _height = height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
                break;
            default:
                throw std::runtime_error("The requested format is not suported by this demo!");
        }
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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
        
        draw_text(r.x + r.w - 60, r.y + r.h - 20, text ? text : rs2_stream_to_string(stream));
    }
    
    void draw_box(const float box_wire_endpt[12][2][2], int app_height, float line_width = -1.0f)
    {
        if (line_width < 2.5f) { line_width = std::fmax(4.0f, _r.w * 4.0f / _width); }
        draw_box_wire(_width, _height, box_wire_endpt, _r, app_height, line_width);
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
    
    std::function<void(double, double)> on_mouse_scroll = [](double, double) {};
    std::function<void(double, double)> on_mouse_move = [this](double x, double y) {_mouse_pos[0] = x; _mouse_pos[1] = y; };
    std::function<void(int)>            on_key_release = [this](int key) { if (key == 'q' || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE); };
    std::function<void(bool)>           on_left_mouse = [this](bool click)
    {
        if (click) {
            if (_mouse_pos[0] > width() / 3) { _tgscn = true; }
            else if (_mouse_pos[1] < win_rs_logo().ey()) { _dense = !_dense; }
            else if (_mouse_pos[1] > win_depth_image().y) {
                if (_mouse_pos[1] < height() * 7 / 8) { _dwin_opt = (_dwin_opt + 1) % 3; }
                else {
                    if (_mouse_pos[0] <= width() / 6) { _close = true; }
                    else if (_mouse_pos[0] <= width() * 3) { _reset = true; }
                }
            }
        }
    };
    
    window(int width, int height, const char* title) : _title(title), _icon_close(get_icon(close), bkg_black), _icon_reset(get_icon(reset), bkg_black), _num_icons(bkg_black)
    {
        glfwInit();
        reset_screen(false, width, height);
    }
    
    void reset_screen(bool fullscreen = true, const int width = 0, const int height = 0)
    {
        _tgscn = false; _fullscreen = fullscreen;
        if (win) { glfwDestroyWindow(win); win = nullptr; }
        
        if (fullscreen)
        {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            win = glfwCreateWindow(_width = mode->width, _height = mode->height, _title, monitor, NULL);
        }
        else
        {
            win = glfwCreateWindow(_width = _win_width = width, _height = _win_height = height, _title, nullptr, nullptr);
        }
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
    
    inline rect win_left_column()  const { return{ 0, 0, width() / 3, height() }; }
    inline rect win_rs_logo()      const { return{ 0, 0, win_left_column().w, win_left_column().h / 6 }; }
    inline rect win_depth_image()  const { return{ win_left_column().x, win_left_column().y + win_left_column().h * 2 / 7, win_left_column().w, win_left_column().h / 2 }; }
    inline rect win_button_area()  const { return{ win_left_column().x, win_left_column().y + win_left_column().h * 7 / 8, win_left_column().w, win_left_column().h / 10 }; }
    inline rect win_close_button() const { return{ win_button_area().x + 1, win_button_area().y, win_button_area().w / 2 - 2, win_button_area().h }; }
    inline rect win_reset_button() const { return{ win_close_button().ex() + 1, win_close_button().y, win_close_button().w, win_close_button().h }; }
    //inline rect win_color_image()  const { return{ win_left_column().ex(), 0, width() - win_left_column().w, height() }; }
    inline rect win_color_image()  const { return{ 0, 0, width()-1, height()-1 }; }
    //inline rect win_box_msg()      const { return{ win_left_column().x + win_left_column().w / 10, win_rs_logo().ey() + win_rs_logo().h / 8, win_left_column().w * 4 / 5, win_rs_logo().h * 3 / 8 }; }
    inline rect win_box_msg() const { return{win_rs_logo().w + (width()-win_left_column().w)/6, win_rs_logo().h/6, (width()-win_left_column().w)*4/6, win_rs_logo().h*4/6};}
    
    operator bool()
    {
        process_event();
        
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
        
#ifdef __APPLE__
        static bool init_moved = false;
        if(!init_moved){
            int x, y;
            glfwGetWindowPos(win, &x, &y);
            glfwSetWindowPos(win, ++x, y);
            glfwSetWindowPos(win, --x, y);
            init_moved=true;
        }
#endif //__APPLE__
        
        return res;
    }
    
    void render_rect(const rect& r, bool default_color, const std::vector<unsigned char>& color)
    {
        float original_color[4];
        glGetFloatv(GL_CURRENT_COLOR, original_color);
        if (!default_color) glColor3b(color[0], color[1], color[2]);
        glPushMatrix(); glTranslatef(r.x, r.y, 0.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(0, 0); glVertex2f(r.w, 0.0f); glVertex2f(r.w, r.h); glVertex2f(0.0f, r.h);
        glEnd();
        glPopMatrix(); glColor4fv(original_color);
    }
    
    void render_ui(const rs_sf_image* depth_frame, const rs_sf_image* color_frame, bool render_buttons = true)
    {
        glClearColor(bkg_blue[0] / 255.0f, bkg_blue[1] / 255.0f, bkg_blue[2] / 255.0f, 1);
        
        //_texture_depth.render(depth_frame, win_depth_image(), "");
        _texture_color.render(color_frame, win_color_image(), _dense ? "" : "bypass");
        _texture_realsense_logo.render_middle(0, win_rs_logo());
        
        if (render_buttons)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            _texture_close_button.render(_icon_close.data(_close), _icon_close._width, _icon_close._height, win_close_button().middle(), "", _icon_close.format());
            _texture_reset_button.render(_icon_reset.data(_reset), _icon_reset._width, _icon_reset._height, win_reset_button().middle(), "", _icon_reset.format());
            //render_rect(win_close_button(), !_close, { 128, 128, 128 });
            //render_rect(win_reset_button(), !_reset, { 128, 128, 128 });
            glDisable(GL_BLEND);
        }
    }
    
    void render_box_dim(const std::string& box_dim)
    {
        if(box_dim.empty()) return;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        _texture_box_msg.render(_num_icons.print(box_dim), _num_icons.width(), _num_icons.height(), win_box_msg(), "", _num_icons.format());
        glDisable(GL_BLEND);
    }
    
    /**
    void render_box_on_depth_frame(const rs2::box::wireframe& wireframe, const float line_width = -1.0f)
    {
        _texture_depth.draw_box(wireframe.end_pt, _height, line_width);
    }
    
    void render_box_on_color_frame(const rs2::box::wireframe& wireframe, const float line_width = -1.0f)
    {
        _texture_color.draw_box(wireframe.end_pt, _height, line_width);
    }
    
    float render_raycast_depth_frame(rs2::box_measure& boxscan, rs2::box_frameset& fs, rs2::box& box, rs2::frame& depth_display)
    {
        const int max_depth_tolerance = 50;          // definition of box depth fitness in mm
        const float mismatch_ratio_tolerance = 0.4f; // percentage of mismatched pixel allowed if boxcast requested
        auto fs2 = boxscan.raycast_box_onto_frame(box, fs); // do box raycasting
        
        auto src_depth = (const uint16_t*)fs2[RS2_STREAM_DEPTH].get_data();
        auto box_depth = (const uint16_t*)fs2[RS2_STREAM_BOXCAST].get_data();
        auto dst_color = (uint8_t*)depth_display.get_data();
        
        int num_box_pix = 0, num_box_err = 0, num_pix = boxscan.stream_h()*boxscan.stream_w();
        for (int p = num_pix - 1, p3 = p * 3; p >= 0; --p, p3 -= 3) {
            if (box_depth[p] != 0) {
                ++num_box_pix;
                if (std::abs(src_depth[p] - box_depth[p]) > max_depth_tolerance) {
                    ++num_box_err;
                    memset(dst_color + p3, 255, 3); // mark bad pixel white
                }
            }
            else { memset(dst_color + p3, 0, 3); } // mark non-box pixel black
        }
        
        const float mismatch = (float)num_box_err / std::max(1, num_box_pix);
        _texture_depth.render(depth_display, win_depth_image(), (std::to_string((int)(mismatch * 100 + .5f)) + "% mismatch").c_str());
        
        // request reset if too many frames having high box detection errors
        if (mismatch > mismatch_ratio_tolerance && ++_high_mismatch_count > 30) { _reset = true; }
        return mismatch;
    }
    
    void validate_dense_depth_request(const rs2_measure_camera_state& dense_depth_state)
    {
        if ( !_dense ) return;
        for( auto& pose_val : dense_depth_state.camera_pose )
            if ( pose_val != 0 ) return; //if dense depth available, pose must not all zeros
        _dense = false; //all zero pose means camera tracker not available
    }
     */
    
    void process_event()
    {
        if (_close) std::thread([this]{ std::this_thread::sleep_for(std::chrono::milliseconds(100)); on_key_release('q');}).detach();
        if (_tgscn) reset_screen(!_fullscreen, _win_width, _win_height);
        if (_reset) { _high_mismatch_count = 0; }
        _close = _reset = false;
    }
    
    ~window()
    {
        glfwDestroyWindow(win);
        glfwTerminate();
    }
    
    operator GLFWwindow*() { return win; }
    bool reset_request() const { return _reset; }
    bool plane_request() const { return _dwin_opt == 1; }
    bool boxca_request() const { return _dwin_opt == 2; }
    bool dense_request() const { return _dense; }
    
    double _mouse_pos[2] = {};
    const unsigned char bkg_blue[3] = { 0, 66, 128 };
    const unsigned char bkg_black[3] = {0,0,0};
    texture _texture_depth, _texture_color;
    
private:
    GLFWwindow* win = nullptr;
    int _width, _height, _win_width, _win_height, _dwin_opt = 0, _high_mismatch_count = 0;
    bool _close = false, _reset = false, _tgscn = false, _fullscreen, _dense = true;
    const char* _title;
    color_icon _icon_close, _icon_reset;
    texture _texture_realsense_logo, _texture_close_button, _texture_reset_button, _texture_box_msg;
    number_icons _num_icons;
};
}

#endif /*rs_sf_demo_d435i_hpp*/
