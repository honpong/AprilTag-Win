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
#ifndef rs_box_app_hpp
#define rs_box_app_hpp

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
#include "rs_icon.h"

struct color_icon
{
    color_icon(const char* data, int width, int height, const unsigned char bkg_color[3]) : _width(width), _height(height)
    {
        for (int j = 0; j < 2; ++j) {
            _rgb[j].reserve(_width*_height * 3);
            for (int i = 0, n = _width*_height; i < n; ++i) {
                _rgb[j].push_back((j ? 0 : data[i]) | bkg_color[0]);
                _rgb[j].push_back((j ? 0 : data[i]) | bkg_color[1]);
                _rgb[j].push_back((j ? 0 : data[i]) | bkg_color[2]);
            }
        }
    }
    inline const char* data(bool toogle = false) const { return (const char*)_rgb[toogle ? 1 : 0].data(); }
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
            _data[digit].reserve(_height*_width * 3);
            for (int y = 0, digit_width = digit*_width; y < _height; ++y)
                for (int x = 0, y_offset = y*palettew + digit_width; x < _width; ++x) {
                    const unsigned char v = ((digit == 11) ? 0 : data[y_offset + x]);
                    _data[digit].push_back(v | bkg_color[0]);
                    _data[digit].push_back(v | bkg_color[1]);
                    _data[digit].push_back(v | bkg_color[2]);
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
        resize(fullw * _height * 3);
        auto _img = data(), _mm3_icon_data = (unsigned char*)_mm3_icon.data();
        for (int y = 0; y < _height; ++y)
            for (int c = 0, y_width = y*_width, y_fullw = y*fullw; c < 3; ++c) {
                for (int d = 0; d < _num_char; ++d) {
                    for (int x = 0, y_offset = (y_fullw + d*_width) * 3 + c; x < _width; ++x)
                        _img[y_offset + x * 3] = icon[d][(y_width + x) * 3 + c];
                }
                for (int x = 0, y_offset = (y_fullw + _num_char_width) * 3 + c, y_mm3_width = y*_mm3_icon._width; x < _mm3_icon._width; ++x)
                    _img[y_offset + x * 3] = _mm3_icon_data[(y_mm3_width + x) * 3 + c];
            }
        
        return _img;
    }
    
    inline int width() const { return _width * _num_char + _mm3_icon._width; }
    inline int height() const { return _height; }
    
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
    
    template<typename ICON>
    void render_middle(ICON icon, const rect& r, const rs2_format& format = RS2_FORMAT_RGB8)
    {
        glRectf(r.x, r.y, r.x + r.w, r.y + r.h);
        
        static int2 dim; rs2_format fm = format;
        render(icon(dim.w, dim.h, fm), dim.w, dim.h, rect{ r.x + r.w / 8, r.y, r.w * 3 / 4, r.h }.adjust_ratio({ float(dim.w),float(dim.h) }), "", fm);
    }
    
    void render(const rs2::video_frame& frame, const rect& r, const char* text = nullptr)
    {
        if (!frame) return;
        render(frame.get_data(), frame.get_width(), frame.get_height(), r, text, frame.get_profile().format());
    }
    
    void render(const void* data, const int width, const int height, const rect& r, const char* text = nullptr, const rs2_format& format = RS2_FORMAT_RGB8)
    {
        upload(data, width, height, format);
        show(_r = set_adjust_ratio ? r.adjust_ratio({ float(width), float(height) }) : r, text);
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
            case RS2_FORMAT_RGB8:
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width = width, _height = height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                break;
            case RS2_FORMAT_BGRA8:
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width = width, _height = height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
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
        
        draw_text(r.x + 15, r.y + 20, text ? text : rs2_stream_to_string(stream));
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
    
    window(int width, int height, const char* title) : _title(title), _icon_close(get_icon(close), bkg_blue), _icon_reset(get_icon(reset), bkg_blue), _num_icons(bkg_blue)
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
    inline rect win_color_image()  const { return{ win_left_column().ex(), 0, width() - win_left_column().w, height() }; }
    inline rect win_box_msg()      const { return{ win_left_column().x + win_left_column().w / 10, win_rs_logo().ey() + win_rs_logo().h / 8, win_left_column().w * 4 / 5, win_rs_logo().h * 3 / 8 }; }
    
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
    
    void render_ui(const rs2::frame& depth_frame, const rs2::frame& color_frame, bool render_buttons = true)
    {
        glClearColor(bkg_blue[0] / 255.0f, bkg_blue[1] / 255.0f, bkg_blue[2] / 255.0f, 1);
        
        _texture_realsense_logo.render_middle(rs2::box_measure::get_icon, win_rs_logo());
        _texture_depth.render(depth_frame, win_depth_image(), "");
        _texture_color.render(color_frame, win_color_image(), "");
        
        if (render_buttons)
        {
            _texture_close_button.render(_icon_close.data(_close), _icon_close._width, _icon_close._height, win_close_button().middle(), "");
            _texture_reset_button.render(_icon_reset.data(_reset), _icon_close._width, _icon_close._height, win_reset_button().middle(), "");
            render_rect(win_close_button(), !_close, { 128, 128, 128 });
            render_rect(win_reset_button(), !_reset, { 128, 128, 128 });
        }
    }
    
    void render_box_on_depth_frame(const rs2::box::wireframe& wireframe, const float line_width = -1.0f)
    {
        _texture_depth.draw_box(wireframe.end_pt, _height, line_width);
    }
    
    void render_box_on_color_frame(const rs2::box::wireframe& wireframe, const float line_width = -1.0f)
    {
        _texture_color.draw_box(wireframe.end_pt, _height, line_width);
    }
    
    void render_box_dim(const std::string& box_dim)
    {
        _texture_box_msg.render(_num_icons.print(box_dim), _num_icons.width(), _num_icons.height(), win_box_msg(), "");
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

#include <json/json.h>
#include <fstream>
static std::shared_ptr<rs2::box_measure::calibration> read_calibration(const std::string& path_name)
{
    if (path_name.size() > 1) try
    {
        rs2::box_measure::calibration calibration = {};
        Json::Value calibration_data;
        std::ifstream infile;
        infile.open(path_name, std::ifstream::binary);
        infile >> calibration_data;
        
        int i = 0;
        for (auto cam : { "depth_cam" , "color_cam" })
        {
            Json::Value intrinsics = calibration_data[cam]["intrinsics"];
            calibration.intrinsics[i].fx = intrinsics["fx"].asFloat();
            calibration.intrinsics[i].fy = intrinsics["fy"].asFloat();
            calibration.intrinsics[i].ppx = intrinsics["ppx"].asFloat();
            calibration.intrinsics[i].ppy = intrinsics["ppy"].asFloat();
            calibration.intrinsics[i].width = intrinsics["width"].asInt();
            calibration.intrinsics[i++].height = intrinsics["height"].asInt();
        }
        
        Json::Value extrinsics = calibration_data["color_cam"]["extrinsics"]["depth_cam"];
        for (auto r : { 0,1,2,3,4,5,6,7,8 })
            calibration.color_to_depth.rotation[r] = extrinsics["rotation"][r].asFloat();
        for (auto t : { 0,1,2 })
            calibration.color_to_depth.translation[t] = extrinsics["translation"][t].asFloat();
        
        printf("calibration read from %s\n", path_name.c_str());
        return std::make_shared<decltype(calibration)>(calibration);
    }
    catch (...) {}
    return std::shared_ptr<rs2::box_measure::calibration>(nullptr);
}

static void save_calibration(const std::string& path_name, rs2::pipeline_profile& p)
{
    if (path_name.size() > 1) try
    {
        auto depth_cam = p.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
        auto color_cam = p.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
        rs2_intrinsics intrinsics[2] = { depth_cam.get_intrinsics(), color_cam.get_intrinsics() };
        
        // write intrinsics
        Json::Value calibration_data;
        int i = 0;
        for (auto cam : { "depth_cam", "color_cam" })
        {
            calibration_data[cam]["intrinsics"]["fx"] = intrinsics[i].fx;
            calibration_data[cam]["intrinsics"]["fy"] = intrinsics[i].fy;
            calibration_data[cam]["intrinsics"]["ppx"] = intrinsics[i].ppx;
            calibration_data[cam]["intrinsics"]["ppy"] = intrinsics[i].ppy;
            calibration_data[cam]["intrinsics"]["width"] = intrinsics[i].width;
            calibration_data[cam]["intrinsics"]["height"] = intrinsics[i++].height;
        }
        
        // write extrinsics
        const auto extrinsics = color_cam.get_extrinsics_to(depth_cam);
        for (auto r : { 0,1,2,3,4,5,6,7,8 })
            calibration_data["color_cam"]["extrinsics"]["depth_cam"]["rotation"][r] = extrinsics.rotation[r];
        for (auto t : { 0,1,2 })
            calibration_data["color_cam"]["extrinsics"]["depth_cam"]["translation"][t] = extrinsics.translation[t];
        
        // write device name
        calibration_data["device"]["name"] = p.get_device().get_info(RS2_CAMERA_INFO_NAME);
        
        // write file
        Json::StyledStreamWriter writer;
        std::ofstream outfile;
        outfile.open(path_name);
        writer.write(outfile, calibration_data);
        
        printf("calibration written to %s\n", path_name.c_str());
    }
    catch (...) {
        printf("error in writing calibration files! \n");
    }
}

using namespace rs2;
class box_depth_stablize
{
public:
    box_depth_stablize(int mov_color_thr = 15, float mov_percent = 0.02f)
    {
        set_thresholds(mov_color_thr, mov_percent);
        _block = std::make_unique<processing_block>([this](frame f, frame_source& fs){proc(f,fs);});
        _block->start(_queue);
    }
    
    frameset operator()(frame f)
    {
        _block->invoke(f);
        frame dst = _queue.wait_for_frame();
        return rs2::frameset(dst);
    }
    
    void set_thresholds(int mov_color_thr, float mov_percent)
    {
        _mov_color_pixel_thr = mov_color_thr;
        _mov_color_pixel_min_percent = mov_percent;
    }
    
protected:
    static void copy_to(const video_frame& src, frame& dst)
    {
        memcpy((void*)dst.get_data(), src.get_data(), src.get_width() * src.get_height() * src.get_bytes_per_pixel());
    }
    
    bool detect_motion_in_color(const video_frame curr_c)
    {
        const int width = curr_c.get_width(), height = curr_c.get_height();
        const int num_pix = width * height;
        const int min_mov_pixel = (int)(num_pix * _mov_color_pixel_min_percent);
        auto* c0 = (uint8_t*)_prev_c.get_data();
        auto* c1 = (uint8_t*)curr_c.get_data();
        for (int i = 0, i3 = 0, num_mov_pix = 0; i < num_pix; ++i, i3+=3 )
        {
            if (std::abs(c0[i3+0]-c1[i3+0]) > _mov_color_pixel_thr ||
                std::abs(c0[i3+1]-c1[i3+1]) > _mov_color_pixel_thr ||
                std::abs(c0[i3+2]-c1[i3+2]) > _mov_color_pixel_thr )
            {
                if(++num_mov_pix > min_mov_pixel)
                {
                    _no_mov_frame_count = 0;
                    return true;
                }
            }
        }
        ++_no_mov_frame_count;
        return false;
    }
    
    void proc(frame f, frame_source& src)
    {
        frameset fs = f.as<frameset>();
        
        depth_frame curr_d = fs.get_depth_frame();
        video_frame curr_c = fs.get_color_frame();
        if (!_prev_d || !_prev_c) {
            _prev_d = src.allocate_video_frame(curr_d.get_profile(), curr_d, 0,0,0,0, RS2_EXTENSION_DEPTH_FRAME);
            _prev_c = src.allocate_video_frame(curr_c.get_profile(), curr_c);
        }
        
        if (detect_motion_in_color(curr_c) || _no_mov_frame_count < 30)
        {
            copy_to(curr_c, _prev_c);
            copy_to(curr_d, _prev_d);
            src.frame_ready(std::move(f));
        }
        else
        {
            std::vector<frame> dst{_prev_d, _prev_c};
            src.frame_ready(src.allocate_composite_frame(std::move(dst)));
        }
    }
    
private:
    std::unique_ptr<processing_block> _block;
    frame _prev_d, _prev_c;
    frame_queue _queue;
    int _no_mov_frame_count = 0;
    int _mov_color_pixel_thr;           // motion color pixel threshold
    float _mov_color_pixel_min_percent; // minimum percent of moving color pixel
};
#endif /* rs_box_app_hpp */
