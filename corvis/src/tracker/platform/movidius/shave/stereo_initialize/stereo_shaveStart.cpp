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
#define DEBUG_PRINTS
#include <stdio.h>
// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
extern ShavekpMatchingSettings kpMatchingParams;
// ----------------------------------------------------------------------------
// 4: Static Local Data
static stereo_matching stereo_matching_o;
// ----------------------------------------------------------------------------
// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------
extern "C" void stereo_kp_matching_main(u8* p_kp1, u8* p_kp2,int* kp_intersect,float* kp_intersect_depth)
{
    stereo_matching_o.init();
    stereo_matching_o.stereo_kp_matching(p_kp1,p_kp2,kp_intersect,kp_intersect_depth) ;
    //todo::Amir we can call init only at first time
SHAVE_HALT;
return;
}
