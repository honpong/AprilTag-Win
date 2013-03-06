// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __TRACKER_H
#define __TRACKER_H
/*
#include <opencv/cv.h>
#include <stdint.h>

tracker(int width, int height, int spacing = 10, int levels = 2, double thresh=.001);
~tracker();
int test(unsigned char *a, int d) {
    return d;
}
int track2(char *oldframe, char *frame, int features_count, float *features, int dimf, signed char *found_features, int dimc) {
    return track((uint8_t *)oldframe, (uint8_t *)frame, features_count, features, (char *)found_features);
}
int track(uint8_t *oldframe, uint8_t *frame, int features_count, float *features, char *found_features);

int addpoints2(int features_count, int toadd, float *features, int dimf, float *newfeatures, int dimn) {
    return addpoints(features_count, toadd, features, newfeatures);
}
int addpoints(int features_count, int toadd, float *features, float *newfeatures);


};
*/
extern "C" {
#include "cor.h"
}

#include <opencv2/core/core_c.h>

struct tracker {
    // parameters
    int width, height;
    int spacing, levels;
    int iter;
    CvSize optical_flow_window;
    float eps;
    float thresh;
    float max_tracking_error;
    CvTermCriteria optical_flow_termination_criteria;
    int blocksize;
    bool harris;
    float harrisk;

    struct camera_calibration *cal;
    int maxfeats, groupsize, maxgroupsize;

    //state
    bool pyramidgood;
    int framecount;

    packet_camera_t *oldframe;
    struct mapbuffer *sink;
    feature_vector_t features;

    feature_t *calibrated;

    //storage
    IplImage *header1, *header2;
    IplImage *eig_image, *temp_image;
    IplImage *mask;
    CvMat *pyramid1, *pyramid2;
    //char *optical_flow_found_features;
};

extern "C" void init(struct tracker *t);
#ifdef SWIG
%callback("%s_cb");
#endif
extern "C" void frame(void *t, packet_t *p);
extern "C" void control(void *t, packet_t *p);
#ifdef SWIG
%nocallback;
#endif

#endif
