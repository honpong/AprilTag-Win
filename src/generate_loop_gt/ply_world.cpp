#include <fstream>
#include "ply_world.h"

const ply_world::color ply_world::BLACK = (ply_world::color() << 0, 0, 0).finished();
const ply_world::color ply_world::RED = (ply_world::color() << 255, 0, 0).finished();
const ply_world::color ply_world::GREEN = (ply_world::color() << 0, 255, 0).finished();
const ply_world::color ply_world::BLUE = (ply_world::color() << 0, 0, 255).finished();
const ply_world::color ply_world::YELLOW = (ply_world::color() << 255, 255, 0).finished();

size_t ply_world::add_point(const v3 &position, const color& c) {
    points_.push_back({position, c});
    return points_.size() - 1;
}

void ply_world::add_axes(const transformation &pose, float axis_length) {
    v3 origin = pose * make_v3(0, 0, 0);
    v3 x = pose * make_v3(axis_length, 0, 0);
    v3 y = pose * make_v3(0, axis_length, 0);
    v3 z = pose * make_v3(0, 0, axis_length);
    add_line(origin, x, RED);
    add_line(origin, y, GREEN);
    add_line(origin, z, BLUE);
}

void ply_world::add_line(const v3& a, const v3& b, const color& c) {
    // rasterized line because meshlab does not show edges correctly
    constexpr float gap = 0.001;
    int steps = std::ceil((b - a).norm() / gap);
    v3 step = (b - a) / steps;
    v3 next = a;
    for (int i = 0; i < steps; ++i, next += step)
        add_point(next, c);
}

void ply_world::add_cube(const transformation& pose, const color& c, float side_length) {
    const float L2 = side_length / 2.f;
    float x0 = (pose * make_v3(-L2, 0, 0)).x();
    float x1 = (pose * make_v3(+L2, 0, 0)).x();
    float y0 = (pose * make_v3(0, -L2, 0)).y();
    float y1 = (pose * make_v3(0, +L2, 0)).y();
    float z0 = (pose * make_v3(0, 0, -L2)).z();
    float z1 = (pose * make_v3(0, 0, +L2)).z();

    size_t i0 = add_point(make_v3(x1, y1, z1), c);
    size_t i1 = add_point(make_v3(x0, y1, z1), c);
    size_t i2 = add_point(make_v3(x1, y0, z1), c);
    size_t i3 = add_point(make_v3(x1, y1, z0), c);
    size_t i4 = add_point(make_v3(x0, y0, z0), c);
    size_t i5 = add_point(make_v3(x1, y0, z0), c);
    size_t i6 = add_point(make_v3(x0, y1, z0), c);
    size_t i7 = add_point(make_v3(x0, y0, z1), c);

    add_face({i0, i3, i6, i1});
    add_face({i0, i2, i5, i3});
    add_face({i0, i1, i7, i2});
    add_face({i3, i5, i4, i6});
    add_face({i2, i7, i4, i5});
    add_face({i1, i6, i4, i7});
}

void ply_world::add_edge(size_t pt_idx0, size_t pt_idx1, const color& c) {
    edges_.push_back({pt_idx0, pt_idx1, c});
}

void ply_world::add_face(std::vector<size_t>&& pt_indices) {
    faces_.emplace_back(pt_indices);
}

void ply_world::save(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "ply\n"
            "format ascii 1.0\n"
            "element vertex " << points_.size() << "\n"
            "property float x\n"
            "property float y\n"
            "property float z\n"
            "property uchar red\n"
            "property uchar green\n"
            "property uchar blue\n"
            "element face " << faces_.size() << "\n"
            "property list uchar int vertex_index\n"
            "element edge " << edges_.size() << "\n"
            "property int vertex1\n"
            "property int vertex2\n"
            "property uchar red\n"
            "property uchar green\n"
            "property uchar blue\n"
            "end_header\n";

    for (const point& pt : points_) {
        file << pt.p.x() << " " << pt.p.y() << " " << pt.p.z() << " " <<
                pt.c.x() << " " << pt.c.y() << " " << pt.c.z() << "\n";
    }
    for (const face& f : faces_) {
        file << f.size() << " ";
        for (size_t idx : f) {
            file << idx << " ";
        }
        file << "\n";
    }
    for (const edge& e : edges_) {
        file << e.pt_idx0 << " " << e.pt_idx1 << " " <<
                e.c.x() << " " << e.c.y() << " " << e.c.z() << "\n";
    }
}
