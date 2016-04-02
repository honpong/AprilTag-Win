#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include "../cor/platform/sensor_clock.h"
#include "../numerics/transformation.h"

struct tpose_raw {
    uint64_t t_100ns; m4 R; v4 T_mm;
    tpose_raw() : t_100ns(0), R(m4::Identity()), T_mm(v4::Zero()) {}
    tpose_raw(const char *line) : tpose_raw() {
        size_t end = 0;
        t_100ns = std::stoll(line+=end, &end);
        for (int i=0; i<3; i++) {
            for (int j=0; j<3; j++)
                R(i,j) = std::stod(line+=end, &end);
            T_mm(i) = std::stod(line+=end, &end);
        }
    }
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct tpose {
    tpose(const tpose_raw &r) : t(sensor_clock::ns100_to_tp(r.t_100ns)), G(to_quaternion(r.R), r.T_mm / 1000) {}
    tpose(sensor_clock::time_point t_) : t(t_) {}
    sensor_clock::time_point t;
    transformation G;
    bool operator<(const struct tpose &tp) const {
        return t < tp.t;
    };
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct tpose_sequence {
    aligned_vector<tpose> tposes;
    f_t get_length() {
        return tposes.empty() ? 0 : (tposes.front().G.T - tposes.back().G.T).norm();
    }
    f_t get_path_length() {
        f_t total_distance = 0;
        if (!tposes.empty()) {
            v4 last_position = tposes.front().G.T;
            for (auto &tp : tposes) {
                f_t delta_T = (tp.G.T - last_position).norm();
                if (delta_T > .01) {
                    total_distance += delta_T;
                    last_position = tp.G.T;
                }
            }
        }
        return total_distance;
    }
    tpose get_pose(sensor_clock::time_point t) {
        tpose pt {t};
        auto i = std::lower_bound(tposes.begin(), tposes.end(), pt);
        // FIXME: check for i+1 == tposes.end() then interpolate;
        return *i;
    }
    bool load_from_file(const std::string &filename) {
        return static_cast<bool>(std::ifstream(filename) >> *this);
    }
    friend inline std::istream &operator>>(std::istream &file, tpose_sequence &s);
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

inline std::istream &operator>>(std::istream &file, tpose_sequence &s) {
    std::string line;
    for (int num = 1; std::getline(file, line); num++) {
        if (line.find("NA") != std::string::npos)
            continue;
        try {
            s.tposes.emplace_back(tpose_raw(line.c_str()));
        } catch (const std::exception&) { // invalid_argument or out_of_range
            std::cerr << "error on line "<< num <<": " << line << "\n";
            file.setstate(std::ios_base::failbit);
            return file;
        }
    }
    if (file.eof() && !file.bad()) // getline() can set fail on eof() :(
        file.clear(file.rdstate() & ~std::ios::failbit);
    if(s.tposes.size()) {
        transformation origin_inv = invert(s.tposes[0].G);
        for(size_t i = 0; i < s.tposes.size(); i++) {
            s.tposes[i].G = compose(origin_inv, s.tposes[i].G);
        }
    }
    return file;
}
