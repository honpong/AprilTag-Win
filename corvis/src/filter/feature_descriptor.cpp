#include "feature_descriptor.h"
#include <stdio.h>

extern "C" {
#include "../vlfeat-0.9.18/vl/imopv.h"
}

#define interp(c0, c1, t) ((c0)*(1-(t)) + ((c1)*(t)))
float bilinear_interp(const uint8_t * image, int stride, float x, float y)
{
    float result = 0;
    int xi = (int)x;
    int yi = (int)y;
    float tx = x - xi;
    float ty = y - yi;
    //if(xi < 0 || xi >= width-1 || yi < 0 || yi >= height-1)
    //    return 0;
    float c00 = image[yi*stride + xi];
    float c01 = image[(yi+1)*stride + xi];
    float c10 = image[yi*stride + xi + 1];
    float c11 = image[(yi+1)*stride + xi + 1];
    result = interp(interp(c00, c10, tx), interp(c01, c11, tx), ty);
    return result;
}

void liop_fill_patch(const uint8_t * I, int stride, float center_x, float center_y, float radius, float * patch)
{
    int i = 0;
    float lx = center_x - radius;
    float ly = center_y - radius;
    for(int dy = 0; dy < liop_side_length; dy++) {
        float ys = ly + 2.f*radius * dy/liop_side_length;
        for(int dx = 0; dx < liop_side_length; dx++) {
            float xs = lx + 2.f*radius * dx/liop_side_length;
            // TODO patch div 255?
            patch[i++] = bilinear_interp(I, stride, xs, ys);
        }
    }
    // from the liop paper, smooth the patch with a gaussian sigma = 1.2
    vl_imsmooth_f(patch, liop_side_length, patch, liop_side_length, liop_side_length, liop_side_length, 1.2, 1.2);
}

void liop_compute_desc(VlLiopDesc * liop, const uint8_t * I, int stride, float center_x, float center_y, float radius, float * patch, float * desc)
{
    liop_fill_patch(I, stride, center_x, center_y, radius, patch);
    vl_liopdesc_process(liop, desc, patch);
}


bool descriptor_compute(const uint8_t * image, int width, int height, int stride, float cx, float cy, float radius, descriptor & desc)
{
    if(cx + radius >= width || cx - radius < 0 || cy + radius >= height || cy - radius < 0)
        return false;

    VlLiopDesc * liop = vl_liopdesc_new(liop_neighbors, liop_spatial_bins, liop_sample_radius, liop_side_length);

    liop_compute_desc(liop, image, stride, cx, cy, radius, desc.patch, desc.d);

    vl_liopdesc_delete(liop);

    return true;
}

float descriptor_dist2(const descriptor & d1, const descriptor & d2)
{
    float dist2 = 0;
    for(int i = 0; i < descriptor_size; i++) {
        dist2 += (d1.d[i] - d2.d[i])*(d1.d[i] - d2.d[i]);
    }
    // since d1 and d2 guaranteed norm to 1, can do ||d1 - d2|| = sqrt(1 + 1 - 2*d1*d2)
    return dist2;
}
