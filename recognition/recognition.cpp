// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

extern "C" {
#include "cor.h"
}

#include "recognition.h"

void recognition_init(struct recognition *rec, int width, int height) {
    assert(rec->octaves);
    assert(rec->maxkeys);
    assert(rec->sink);
    assert(width);
    assert(height);
    rec->width = width;
    rec->height = height;
    rec->sift = vl_sift_new(width, height, rec->octaves, rec->levels, -1) ; 
    rec->initialized = true;
    rec->imageindex = 0;
    rec->dictionary = new int32_t[rec->dict_size * 128];
    rec->numdescriptors = 0;
    FILE *clusterdata = fopen(rec->dict_file, "rt");
    for(int i = 0; i < rec->dict_size * 128; ++i) {
        fscanf(clusterdata, "%d ", rec->dictionary + i);
    }
    fclose(clusterdata);
}

#define PATCH_SIZE 32

#define do_grad(right, left, down, up)                                  \
    Ix = (img.data[loc + right] - img.data[loc - left]) / 255. * .5;    \
    Iy = (img.data[loc + down] - img.data[loc - up]) / 255. * .5;       \
    grad[(loc<<1)] = sqrt(Ix * Ix + Iy *Iy);                            \
    grad[(loc<<1)+1] = atan2(Iy, Ix);                                   \
    ++loc

void compute_vl_gradient(image_t img, vl_sift_pix grad[])
{
    vl_sift_pix Ix, Iy;
    int loc = 0;
    //top left
    do_grad(1, 0, img.size.width, 0);
    //top
    for(int x = 1; x < img.size.width-1; ++x) {
        do_grad(1, 1, img.size.width, 0);
    }
    //top right
    do_grad(0, 1, img.size.width, 0);
    for(int y = 1; y < img.size.height-1; ++y) {
        //left
        do_grad(1, 0, img.size.width, img.size.width);
        for(int x = 1; x < img.size.width-1; ++x) {
            do_grad(1, 1, img.size.width, img.size.width);
        }
        //right
        do_grad(0, 1, img.size.width, img.size.width);
    }
    //bottom left
    do_grad(1, 0, 0, img.size.width);
    //bottom
    for(int x = 1; x < img.size.width-1; ++x) {
        do_grad(1, 1, 0, img.size.width);
    }
    //bottom right
    do_grad(0, 1, 0, img.size.width);
}

void recognition_build_dictionary(struct recognition *rec)
{
    int N = rec->numdescriptors;
    uint8_t *alldescs = new uint8_t[128 * N];
    uint64_t index = 0;
    fprintf(stderr, "Getting %d descriptors...\n", N);
    for(int i = 0; i < N; ++i) {
        packet_t *p;
        do {
            p = mapbuffer_read(rec->sink, &index);
        } while(p && p->header.type != packet_recognition_descriptor);
        if(!p) fprintf(stderr, "Something went wrong -- didn't find enough descriptors. Are you sure the mapbuffer is file-backed?\n");
        packet_recognition_descriptor_t *dp = (packet_recognition_descriptor_t *)p;
        memcpy(alldescs + i * 128, dp->descriptor, 128);
    }
    fprintf(stderr, "Building clusters...\n");
    VlIKMFilt *ikm = vl_ikm_new(VL_IKM_ELKAN);
    fprintf(stderr, "Dictionary size is %d\n" , rec->dict_size);
    vl_ikm_init_rand_data(ikm, alldescs, 128, N, rec->dict_size);
    vl_ikm_train(ikm, alldescs, N);
    memcpy(rec->dictionary, ikm->centers, rec->dict_size * 128 * sizeof(*rec->dictionary));
    vl_ikm_delete(ikm);
    delete alldescs;
    FILE *clusterdata = fopen(rec->dict_file, "wt");
    for(int i = 0; i < rec->dict_size; ++i) {
        for(int j = 0; j < 128; ++j) {
            fprintf(clusterdata, "%d ", rec->dictionary[i * 128 + j]);
        }
        fprintf(clusterdata, "\n");
    }
    fclose(clusterdata);
}

void recognition_group_new(struct recognition *rec, uint64_t groupid, float orientation, uint64_t time)
{
    if(!rec->initialized) rec->imageindex = 0;

    packet_camera_t *frame = (packet_camera_t *)mapbuffer_find_packet(rec->imagebuf, &rec->imageindex, time, packet_camera);
    assert(frame);

    image_t img = packet_camera_t_image(frame);
    if(!rec->initialized)
        recognition_init(rec, img.size.width, img.size.height);
    
    vl_sift_pix *grad = new vl_sift_pix[img.size.width * img.size.height * 2];
    uint8_t *image_local = new uint8_t[img.size.width*img.size.height];
    rec->groups[groupid] = (struct group) { grad, image_local, orientation };
    memcpy(image_local, img.data, img.size.width * img.size.height);
    compute_vl_gradient(img, grad);
}

void recognition_group_done(struct recognition *rec, uint64_t groupid)
{
    map<uint64_t, struct group>::iterator i = rec->groups.find(groupid);
    delete [] i->second.grad;
    delete [] i->second.img;
    rec->groups.erase(i);
}

void recognition_feature_new(struct recognition *rec, packet_recognition_feature_t *p)
{
    struct group g = rec->groups[p->groupid];
    vl_sift_pix descr[128];
    int patchsize = 32;
    float depth = p->depth;
    f_t std_dev_percent = sqrt(p->variance) / depth;
    if(std_dev_percent > rec->maximum_feature_std_dev_percent) return;
    if(depth > patchsize) depth /= patchsize;
    if(depth > patchsize || p->depth < 1.) return;
    f_t scale = 1. / depth;
    f_t sigma0 = patchsize * scale;
    f_t sigma = sigma0 / 3.;
    float color = g.img[((int)(p->iy + .5)) * rec->width + ((int)(p->ix + .5))] / 255.;
    /*int sx = p->ix - sigma0,
        ex = p->ix + sigma0,
        sy = p->iy - sigma0,
        ey = p->iy + sigma0;
        if(sx < 0) sx = 0;
    if(ex > 767) ex = 767;
    if(sy < 0) sy = 0;
    if(ey > 639) ey = 639;
    
    vl_sift_pix patch[(ey-sy+1)][(ex-sx+1)][2];
    float color = 0;
    for(int y = sy; y <= ey; ++y) {
        memcpy(patch[y-sy], g.grad + y * rec->width*2 + sx * 2, 8 * (ex-sx+1));
        for(int x = sx; x <= ex; ++x) {
            color += g.img[y*rec->width + x];
        }
        }
    float W = ex-sx+1;
    float H = ey-sy+1;
    sigma = (W>H)?W/6.:H/6.;*/
    vl_sift_calc_raw_descriptor(rec->sift, g.grad, descr, rec->width, rec->height, p->ix, p->iy, sigma, 0.); //TODO: g->orientation, but check sign!!
    //vl_sift_calc_raw_descriptor(rec->sift, patch[0][0], descr, W, H, W/2, H/2, sigma, 0.); //TODO: g->orientation, but check sign!!
    packet_recognition_descriptor_t *dp = (packet_recognition_descriptor_t *)mapbuffer_alloc(rec->sink, packet_recognition_descriptor, sizeof(packet_recognition_descriptor_t));

    for(int i = 0; i < 128; ++i) {
        dp->descriptor[i] = (descr[i] * 512.) < 255. ? (descr[i] * 512.) : 255;
    }

    dp->x = p->x;
    dp->y = p->y;
    dp->z = p->z;
    dp->variance = p->variance;
    dp->color = color;// / (W * H * 255.);
    dp->label = vl_ikm_push_one(rec->dictionary, dp->descriptor, 128, rec->dict_size);
    dp->id = p->id;
    dp->groupid = p->groupid;
    ++rec->numdescriptors;
    mapbuffer_enqueue(rec->sink, (packet_t *)dp, p->header.time);
}

void recognition_packet(void *handle, packet_t *p)
{
    struct recognition *rec = (struct recognition *)handle;
    packet_recognition_group_t *pg = (packet_recognition_group_t *)p;
    switch(p->header.type) {
    case packet_recognition_feature:
        recognition_feature_new(rec, (packet_recognition_feature_t *)p);
        break;
    case packet_recognition_group: {
        if(pg->header.user) {
            recognition_group_done(rec, pg->id);
        } else {
            /*v4 reference_orientation = v4(pg->W[0], pg->W[1], pg->W[2], 0.);
            m4 R = rodrigues(reference_orientation, NULL);
            v4 local_down = R * v4(0., 0., 1., 0.);
            local_down[*/
            
            recognition_group_new(rec, pg->id, 0. /*orientation*/, pg->header.time);
        }
        packet_t *dp = mapbuffer_alloc(rec->sink, packet_recognition_group, sizeof(packet_recognition_group_t) - sizeof(packet_header_t));
        memcpy(dp->data, p->data, sizeof(packet_recognition_group_t) - sizeof(packet_header_t));
        dp->header.user = p->header.user;
        mapbuffer_enqueue(rec->sink, dp, p->header.time);
        break;
    }
    default:
        return;
    }
}

void sift_frame(void *handle, packet_t *p)
{
    if(p->header.type != packet_camera) return;
    struct recognition *rec = (struct recognition *)handle;
    image_t img = packet_camera_t_image((packet_camera_t *)p);
    if(!rec->initialized)
        recognition_init(rec, img.size.width, img.size.height);

    vl_sift_pix fimg[img.size.height * img.size.width];

    for(int i = 0; i < img.size.height*img.size.width; ++i)
        fimg[i] = img.data[i] / 255.;

    int numkeys = 0;
    /* Compute vlfeat descriptors */
    packet_t *outp = mapbuffer_alloc_unbounded(rec->sink, packet_sift);

    int packet_size = 0;
    for (int octave = 0 ; octave < rec->octaves ; ++octave) {
        if (octave == 0)
            vl_sift_process_first_octave (rec->sift, fimg);
        else
            vl_sift_process_next_octave (rec->sift);
        
        vl_sift_detect(rec->sift) ;
        int K = vl_sift_get_nkeypoints (rec->sift) ;
        VlSiftKeypoint const *skeys = vl_sift_get_keypoints (rec->sift) ;
        for (int k = 0 ; k < K ; ++k, ++numkeys) {
            struct sift_point *key_point = (struct sift_point *)(outp->data + packet_size);
            key_point->x = skeys[k].x;
            key_point->y = skeys[k].y;
            key_point->scale = skeys[k].sigma;
            key_point->orientation = 0.;
            packet_size += sizeof(struct sift_point);
            /* get the descriptor too */
            vl_sift_pix descrf [128] ;
            vl_sift_calc_keypoint_descriptor (rec->sift, descrf, skeys + k, 0) ;
            
            for (int i = 0 ; i < 128 ; ++i)
                outp->data[packet_size++] = (unsigned char)(descrf[i]*512.0) ;
        }
    }
    outp->header.user = numkeys;
    mapbuffer_enqueue_unbounded(rec->sink, outp, p->header.time, packet_size);
}
