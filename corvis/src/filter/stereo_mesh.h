#ifndef __STEREO_MESH_H
#define __STEREO_MESH_H

extern "C" {
#include "cor.h"
}
#include "../numerics/vec4.h"
#include "../numerics/matrix.h"

#include <vector>
using namespace std;

typedef struct _image_coordinate {
    f_t x, y;
} image_coordinate;

typedef struct _stereo_triangle {
    int vertices[3];
} stereo_triangle;

typedef struct _stereo_mesh {
    vector<v4> vertices;
    vector<image_coordinate> vertices_image;
    vector<image_coordinate> correspondences_image;
    vector<float> match_scores;
    vector<stereo_triangle> triangles;
} stereo_mesh;

// declared in stereo.h
class stereo_frame;
class stereo;

// F is from s1 to s2
stereo_mesh stereo_mesh_create(const stereo &global, void (*progress_callback)(float));
void stereo_remesh_delaunay(stereo_mesh & mesh);
bool stereo_mesh_triangulate(const stereo_mesh & mesh, const stereo &global, int x, int y, v4 & intersection);

void stereo_mesh_write(const char * result, const stereo_mesh & mesh, const char * texturename);
void stereo_mesh_write_json(const char * filename, const stereo_mesh & mesh, const char * texturename);
void stereo_mesh_write_correspondences(const char * filename, const stereo_mesh & mesh);

#endif
