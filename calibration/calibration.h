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

static inline void cal_get_params(struct camera_calibration *cal, feature_t x, feature_t *kr, feature_t *delta)
{
    feature_t x2;
    float xyd, r2, r4, r6;

    x2.x = x.x * x.x;
    x2.y = x.y * x.y;
    r2 = x2.x + x2.y;
    r6 = r2 * (r4 = r2 * r2);
    kr->x = kr->y = 1. + cal->K[0] * r2 + cal->K[1] * r4 + cal->K[2] * r6;

    xyd = 2 * x.x * x.y;
    delta->x = xyd * cal->p.x + cal->p.y * (r2 + 2 * x2.x);
    delta->y = xyd * cal->p.y + cal->p.x * (r2 + 2 * x2.y);
}   

void calibration_normalize(struct camera_calibration *cal, feature_t *pts, feature_t *xns, int n);
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
