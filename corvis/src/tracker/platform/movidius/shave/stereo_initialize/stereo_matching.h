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

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------
// 5: Static Function Prototypes

class stereo_matching
{
    private:
        float4x4 R1w_transpose;
        float4x4 R2w_transpose;
        float4 camera1_extrinsics_T_v;
        float4 camera2_extrinsics_T_v;
        float4 p_o1_transformed;
        float4 p_o2_transformed;
        float EPS;
        bool l_l_intersect_shave(int i , int j ,float4 *pa,float4 *pb);


    public:
        void init();
        void stereo_kp_matching(u8* p_kp1, u8* p_kp2,int* kp_intersect , float* kp_intersect_depth);
};
#endif /* CORE_TARGET_SHAVE_STEREO_MATCHING_H_ */
