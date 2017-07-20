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
#include "stereo_commonDefs.hpp"
#include "stereo_matching.h"


// 2:  Source Specific #defines and types  (typedef,enum,struct)
//#define DEBUG_PRINTS
//#include <stdio.h>
// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
//extern ShavekpMatchingSettings cvrt0_kpMatchingParams;
// ----------------------------------------------------------------------------
// 4: Static Local Data
static stereo_matching stereo_matching_o;
// ----------------------------------------------------------------------------
// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------


extern "C" void stereo_kp_matching_and_compare(ShavekpMatchingSettings *kpMatchingParams, u8* p_kp1, u8* p_kp2, u8 * patches1[] , u8 * patches2[], float * depths1)
{
    stereo_matching_o.init(*kpMatchingParams);
    stereo_matching_o.stereo_kp_matching_and_compare(p_kp1, p_kp2, patches1, patches2, depths1);

SHAVE_HALT;
return;
}
