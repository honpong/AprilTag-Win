// Dorian, October 2017

#ifndef PLY_WORLD_H
#define PLY_WORLD_H

#include <vector>
#include <string>
#include "transformation.h"

/** Minimal support to write ply files for 3d viewing.
 */
class ply_world {
 public:
    using color = Eigen::Matrix<int, 3, 1>;  // [0, 255]
    static const color BLACK;
    static const color RED;
    static const color GREEN;
    static const color BLUE;
    static const color YELLOW;

    size_t add_point(const v3& position, const color& c = BLUE);
    void add_line(const v3& a, const v3& b, const color& c);
    void add_cube(const transformation& pose, const color& c, float side_length);
    void add_axes(const transformation& pose, float axis_length);
    void clear() { points_.clear(); edges_.clear(); faces_.clear(); }
    bool empty() const { return points_.empty(); }
    void save(const std::string& filename) const;

 private:
    static v3 make_v3(v3::Scalar x, v3::Scalar y, v3::Scalar z) {
        return (v3() << x, y, z).finished();
    }

    void add_edge(size_t pt_idx0, size_t pt_idx1, const color& c);
    void add_face(std::vector<size_t>&& pt_indices);

    struct point {
        v3 p;
        color c;
    };

    struct edge {
        size_t pt_idx0, pt_idx1;
        color c;
    };

    using face = std::vector<size_t>;

    std::vector<point> points_;
    std::vector<edge> edges_;
    std::vector<face> faces_;
};

#endif // PLY_WORLD_H
