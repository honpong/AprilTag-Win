#include "fast_detect.h"
#include "tracker.h"
#include "Shave.h"
#include "DrvSvu.h"

#include <algorithm>

struct shave_entry_point { int shave; u32 *entry_point; };

extern u32 detect4_fast_detect;
extern u32 detect8_fast_detect;
extern u32 detect9_fast_detect;
extern u32 detect10_fast_detect;

#define DETECT_SHAVES 4
static shave_entry_point fast_detect[DETECT_SHAVES] = {
    {4,  &detect4_fast_detect},
    {8,  &detect8_fast_detect},
    {9,  &detect9_fast_detect},
    {10, &detect10_fast_detect},
};

fast_tracker::xy *platform_fast_detect(size_t id, const tracker::image &image, scaled_mask &mask, size_t need, size_t &found)
{
    __attribute__((section(".cmx_direct.data"))) static unsigned threshold;
    __attribute__((section(".cmx_direct.data"))) static fast_tracker::xy cmx_features[500];
    __attribute__((section(".cmx_direct.data"))) static size_t cmx_features_size = sizeof(cmx_features)/sizeof(*cmx_features);
    assert(sizeof(cmx_features)/sizeof(*cmx_features) >= need + 1); // Note that features should have at least need+1 entries on entry (as we use a heap and need to push (before we can pop) when full)

    threshold = fast_detect_threshold;
    cmx_features_size = 0;
    for (int i=0; i< DETECT_SHAVES; ++i) {
        struct int2 { int x,y; } image_size, win_xy, win_size;
        DrvSvuL1DataCacheCtrl(fast_detect[i].shave, SVUL1DATACACHE_INVALIDATE_ALL);
        //OsDrvShaveL2CachePartitionInvalidate(DETECT_PARTITION);
        Shave::get_handle(fast_detect[i].shave)->start(
            (u32)fast_detect[i].entry_point, "ivi" "vv" "iii" "iii",
            image.image,
            &(image_size = (int2) { image.width_px, image.height_px }),
            image.stride_px,

            &(win_xy   = (int2) { 0,               i    * image.height_px / DETECT_SHAVES }),
            &(win_size = (int2) { image.width_px, (i+1) * image.height_px / DETECT_SHAVES
                                                 - i    * image.height_px / DETECT_SHAVES }),
            need,
            cmx_features,
            &cmx_features_size,

            &threshold,
            &mask.mask[0],
            mask.scaled_width
        );
    }
    for (int i = 0; i < DETECT_SHAVES; ++i)
        Shave::get_handle(fast_detect[i].shave)->wait();

    std::sort_heap(cmx_features, cmx_features+cmx_features_size, fast_tracker::xy_comp);

    found = cmx_features_size;
    return cmx_features;
}
