#ifndef __STEREO_MESH_H
#define __STEREO_MESH_H

#include "stereo.h"

typedef struct _image_coordinate {
    f_t x, y;
} image_coordinate;

typedef struct _stereo_triangle {
    int vertices[3];
} stereo_triangle;

typedef struct _stereo_mesh {
    vector<v4> vertices;
    vector<image_coordinate> vertices_image;
    vector<stereo_triangle> triangles;
} stereo_mesh;

stereo_mesh stereo_mesh_states(const stereo_state & s1, const stereo_state & s2, m4 F, void (*progress_callback)(float));
void stereo_remesh_delaunay(stereo_mesh & mesh);
bool stereo_triangulate_mesh(const stereo_state & s1, const stereo_state & s2, const stereo_mesh & mesh, int s2_x1, int s2_y1, v4 & intersection);

void stereo_mesh_write(const char * result, const stereo_mesh & mesh, const char * texturename);

#endif
