#include <fstream>
#include <iostream>

#include "transformation.h"
#include "rc_tracker.h"

static rc_Pose to_rc_Pose(const transformation &g)
{
    rc_Pose p;
    v_map(p.Q.v) = g.Q.coeffs().cast<float>();
    v_map(p.T.v) = g.T.cast<float>();
    return p;
}

static transformation to_transformation(const rc_Pose p)
{
    return transformation(quaternion(v_map(p.Q.v).cast<f_t>()),
                          v_map(p.T.v).cast<f_t>());
}

static rc_Extrinsics operator*(rc_Pose a, rc_Extrinsics b)
{
    transformation a_b = to_transformation(a) * transformation(rotation_vector(b.W.x, b.W.y, b.W.z), v_map(b.T.v));
    v_map(b.W.v) = to_rotation_vector(a_b.Q).raw_vector();
    v_map(b.T.v) = a_b.T;
    return b; // b = a * b
}

bool read_file(const std::string filename, std::string &contents)
{
    std::ifstream f(filename);
    if(!f.is_open())
        return false;
    contents.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

int main(int c, char **v)
{
    if (0) { usage:
        std::cerr << "Usage: " << v[0] << " <right> <left>\n";
        return 1;
    }

    if (c != 3)
        goto usage;

    const char *r_file = v[1], *l_file = v[2];

    const float fourty = 40*M_PI/180, S = std::sin(fourty) / M_SQRT2, C = std::cos(fourty) / M_SQRT2;
    rc_Pose r_b_from_fe = {{ S, C, C, S}, { 0.160668f/2, 0.0256f, -0.01216f}};
    rc_Pose l_b_from_fe = {{-S, C, C,-S}, {-0.160668f/2, 0.0256f, -0.01216f}};

    std::string r_json; if (!read_file(r_file, r_json)) { std::cerr << "Error reading " << r_file << "\n"; return 1; }
    std::string l_json; if (!read_file(l_file, l_json)) { std::cerr << "Error reading " << l_file << "\n"; return 1; }

    rc_Tracker *r_rc = rc_create(), *l_rc = rc_create(), *j_rc = rc_create();

    if (!rc_setCalibration(r_rc, r_json.c_str())) { std::cerr << "Error parsing JSON " << r_file << "\n"; return 1; }
    if (!rc_setCalibration(l_rc, l_json.c_str())) { std::cerr << "Error parsing JSON " << l_file << "\n"; return 1; }

    rc_Extrinsics ex; rc_CameraIntrinsics c_in; rc_GyroscopeIntrinsics w_in; rc_AccelerometerIntrinsics a_in;
    int i, j;

    j=0;
    for (i=0; rc_describeCamera(r_rc, i, rc_FORMAT_GRAY8, &ex, &c_in); i++) { ex = r_b_from_fe * ex; rc_configureCamera(j_rc, j++, rc_FORMAT_GRAY8, &ex, &c_in); }
    for (i=0; rc_describeCamera(l_rc, i, rc_FORMAT_GRAY8, &ex, &c_in); i++) { ex = l_b_from_fe * ex; rc_configureCamera(j_rc, j++, rc_FORMAT_GRAY8, &ex, &c_in); }

    j=0;
    for (i=0; rc_describeCamera(r_rc, i, rc_FORMAT_DEPTH16, &ex, &c_in); i++) { ex = r_b_from_fe * ex; rc_configureCamera(j_rc, j++, rc_FORMAT_DEPTH16, &ex, &c_in); }
    for (i=0; rc_describeCamera(l_rc, i, rc_FORMAT_DEPTH16, &ex, &c_in); i++) { ex = l_b_from_fe * ex; rc_configureCamera(j_rc, j++, rc_FORMAT_DEPTH16, &ex, &c_in); }

    j=0;
    for (i=0; rc_describeAccelerometer(r_rc, i, &ex, &a_in); i++) { ex = r_b_from_fe * ex; rc_configureAccelerometer(j_rc, j++, &ex, &a_in); }
    for (i=0; rc_describeAccelerometer(l_rc, i, &ex, &a_in); i++) { ex = l_b_from_fe * ex; rc_configureAccelerometer(j_rc, j++, &ex, &a_in); }

    j=0;
    for (i=0; rc_describeGyroscope(r_rc, i, &ex, &w_in); i++) { ex = r_b_from_fe * ex; rc_configureGyroscope(j_rc, j++, &ex, &w_in); }
    for (i=0; rc_describeGyroscope(l_rc, i, &ex, &w_in); i++) { ex = l_b_from_fe * ex; rc_configureGyroscope(j_rc, j++, &ex, &w_in); }

    const char *j_json = nullptr;
    size_t len = rc_getCalibration(j_rc, &j_json); if (!len) { std::cerr << "Error generating JSON"; return 1; }
    std::cout << j_json << "\n";
    return 0;
}
