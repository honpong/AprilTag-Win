// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/calib3d/calib3d.hpp>

#include "calibration.h"

void calibration_normalize(struct camera_calibration *cal, feature_t *pts, feature_t *xns, int n)
{
    feature_t *pt, *xn, denormed;
    int i;
    for(i = 0, pt = pts, xn = xns; i < n; ++i, ++pt, ++xn) {
        if(pts[i].x != INFINITY) {
            feature_t xd, delta, kr, eps;
            xn->x = (pt->x - cal->C.x) * cal->invF.x;
            xn->y = (pt->y - cal->C.y) * cal->invF.y;
            xd = *xn;
            float lasterr = 1.e6;
            float err = lasterr/2.;
            for(int iter = 0; iter < cal->niter; iter++) {
                cal_get_params(cal, *xn, &kr, &delta);
                denormed.x = xn->x * kr.x + delta.x;
                denormed.y = xn->y * kr.y + delta.y;
                eps.x = denormed.x - xd.x;
                eps.y = denormed.y - xd.y;
                err = eps.x*eps.x + eps.y*eps.y;
                xn->x = (xd.x - delta.x) / kr.x;
                xn->y = (xd.y - delta.y) / kr.y;
                if(err <= cal->maxerr || err > lasterr) break;
                lasterr = err;
            }
            if(err > cal->maxerr) *xn = (feature_t) {INFINITY, INFINITY};
        } else {
            *xn = (feature_t) {INFINITY, INFINITY};
        }
    }
}

void calibration_normalize_rectified(struct camera_calibration *cal, feature_t *pts, feature_t *xns, int n)
{
    feature_t *pt, *xn;
    int i;
    for(i = 0, pt = pts, xn = xns; i < n; ++i, ++pt, ++xn) {
        if(pts[i].x != INFINITY) {
            xn->x = (pt->x - cal->out_C.x) * cal->out_invF.x;
            xn->y = (pt->y - cal->out_C.y) * cal->out_invF.y;
        } else {
            *xn = (feature_t) {INFINITY, INFINITY};
        }
    }
}

void calibration_denormalize_rectified(struct camera_calibration *cal, feature_t *pts, feature_t *xns, int n)
{
    feature_t *pt, *xn;
    int i;
    for(i = 0, pt = pts, xn = xns; i < n; ++i, ++pt, ++xn) {
        if(pts[i].x != INFINITY) {
            xn->x = pt->x * cal->out_F.x + cal->out_C.x;
            xn->y = pt->y * cal->out_F.y + cal->out_C.y;
        } else {
            *xn = (feature_t) {INFINITY, INFINITY};
        }
    }
}

void calibration_denormalize(struct camera_calibration *cal, feature_t *pts, feature_t *xns, int n)
{
    feature_t *pt, *xn;
    int i, index;
    for(i = 0, index = 0, pt = pts, xn = xns; i < n; ++i, ++pt, ++xn) {
        feature_t delta, kr;
        cal_get_params(cal, *pt, &kr, &delta);
        f_t xd = pt->x * kr.x + delta.x;
        f_t yd = pt->y * kr.y + delta.y;
        xn->x = (xd + cal->alpha_c * yd) * cal->F.x + cal->C.x;
        xn->y = yd * cal->F.y + cal->C.y;
        ++index;
    }
}

void calibration_feature(void *_cal, packet_t *p)
{
    if(p->header.type != packet_feature_track && p->header.type != packet_feature_select) return;
    struct camera_calibration *cal = _cal;
    packet_t *outp = mapbuffer_alloc(cal->feature_sink, p->header.type, p->header.user * sizeof(feature_t));
    outp->header.user = p->header.user;
    calibration_normalize(cal, (feature_t *)p->data, (feature_t *)outp->data, p->header.user);
    mapbuffer_enqueue(cal->feature_sink, outp, p->header.time);
}

void calibration_feature_rectified(void *_cal, packet_t *p)
{
    if(p->header.type != packet_feature_track && p->header.type != packet_feature_select) return;
    struct camera_calibration *cal = _cal;
    packet_t *outp = mapbuffer_alloc(cal->feature_sink, p->header.type, p->header.user * sizeof(feature_t));
    outp->header.user = p->header.user;
    calibration_normalize_rectified(cal, (feature_t *)p->data, (feature_t *)outp->data, p->header.user);
    mapbuffer_enqueue(cal->feature_sink, outp, p->header.time);
}

void calibration_feature_denorm_rectified(void *_cal, packet_t *p)
{
    if(p->header.type != packet_feature_track && p->header.type != packet_feature_select) return;
    struct camera_calibration *cal = _cal;
    packet_t *outp = mapbuffer_alloc(cal->denorm_sink, p->header.type, p->header.user * sizeof(feature_t));
    outp->header.user = p->header.user;
    calibration_denormalize_rectified(cal, (feature_t *)p->data, (feature_t *)outp->data, p->header.user);
    mapbuffer_enqueue(cal->denorm_sink, outp, p->header.time);
}

void calibration_rectify_denorm_map(struct camera_calibration *cal, uint8_t *input, uint8_t *output)
{
    for(int i = 0; i < cal->out_width*cal->out_height; ++i) {
        float scaled =
            input[cal->denorm_map[i]] * cal->denorm_factor[i][0] +
            input[cal->denorm_map[i]+1] * cal->denorm_factor[i][1] +
            input[cal->denorm_map[i]+cal->in_width] * cal->denorm_factor[i][2] +
            input[cal->denorm_map[i]+cal->in_width+1] * cal->denorm_factor[i][3];
        output[i] = (uint8_t)(scaled);
    }
}

void calibration_rectify_smooth(struct camera_calibration *cal, uint8_t *input, uint8_t *output)
{
    for(int i = 0; i < cal->out_width*cal->out_height; ++i) {
        float scaled =
            .66666665 * ((input[cal->denorm_map[i]] + ((float)input[cal->denorm_map[i]-1] + input[cal->denorm_map[i] - cal->in_width]) * .25 ) * cal->denorm_factor[i][0] +
                         (input[cal->denorm_map[i]+1] + ((float)input[cal->denorm_map[i]+2] + input[cal->denorm_map[i] + 1 - cal->in_width]) * .25) * cal->denorm_factor[i][1] +
                         (input[cal->denorm_map[i]+cal->in_width] + ((float)input[cal->denorm_map[i]+cal->in_width-1] + input[cal->denorm_map[i]+cal->in_width*2]) * .25) * cal->denorm_factor[i][2] +
                         (input[cal->denorm_map[i]+cal->in_width+1] + ((float)input[cal->denorm_map[i]+cal->in_width+2] + input[cal->denorm_map[i]+1+cal->in_width*2]) * .25)* cal->denorm_factor[i][3]);
        output[i] = (uint8_t)scaled;
    }
}  

//TODO: spherical mapping
//TODO: larger sampling window, integrate debayer step
void calibration_rectify(void *_cal, packet_t *p)
{
    if(p->header.type != packet_camera) return;
    struct camera_calibration *cal = _cal;
    image_t inimg = packet_camera_t_image((packet_camera_t *)p);
    packet_t *outp = mapbuffer_alloc(cal->image_sink, packet_camera, inimg.size.width * inimg.size.height + 16);
    snprintf((char *)outp->data, 16, "P5 %d %d 255\n", cal->out_width, cal->out_height);
    outp->header.user = 16;
    unsigned char *img = inimg.data;
    unsigned char *out = outp->data+outp->header.user;
    calibration_rectify_smooth(cal, img, out);
    mapbuffer_enqueue(cal->image_sink, outp, p->header.time);
}

void calibration_build_denorm_map(struct camera_calibration *cal)
{
    feature_t x1, x2;
    int i = 0;
    for(int y = 0; y < cal->out_height; ++y) {
        for(int x = 0; x < cal->out_width; ++x) {
            f_t tx = (x - cal->out_C.x) / cal->out_F.x;
            f_t ty = (y - cal->out_C.y) / cal->out_F.y;
            x1.x = cal->rotation[0] * tx + cal->rotation[1] * ty;
            x1.y = cal->rotation[2] * tx + cal->rotation[3] * ty;
            calibration_denormalize(cal, &x1, &x2, 1);
            int px = (int)x2.x;
            int py = (int)x2.y;
            f_t deltax = x2.x - px;
            f_t deltay = x2.y - py;
            cal->denorm_map[i] = px + py * cal->in_width;
            cal->denorm_factor[i][0] = (1.-deltax)*(1.-deltay);
            cal->denorm_factor[i][1] = deltax * (1.-deltay);
            cal->denorm_factor[i][2] = (1.-deltax)*deltay;
            cal->denorm_factor[i][3] = deltax*deltay;
            ++i;
        }
    }
}

void calibration_normalize_map(struct camera_calibration *cal, feature_t *pts, feature_t *xns, int n)
{
    feature_t *pt, *xn;
    int i;
    for(i = 0, pt = pts, xn = xns; i < n; ++i, ++pt, ++xn) {
        if(pts[i].x != INFINITY) {
            if(pts[i].x > 0.0 && pts[i].y > 0.0 && pts[i].x < cal->in_width-1 && pts[i].y < cal->in_height-1) {
                int x = pts[i].x + .5;
                int y = pts[i].y + .5;
                double fx = pts[i].x - x;
                double fy = pts[i].y - y;
                int index = x + cal->in_width * y;
                f_t ox = cal->norm_mapx.data[index] * (1.-fx) * (1.-fy) + cal->norm_mapx.data[index+1] * (fx) * (1.-fy) + cal->norm_mapx.data[index+cal->in_width] * (1.-fx) * (fy) + cal->norm_mapx.data[index+1+cal->in_width] * (fx)*(fy);
                f_t oy = cal->norm_mapy.data[index] * (1.-fx) * (1.-fy) + cal->norm_mapy.data[index+1] * (fx) * (1.-fy) + cal->norm_mapy.data[index+cal->in_width] * (1.-fx) * (fy) + cal->norm_mapy.data[index+1+cal->in_width] * (fx)*(fy);
                *xn = (feature_t) {ox, oy};
            } else assert(0); //*xn = (feature_t) {INFINITY, INFINITY};
        } else {
            *xn = (feature_t) {INFINITY, INFINITY};
        }
    }
}

void calibration_feature_map(void *_cal, packet_t *p)
{
    if(p->header.type != packet_feature_track && p->header.type != packet_feature_select) return;
    struct camera_calibration *cal = _cal;
    packet_t *outp = mapbuffer_alloc(cal->feature_sink, p->header.type, p->header.user * sizeof(feature_t));
    outp->header.user = p->header.user;
    calibration_normalize_map(cal, (feature_t *)p->data, (feature_t *)outp->data, p->header.user);
    mapbuffer_enqueue(cal->feature_sink, outp, p->header.time);
}

void calibration_recognition_feature_denormalize_inplace(void *_cal, packet_t *p)
{
    if(p->header.type != packet_recognition_feature) return;
    struct camera_calibration *cal = _cal;
    feature_t norm = (feature_t) {((packet_recognition_feature_t *)p)->ix, ((packet_recognition_feature_t *)p)->iy};
    feature_t denorm;
    calibration_denormalize_rectified(cal, &norm, &denorm, 1);
    ((packet_recognition_feature_t *)p)->ix = denorm.x;
    ((packet_recognition_feature_t *)p)->iy = denorm.y;
}

void calibration_init(struct camera_calibration *cal)
{
    assert(cal->in_width != 0 && cal->in_height != 0 && cal->out_width != 0 && cal->out_height != 0);
    cal->invF.x = 1. / cal->F.x;
    cal->invF.y = 1. / cal->F.y;
    cal->out_invF.x = 1. / cal->out_F.x;
    cal->out_invF.y = 1. / cal->out_F.y;
    cal->denorm_map = malloc(cal->out_width*cal->out_height*sizeof(*cal->denorm_map));
    cal->denorm_factor = malloc(cal->out_width*cal->out_height*sizeof(*cal->denorm_factor));
    cal->norm_mapx.data = malloc(cal->in_width * cal->in_height * sizeof(*cal->norm_mapx.data));
    cal->norm_mapx.size = cal->in_width * cal->in_height;
    cal->norm_mapy.data = malloc(cal->in_width * cal->in_height * sizeof(*cal->norm_mapy.data));
    cal->norm_mapy.size = cal->in_width * cal->in_height;
    calibration_build_denorm_map(cal);
}

static int framecount = 0;
static CvPoint2D32f *corners = 0;
static CvPoint3D32f *object = 0;
static int *cornercount = 0;
static CvSize size;

void do_calibration(struct camera_calibration *cal)
{
    float distortion[5];
    distortion[0] = cal->p.x;
    distortion[1] = cal->p.y;
    distortion[2] = cal->K[0];
    distortion[3] = cal->K[1];
    distortion[4] = cal->K[2];
    float camera[9] = {0., 0., 0., 0., 0., 0., 0., 0., 0.};
    camera[0] = cal->F.x;
    camera[4] = cal->F.y;
    camera[2] = cal->C.x;
    camera[5] = cal->C.y;

    for(int i = 0; i < 5; ++i) {
        fprintf(stderr, "dist[%d] = %f\n", i, distortion[i]);
    }
    
    for(int j = 0; j < 3; ++j) {
        for(int i = 0; i < 3; ++i) {
            fprintf(stderr, "%f  ", camera[i+j*3]);
        }
        fprintf(stderr, "\n");
    }

    int i, total = 0;
    CvMat point_counts = cvMat( framecount, 1, CV_32SC1, cornercount );
    CvMat image_points, object_points;
    CvMat dist_coeffs = cvMat( 4, 1, CV_32FC1, distortion );
    CvMat camera_matrix = cvMat( 3, 3, CV_32FC1, camera );
    //CvMat rotation_matrices = cvMat( image_count, 9, CV_32FC1, _rotation_matrices );
    //CvMat translation_vectors = cvMat( image_count, 3, CV_32FC1, _translation_vectors );

    for( i = 0; i < framecount; i++ )
        total += cornercount[i];

    image_points = cvMat( total, 1, CV_32FC2, corners );
    object_points = cvMat( total, 1, CV_32FC3, object );

    int flags = CV_CALIB_USE_INTRINSIC_GUESS;

    cvCalibrateCamera2( &object_points, &image_points, &point_counts, size,
        &camera_matrix, &dist_coeffs, 0, 0,
			flags, cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS,30,DBL_EPSILON));

    for(int i = 0; i < 5; ++i) {
        fprintf(stderr, "dist[%d] = %f\n", i, distortion[i]);
    }
    
    for(int j = 0; j < 3; ++j) {
        for(int i = 0; i < 3; ++i) {
            fprintf(stderr, "%f  ", camera[i+j*3]);
        }
        fprintf(stderr, "\n");
    }
   
}

void calibration_frame(void *_cal, packet_t *p)
{
    struct camera_calibration *cal = _cal;
    if(!cal->capture || p->header.type != packet_camera) return;
    int numcorners = cal->chess_internal_rows * cal->chess_internal_cols;
    image_t inimg = packet_camera_t_image((packet_camera_t *)p);
    size = cvSize(inimg.size.width, inimg.size.height);
    IplImage *header = cvCreateImageHeader(size, IPL_DEPTH_8U, 1);
    cvSetData(header, p->data+16, inimg.size.width);

    CvSize cornersize = cvSize(cal->chess_internal_rows, cal->chess_internal_cols);
    CvPoint2D32f tempcorners[numcorners];
    CvPoint3D32f tempobject[numcorners];
    
    for(int j = 0; j < cal->chess_internal_rows; ++j) {
        for(int i = 0; i < cal->chess_internal_cols; ++i) {
            tempobject[j * cal->chess_internal_cols + i].x = i * cal->chess_size;
            tempobject[j * cal->chess_internal_cols + i].y = i * cal->chess_size;
            tempobject[j * cal->chess_internal_cols + i].z = 0.;
        }
    }

    int found = 0;
    bool success = cvFindChessboardCorners(header, cornersize, tempcorners, &found, CV_CALIB_CB_ADAPTIVE_THRESH);
    if(!success) {
        fprintf(stderr, "failed\n");
        return;
    }
    fprintf(stderr, "success\n");
    cvFindCornerSubPix(header, tempcorners, found, cvSize(5,5), cvSize(-1,-1), cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 50, .0001));
    corners = realloc(corners, (framecount + 1) * numcorners * sizeof(cvPoint2D32f));
    object = realloc(object, (framecount + 1) * numcorners * sizeof(cvPoint3D32f));
    cornercount = realloc(cornercount, (framecount + 1) * sizeof(int));
    memcpy(corners + framecount, tempcorners, numcorners * sizeof(cvPoint2D32f));
    memcpy(object + framecount, tempobject, numcorners * sizeof(cvPoint3D32f));
    cornercount[framecount] = numcorners;
    ++framecount;
    cal->capture = false;
}   

void calibration_capture(struct camera_calibration *cal)
{
    cal->capture = true;
}
