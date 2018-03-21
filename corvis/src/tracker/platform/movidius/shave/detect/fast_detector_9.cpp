#include "fast_detector_9.hpp"
#include "swcCdma.h"
#include "fast9ScoreCv.h"
#include "svuCommonShave.h"
#include <math.h>
#include <string.h> //memcpy

static u8 bulkBuff[8 + 20*MAX_WIDTH];

void fast_detector_9::detect(const u8 *pImage, int bthresh, int winx, int winy, int winwidth, int winheight, u8 *pScores, u16 *pOffsets, const int xsize, const int ysize, const int stride) {
    int x1 = (winx < 8) ? 8 : winx;
    int y1 = (winy < 8) ? 8 : winy;
    int x2 = (winx + winwidth > xsize - 8) ? xsize - 8 : winx + winwidth;
    int y2 = (winy + winheight > ysize - 8) ? ysize - 8 : winy + winheight;
    int width = x2-x1;
    int paddedWidth = x2 - x1 + 2 * PADDING;
    u8 *topLeft = const_cast<u8*>(pImage) - 3 * stride + x1 - PADDING;
    u32 dmaId = dmaInitRequester(1);
    {
        dmaTransactionList_t dma7Rows;
        dmaCreateTransactionSrcStride(dmaId, &dma7Rows, topLeft + y1 * stride, dataBuffer, 7 * paddedWidth, paddedWidth, stride);
        dmaStartListTask(&dma7Rows);
        dmaWaitTask(&dma7Rows);
    }
    for (int y = y1, dy = 0; y < y2; y++, dy++) {
        {
            dmaTransactionList_t dmaNextRow;
            dmaStartListTask(dmaCreateTransaction(dmaId, &dmaNextRow, topLeft + (y + 7) * stride, dataBuffer + (dy + 7) % 8 * paddedWidth, paddedWidth));

            u8 *lines[7];
            for (int i=0; i<7; i++)
                lines[i] = dataBuffer + (dy + i) % 8 * paddedWidth + PADDING;
            mvcvfast9ScoreCv_asm(lines, scoreBuffer, baseBuffer, bthresh, width, (void*)bulkBuff);

            dmaWaitTask(&dmaNextRow);
        }
        memcpy(     pScores  + y * (sizeof(u32) + MAX_WIDTH * sizeof(u8)), scoreBuffer, sizeof(u32) + *(u32*)scoreBuffer * sizeof(u8));
        memcpy((u8*)pOffsets + y * (sizeof(u32) + MAX_WIDTH * sizeof(u16)), baseBuffer, sizeof(u32) + *(u32*)baseBuffer  * sizeof(u16));
    }
}

/*--------------------------------------------*/
static fast_detector_9 detector;
extern "C" void fast_detect(u8 *scores, u16 *offsets, int threshold, int2 xy, const uint8_t *image, int stride, int2 image_size, int2 win_size)
{
	detector.detect(image, threshold, xy.x, xy.y, win_size.x, win_size.y, scores, offsets, image_size.x, image_size.y, stride);

    SHAVE_HALT;
    return;
}
