// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

extern "C" {
#include "cor/cor.h"
}
#include "renderable.h"

const static int point_size = 2.;
const static float line_width = 3;

void structure::new_point(float x, float y, float z)
{
    features.push_back((point){{x, y, z}});
}

void structure::new_intensity(uint8_t intensity)
{
    colors.push_back((struct color){{intensity, intensity, intensity, intensity}});
}

void measurement::new_position(float x, float y, float z)
{
    last = current;
    current.data[0] = x;
    current.data[1] = y;
    current.data[2] = z;
    
    double
        dx = x - last.data[0],
        dy = y - last.data[1],
        dz = z - last.data[2];

    total += sqrt(dx*dx + dy*dy + dz*dz);
}

void motion::new_position(float x, float y, float z, float u, float v, float w)
{
    location.push_back((point){{x, y, z}});
    imutheta = sqrt(u*u + v*v + w*w);
    imuaxis[0] = u / imutheta;
    imuaxis[1] = v / imutheta;
    imuaxis[2] = w / imutheta;
}

void structure::set_current(int points, point c[])
{
    current.resize(points);
    for(int i = 0; i < points; ++i) {
        current[i] = c[i];
    }
}

void measurement_packet(void *_t, packet_t *p)
{
    measurement *t = (measurement *) _t;
    switch(p->header.type) {
    case packet_filter_position: {
        packet_filter_position_t *pos = (packet_filter_position_t *)p;
        t->new_position(pos->position[0], pos->position[1], pos->position[2]);
        break;
    }
    default:
        return;
    }
}


void motion_packet(void *_t, packet_t *p)
{
    motion *t = (motion *) _t;
    switch(p->header.type) {
    case packet_filter_position: {
        packet_filter_position_t *pos = (packet_filter_position_t *)p;
        t->new_position(pos->position[0], pos->position[1], pos->position[2], pos->orientation[0], pos->orientation[1], pos->orientation[2]);
        break;
    }
    default:
        return;
    }
}

void structure_packet(void *_t, packet_t *p)
{
    structure *t = (structure *)_t;
    switch(p->header.type) {
    case packet_filter_reconstruction: {
        packet_filter_reconstruction_t *r = (packet_filter_reconstruction_t *)p;
        for(int i = 0; i < p->header.user; ++i) {
            t->new_point(r->points[i][0], r->points[i][1], r->points[i][2]);
        }
        break;
    }
    case packet_filter_current: {
        packet_filter_current_t *r = (packet_filter_current_t *)p;
        t->set_current(r->header.user, (point *)r->points);
        break;
    }
    case packet_feature_intensity: {
        packet_feature_intensity_t *r = (packet_feature_intensity_t *)p;
        for(int i = 0; i < p->header.user; ++i) {
            t->new_intensity(r->intensity[i]);
        }
        break;
    }
    default:
        return;
    }
}

void renderable::transform()
{
    glColor4fv(color);
    glRotatef(theta, axis[0], axis[1], axis[2]);
    glTranslatef(origin[0], origin[1], origin[2]);
}

void texture::render()
{
    glPushMatrix(); {
        transform();
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureid);
        glBegin(GL_QUADS);
        glTexCoord2f(0., 0.); glVertex3f(0., 0., 0.);
        glTexCoord2f(1., 0.); glVertex3f(width, 0., 0.);
        glTexCoord2f(1., 1.); glVertex3f(width, -height, 0.);
        glTexCoord2f(0., 1.); glVertex3f(0., -height, 0.);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    } glPopMatrix();
}

//draw this as a vertex array - 8 vertices, and draw both triangles (for quads) and lines
void bounding_box::render()
{
    //triangles
    GLubyte faces[] = { 0,1,2, 2,3,0,
                        4,5,6, 6,7,4,
                        0,4,1, 0,7,4,
                        1,4,5, 5,2,1,
                        2,5,6, 6,3,2,
                        3,6,7, 7,0,3 };

    GLubyte edges[] = { 0,1, 1,2, 2,3, 3,0,
                        4,5, 5,6, 6,7, 7,4,
                        0,4, 1,5, 2,6, 3,7 };


    glLineWidth(line_width);
    glPointSize(point_size);
    glPushMatrix(); {
        transform();
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, v);
        if(show_faces) glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, faces);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_BYTE, edges);
        glDisableClientState(GL_VERTEX_ARRAY);
    } glPopMatrix();
}

void label::render()
{
    glPushMatrix(); {
        transform();
        // Set the font size and render a small text.
        font.FaceSize(size);
        glRasterPos3f(0., 0., 0.);
        font.Render(text);
    } glPopMatrix();
}

label::label(char *fontname, char *_text, float _size): font(fontname), size(_size), text(_text)
{
     // If something went wrong, bail out.
    if(font.Error())
        fprintf(stderr, "Couldn't initialize my font\n");
}

float line_label::get_length()
{
    float dx = (stop.data[0] - start.data[0]);
    float dy = (stop.data[1] - start.data[1]);
    float dz = (stop.data[2] - start.data[2]);
    return sqrt(dx*dx + dy*dy + dz*dz); 
}

void line_label::render()
{
    glPushMatrix(); {
        glBegin(GL_LINES);
        glVertex3f(start.data[0], start.data[1], start.data[2]);
        glVertex3f(stop.data[0], stop.data[1], stop.data[2]);
        glEnd();
        font.FaceSize(size);
        float dx = (stop.data[0] - start.data[0]);
        float dy = (stop.data[1] - start.data[1]);
        float dz = (stop.data[2] - start.data[2]);
        glRasterPos3f(start.data[0] + dx * .5, start.data[1] + dy * .5, start.data[2] + dz * .5);
        sprintf(text, "%0.02fm", sqrt(dx*dx + dy*dy + dz*dz)); 
        font.Render(text);
    } glPopMatrix();
}

line_label::line_label(char *fontname, float _size): font(fontname), size(_size)
{
     // If something went wrong, bail out.
    if(font.Error())
        fprintf(stderr, "Couldn't initialize my font\n");
}

bounding_box::bounding_box(v4 x0, v4 x1, v4 x2, v4 x3, v4 x4, v4 x5, v4 x6, v4 x7): show_faces(true)
{
    for(int i = 0; i < 3; ++i) {
        v[0][i] = x0[i];
        v[1][i] = x1[i];
        v[2][i] = x2[i];
        v[3][i] = x3[i];
        v[4][i] = x4[i];
        v[5][i] = x5[i];
        v[6][i] = x6[i];
        v[7][i] = x7[i];
    }
}

texture::texture(char *filename)
{
    GLint texsize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texsize);
    fprintf(stderr, "maximum texture size is %d\n", texsize);
    FILE *tf = fopen(filename, "rb");
    if(!tf) {
        fprintf(stderr, "couldn't open texture file %s\n", filename);
        return;
    }
    uint8_t spc;
    if(fscanf(tf, "P5 %d %d 255%c", &texwidth, &texheight, &spc) != 3) {
        fprintf(stderr, "bad pgm header in file %s\n", filename);
        return;
    }
    texture_data = new uint8_t[texwidth * texheight];
    if(fread(texture_data, texwidth, texheight, tf) != texheight) {
        fprintf(stderr, "didn't get enough image data in file %s\n", filename);
        return;
    }
    glGenTextures(1, &textureid);
    glBindTexture(GL_TEXTURE_2D, textureid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, texwidth, texheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, texture_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

measurement::measurement(char *font, float size): horiz(font, size), vert(font, size), straight(font, size), total_label(font, total_text, size) {
    total_label.color[0] = 0.;
    total_label.color[1] = 0.;
    total_label.color[2] = 1.;
    total_label.color[3] = 1.;
    for(int i = 0; i < 3; ++i) {
        current.data[i] = last.data[i] = 0.;
    }
}

filter_state::filter_state(struct filter *sfm) {
    filter = sfm;
}

void filter_state::render() {
    glLineWidth(line_width);
    glPointSize(point_size);
    glPushMatrix(); {
        transform();

        double u = filter->s.W.v.raw_vector()[0],
            v = filter->s.W.v.raw_vector()[1],
            w = filter->s.W.v.raw_vector()[2],
            sfmtheta = sqrt(u*u + v*v + w*w);
        glTranslatef(filter->s.T.v[0], filter->s.T.v[1], filter->s.T.v[2]);
        if(sfmtheta) glRotatef(sfmtheta * 180. / M_PI, u / sfmtheta, v / sfmtheta, w / sfmtheta);


        /* iphone 5 size
        Height: 4.87 inches (123.8 mm)
        Width: 2.31 inches (58.6 mm)
        Depth: 0.30 inch (7.6 mm)
        */

        // Front
        glBegin(GL_QUADS);
        glColor4f(1., 1., 1., 1);
        glVertex3f( 0.0293, -0.06019, 0.0038);
        glVertex3f( 0.0293,  0.06019, 0.0038);
        glVertex3f(-0.0293,  0.06019, 0.0038);
        glVertex3f(-0.0293, -0.06019, 0.0038);

        glColor4f(0., 0, 0., 1);
        glVertex3f( 0.0273, -0.05019, 0.00382);
        glVertex3f( 0.0273,  0.05019, 0.00382);
        glVertex3f(-0.0273,  0.05019, 0.00382);
        glVertex3f(-0.0273, -0.05019, 0.00382);

        // Back
        glColor4f(.5, .5, .5, 1);
        glVertex3f( 0.0293, -0.06019, -0.0038);
        glVertex3f( 0.0293,  0.06019, -0.0038);
        glVertex3f(-0.0293,  0.06019, -0.0038);
        glVertex3f(-0.0293, -0.06019, -0.0038);
        glEnd();

        glBegin(GL_LINES);
        glColor3f(1., 0., 0.);
        glVertex3f(0, 0, 0);
        glVertex3f(.2, 0, 0);
        glColor3f(0., 1, 0.);
        glVertex3f(0, 0, 0);
        glVertex3f(0, .2, 0);
        glColor3f(0., 0., 1.);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, .2);
        glEnd();
        
        u = filter->s.Wc.v[0];
        v = filter->s.Wc.v[1];
        w = filter->s.Wc.v[2];
        sfmtheta = sqrt(u*u + v*v + w*w);
        glTranslatef(filter->s.Tc.v[0], filter->s.Tc.v[1], filter->s.Tc.v[2]);
        if(sfmtheta) glRotatef(sfmtheta * 180. / M_PI, u / sfmtheta, v / sfmtheta, w / sfmtheta);

        glBegin(GL_LINES);
        glColor3f(1., 0., 0.);
        glVertex3f(0, 0, 0);
        glVertex3f(.1, 0, 0);
        glColor3f(0., 1, 0.);
        glVertex3f(0, 0, 0);
        glVertex3f(0, .1, 0);
        glColor3f(0., 0., 1.);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, .2);
        glEnd();
    } glPopMatrix();
}

point3d_vector_t structure::get_features()
{
    point3d_vector_t rv= { features.size(), (float *)&features[0] };
    return rv;
}

point3d_vector_t motion::get_path()
{
    point3d_vector_t rv = { location.size(), (float *)&location[0] };
    return rv;
}

void measurement::render()
{
    glLineWidth(line_width);
    glPointSize(point_size);
    sprintf(total_text, "Path: %0.02f", total);
    total_label.origin[0] = current.data[0];
    total_label.origin[1] = current.data[1];
    total_label.origin[2] = current.data[2];
    //    total_label.render();
    glPushMatrix(); {
        transform();
        horiz.start = (point){{0., 0., 0.}};
        horiz.stop = (point){{current.data[0], current.data[1], 0.}};
        horiz.render();
        float hlen = horiz.get_length();

        vert.start = (point){{current.data[0], current.data[1], 0.}};
        vert.stop = current;
        straight.start = (point){{0., 0., 0.}};
        straight.stop = current;
        
        float vlen = vert.get_length();
        if(vlen > 0.75 || (vlen / hlen > .20)) {
            vert.render();
            straight.render();
        }
        
        /*    glBegin(GL_LINES);
        glVertex3f(0., 0., 0.);
        glVertex3f(current.data[0], current.data[1], current.data[2]);

        glVertex3f(0., 0., 0.);
        glVertex3f(current.data[0], current.data[1], 0.);
        
        glVertex3f(current.data[0], current.data[1], 0.);
        glVertex3f(current.data[0], current.data[1], current.data[2]);
        glEnd();*/
    } glPopMatrix();
}    

void motion::render()
{
    glLineWidth(line_width);
    glPointSize(point_size);
    glPushMatrix(); {
        transform();
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, &location[0]);
        glDrawArrays(GL_LINE_STRIP, 0, location.size());
        glDisableClientState(GL_VERTEX_ARRAY);
    } glPopMatrix();
}

void structure::render()
{
    glLineWidth(line_width);
    glPointSize(point_size);
    glPushMatrix(); {
        transform();
        glEnableClientState(GL_VERTEX_ARRAY);
        //glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, &features[0]);
        //glColorPointer(4, GL_UNSIGNED_BYTE, 0, &colors[0]);
        glDrawArrays(GL_POINTS, 0, features.size());
        //glDisableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, &current[0]);
        glDrawArrays(GL_POINTS, 0, current.size());
        glDisableClientState(GL_VERTEX_ARRAY);
    } glPopMatrix();
}

