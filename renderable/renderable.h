// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __TEXTURED_H
#define __TEXTURED_H

#include <vector>
#include <list>
using namespace std;

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "cor.h"
#include "../numerics/vec4.h"
#include "../filter/filter.h"

#include "FTGL/ftgl.h"

struct point {
    float data[3];
};

struct color {
    uint8_t data[4];
};

class renderable {
 protected:
    void transform();
 renderable(): theta(0.) {
        color[0] = color[1] = color[2] = color[3] = 1.;
        origin[0] = origin[1] = origin[2] = 0.;
        theta = axis[0] = axis[1] = 0.;
        axis[2] = 1.;
    }
 public:
    float color[4];
    float origin[3];
    float theta;
    float axis[3];
};

class filter_state: public renderable {
    struct filter *filter;
 public:
    filter_state(struct filter *sfm);
    void render();
};

class label: public renderable {
    FTGLPixmapFont font;
 public:
    float size;
    char *text;
    label(char *font, char *text, float size);
    void render();
};

class line_label {
    FTGLPixmapFont font;
 public:
    point start, stop;
    float size;
    char text[64];
    line_label(char *font, float size);
    void render();
    float get_length();
};

class bounding_box: public renderable {
 public:
    bool show_faces;
    float v[8][3];
    bounding_box(v4 x0, v4 x1, v4 x2, v4 x3, v4 x4, v4 x5, v4 x6, v4 x7);
    void render();
};

class texture: public renderable {
    uint8_t *texture_data;
    GLuint textureid;
    unsigned int texwidth, texheight;
 public:
    texture(char *filename);
    void render();
    float width, height;
};

class structure: public renderable {
    vector<point> features;
    vector<point> current;
    vector<struct color> colors;
 public:
    point3d_vector_t get_features();
    structure(struct mapbuffer *mb = 0);
    void render();
    void new_point(float x, float y, float z);
    void new_intensity(uint8_t);
    void set_current(int count, point c[]);
};

class motion: public renderable {
    vector<point> location;
    float imutheta, imuaxis[3];
 public:
    point3d_vector_t get_path();
    motion(struct mapbuffer *mb = 0);
    void render();
    void new_position(float x, float y, float z, float u, float v, float w);
};

class measurement: public renderable {
    point current, last;
    float total;
    char total_text[64];
    line_label horiz, vert, straight;
    label total_label;
 public:
    measurement(struct mapbuffer *mb, char *font, float size);
    void render();
    void new_position(float x, float y, float z);
};

#ifdef SWIG
%callback("%s_cb");
#endif
void structure_packet(void *obj, packet_t *p);
void measurement_packet(void *obj, packet_t *p);
void motion_packet(void *obj, packet_t *p);
#ifdef SWIG
%nocallback;
#endif

#endif
