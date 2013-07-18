// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __PACKET_H
#define __PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include "cor_types.h"


//WARNING: Do not change the order of this enum, or insert new packet types in the middle
//Only append new packet types after the last one previously defined.
enum packet_type {
    packet_none = 0,
    packet_camera = 1,
    packet_imu = 2,
    packet_feature_track = 3,
    packet_feature_select = 4,
    packet_navsol = 5,
    packet_feature_status = 6,
    packet_filter_position = 7,
    packet_filter_reconstruction = 8,
    packet_feature_drop = 9,
    packet_sift = 10,
    packet_plot_info = 11,
    packet_plot = 12,
    packet_plot_drop = 13,
    packet_recognition_group = 14,
    packet_recognition_feature = 15,
    packet_recognition_descriptor = 16,
    packet_map_edge = 17,
    packet_filter_current = 18,
    packet_feature_variance = 19,
    packet_accelerometer = 20,
    packet_gyroscope = 21,
    packet_filter_feature_id_visible = 22,
    packet_filter_feature_id_association = 23,
    packet_feature_intensity = 24,
    packet_filter_control = 25,
};

typedef struct {
    uint32_t bytes; //size of packet including header
    uint16_t type;  //id of packet
    uint16_t user;  //packet-defined data
    uint64_t time;  //time in microseconds
} packet_header_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} packet_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} packet_camera_t;

typedef struct {
    packet_header_t header;
    float a[3]; // m/s^2
    float w[3]; // rad/s
} packet_imu_t;

typedef struct {
    packet_header_t header;
    float a[3]; // m/s^2
} packet_accelerometer_t;

typedef struct {
    packet_header_t header;
    float w[3]; // rad/s
} packet_gyroscope_t;

typedef struct {
    packet_header_t header;
    feature_t features[];
} packet_feature_track_t;

typedef struct {
    packet_header_t header;
    uint16_t indices[];
} packet_feature_drop_t;

typedef struct {
    packet_header_t header;
    uint8_t status[];
} packet_feature_status_t;

typedef struct {
    packet_header_t header;
    uint8_t intensity[];
} packet_feature_intensity_t;

typedef struct {
    packet_header_t header;
    float latitude, longitude;
    float altitude;
    float velocity[3];
    float orientation[3];
} packet_navsol_t;

typedef struct {
    packet_header_t header;
    float position[3];
    float orientation[3];
} packet_filter_position_t;

typedef struct {
    packet_header_t header;
    float points[][3];
} packet_filter_reconstruction_t;

typedef struct {
    packet_header_t header;
    uint64_t reference;
    float T[3];
    float W[3];
    float points[][3];
} packet_filter_current_t;

typedef struct {
    packet_header_t header;
    float T[3];
    float W[3];
    uint64_t feature_id[];
} packet_filter_feature_id_visible_t;

typedef struct {
    packet_header_t header;
    uint64_t feature_id[];
} packet_filter_feature_id_association_t;

typedef struct packet_listener {
    packet_t *latest;
    int type;
    bool isnew;
} packet_listener_t;

typedef struct {
    packet_header_t header;
    float nominal;
    char identity[];
} packet_plot_info_t;

typedef struct {
    packet_header_t header;
    int count;
    float data[];
} packet_plot_t;

typedef struct {
    packet_header_t header;
    uint64_t first, second;
    float T[3], W[3];
    float T_var[3], W_var[3];
} packet_map_edge_t;

typedef struct {
    packet_header_t header;
    int64_t id; //signed to indicate add/drop
    float W[3], W_var[3];
} packet_recognition_group_t;

typedef struct {
    packet_header_t header;
    uint64_t groupid;
    uint64_t id;
    float ix;
    float iy;
    float depth;
    float x, y, z;
    float variance;
} packet_recognition_feature_t;

typedef struct {
    packet_header_t header;
    float variance;
} packet_feature_variance_t;

typedef struct {
    packet_header_t header;
    uint64_t groupid;
    uint32_t id;
    float color;
    float x, y, z;
    float variance;
    uint32_t label;
    uint8_t descriptor[128];
} packet_recognition_descriptor_t;

enum packet_plot_type {
    packet_plot_inn_a,
    packet_plot_inn_w,
    packet_plot_meas_a,
    packet_plot_meas_w,
    packet_plot_inn_v, //do not add new plots past this one - groups are defined as packet_plot_inn_v + i
    packet_plot_unknown = 256
};

#include "cor_types.h"
cor_size_t packet_camera_t_size(packet_camera_t *p);
image_t packet_camera_t_image(packet_camera_t *p);
feature_vector_t packet_feature_track_t_features(packet_feature_track_t *p);
char_vector_t packet_feature_status_t_status(packet_feature_status_t *p);
point3d_vector_t packet_filter_reconstruction_t_points(packet_filter_reconstruction_t *p);
uint64_vector_t packet_filter_feature_id_visible_t_features(packet_filter_feature_id_visible_t *p);
uint64_vector_t packet_filter_feature_id_association_t_features(packet_filter_feature_id_association_t *p);
short_vector_t packet_feature_drop_t_indices(packet_feature_drop_t *p);
float_vector_t packet_plot_t_data(packet_plot_t *p);
void packet_camera_write_image(packet_camera_t *p, const char *fn);
void packet_camera_read_bmp(packet_camera_t *p, const char *name);
struct mapbuffer;
void packet_plot_setup(struct mapbuffer *mb, uint64_t time, uint16_t id, const char *name, float nominal);
void packet_plot_send(struct mapbuffer *mb, uint64_t time, uint16_t id, int count, float *data);
void packet_plot_stop(struct mapbuffer *mb, uint64_t time, uint16_t id);
#ifdef SWIG
%callback("%s_cb");
#endif
void packet_receive(void *listener, packet_t *p);
#ifdef SWIG
%nocallback;
#endif

#endif

