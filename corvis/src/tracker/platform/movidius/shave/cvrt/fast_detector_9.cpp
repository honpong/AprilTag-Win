/*
 * fast_detector_9.cpp
 *
 *  Created on: Nov 6, 2016
 *      Author: itzhakt
 */
#include "fast_detector_9.hpp"
#include "swcCdma.h"
#include "fast9M2.h"
#include "svuCommonShave.h"
#include <math.h>
#include <string.h> //memcpy


void fast_detector_9::init(const int x, const int y, const int s, const int ps, const int phw)
{
    xsize = x;
    ysize = y;
    stride = s;
    patch_stride = ps;
    patch_win_half_width = phw;

}

void fast_detector_9::detect(const u8 *pImage, int bthresh, int winx, int winy, int winwidth, int winheight, u8 *pScores, u16 *pOffsets) {
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
            mvcvFast9M2_asm(lines, scoreBuffer, baseBuffer, bthresh, width);

            dmaWaitTask(&dmaNextRow);
        }
        int  scoreSize = sizeof(u32) + *(u32*)scoreBuffer * sizeof(u8);
        int offsetSize = sizeof(u32) + *(u32*)baseBuffer  * sizeof(u16);
        u8 *pScoresY  =      pScores  + y * (sizeof(u32) + MAX_WIDTH * sizeof(u8));
        u8 *pOffsetsY = (u8*)pOffsets + y * (sizeof(u32) + MAX_WIDTH * sizeof(u16));
        dmaTransactionList_t dmaOutScores, dmaOutOffsets;
        dmaCreateTransaction(dmaId, &dmaOutScores, scoreBuffer, pScoresY, scoreSize);
        dmaCreateTransaction(dmaId, &dmaOutOffsets, (u8*)baseBuffer, pOffsetsY, offsetSize);
        dmaLinkTasks(&dmaOutScores, 1, &dmaOutOffsets);
        dmaStartListTask(&dmaOutScores);
        dmaWaitTask(&dmaOutScores);
    }
}


xy fast_detector_9::track(u8* im1,
							const u8* im2,
							int xcurrent,
							float predx,
							float predy,
							float radius,
							int bthresh,
							float min_score) {

	int x, y, x1, y1, x2, y2, paddedWidth, width;
	xy pBest = {INFINITY, INFINITY, min_score, 0};
	unsigned short mean1, mean2;
	x1 = (int) (predx - radius + 0.5);
	x2 = (int) (predx + radius - 0.5);
	y1 = (int) (predy - radius + 0.5);
	y2 = (int) (predy + radius - 0.5);

	if (x1 < PADDING || x2 >= xsize - PADDING || y1 < PADDING || y2 >= ysize - PADDING)
		return pBest;

	width = x2 - x1;
	paddedWidth = x2 - x1 + 2 * PADDING;

	byte *pFastLines[TOTAL_ROWS];

    pFastLines[0] = dataBuffer;

	for (int i = 1; i <= FAST_ROWS; ++i) {
		pFastLines[i] = pFastLines[i - 1] + paddedWidth;
	}

	//read 7 lines for the fast algorithm
	for (int i = 0; i < FAST_ROWS; ++i) {
		memcpy(pFastLines[i], (u8*)(im2 + (y1 - 3 + i) * stride + x1 - PADDING), paddedWidth);
	}
	memcpy(singleFeatureBuffer, (u8*) (im1), patch_stride * patch_stride);

	u8* patch1_pa[7];
	for(int i = 0; i < 7; ++i){
	    patch1_pa[i] = singleFeatureBuffer + i * (patch_win_half_width * 2 + 1);
	}
	mean1 = compute_mean7x7_from_pointer_array(patch_win_half_width,patch_win_half_width ,patch1_pa) ;
	for (y = y1; y < y2; y++) {
		const byte* pSrc = im2 + y * stride;

		//call fast 9 filter
		mvcvFast9M2_asm(pFastLines, scoreBuffer, baseBuffer, bthresh, paddedWidth);

		int numberOfScores = *(unsigned int*) scoreBuffer;
		//for each of the scores found, call score matching
		for (int i = 0; i < numberOfScores; ++i) {
			x = (int) *(baseBuffer + 2 + i); // 0 <= x <= 10
            if (x < PADDING || x >= width + PADDING) continue;

			//float score = score_match(pFastLines, xcurrent,x, pBest.score, mean1);
            mean2 = compute_mean7x7_from_pointer_array(x,patch_win_half_width ,pFastLines) ;
            float score = score_match_from_pointer_array(patch1_pa, pFastLines,patch_win_half_width , x,patch_win_half_width,pBest.score ,mean1, mean2);

			if (score > pBest.score) {
				pBest.x = (float) x + x1 - PADDING;
				pBest.y = (float) y;
				pBest.score = score;
			}
		}

		//copy row 8
		memcpy(pFastLines[FAST_ROWS], (u8*) (pSrc + 4 * stride + x1 - PADDING), paddedWidth);
		//roll buffers
		u8* temp = pFastLines[0];
		for (int i = 0; i < FAST_ROWS; ++i) {
			pFastLines[i] = pFastLines[i + 1];
		}
		pFastLines[FAST_ROWS] = temp;

	}
	return pBest;
}

#include "fast_constants.h"
#include "patch_constants.h"

void fast_detector_9::trackFeature(TrackingData* trackingData,int index, const uint8_t* image, xy* out)
{
	TrackingData& data = trackingData[index];
	xy bestkp = {INFINITY, INFINITY, patch_min_score, 0};
	if (data.x_dx != INFINITY)
	    bestkp = track(data.patch, image,
	                   patch_win_half_width,
	                   data.x_dx, data.y_dy, fast_track_radius,
	                   fast_track_threshold, bestkp.score);
//
	// Not a good enough match, try the filter prediction
	if(data.pred_x != INFINITY && bestkp.score < patch_good_score) {
		xy bestkp2 = track(data.patch, image,
				patch_win_half_width,
				data.pred_x, data.pred_y, fast_track_radius,
				fast_track_threshold, bestkp.score);
		if(bestkp2.score > bestkp.score)
			bestkp = bestkp2;
	}

	out[index].x = bestkp.x;
	out[index].y = bestkp.y;
	out[index].score = bestkp.score;
}
