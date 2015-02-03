//  qr.h
//
//  Created by Brian on 12/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef __QR_H__
#define __QR_H__

#include "../cor/cor_types.h"
#include "../numerics/transformation.h"

#include <vector>

using namespace std;

struct qr_detection {
    feature_t upper_left;
    feature_t upper_right;
    feature_t lower_right;
    feature_t lower_left;
    int modules;
    char data[1024];
};

bool qr_detect_one(const uint8_t * image, int width, int height, struct qr_detection & detection);

bool qr_code_homography(const struct filter *f, struct qr_detection detection, float qr_size_m, transformation & qr);
bool qr_code_origin(const struct filter *f, struct qr_detection detection, float qr_size_m, transformation & origin);

class qr_detector {
public:
    bool valid;
    bool running;
    transformation origin;

    float size_m;
    bool use_gravity;
    bool filter;
    char data[1024];

    qr_detector() : valid(false), running(false) {};

    void start(const char * desired_code, float dimension, bool gravity)
    {
        valid = false;
        filter = false;
        if(desired_code != NULL) {
            filter = true;
            strncpy(data, desired_code, 1024);
        }
        size_m = dimension;
        use_gravity = gravity;
        running = true;
    }

    void stop() { running = false; }
    void process_frame(const struct filter * f, const uint8_t * image, int width, int height);
};

class qr_benchmark {
public:
    bool enabled;
    float size_m;

    transformation origin_qr;
    transformation origin_state;
    bool origin_valid;

    qr_benchmark() : enabled(false), origin_valid(false) {};
    void start(float dimension) { size_m = dimension; enabled = true; };
    void process_frame(const struct filter * f, const uint8_t * image, int width, int height);
};

#endif
