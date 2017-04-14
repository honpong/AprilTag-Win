// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <vector>
#include <algorithm>
#include <cstring>
#include <ctype.h>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <math.h>

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


inline float clamp(float x, float min, float max)
{
    return std::max(std::min(max, x), min);
}

inline float smoothstep(float x, float min, float max)
{
    x = clamp((x - min) / (max - min), 0.0, 1.0);
    return x*x*(3 - 2 * x);
}

inline float lerp(float a, float b, float t)
{
    return b * t + a * (1 - t);
}

struct float3
{
    float x, y, z;
};

struct float2
{
    float x, y;
};

struct rect
{
    float x, y;
    float w, h;

    bool operator==(const rect& other) const
    {
        return x == other.x && y == other.y && w == other.w && h == other.h;
    }

    rect center() const
    {
        return{ x + w / 2, y + h / 2, 0, 0 };
    }

    rect lerp(float t, const rect& other) const
    {
        return{
            ::lerp(x, other.x, t), ::lerp(y, other.y, t),
            ::lerp(w, other.w, t), ::lerp(h, other.h, t),
        };
    }

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

#include "stb_easy_font.h"

inline void draw_text(int x, int y, const char * text)
{
    char buffer[60000]; // ~300 chars
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, 4 * stb_easy_font_print((float)x, (float)(y - 7), (char *)text, nullptr, buffer, sizeof(buffer)));
    glDisableClientState(GL_VERTEX_ARRAY);
}

////////////////////////
// Image display code //
////////////////////////

class texture_buffer
{
    GLuint texture;
    std::vector<uint8_t> rgb;

public:
    texture_buffer() : texture() {}


    GLuint get_gl_handle() const { return texture; }

    void draw_axis()
    {

        // Traingles For X axis
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(1.1f, 0.0f, 0.0f);
        glVertex3f(1.0f, 0.05f, 0.0f);
        glVertex3f(1.0f, -0.05f, 0.0f);
        glEnd();

        // Traingles For Y axis
        glBegin(GL_TRIANGLES);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, -1.1f, 0.0f);
        glVertex3f(0.0f, -1.0f, 0.05f);
        glVertex3f(0.0f, -1.0f, -0.05f);
        glEnd();
        glBegin(GL_TRIANGLES);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, -1.1f, 0.0f);
        glVertex3f(0.05f, -1.0f, 0.0f);
        glVertex3f(-0.05f, -1.0f, 0.0f);
        glEnd();

        // Traingles For Z axis
        glBegin(GL_TRIANGLES);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 1.1f);
        glVertex3f(0.0f, 0.05f, 1.0f);
        glVertex3f(0.0f, -0.05f, 1.0f);
        glEnd();

        auto axisWidth = 4;
        glLineWidth((float)axisWidth);

        // Drawing Axis
        glBegin(GL_LINES);
        // X axis - Red
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(1.0f, 0.0f, 0.0f);

        // Y axis - Green
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, -1.0f, 0.0f);

        // Z axis - White
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 1.0f);
        glEnd();
    }

    void draw_cyrcle(float xx, float xy, float xz, float yx, float yy, float yz, float radius = 1.1)
    {
        const auto N = 50;
        glColor3f(0.5f, 0.5f, 0.5f);
        glLineWidth(2);
        glBegin(GL_LINE_STRIP);

        for (int i = 0; i <= N; i++)
        {
            const double theta = (2 * M_PI / N) * i;
            const auto cost = (float)cos(theta);
            const auto sint = (float)sin(theta);
            glVertex3f(
                radius * (xx * cost + yx * sint),
                radius * (xy * cost + yy * sint),
                radius * (xz * cost + yz * sint)
                );
        }

        glEnd();
    }

    void multiply_vector_by_matrix(GLfloat vec[], GLfloat mat[], GLfloat* result)
    {
        const auto N = 4;
        for (int i = 0; i < N; i++)
        {
            result[i] = 0;
            for (int j = 0; j < N; j++)
            {
                result[i] += vec[j] * mat[N*j + i];
            }
        }
        return;
    }

    float2 xyz_to_xy(float x, float y, float z, GLfloat model[], GLfloat proj[], float vec_norm)
    {
        GLfloat vec[4] = { x, y, z, 0 };
        float tmp_result[4];
        float result[4];

        const auto canvas_size = 230;

        multiply_vector_by_matrix(vec, model, tmp_result);
        multiply_vector_by_matrix(tmp_result, proj, result);

        return{ canvas_size * vec_norm *result[0], canvas_size * vec_norm *result[1] };
    }

    void print_text_in_3d(float x, float y, float z, const char* text, bool center_text, GLfloat model[], GLfloat proj[], float vec_norm)
    {
        auto xy = xyz_to_xy(x, y, z, model, proj, vec_norm);
        auto w = (center_text) ? stb_easy_font_width((char*)text) : 0;
        glColor3f(1.0f, 1.0f, 1.0f);
        draw_text((int)(xy.x - w / 2), (int)xy.y, text);
    }

    void draw_motion_data(float x, float y, float z)
    {
        glViewport(0, 0, 1024, 1024);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glOrtho(-2.8, 2.8, -2.4, 2.4, -7, 7);

        glRotatef(-25, 1.0f, 0.0f, 0.0f);

        glTranslatef(0, 0.33f, -1.f);

        float norm = std::sqrt(x*x + y*y + z*z);

        glRotatef(-45, 0.0f, 1.0f, 0.0f);

        draw_axis();
        draw_cyrcle(1, 0, 0, 0, 1, 0);
        draw_cyrcle(0, 1, 0, 0, 0, 1);
        draw_cyrcle(1, 0, 0, 0, 0, 1);

        const auto vec_threshold = 0.01f;
        if ( norm < vec_threshold )
        {
            const auto radius = 0.05;
            static const int circle_points = 100;
            static const float angle = 2.0f * 3.1416f / circle_points;

            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_POLYGON);
            double angle1 = 0.0;
            glVertex2d(radius * cos(0.0), radius * sin(0.0));
            int i;
            for (i = 0;  i < circle_points; i++)
            {
                glVertex2d(radius * cos(angle1), radius *sin(angle1));
                angle1 += angle;
            }
            glEnd();
        }
        else
        {
            auto vectorWidth = 5;
            glLineWidth((float)vectorWidth);
            glBegin(GL_LINES);
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(x / norm, y / norm, z / norm);
            glEnd();

            // Save model and projection matrix for later
            GLfloat model[16];
            glGetFloatv(GL_MODELVIEW_MATRIX, model);
            GLfloat proj[16];
            glGetFloatv(GL_PROJECTION_MATRIX, proj);

            const auto canvas_size = 230;
            glLoadIdentity();
            glOrtho(-canvas_size, canvas_size, -canvas_size, canvas_size, -1, +1);

            std::ostringstream s1;
            const auto presicion = 3;

            s1 << "(" << std::fixed << std::setprecision(presicion) << x << "," << std::fixed << std::setprecision(presicion) << y << "," << std::fixed << std::setprecision(presicion) << z << ")";
            print_text_in_3d(x, y, z, s1.str().c_str(), false, model, proj, 1/norm);

            std::ostringstream s2;
            s2 << std::setprecision(presicion) << norm;
            print_text_in_3d(x / 2, y / 2, z / 2, s2.str().c_str(), true, model, proj, 1/norm);
        }

        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 1024, 1024, 0);
    }

    double t = 0;

    void draw_gyro_texture(const void * data)
    {
        const static float gyro_range   = 1000.f;                   // Preconfigured angular velocity range [-1000...1000] Deg_C/Sec
        const static float gyro_transform_factor = float((gyro_range * M_PI) / (180.f * 32767.f));
        auto shrt = (short*)data;
        auto x = static_cast<float>(shrt[0]) * gyro_transform_factor;
        auto y = static_cast<float>(shrt[1]) * gyro_transform_factor;
        auto z = static_cast<float>(shrt[2]) * gyro_transform_factor;



        draw_motion_data(x, y, z);
    }

    void draw_accel_texture(const void * data)
    {
        const static float gravity = 9.80665f; // Standard Gravitation Acceleration
        const static float accel_range = 4.f;                       // Accelerometer is preset to [-4...+4]g range
        const static float accelerator_transform_factor = float(gravity * accel_range / 2048.f);

        auto shrt = (short*)data;
        auto x = static_cast<float>(shrt[0]) * accelerator_transform_factor;
        auto y = static_cast<float>(shrt[1]) * accelerator_transform_factor;
        auto z = static_cast<float>(shrt[2]) * accelerator_transform_factor;
        draw_motion_data(x, y, z);
    }

    void upload(const void * data, int width, int height, rs_format format, int stride = 0, rs_stream stream = RS_STREAM_ANY)
    {
        // If the frame timestamp has changed since the last time show(...) was called, re-upload the texture
        if(!texture)
            glGenTextures(1, &texture);

        glBindTexture(GL_TEXTURE_2D, texture);
        stride = stride == 0 ? width : stride;
        //glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);

        switch(format)
        {
        case RS_FORMAT_ANY:
        throw std::runtime_error("not a valid format");
        case RS_FORMAT_Z16:
        case RS_FORMAT_DISPARITY16:
            rgb.resize(width * height * 4);
            make_depth_histogram(rgb.data(), reinterpret_cast<const uint16_t *>(data), width, height);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            
            break;
        case RS_FORMAT_XYZ32F:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, data);
            break;
        case RS_FORMAT_YUYV: // Display YUYV by showing the luminance channel and packing chrominance into ignored alpha channel
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_RGB8: case RS_FORMAT_BGR8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_RGBA8: case RS_FORMAT_BGRA8: // Display both RGBA and BGRA by interpreting them RGBA, to show the flipped byte ordering. Obviously, GL_BGRA could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_Y8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_MOTION_DATA:
            switch (stream) {
            case RS_STREAM_GYRO:
                draw_gyro_texture(data);
                break;
            case RS_STREAM_ACCEL:
                draw_accel_texture(data);
                break;
            default:
                throw std::runtime_error("Motion data stream not found!");
                break;
            }
            break;
        case RS_FORMAT_Y16:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
            break;
        case RS_FORMAT_RAW8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_RAW10:
            {
                // Visualize Raw10 by performing a naive downsample. Each 2x2 block contains one red pixel, two green pixels, and one blue pixel, so combine them into a single RGB triple.
                rgb.clear(); rgb.resize(width/2 * height/2 * 3);
                auto out = rgb.data(); auto in0 = reinterpret_cast<const uint8_t *>(data), in1 = in0 + width*5/4;
                for(auto y=0; y<height; y+=2)
                {
                    for(auto x=0; x<width; x+=4)
                    {
                        *out++ = in0[0]; *out++ = (in0[1] + in1[0]) / 2; *out++ = in1[1]; // RGRG -> RGB RGB
                        *out++ = in0[2]; *out++ = (in0[3] + in1[2]) / 2; *out++ = in1[3]; // GBGB
                        in0 += 5; in1 += 5;
                    }
                    in0 = in1; in1 += width*5/4;
                }
                glPixelStorei(GL_UNPACK_ROW_LENGTH, width / 2);        // Update row stride to reflect post-downsampling dimensions of the target texture
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width/2, height/2, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            }
            break;
        default:
            throw std::runtime_error("The requested format is not provided by demo");
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void upload(rs::frame& frame)
    {
        upload(frame.get_data(), frame.get_width(), frame.get_height(), frame.get_format(), 
            (frame.get_stride_in_bytes() * 8) / frame.get_bits_per_pixel(), frame.get_stream_type());
    }

    void show(const rect& r, float alpha) const
    {
        glEnable(GL_BLEND);

        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 1 - alpha);
        glVertex2f(r.x, r.y);
        glVertex2f(r.x + r.w, r.y);
        glVertex2f(r.x + r.w, r.y + r.h);
        glVertex2f(r.x, r.y + r.h);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, texture);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(r.x, r.y);
        glTexCoord2f(1, 0); glVertex2f(r.x + r.w, r.y);
        glTexCoord2f(1, 1); glVertex2f(r.x + r.w, r.y + r.h);
        glTexCoord2f(0, 1); glVertex2f(r.x, r.y + r.h);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDisable(GL_BLEND);
    }
};

inline void draw_depth_histogram(const uint16_t depth_image[], int width, int height)
{
    static uint8_t rgb_image[640*480*3];
    make_depth_histogram(rgb_image, depth_image, width, height);
    glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb_image);
}

inline bool is_integer(float f)
{
    return abs(f - floor(f)) < 0.001f;
}

struct to_string
{
    std::ostringstream ss;
    template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
    operator std::string() const { return ss.str(); }
};

inline std::string error_to_string(const rs::error& e)
{
    return to_string() << e.get_failed_function() << "("
        << e.get_failed_args() << "):\n" << e.what();
}
