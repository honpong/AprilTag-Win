#include "fast_tracker_9.hpp"
#include "swcCdma.h"
#include "fast9M2.h"
#include "svuCommonShave.h"
#include <math.h>
#include <string.h> //memcpy

void fast_tracker_9::init(const int x, const int y, const int s, const int ps, const int phw)
{
    xsize = x;
    ysize = y;
    stride = s;
    patch_stride = ps;
    patch_win_half_width = phw;

}

xy fast_tracker_9::track(u8* im1, const u8* im2, float predx, float predy, float radius, int bthresh, float max_distance) {
	int x, y, x1, y1, x2, y2, paddedWidth, width;
	xy pBest = {INFINITY, INFINITY, max_distance, 0};
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

            mean2 = compute_mean7x7_from_pointer_array(x,patch_win_half_width ,pFastLines) ;
            float distance = score_match_from_pointer_array(patch1_pa, pFastLines,patch_win_half_width, x, patch_win_half_width, mean1, mean2);

			if (distance < pBest.score) {
				pBest.x = (float) x + x1 - PADDING;
				pBest.y = (float) y;
				pBest.score = distance;
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

void fast_tracker_9::trackFeature(TrackingData* trackingData,int index, const uint8_t* image, xy* out)
{
	TrackingData& data = trackingData[index];
	xy bestkp = {INFINITY, INFINITY, patch_max_track_distance, 0};
	if (data.x_dx != INFINITY)
	    bestkp = track(data.patch, image,
	                   data.x_dx, data.y_dy, fast_track_radius,
	                   fast_track_threshold, bestkp.score);
//
	// Not a good enough match, try the filter prediction
	if(data.pred_x != INFINITY && bestkp.score > patch_good_track_distance) {
		xy bestkp2 = track(data.patch, image,
				data.pred_x, data.pred_y, fast_track_radius,
				fast_track_threshold, bestkp.score);
		if(bestkp2.score < bestkp.score)
			bestkp = bestkp2;
	}

	out[index].x = bestkp.x;
	out[index].y = bestkp.y;
	out[index].score = bestkp.score;
}

/*--------------------------------------------*/
static fast_tracker_9 tracker;
extern "C" void fast_track(TrackingData *data, int size, xy *out, const uint8_t *image, int stride, int2 image_size, int full_patch_width, int half_patch_width)
{
	tracker.init(image_size.x, image_size.y, stride, full_patch_width, half_patch_width);
	for (int i = 0; i < size; i++)
		tracker.trackFeature(data, i, image, out);

	SHAVE_HALT;
	return;
}
