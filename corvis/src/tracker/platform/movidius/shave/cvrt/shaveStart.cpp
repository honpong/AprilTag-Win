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

extern ShaveFastDetectSettings detectParams;

extern "C" void fast9Detect(const unsigned char *pImage, u8* pScores, u16* pOffsets) {
	detector.init(detectParams.imgWidth, detectParams.imgHeight, detectParams.imgStride, detectParams.winWidth, detectParams.halfWindow );
	detector.detect(pImage, detectParams.threshold , detectParams.x, detectParams.y, detectParams.winWidth, detectParams.winHeight, pScores, pOffsets);

	SHAVE_HALT;

	return;
}

extern "C" void fast9Track(TrackingData* trackingData, int size, const uint8_t *image, xy* out) {

	detector.init(detectParams.imgWidth, detectParams.imgHeight, detectParams.imgStride, detectParams.winWidth, detectParams.halfWindow );
	for (int i = 0; i < size; i++)
	{
		detector.trackFeature(trackingData, i, image, out);
	}
	SHAVE_HALT;

	return;
}


