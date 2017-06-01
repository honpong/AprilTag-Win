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


#define FAST_ROWS 7
#define TOTAL_ROWS (FAST_ROWS + 1)
#define DMA_MASK NUM_ROWS

static byte __attribute__((section(".cmx.data")))  dataBuffer[TOTAL_ROWS * MAX_WIDTH]; //8 lines
static byte __attribute__((section(".cmx.data")))  singleFeatureBuffer[128];
static u8 __attribute__((section(".cmx.data")))  scoreBuffer[MAX_WIDTH + 4];
static u16 __attribute__((section(".cmx.data")))  baseBuffer[MAX_WIDTH + 2];
dmaTransactionList_t  dmaFastTask[TOTAL_ROWS];
dmaTransactionList_t  dmaSingleFeatureTask[1];
dmaTransactionList_t  dmaOutTask[2];

//NCC: use with threshold of -0.50 - -0.70(we negate at the bottom to get error-like value
//NCC doesn't seem to benefit from double-weighting the center
float inline fast_detector_9::score_match(byte **pFastLines, const int x1, const int x2, float max_error, unsigned short mean1)
{
    int window = patch_win_half_width;
    short mean2 = compute_mean7x7(pFastLines, x2);
    const unsigned char *p1 = singleFeatureBuffer + x1 - window;
    const unsigned char *p2;
    int8  top = 0, bottom1 = 0, bottom2 = 0;

    for(int dy = 0; dy < FAST_ROWS; ++dy, p1+=patch_stride) {
        p2 = pFastLines[dy] + x2 - window;
        int8 t1 = {p1[0], p1[1], p1[2], p1[3], p1[4], p1[5], p1[6], 0};
        t1 = t1 - (int)mean1;
        int8 t2 = {p2[0], p2[1], p2[2], p2[3], p2[4], p2[5], p2[6], 0};
        t2 = t2 - (int)mean2;
        int8 t12 = t1 *t2;;
        top += t12;
        int8 t1square = t1 * t1;
        int8 t2square = t2 * t2;
        bottom1 += t1square;
        bottom2 += t2square;
        if( (dy > 1) && (dy < 5))
        {
            top.s234 += t1.s234 * t2.s234;
            bottom1.s234 += (t1.s234 * t1.s234);
            bottom2.s234 += (t2.s234 * t2.s234);
        }
    }

    float sumTop = (float)top[0] + (float)top[1] + (float)top[2]+ (float)top[3] +
            (float)top[4] + (float)top[5] + (float)top[6];
    float sumBottom1 = (float)bottom1[0] + (float)bottom1[1] + (float)bottom1[2]+ (float)bottom1[3] +
            (float)bottom1[4] + (float)bottom1[5] + (float)bottom1[6];
    float sumBottom2 = (float)bottom2[0] + (float)bottom2[1] + (float)bottom2[2]+ (float)bottom2[3] +
            (float)bottom2[4] + (float)bottom2[5] + (float)bottom2[6];
    // constant patches can't be matched
    if(sumBottom1 < 1e-15 || sumBottom2 < 1e-15 || sumTop < 0.f){
      return max_error;
   }
    return (sumTop * sumTop)/(sumBottom1 * sumBottom2);
}

/*
 * compute the mean of a 7x7 patch
 * double the weight of the inner 3x3
 * neglects the value after the decimal point
 */
unsigned short inline fast_detector_9::compute_mean7x7(u8 **pPatch, const int x)
{
    int full = patch_win_half_width * 2 + 1;
    int area = full * full + 3 * 3;

    ushort8 sum = 0;

    for (int y = 0; y < FAST_ROWS; ++y) {
        //TODO: gather load
        u8* pLine = pPatch[y] + x - patch_win_half_width;
        ushort8 line = {pLine[0], pLine[1], pLine[2], pLine[3], pLine[4], pLine[5], pLine[6], 0};
        sum += line;
        if(y > 1 && y < 5){
            sum.s234 += line.s234;
        }
    }
    //TODO: reduction function
    unsigned short sum1 = sum[0] + sum[1] + sum [2]+ sum[3] + sum[4] + sum [5] + sum[6];
    unsigned short mean = sum1 / area;
    return mean;
}

void fast_detector_9::init(const int x, const int y, const int s, const int ps, const int phw)
{
    xsize = x;
    ysize = y;
    stride = s;
    patch_stride = ps;
    patch_win_half_width = phw;

}


void fast_detector_9::detect(const u8* pImage,
		int bthresh,
		int winx,
		int winy,
		int winwidth,
		int winheight,
		u8* pScores,
		u16* pOffsets) {


	byte* pFastLines[TOTAL_ROWS];
	u8* pFileterLines[FAST_ROWS];
	int y, x1, y1, x2, y2, paddedWidth, width;

	x1 = (winx < 8) ? 8 : winx;
	y1 = (winy < 8) ? 8 : winy;
	x2 = (winx + winwidth > xsize - 8) ? xsize - 8 : winx + winwidth;
	y2 = (winy + winheight > ysize - 8) ? ysize - 8 : winy + winheight;
	width = x2-x1;
	paddedWidth = x2 - x1 + 2 * PADDING;
#ifdef DEBUG_PRINTS
	int shaveNum = scGetShaveNumber();
    if(shaveNum == 1)
    	printf("shave %d x1 %d x2 %d y1 %d y2 %d width %d paddedwidth %d\n",shaveNum, x1, x2, y1, y2, width, paddedWidth);
#endif
    pFastLines[0] = dataBuffer;
    for(int i = 1; i <= FAST_ROWS; ++i)
	{
		pFastLines[i] = pFastLines[i-1] + paddedWidth;
	}

	//dma transactions
	dmaTransactionList_t* dmaRef[FAST_ROWS + 1];
	dmaTransactionList_t* dmaOut[2];

    u32 dmaRequsterId = dmaInitRequester(1);

    //read 7 lines for the fast algorithm
    for(int i = 0; i < FAST_ROWS; ++i)
    {
    	dmaRef[i] = dmaCreateTransaction(dmaRequsterId, &dmaFastTask[i], (u8*)(pImage + (y1 - 3 + i) * stride + x1 - PADDING), pFastLines[i], paddedWidth);
    }
    dmaLinkTasks(dmaRef[0], 6, dmaRef[1],dmaRef[2],dmaRef[3],dmaRef[4],dmaRef[5],dmaRef[6]);
    dmaStartListTask(dmaRef[0]);
	dmaWaitTask(dmaRef[0]);
#ifdef DEBUG_PRINTS
	if(shaveNum == 1)
		printf("shaveNum %d got 7 lines\n", shaveNum);
#endif

	for (y = y1; y < y2; y++)
	{
		const u8* pSrc = pImage + (y + 4) * stride + x1 - PADDING;
		u8* pScoresOut = pScores + y * (MAX_WIDTH + 4);
		u16* pOffsetsOut = pOffsets + y * (MAX_WIDTH + 2);

		//every eight rows bring a mask line TODO:bring one in advance
	   /*if(0 == y % 8)
	   {
		   dmaRef[DMA_MASK] = dmaCreateTransaction(dmaRequsterId, &dmaTask[DMA_MASK], (u8*)(mask + (y>>3) * xsize + (x1>>3)), maskBuffer, (winwidth>>3));
		   dmaStartListTask(dmaRef[DMA_MASK]);
		   dmaWaitTask(dmaRef[DMA_MASK]);
	   }*/

		//start dma transaction for row 8 - not waiting here
		dmaRef[FAST_ROWS] = dmaCreateTransaction(dmaRequsterId, &dmaFastTask[FAST_ROWS], (u8*)pSrc, pFastLines[FAST_ROWS], paddedWidth);
		dmaStartListTask(dmaRef[FAST_ROWS]);

		//call fast 9 filter
		for(int i = 0; i < FAST_ROWS; ++i){
			pFileterLines[i] = pFastLines[i] + PADDING;
		}
		mvcvFast9M2_asm(pFileterLines, scoreBuffer, baseBuffer, bthresh, width);

		//wait for the next line of data
		dmaWaitTask(dmaRef[FAST_ROWS]);

		//output dma
		int numberOfScores = *(unsigned int*)scoreBuffer + 4;
		int numOfOffsets = *(unsigned int*)baseBuffer * 2 + 4;

		dmaOut[0] = dmaCreateTransaction(dmaRequsterId, &dmaOutTask[0], scoreBuffer, pScoresOut, numberOfScores);
		dmaOut[1] = dmaCreateTransaction(dmaRequsterId, &dmaOutTask[1], (u8*)baseBuffer, (u8*)pOffsetsOut, numOfOffsets);
		dmaLinkTasks(dmaOut[0], 1, dmaOut[1]);
		dmaStartListTask(dmaOut[0]);

		//roll buffers
		u8* temp = pFastLines[0];
		for(int i = 0; i < FAST_ROWS; ++i)
		{
			pFastLines[i] = pFastLines[i+ 1];
		}
		pFastLines[FAST_ROWS] = temp;
		dmaWaitTask(dmaOut[0]);
	}

}


xy fast_detector_9::track(u8* im1,
							const u8* im2,
							int xcurrent,
							int ycurrent,
							float predx,
							float predy,
							float radius,
							int bthresh,
							float min_score) {

	int x, y, x1, y1, x2, y2, paddedWidth, width;
	xy pBest = { -1, -1, min_score, 0.f };

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

	//dma transactions
	dmaTransactionList_t* dmaRef[FAST_ROWS + 1];
	dmaTransactionList_t* dmaSingleFeature[1];

	u32 dmaRequsterId = dmaInitRequester(1);

	//read 7 lines for the fast algorithm
	for (int i = 0; i < FAST_ROWS; ++i) {
		dmaRef[i] = dmaCreateTransaction(dmaRequsterId, &dmaFastTask[i],
				(u8*) (im2 + (y1 - 3 + i) * stride + x1 - PADDING),
				pFastLines[i], paddedWidth);
	}
	dmaSingleFeature[0] = dmaCreateTransaction(dmaRequsterId, &dmaSingleFeatureTask[0], (u8*) (im1), singleFeatureBuffer, patch_stride * patch_stride);
	dmaLinkTasks(dmaRef[0], 7, dmaRef[1], dmaRef[2], dmaRef[3], dmaRef[4],
			dmaRef[5], dmaRef[6], dmaSingleFeature[0]);
	dmaStartListTask(dmaRef[0]);
	dmaWaitTask(dmaRef[0]);

	u8* patch[7];
	for(int i = 0; i < 7; ++i){
	    patch[i] = singleFeatureBuffer + i * (patch_win_half_width * 2 + 1);
	}
	unsigned short mean1 = compute_mean7x7(patch, 3);

	for (y = y1; y < y2; y++) {
		const byte* pSrc = im2 + y * stride;

		//start dma transaction for row 8 - not waiting here
		dmaRef[FAST_ROWS] = dmaCreateTransaction(dmaRequsterId,
				&dmaFastTask[FAST_ROWS],
				(u8*) (pSrc + 4 * stride + x1 - PADDING), pFastLines[FAST_ROWS],
				paddedWidth);
		dmaStartListTask(dmaRef[FAST_ROWS]);

		//call fast 9 filter
		mvcvFast9M2_asm(pFastLines, scoreBuffer, baseBuffer, bthresh, paddedWidth);

		//wait for the next line of data
		dmaWaitTask(dmaRef[FAST_ROWS]);

		int numberOfScores = *(unsigned int*) scoreBuffer;
		//for each of the scores found, call score matching
		for (int i = 0; i < numberOfScores; ++i) {
			x = (int) *(baseBuffer + 2 + i); // 0 <= x <= 10
            if (x < PADDING || x >= width + PADDING) continue;

			float score = score_match(pFastLines, xcurrent,
			                    x, pBest.score, mean1);

			if (score > pBest.score) {
				pBest.x = (float) x + x1 - PADDING;
				pBest.y = (float) y;
				pBest.score = score;
			}
		}

		//roll buffers
		u8* temp = pFastLines[0];
		for (int i = 0; i < FAST_ROWS; ++i) {
			pFastLines[i] = pFastLines[i + 1];
		}
		pFastLines[FAST_ROWS] = temp;

	}
	return pBest;
}

void fast_detector_9::trackFeature(TrackingData* trackingData,int index, const uint8_t* image, xy* out)
{
	TrackingData& data = trackingData[index];
	int fast_track_threshold = 5;
	float fast_track_radius = 5.5f;
	float fast_min_match = 0.2f*0.2f;
	float fast_good_match = 0.65f*0.65f;
	xy bestkp = track(data.patch, image,
	                patch_win_half_width, patch_win_half_width,
	                data.x1, data.y1, fast_track_radius,
	                fast_track_threshold, fast_min_match);
//
	// Not a good enough match, try the filter prediction
	if(bestkp.score < fast_good_match) {
		xy bestkp2 = track(data.patch, image,
				patch_win_half_width, patch_win_half_width,
				data.x2, data.y2, fast_track_radius,
				fast_track_threshold, bestkp.score);
		if(bestkp2.score > bestkp.score)
			bestkp = bestkp2;
	}

	// Still no match? Guess that we haven't moved at all
	if(bestkp.score < fast_min_match) {
		xy bestkp2 = track(data.patch, image,
				patch_win_half_width, patch_win_half_width,
				data.x3, data.y3, fast_track_radius,
				fast_track_threshold, bestkp.score);
		if(bestkp2.score > bestkp.score)
			bestkp = bestkp2;
	}

	out[index].x = bestkp.x;
	out[index].y = bestkp.y;
	out[index].score = bestkp.score;
}
