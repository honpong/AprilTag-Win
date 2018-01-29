#pragma once

#include <mv_types.h>
#include "commonDefs.hpp"
#include "../stereo_initialize/common_shave.h"

typedef unsigned char byte;

class fast_detector_9 {

public:
    void detect(const u8* pImage, int bthresh, int winx, int winy, int winwidth, int winheight, u8* pScores, u16* pOffsets, int image_width, int image_height, int image_stride);
private:

    static constexpr int FAST_ROWS = 7;
    static constexpr int TOTAL_ROWS = FAST_ROWS + 1;

    byte dataBuffer[TOTAL_ROWS * MAX_WIDTH]; //8 lines
    u8 scoreBuffer[MAX_WIDTH + 4];
    u16 baseBuffer[MAX_WIDTH + 2];
};
