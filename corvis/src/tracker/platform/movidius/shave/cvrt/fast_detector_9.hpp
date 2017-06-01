/*
 * fast_detector_9.hpp
 *
 *  Created on: Nov 6, 2016
 *      Author: itzhakt
 */

#ifndef SHAVE_FAST_DETECTOR_9_HPP_
#define SHAVE_FAST_DETECTOR_9_HPP_
#include <mv_types.h>
#include "commonDefs.hpp"

typedef struct { float x, y, score, reserved;} xy;
typedef unsigned char byte;

class fast_detector_9 {
 private:
    int xsize, ysize, stride, patch_stride, patch_win_half_width;
    float inline score_match(byte **pFastLines, const int x1, const int x2, float max_error, unsigned short mean1);

    unsigned short inline compute_mean7x7(u8** pPatch, const int x);
    void detectC(u8** pFastLines, u8* pscores, u16* pOffsets, int bthresh,  int width);
 public:
    //std::vector<xy> features;
    void detect(const u8* pImage,
    		int bthresh,
    		int winx,
    		int winy,
    		int winwidth,
    		int winheight,
    		u8* pScores,
    		u16* pOffsets);
    //void detect(const unsigned char *im, const u8 *mask, int number_wanted, int bthresh, u8* pFeatures) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize, pFeatures); }
    //xy track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, float predx, float predy, float radius, int b, float min_score);
    void init(const int image_width, const int image_height, const int image_stride, const int tracker_patch_stride, const int tracker_patch_win_half_width);
    xy track(u8* im1,
    			const u8* im2,
    			int xcurrent,
    			int ycurrent,
    			float predx,
    			float predy,
    			float radius,
    			int bthresh,
    			float min_score);
    void trackFeature(TrackingData* trackingData,int index, const uint8_t* image, xy* out);

};


#endif /* SHAVE_FAST_DETECTOR_9_HPP_ */
