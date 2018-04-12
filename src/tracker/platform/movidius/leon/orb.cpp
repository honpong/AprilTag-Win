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

void compute_orb_multiple_shaves(const tracker::image &image, orb_desc_keypoint *keypoints, size_t num_keypoints)
{
    assert(num_kpoints <= MAX_NUM_DESC);
    static std::mutex orb_shaves_mutex; std::lock_guard<std::mutex> lock(orb_shaves_mutex);
    for(size_t i = 0; i < num_keypoints; ++i)
        kp_xy[i] = (float2_t*)keypoints[i].xy.data();

    // at least one descriptor to compute
    if (num_keypoints) {
        // distribute keypoints among shaves
        for (int i = 0; i< ORB_SHAVES; ++i) {
            // launch orb computation on shave
            Shave::get_handle(compute_descriptors[i].shave)->start(
                    (u32)compute_descriptors[i].entry_point, "iiii",
                    &descriptors[i* num_keypoints / ORB_SHAVES],
                    kp_xy[i * num_keypoints / ORB_SHAVES],
                    image,
                    num_keypoints*(i+1)/ORB_SHAVES-num_keypoints*i/ORB_SHAVES);
        }

        // wait until all shaves finish
        for (int i = 0; i < ORB_SHAVES; ++i)
            Shave::get_handle(compute_descriptors[i].shave)->wait();

        // copy in the same order of keypoints
        for(size_t i = 0; i < num_keypoints; ++i) {
            auto &orb_dec = keypoints[i].kp->descriptor;
            orb_dec.orb_computed = true;
            memcpy(orb_dec.orb.descriptor.data(), descriptors[i].d.data(), orb_length);
            orb_dec.orb.cos_ = descriptors[i].cos_;
            orb_dec.orb.sin_ = descriptors[i].sin_;
        }
    }
}
