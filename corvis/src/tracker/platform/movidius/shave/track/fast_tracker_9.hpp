#pragma once

#include <mv_types.h>
#include "image_defines.h"
#include "../stereo_initialize/common_shave.h"

typedef struct { float x, y, score, reserved;} xy;
typedef unsigned char byte;

class fast_tracker_9 {
 private:
    int xsize, ysize, stride, patch_stride, patch_win_half_width;

 public:
    void init(const int image_width, const int image_height, const int image_stride, const int tracker_patch_stride, const int tracker_patch_win_half_width);
    xy track(u8* im1, const u8* im2, float predx, float predy, float radius, int bthresh, float max_distance);
    void trackFeature(TrackingData* trackingData,int index, const uint8_t* image, xy* out);
private:

    static constexpr int FAST_ROWS = 7;
    static constexpr int TOTAL_ROWS = FAST_ROWS + 1;

    byte singleFeatureBuffer[128];
    u8 scoreBuffer[MAX_WIDTH + 4];
    u16 baseBuffer[MAX_WIDTH + 2];
};
