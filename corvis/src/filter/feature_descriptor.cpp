#include "feature_descriptor.h"
#include <stdio.h>
 
void liop_fill_patch(const uint8_t * I, int stride, float center_x, float center_y, int side_length, float * patch)
{
    int i = 0;
    int lx = center_x - side_length/2;
    int ly = center_y - side_length/2;
    for(int dy = 0; dy < side_length; dy++) {
        for(int dx = 0; dx < side_length; dx++) {
            patch[i++] = I[(ly + dy)*stride + (lx + dx)]/1.;
        }
    }
}

void liop_compute_desc(VlLiopDesc * liop, const uint8_t * I, int stride, float center_x, float center_y, int side_length, float * patch, float * desc)
{
    liop_fill_patch(I, stride, center_x, center_y, side_length, patch);
    vl_liopdesc_process(liop, desc, patch);
}


bool descriptor_compute(const uint8_t * image, int width, int height, int stride, float cx, float cy, int side_length, float radius, descriptor & desc)
{
    if(cx + side_length/2 >= width || cx - side_length/2 < 0 ||
       cy + side_length/2 >= height || cy - side_length/2 < 0)
        return false;

    float sample_radius = 5;
    VlLiopDesc * liop = vl_liopdesc_new(liop_neighbors, liop_spatial_bins, sample_radius, side_length);

    float patch[side_length*side_length];
    liop_compute_desc(liop, image, stride, cx, cy, side_length, patch, desc.d);
    // convert the patch to uint8
    for(int i =0; i < side_length*side_length; i++)
        desc.patch[i] = patch[i];

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
