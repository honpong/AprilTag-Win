/*
 * stereo_matching.h
 *
 *  Created on: May 16, 2017
 *      Author: Amir Shalev
 */

#ifndef CORE_TARGET_SHAVE_STEREO_MATCHING_H_
#define CORE_TARGET_SHAVE_STEREO_MATCHING_H_
#include <mv_types.h>
#include <moviVectorUtils.h>
#include <math.h>
#include "stereo_commonDefs.hpp"
#include "../../../../../feature/tracker/fast_constants.h"
// 2:  Source Specific #defines and types  (typedef,enum,struct)
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------
// 5: Static Function Prototypes
struct feature_t
{
    uint8_t patch[full_patch_width*full_patch_width];
    //unsigned char *patch;
    float dx, dy;
};

    class stereo_matching
{
    private:
        //compare
        int patch_stride, patch_win_half_width;
        //kp_intersect
        float4x4 R1w_transpose;
        float4x4 R2w_transpose;
        float4 camera1_extrinsics_T_v;
        float4 camera2_extrinsics_T_v;
        float4 p_o1_transformed;
        float4 p_o2_transformed;
        float EPS;
        bool l_l_intersect_shave(int i , int j ,float4 *pa,float4 *pb);

    public:
        void init(ShavekpMatchingSettings kpMatchingParams);
        void stereo_kp_matching_and_compare(u8* p_kp1, u8* p_kp2, u8 * patches1[] , u8 * patches2[], float * depths1, float* depth2, float * errors1, int* matched_kp);
};
#endif /* CORE_TARGET_SHAVE_STEREO_MATCHING_H_ */
