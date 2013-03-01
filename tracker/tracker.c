// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/video/tracking.hpp>

#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "cor.h"
#include "tracker.h"
#include "callback.h"

#define min(a,b) ((a)<(b)?a:b)

static void releasestorage(struct tracker *t)
{
    if(t->header1) { //just check one
        assert(0);
        cvReleaseImageHeader(&t->header1);
        cvReleaseImageHeader(&t->header2);
        cvReleaseImage(&t->mask);
        cvReleaseImage(&t->eig_image);
        cvReleaseImage(&t->temp_image);
        cvReleaseMat(&t->pyramid1);
        cvReleaseMat(&t->pyramid2);
    }
}

static void allocstorage(struct tracker *t)
{
    CvSize size = cvSize(t->width, t->height);
    releasestorage(t);
    t->header1 = cvCreateImageHeader(size, IPL_DEPTH_8U, 1);
    t->header2 = cvCreateImageHeader(size, IPL_DEPTH_8U, 1);
    
    t->mask = cvCreateImage(size, IPL_DEPTH_8U, 1);
    //turn all selections off by default
    cvSet(t->mask, cvScalarAll(0), NULL);
    
    t->eig_image = cvCreateImage(size, IPL_DEPTH_32F, 1);
    t->temp_image = cvCreateImage(size, IPL_DEPTH_32F, 1);

    int pyrsize = (t->width+8)*t->height/3;
    t->pyramid1 = cvCreateMat(pyrsize, 1, CV_8UC1);
    
    t->pyramid2 = cvCreateMat(pyrsize, 1, CV_8UC1);
}
#include <unistd.h>

static void addfeatures(struct tracker *t, int newfeats, unsigned char *img, unsigned int width)
{
    //don't select near old features
    //turn on everything away from the border
    cvRectangle(t->mask, cvPoint(t->spacing, t->spacing), cvPoint(t->width-t->spacing, t->height-t->spacing), cvScalarAll(255), CV_FILLED, 4, 0);
    for(int i = 0; i < t->features.size; i++) {
        cvCircle(t->mask, cvPoint(t->features.data[i]. x,t->features.data[i].y), t->spacing, CV_RGB(0,0,0), -1, 8, 0);
    }

    feature_t *newfeatures = t->features.data + t->features.size;

    cvGoodFeaturesToTrack(t->header1, t->eig_image, t->temp_image, (CvPoint2D32f *)newfeatures, &newfeats, t->thresh, t->spacing, t->mask, t->blocksize, t->harris, t->harrisk);
    cvFindCornerSubPix(t->header1, (CvPoint2D32f *)newfeatures, newfeats, t->optical_flow_window, cvSize(-1,-1), t->optical_flow_termination_criteria);
    
    packet_t *packet = mapbuffer_alloc(t->sink, packet_feature_select, newfeats * sizeof(feature_t));
    feature_t *out = (feature_t *)packet->data;
    packet_t *intensity_packet = mapbuffer_alloc(t->sink, packet_feature_intensity, newfeats);
    unsigned char *intensity_out = (feature_t *)intensity_packet->data;
    int goodfeats = 0;
    for(int i = 0; i < newfeats; ++i) {
        if(newfeatures[i].x > 0.0 &&
           newfeatures[i].y > 0.0 &&
           newfeatures[i].x < t->width-1 &&
           newfeatures[i].y < t->height-1) {
            int lx = floor(newfeatures[i].x);
            int ly = floor(newfeatures[i].y);
            intensity_out[goodfeats] = (((unsigned int)img[lx + ly*width]) + img[lx + 1 + ly * width] + img[lx + width + ly * width] + img[lx + 1 + width + ly * width]) >> 2;
            out[goodfeats++] = newfeatures[i];
        }
    }
    t->features.size += goodfeats;
    packet->header.user = goodfeats;
    intensity_packet->header.user = goodfeats;
    mapbuffer_enqueue(t->sink, packet, t->oldframe->header.time);
    mapbuffer_enqueue(t->sink, intensity_packet, t->oldframe->header.time);
}

void frame(void *_t, packet_t *p)
{
    struct tracker *t = _t;
    if(p->header.type != packet_camera) return;
    int width, height;
    sscanf((char *)p->data, "P5 %d %d", &width, &height);
    if(width != t->width || height != t->height) {
        t->width = width;
        t->height = height;
        allocstorage(t);
    }
    packet_t *packet = mapbuffer_alloc(t->sink, packet_feature_track, t->features.size * sizeof(feature_t));
    feature_t *trackedfeats = (feature_t *)packet->data;
    //are we tracking anything?
    int newindex = 0;
    if(t->features.size) {
        //we could check timestamps, etc ?
        cvSetData(t->header2, p->data + 16, width);

        float errors[t->features.size];
        char found_features[t->features.size];

        //track
        cvCalcOpticalFlowPyrLK(t->header1, t->header2, 
                               t->pyramid1, t->pyramid2, 
                               (CvPoint2D32f *)t->features.data, (CvPoint2D32f *)trackedfeats, t->features.size,
                               t->optical_flow_window, t->levels, 
                               found_features, 
                               errors, t->optical_flow_termination_criteria, 
                               (t->pyramidgood?CV_LKFLOW_PYR_A_READY:0) );
        t->pyramidgood = 1;
        CvMat *tmp = t->pyramid1;
        t->pyramid1 = t->pyramid2;
        t->pyramid2 = tmp;
        ++t->framecount;

        int area = (t->spacing * 2 + 3);
        area = area * area;

        for(int i = 0; i < t->features.size; ++i) {
            if(found_features[i] &&
               trackedfeats[i].x > 0.0 &&
               trackedfeats[i].y > 0.0 &&
               trackedfeats[i].x < t->width-1 &&
               trackedfeats[i].y < t->height-1 &&
               errors[i] / area < t->max_tracking_error) {
                t->features.data[newindex++] = trackedfeats[i];
            } else {
                trackedfeats[i].x = trackedfeats[i].y = INFINITY;
            }
        }
    }

    packet->header.user = t->features.size;
    t->features.size = newindex;
    //clear everything out to make room for next call
    t->oldframe = (packet_camera_t *)p;
    cvSetData(t->header1, p->data + 16, width);

    mapbuffer_enqueue(t->sink, packet, p->header.time);

    int space = t->maxfeats - t->features.size;
    if(space >= t->groupsize) {
        if(space > t->maxgroupsize) space = t->maxgroupsize;
        addfeatures(t, space, p->data + 16, width);
    }
}

static void setfeatures(struct tracker *t, int count, feature_t * feats)
{
    t->features.size = count;
    for(int i = 0; i < count; ++i) {
        t->features.data[i].x = feats[i].x;
        t->features.data[i].y = feats[i].y;
    }
}

static void dropfeatures(struct tracker *t, int todrop, uint16_t *indices)
{
    int index = 0;
    for(int i = 0; i < t->features.size; ++i) {
        if(!todrop || i != *indices) {
            t->features.data[index++] = t->features.data[i];
        } else {
            ++indices;
            --todrop;
        }
    }
    t->features.size = index;
}


void control(void *_t, packet_t *p)
{
    switch(p->header.type) {
        /*case packet_feature_select:
        addfeatures(_t, p->header.user);
        break;*/
    case packet_feature_drop:
        dropfeatures(_t, p->header.user, (uint16_t*)p->data);
        break;
    case packet_feature_track:
        setfeatures(_t, p->header.user, (feature_t *)p->data);
        break;
    }
}

void init(struct tracker *t)
{
    t->features.data = malloc(sizeof(feature_t) * t->maxfeats);
    t->features.size = 0;
    t->optical_flow_termination_criteria = cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, t->iter, t->eps);
    t->optical_flow_window = cvSize(t->spacing, t->spacing);
}
