#ifndef __FAST9M2_H__
#define __FAST9M2_H__

#include <mv_types.h>
#include <mvcv_macro.h>

//!@{
/// This kernel identify in one image line the position where the image have other context
/// @param[in] row       - Input lines of the image, 7 lines
/// @param[out] score    - First unsigned int contain number of valid points in the line, after score for every point
/// @param[out] base     - First unsigned int contain number of valid points in the line, after valid pixel position
/// @param[in] thresh    - Therehold, define sensitivity of the filter
/// @param[in] width     - Width of the input lines
/// @return    Nothing

MVCV_FUNC(void, mvcvFast9M2, u8** row, u8* score, u16 *base, unsigned int thresh, unsigned int width)
//!@}
	
#endif //__FAST9M2_H__