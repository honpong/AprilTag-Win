#pragma once

#include "transformation.h"

struct Essential {
    m3 E = m3::Identity();
    float error = std::numeric_limits<f_t>::infinity();
};

bool estimate_transformation(const aligned_vector<v3> & src, const aligned_vector<v3> & dst, transformation & transform);
f_t  estimate_transformation(const aligned_vector<v3> &P, const aligned_vector<v2> &p, transformation &transform);
f_t  estimate_five_point(const aligned_vector<v2> &src, const aligned_vector<v2> &dst, std::vector<Essential>& Ev);
#include <random> // FIXME: remove these includes and use template parameters
#include <set>
f_t estimate_transformation(const aligned_vector<v3> &src, const aligned_vector<v2> &dst, transformation &transform, std::minstd_rand &gen,
                            int max_iterations = 20, f_t max_reprojection_error = .00001f, f_t confidence = .90f, unsigned min_matches = 6,
                            std::set<size_t> *inliers = nullptr);
f_t estimate_3d_point(const aligned_vector<v2> &src, const std::vector<transformation> &camera_poses, f_t &depth_m);
f_t estimate_fundamental(const aligned_vector<v2> &src, const aligned_vector<v2> &dst, m3 &fundamental, std::minstd_rand &gen,
                         int max_iterations = 40, f_t max_reprojection_error = 1, f_t confidence = .90f, unsigned min_matches = 8,
                         std::set<size_t> *inliers = nullptr);
f_t estimate_five_point(const aligned_vector<v2> &src, const aligned_vector<v2> &dst, m3 &E, std::minstd_rand &gen,
                        int max_iterations = 20, f_t max_reprojection_error = .00001f, f_t confidence = .90f, unsigned min_matches = 5,
                        std::set<size_t> *inliers = nullptr);
