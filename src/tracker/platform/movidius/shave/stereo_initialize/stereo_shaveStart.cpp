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


extern "C" void stereo_match(ShavekpMatchingSettings *kpMatchingParams)
{
    stereo_matching_o.init(*kpMatchingParams);
    stereo_matching_o.stereo_kp_matching_and_compare(kpMatchingParams->kp1, kpMatchingParams->kp2,
            kpMatchingParams->patches1, kpMatchingParams->patches2, kpMatchingParams->depth1,
            kpMatchingParams->depth2, kpMatchingParams->matched_kp);

SHAVE_HALT;
return;
}
