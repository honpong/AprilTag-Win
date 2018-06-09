#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "platform/sensor_clock.h"
#include "transformation.h"

struct tpose_tum {
    double t_s = 0; v3 T_m = v3::Zero(); quaternion Q = quaternion::Identity(); int id = 0; int confidence = 0;
    tpose_tum() {}
    tpose_tum(double t_s_, const transformation &G_m, int id_ = 0, int confidence_ = 0) : t_s(t_s_), T_m(G_m.T), Q(G_m.Q), id(id_), confidence(confidence_) {};
    tpose_tum(const std::string &line) {
        std::stringstream ss(line);
        ss >> t_s >> T_m(0) >> T_m(1) >> T_m(2) >> Q.x() >> Q.y() >> Q.z() >> Q.w();
#if defined(__cpp_exceptions)
        if (!ss)
            throw std::invalid_argument(line);
#endif
        ss >> id >> confidence;
    }
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

static inline std::ostream& operator<<(std::ostream &stream, const tpose_tum &tp)
{
    return stream  << std::setprecision(9) << std::fixed << tp.t_s << " " <<
        tp.T_m.x() << " " << tp.T_m.y()    << " " << tp.T_m.z() << " " <<
        tp.Q.x()   << " " << tp.Q.y()      << " " << tp.Q.z()   << " " << tp.Q.w() << " " <<
        tp.id      << " " << tp.confidence << "\n";
}

struct tpose_vicon {
    uint64_t t_s, t_ns, seq_no; v3 T_m; quaternion Q;
    tpose_vicon() : t_s(0), t_ns(0), seq_no(0), T_m(v3::Zero()), Q(quaternion::Identity()) {}
    tpose_vicon(const std::string &line) : tpose_vicon() {
        std::stringstream ss(line);
        ss >> t_s >> t_ns >> seq_no >> T_m(0) >> T_m(1) >> T_m(2) >> Q.x() >> Q.y() >> Q.z() >> Q.w();
#if defined(__cpp_exceptions)
        if (!ss)
            throw std::invalid_argument(line);
#endif
    }
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct tpose_raw {
    uint64_t t_100ns; m3 R; v3 T_mm;
    tpose_raw() : t_100ns(0), R(m3::Identity()), T_mm(v3::Zero()) {}
    tpose_raw(const std::string &line) : tpose_raw() {
        std::stringstream ss(line);

        {
            std::string s_t;
            ss >> s_t;
            if (s_t.find('.') != std::string::npos)
                t_100ns = std::stod(s_t.c_str()) * 10000; // floating => ms
            else
                t_100ns = std::stoull(s_t.c_str()); // integer => 100ns
        }

        ss  >> R(0,0) >> R(0,1) >> R(0,2) >> T_mm(0)
        /**/>> R(1,0) >> R(1,1) >> R(1,2) >> T_mm(1)
        /**/>> R(2,0) >> R(2,1) >> R(2,2) >> T_mm(2);

#if defined(__cpp_exceptions)
        if (!ss)
            throw std::invalid_argument(line);
#endif
    }
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct tpose {
    tpose(const tpose_raw &r) : t(sensor_clock::ns100_to_tp(r.t_100ns)), G(to_quaternion(r.R), r.T_mm / 1000), id(0), confidence(0) {}
    tpose(const tpose_vicon &v) : t(sensor_clock::s_ns_to_tp(v.t_s, v.t_ns)), G(v.Q, v.T_m), id(0), confidence(0) {}
    tpose(const tpose_tum &v) : t(sensor_clock::s_to_tp(v.t_s)), G(v.Q, v.T_m), id(v.id), confidence(v.confidence) {}
    tpose(sensor_clock::time_point t_) : t(t_) {}
    tpose(const sensor_clock::time_point & t_, const transformation & G_) : t(t_), G(G_) {}
    tpose(const sensor_clock::time_point & t_, const tpose &tp0, const tpose &tp1) : t(t_) {
        auto  t_t0 = std::chrono::duration_cast<std::chrono::duration<f_t>>(    t-tp0.t);
        auto t1_t0 = std::chrono::duration_cast<std::chrono::duration<f_t>>(tp1.t-tp0.t);
        G = t1_t0.count() < F_T_EPS ? tp0.G : transformation(t_t0.count() / t1_t0.count(), tp0.G, tp1.G);
    }
    sensor_clock::time_point t;
    transformation G;
    int id;
    int confidence;
    bool operator<(const struct tpose &tp) const {
        return t < tp.t;
    };
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

static inline std::ostream& operator<<(std::ostream &stream, const tpose &tp)
{
    return stream << tpose_tum(sensor_clock::tp_to_micros(tp.t)/1.e6 , tp.G, tp.id, tp.confidence);
}

struct tpose_sequence {
    aligned_vector<tpose> tposes;
    enum {FORMAT_POSE = 0, FORMAT_VICON, FORMAT_TUM} format = FORMAT_POSE;
    f_t get_length() {
        return tposes.empty() ? 0 : (tposes.front().G.T - tposes.back().G.T).norm();
    }
    f_t get_path_length() {
        f_t total_distance = 0;
        if (!tposes.empty()) {
            v3 last_position = tposes.front().G.T;
            for (auto &tp : tposes) {
                f_t delta_T = (tp.G.T - last_position).norm();
                if (delta_T > .01f) {
                    total_distance += delta_T;
                    last_position = tp.G.T;
                }
            }
        }
        return total_distance;
    }
    bool get_pose(sensor_clock::time_point t, tpose &tp) const {
        tpose pt {t};
        if (tposes.empty() || pt < tposes.front())
            return false;
        auto i = std::lower_bound(tposes.begin(), tposes.end(), pt); // pt <= *i
        if (i == tposes.end())
            return false;
        auto i1 = i == tposes.begin() ? *i : *(i-1);
        auto i2 = *i;
        if(i2.t - i1.t > std::chrono::milliseconds(150))
            return false;
        tp = tpose(t, i1, i2);
        return true;
    }
    bool load_from_file(const std::string &filename) {
        if(filename.find(".tum") != std::string::npos)
            format = FORMAT_TUM;
        else if(filename.find(".vicon") != std::string::npos)
            format = FORMAT_VICON;
        else
            format = FORMAT_POSE;
        return static_cast<bool>(std::ifstream(filename) >> *this);
    }
    void set_relative_pose(sensor_clock::time_point t, const tpose & g) {
        tpose gt {t};
        if(!get_pose(t, gt))
            return;

        transformation offset = g.G*invert(gt.G);
        for(size_t i = 0; i < tposes.size(); i++) {
            tposes[i].G = offset*tposes[i].G;
        }
    }
    size_t size() const { return tposes.size(); }

    friend inline std::istream &operator>>(std::istream &file, tpose_sequence &s);
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

inline std::istream &operator>>(std::istream &file, tpose_sequence &s) {
    std::string line;
    for (int num = 1; std::getline(file, line); num++) {
        if(s.format == tpose_sequence::FORMAT_VICON && num == 1) continue; // skip header row
        if (line.find("NA") != std::string::npos)
            continue;
#if defined(__cpp_exceptions)
        try {
#endif
            switch(s.format) {
                case tpose_sequence::FORMAT_TUM:
                s.tposes.emplace_back(tpose_tum(line.c_str()));
                break;

                case tpose_sequence::FORMAT_VICON:
                s.tposes.emplace_back(tpose_vicon(line.c_str()));
                break;

                case tpose_sequence::FORMAT_POSE:
                tpose_raw p(line.c_str());
                if(p.T_mm != v3(0, 0, 0))
                    s.tposes.emplace_back(p);
                break;
            }
#if defined(__cpp_exceptions)
        } catch (const std::exception&) { // invalid_argument or out_of_range
            std::cerr << "error on line "<< num <<": " << line << "\n";
            file.setstate(std::ios_base::failbit);
            return file;
        }
#endif
    }
    if (file.eof() && !file.bad()) // getline() can set fail on eof() :(
        file.clear(file.rdstate() & ~std::ios::failbit);
    return file;
}

static inline std::ostream& operator<<(std::ostream &stream, const tpose_sequence &s)
{
    for(const auto & tp : s.tposes)
        stream << tp;
    return stream;
}
