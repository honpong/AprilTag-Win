// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __CALIBRATION_H
#define __CALIBRATION_H

#include "cor.h"

struct camera_calibration {
    feature_t F;
    feature_t out_F;
    feature_t invF;
    feature_t out_invF;
    feature_t C;
    feature_t out_C;
    feature_t p;
    f_t rotation[4];
    f_t K[3];
    f_t alpha_c;
    struct mapbuffer *denorm_sink;
    struct mapbuffer *feature_sink;
    struct mapbuffer *image_sink;
    int niter;
    float maxerr;
    int in_width, in_height;
    int out_width, out_height;
    int *denorm_map;
    float (*denorm_factor)[4];
    float_vector_t norm_mapx, norm_mapy;
    bool capture;
    int chess_internal_rows;
    int chess_internal_cols;
    float chess_size;
};

void calibration_init(struct camera_calibration *cal);
void calibration_denormalize_rectified(struct camera_calibration *cal, feature_t *pts, feature_t *xns, int n);
void calibration_capture(struct camera_calibration *cal);
void do_calibration(struct camera_calibration *cal);
#ifdef SWIG
%callback("%s_cb");
#endif
void calibration_feature(void *cal, packet_t *p);
void calibration_rectify(void *cal, packet_t *p);
void calibration_feature_rectified(void *cal, packet_t *p);
void calibration_feature_denorm_rectified(void *cal, packet_t *p);
void calibration_feature_map(void *cal, packet_t *p);
void calibration_frame(void *_cal, packet_t *p);
void calibration_recognition_feature_denormalize_inplace(void *_cal, packet_t *p);
#ifdef SWIG
%nocallback;
#endif
#endif
