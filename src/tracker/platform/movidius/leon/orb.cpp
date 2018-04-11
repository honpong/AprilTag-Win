#include <rtems/rtems/cache.h>
#include <math.h>
#include <mv_types.h>
#include "tracker.h"
#include "orb.h"
#include "Shave.h"

#include "cor_types.h"

typedef float float2_t[2];

#define MAX_NUM_DESC 400
__attribute__((section(".cmx_direct.data"))) orb_data descriptors[MAX_NUM_DESC];
__attribute__((section(".cmx_direct.data"))) float2_t* kp_xy[MAX_NUM_DESC];

extern u32 orb4_compute_descriptors;
extern u32 orb8_compute_descriptors;
extern u32 orb9_compute_descriptors;
extern u32 orb10_compute_descriptors;

#define ORB_SHAVES 4
struct shave_entry_point { int shave; u32 *entry_point; };

shave_entry_point compute_descriptors[ORB_SHAVES] = {
    {4,  &orb4_compute_descriptors},
    {8,  &orb8_compute_descriptors},
    {9,  &orb9_compute_descriptors},
    {10, &orb10_compute_descriptors},
};

void compute_orb_multiple_shaves(const tracker::image &image, fast_tracker::fast_feature<patch_orb_descriptor>* keypoints[], const v2* keypoints_xy[],  size_t num_kpoints, int& actual_num_descriptors)
{
    static std::mutex orb_shaves_mutex; std::lock_guard<std::mutex> lock(orb_shaves_mutex);

    int dsize = (int)sizeof(keypoints[0]->descriptor.orb.descriptor);
    for(int i = 0; i < num_kpoints; ++i) {
        // check if descriptor has to be computed and update actual number of descriptors
        if(!keypoints[i]->descriptor.orb_computed) {
            kp_xy[i] = (float2_t*)keypoints_xy[i]->data();
            actual_num_descriptors++;
        }
    }

    // at least one descriptor to compute
    if (actual_num_descriptors) {
        // distribute keypoints among shaves
        for (int i = 0; i< ORB_SHAVES; ++i) {
            // launch orb computation on shave
            struct int2 { int x,y; } image_size;
            Shave::get_handle(compute_descriptors[i].shave)->start(
                    (u32)compute_descriptors[i].entry_point, "iiii",
                    &descriptors[i* actual_num_descriptors / ORB_SHAVES],
                    kp_xy[i * actual_num_descriptors / ORB_SHAVES],
                    image,
                    actual_num_descriptors*(i+1)/ORB_SHAVES-actual_num_descriptors*i/ORB_SHAVES);
        }

        // wait until all shaves finish
        for (int i = 0; i < ORB_SHAVES; ++i)
            Shave::get_handle(compute_descriptors[i].shave)->wait();

        // copy in the same order of keypoints
        int j = 0;
        for(int i = 0; i < num_kpoints; ++i) {
            if(!keypoints[i]->descriptor.orb_computed) {
                keypoints[i]->descriptor.orb_computed = true;
                memcpy(keypoints[i]->descriptor.orb.descriptor.data(), descriptors[j].d.data(), orb_length);
                keypoints[i]->descriptor.orb.cos_ = descriptors[j].cos_;
                keypoints[i]->descriptor.orb.sin_ = descriptors[j].sin_;
                j++;
            }
        }
    }
}
