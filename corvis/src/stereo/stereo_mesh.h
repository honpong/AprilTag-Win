#ifndef __STEREO_MESH_H
#define __STEREO_MESH_H

#include "vec4.h"
#include "matrix.h"

#include <vector>

typedef struct _image_coordinate {
    f_t x, y;
} image_coordinate;

typedef struct _stereo_triangle {
    int vertices[3];
} stereo_triangle;

typedef struct _stereo_mesh {
    std::vector<v3> vertices;
    std::vector<image_coordinate> vertices_image;
    std::vector<image_coordinate> correspondences_image;
    std::vector<float> match_scores;
    std::vector<stereo_triangle> triangles;
} stereo_mesh;

// declared in stereo.h
class stereo;

// F is from s1 to s2
stereo_mesh stereo_mesh_create(const stereo &global, void (*progress_callback)(float), float progress_start, float progress_end);
void stereo_remesh_delaunay(stereo_mesh & mesh);
bool stereo_mesh_triangulate(const stereo_mesh & mesh, const stereo &global, int x, int y, v3 & intersection);

void stereo_mesh_write(const char * result, const stereo_mesh & mesh, const char * texturename);
void stereo_mesh_write_rotated_json(const char * filename, const stereo_mesh & mesh, const stereo & global, int degrees, const char * texturename);
void stereo_mesh_write_correspondences(const char * filename, const stereo_mesh & mesh);
void stereo_mesh_write_topn_correspondences(const char * filename);
void stereo_mesh_write_unary(const char * filename);
void stereo_mesh_write_pairwise(const char * filename);

#endif
