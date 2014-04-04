// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "packet.h"
#include "cor.h"
#include "mapbuffer.h"

image_t packet_camera_t_image(packet_camera_t *p)
{
    return (image_t) { .size = packet_camera_t_size(p), .data = p->data + (p->header.user ? p->header.user:16) };
}

cor_size_t packet_camera_t_size(packet_camera_t *p)
{
    cor_size_t s;
    sscanf((char *)p->data, "P5 %d %d", &s.width, &s.height);
    return s;
}

void packet_receive(void *listener, packet_t *p)
{
    packet_listener_t *pl = (packet_listener_t *)listener;
    if(p->header.type == pl->type) {
        pl->latest = p;
        pl->isnew = true;
    }
}

feature_vector_t packet_feature_track_t_features(packet_feature_track_t *p)
{
    return (feature_vector_t) {.size = p->header.user, .data = p->features};
}

char_vector_t packet_feature_status_t_status(packet_feature_status_t *p)
{
    return (char_vector_t) {.size = p->header.user, .data = p->status};
}

float_vector_t packet_plot_t_data(packet_plot_t *p)
{
    return (float_vector_t) {.size = p->count, .data = p->data};
}

point3d_vector_t packet_filter_reconstruction_t_points(packet_filter_reconstruction_t *p)
{
    return (point3d_vector_t) {.size = p->header.user, (float *)p->points};
}

uint64_vector_t packet_filter_feature_id_visible_t_features(packet_filter_feature_id_visible_t *p)
{
    return (uint64_vector_t) {.size = p->header.user, .data = p->feature_id};
}

uint64_vector_t packet_filter_feature_id_association_t_features(packet_filter_feature_id_association_t *p)
{
    return (uint64_vector_t) {.size = p->header.user, .data = p->feature_id};
}

short_vector_t packet_feature_drop_t_indices(packet_feature_drop_t *p)
{
    return (short_vector_t) {.size = p->header.user, .data = (uint16_t *)p->indices};
}

void packet_camera_write_image(packet_camera_t *p, const char *fn)
{
    unsigned int mf = open(fn, O_CREAT | O_RDWR | O_TRUNC, 0644);
    int width, height;
    sscanf((char *)p->data, "P5 %d %d", &width, &height);
    if(!p->header.user)
      p->header.user = 16;
    write(mf, p->data, p->header.user + width*height);
    close(mf);
}

void packet_camera_read_bmp(packet_camera_t *p, const char *fn)
{
    unsigned int bmp = open(fn, O_RDONLY);
    int width, height;
    int offset;
    char magic[2];
    read(bmp, magic, 2);
    assert(magic[0] == 'B' && magic[1] == 'M');
    lseek(bmp, 8, SEEK_CUR); // filesize & 2 reserved
    read(bmp, &offset, 4);
    lseek(bmp, 4, SEEK_CUR); //header size
    read(bmp, &width, 4);
    read(bmp, &height, 4);
    sprintf((char *)p->data, "P5 %4d %3d %d\n", width, height, 255);
    p->header.user = 16;
    uint8_t data [height][width];
    lseek(bmp, offset, SEEK_SET);
    read(bmp, data, width * height);
    for(int y = 0; y < height; ++y) {
        memcpy(p->data + 16 + y * width, data[height - 1 - y], width);
    }
    close(bmp);
}


void packet_plot_setup(struct mapbuffer *mb, uint64_t time, uint16_t id, const char *name, float nominal)
{
    packet_plot_info_t *ip = (packet_plot_info_t*)mapbuffer_alloc(mb, packet_plot_info, (uint32_t)(sizeof(packet_plot_info_t) + strlen(name) + 1));
    strcpy((char *)ip->identity, name);
    ip->nominal = nominal;
    ip->header.user = id;
    mapbuffer_enqueue(mb, (packet_t *)ip, time);
}

void packet_plot_send(struct mapbuffer *mb, uint64_t time, uint16_t id, int count, float data[count])
{
    packet_plot_t *ip = (packet_plot_t *)mapbuffer_alloc(mb, packet_plot, sizeof(*data)*count);
    memcpy(ip->data, data, sizeof(*data)*count);
    ip->header.user = id;
    ip->count = count;
    mapbuffer_enqueue(mb, (packet_t *)ip, time);
}

void packet_plot_stop(struct mapbuffer *mb, uint64_t time, uint16_t id)
{
    packet_plot_t *ip = (packet_plot_t *)mapbuffer_alloc(mb, packet_plot_drop, 0);
    ip->header.user = id;
    mapbuffer_enqueue(mb, (packet_t *)ip, time);
}
