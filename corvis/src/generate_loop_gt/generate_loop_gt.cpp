#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "gt_generator.h"
#include "for_each_file.h"

static void show_usage(char *argv0) {
    std::cout << argv0 <<
                 " <capture_file> "
                 "[--fov 80] "
                 "[--format binary | ascii] "
                 "[--cache-image-index] "
                 "[--save-associations] "
                 "\n"
                 "fov:               field of view of camera in degrees.\n"
                 "cache-image-index: save cache of image timestamps.\n"
                 "save-associations: create a file to display associations in matlab.\n";
}

struct configuration {
    std::string capture_file;
    bool save_associations;
    double fov_rad;
    enum { binary, ascii } format;
    bool cache_index;  // save cache file for image timestamps
    double near_z;  // frustum near plane
    double far_z;  // frustum far plane
    bool read(int argc, char *arg[]);
};

int main(int argc, char *argv[]) {
    configuration config;
    if (!config.read(argc, argv)) {
        show_usage(argv[0]);
        return 1;
    }
    gt_generator::camera camera(config.fov_rad, config.near_z, config.far_z);

    if (config.capture_file.back() == '/') {
        bool ok = true;
        for_each_file(config.capture_file.c_str(), [&](const char *file_) {
            std::string file(file_);
            gt_generator generator;
            if (generator.generate(file, camera, false)) {
                ok &= generator.save_loop_file(file, config.format == configuration::binary);
                if (config.save_associations)
                    ok &= generator.save_loop_file(file);
            }
        });
        return ok ? 0 : 1;
    }

    gt_generator generator;
    std::cout << "Generating loop groundtruth..." << std::endl;
    if (!generator.generate(config.capture_file, camera, true))
        return 1;

    bool ok = generator.save_loop_file(config.capture_file, config.format == configuration::binary);
    if (config.save_associations)
        ok &= generator.save_mat_file(config.capture_file);
    return ok ? 0 : 1;
}

bool configuration::read(int argc, char *argv[]) {
    auto parse_double = [argc, argv](int i, double& value) {
        if (i < argc) {
            char* p;
            value = std::strtod(argv[i], &p);
            return (p != argv[i] && value != HUGE_VAL && value > 0);
        } else {
            return false;
        }
    };
    auto parse_string = [argc, argv](int i, std::string& value) {
        if (i < argc) {
            value = argv[i];
            return true;
        } else {
            return false;
        }
    };

    double fov_deg = 80;  // default
    std::string desired_format;
    save_associations = false;
    capture_file.clear();
    format = configuration::binary;
    cache_index = false;
    near_z = 1;
    far_z = 5;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--fov") == 0) {
            if (!parse_double(++i, fov_deg)) return false;
        } else if (strcmp(argv[i], "--format") == 0) {
            if (!parse_string(++i, desired_format)) return false;
            if (desired_format != "binary" && desired_format != "ascii") return false;
        } else if (strcmp(argv[i], "--cache-image-index") == 0) {
            cache_index = true;
        } else if (strcmp(argv[i], "--save-associations") == 0) {
            save_associations = true;
        } else if (argv[i][0] != '-' && capture_file.empty()) {
            capture_file = argv[i];
        } else {
            return false;
        }
    }

    if (!capture_file.empty()) {
        fov_rad = fov_deg * M_PI / 180;
        if (desired_format == "ascii") format = configuration::ascii;
        return true;
    } else {
        return false;
    }
}
