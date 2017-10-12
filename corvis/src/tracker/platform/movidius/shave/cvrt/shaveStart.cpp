#include <mv_types.h>
#include <moviVectorTypes.h>
#include "fast_detector_9.hpp"
#include "commonDefs.hpp"

static fast_detector_9 detector;

extern "C" void fast_detect(u8 *scores, u16 *offsets, int threshold, int2 xy, const uint8_t *image, int stride, int2 image_size, int2 win_size)
{
	detector.init(image_size.x, image_size.y, stride, win_size.x, win_size.y);
	detector.detect(image, threshold, xy.x, xy.y, win_size.x, win_size.y, scores, offsets);

    SHAVE_HALT;
    return;
}

extern "C" void fast_track(TrackingData *data, int size, xy *out, const uint8_t *image, int stride, int2 image_size, int full_patch_width, int half_patch_width)
{
	detector.init(image_size.x, image_size.y, stride, full_patch_width, half_patch_width);
	for (int i = 0; i < size; i++)
		detector.trackFeature(data, i, image, out);

	SHAVE_HALT;
	return;
}
