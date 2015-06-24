#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include "../cor/platform/sensor_clock.h"
#include "../numerics/transformation.h"

struct tpose_raw {
    uint64_t t_100ns; m4 R; v4 T_mm;
    friend inline std::istream &operator>>(std::istream &l, tpose_raw &tp);
};

inline std::istream &operator>>(std::istream &l, tpose_raw &tp) {
    l >> tp.t_100ns;
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++)
            l >> tp.R(i,j);
        l >> tp.T_mm(i);
    }
    return l;
}

struct tpose {
    tpose(const tpose_raw &r) : t(sensor_clock::ns100_to_tp(r.t_100ns)), G(to_quaternion(r.R), r.T_mm / 1000) {}
    tpose(sensor_clock::time_point t_) : t(t_) {}
    sensor_clock::time_point t;
    transformation G;
    bool operator<(const struct tpose &tp) const {
        return t < tp.t;
    };
};

struct tpose_sequence {
    std::vector<tpose> tposes;
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
};

inline std::istream &operator>>(std::istream &file, tpose_sequence &s) {
    std::string line;
    for (int num = 1; std::getline(file, line); num++) {
        std::stringstream l(line);
        tpose_raw tp_raw;
        l >> tp_raw;
        if (!l) {
            if (std::string(line).find("NA") != std::string::npos)
                continue;
            std::cerr << "error on line "<< num <<": " << line << "\n";
            file.setstate(std::ios_base::failbit);
            return file;
        }
        s.tposes.emplace(s.tposes.end(), tp_raw);
    }
    if (file.eof() && !file.bad()) // getline() can set fail on eof() :(
        file.clear(file.rdstate() & ~std::ios::failbit);
    return file;
}
