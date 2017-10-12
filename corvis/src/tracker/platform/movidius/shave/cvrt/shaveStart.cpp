///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>
#include <moviVectorTypes.h>
#include <swcFrameTypes.h>
#include <svuCommonShave.h>
#include "fast_detector_9.hpp"
#include "commonDefs.hpp"


// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------
// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------

static fast_detector_9 detector;

extern "C" void fast9Detect(u8 *scores, u16 *offsets, int threshold, int2 xy, const uint8_t *image, int stride, int2 image_size, int2 win_size)
{
	detector.init(image_size.x, image_size.y, stride, win_size.x, win_size.y);
	detector.detect(image, threshold, xy.x, xy.y, win_size.x, win_size.y, scores, offsets);

    SHAVE_HALT;
    return;
}

extern "C" void fast9Track(TrackingData *data, int size, xy *out, const uint8_t *image, int stride, int2 image_size, int full_patch_width, int half_patch_width)
{
	detector.init(image_size.x, image_size.y, stride, full_patch_width, half_patch_width);
	for (int i = 0; i < size; i++)
		detector.trackFeature(data, i, image, out);

	SHAVE_HALT;
	return;
}


