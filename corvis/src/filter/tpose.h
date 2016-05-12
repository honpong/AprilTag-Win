#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include "../cor/platform/sensor_clock.h"
#include "../numerics/transformation.h"

struct tpose_vicon {
    uint64_t t_s, t_ns, seq_no; v4 T_m; quaternion Q;
    tpose_vicon() : t_s(0), t_ns(0), seq_no(0), T_m(v4::Zero()), Q() {}
    tpose_vicon(const char *line) : tpose_vicon() {
        size_t end = 0;
        // the +1s below skip the ',' delimiter
        t_s = std::stoull(line+=end, &end);
        t_ns = std::stoull(line+=end+1, &end);
        seq_no = std::stoull(line+=end+1, &end);
        for(int i=0; i<3; i++)
            T_m(i) = (f_t)std::stod(line+=end+1, &end);
        Q.x() = (f_t)std::stod(line+=end+1, &end);
        Q.y() = (f_t)std::stod(line+=end+1, &end);
        Q.z() = (f_t)std::stod(line+=end+1, &end);
        Q.w() = (f_t)std::stod(line+=end+1, &end);
    }
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct tpose_raw {
    uint64_t t_100ns; m4 R; v4 T_mm;
    tpose_raw() : t_100ns(0), R(m4::Identity()), T_mm(v4::Zero()) {}
    tpose_raw(const char *line) : tpose_raw() {
        size_t end = 0;
        t_100ns = std::stoull(line+=end, &end);
        for (int i=0; i<3; i++) {
            for (int j=0; j<3; j++)
                R(i,j) = (f_t)std::stod(line+=end, &end);
            T_mm(i) = (f_t)std::stod(line+=end, &end);
        }
    }
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct tpose {
    tpose(const tpose_raw &r) : t(sensor_clock::ns100_to_tp(r.t_100ns)), G(to_quaternion(r.R), r.T_mm / 1000) {}
    tpose(const tpose_vicon &v) : t(sensor_clock::s_ns_to_tp(v.t_s, v.t_ns)), G(v.Q, v.T_m) {}
    tpose(sensor_clock::time_point t_) : t(t_) {}
    tpose(const sensor_clock::time_point & t_, const transformation & G_) : t(t_), G(G_) {}
    tpose(const sensor_clock::time_point & t_, const tpose &tp0, const tpose &tp1) : t(t_) {
        auto  t_t0 = std::chrono::duration_cast<std::chrono::duration<f_t>>(    t-tp0.t);
        auto t1_t0 = std::chrono::duration_cast<std::chrono::duration<f_t>>(tp1.t-tp0.t);
        G = t1_t0.count() < F_T_EPS ? tp0.G : transformation(t_t0.count() / t1_t0.count(), tp0.G, tp1.G);
    }
    sensor_clock::time_point t;
    transformation G;
    bool operator<(const struct tpose &tp) const {
        return t < tp.t;
    };
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

static inline std::ostream& operator<<(std::ostream &stream, const tpose &tp)
{
    return stream << "{" << tp.t.time_since_epoch().count() << ", " << tp.G << "}";
}

struct tpose_sequence {
    aligned_vector<tpose> tposes;
    bool use_vicon{false};
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
    bool get_pose(sensor_clock::time_point t, tpose &tp) {
        tpose pt {t};
        if (tposes.empty() || pt < tposes.front())
            return false;
        auto i = std::lower_bound(tposes.begin(), tposes.end(), pt); // pt <= *i
        if (i == tposes.end())
            return false;
        tp = tpose(t, i == tposes.begin() ? *i : *(i-1), *i);
        return true;
    }
    bool load_from_file(const std::string &filename) {
        std::ifstream file(filename);

        if(filename.find(".vicon") != std::string::npos)
            use_vicon = true;
        return static_cast<bool>(std::ifstream(filename) >> *this);
    }
    friend inline std::istream &operator>>(std::istream &file, tpose_sequence &s);
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

inline std::istream &operator>>(std::istream &file, tpose_sequence &s) {
    std::string line;
    for (int num = 1; std::getline(file, line); num++) {
        if(s.use_vicon && num == 1) continue; // skip header row
        if (line.find("NA") != std::string::npos)
            continue;
        try {
            if(s.use_vicon)
                s.tposes.emplace_back(tpose_vicon(line.c_str()));
            else
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
