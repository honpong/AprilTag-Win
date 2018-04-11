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
    static std::mutex detect_shaves_mutex; std::lock_guard<std::mutex> lock(detect_shaves_mutex); // avoid other ids clearing our L1 cache below

    struct feature_storage {
        unsigned threshold;
        std::array<fast_tracker::xy, 500> features;
        size_t features_size;
    };
    __attribute__((section(".cmx_direct.data")))
    static std::array<feature_storage, 2> storage;
    feature_storage *cmx = &storage[id];

    assert(id < storage.size());
    assert(need + 1 < cmx->features.size()); // Note that features should have at least need+1 entries on entry (as we use a heap and need to push (before we can pop) when full)

    cmx->threshold = fast_detect_threshold;
    cmx->features_size = 0;
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
            cmx->features.data(),
            &cmx->features_size,

            &cmx->threshold,
            &mask.mask[0],
            mask.scaled_width
        );
    }
    for (int i = 0; i < DETECT_SHAVES; ++i)
        Shave::get_handle(fast_detect[i].shave)->wait();

    found = cmx->features_size;
    return cmx->features.data();
}
