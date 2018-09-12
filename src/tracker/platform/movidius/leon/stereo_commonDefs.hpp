#pragma once

#include "state_size.h"
#define MAX_KP1 MAX_STANDBY_TRACKS
#define MAX_KP2 MAX_STANDBY_TRACKS

#define STEREO_SHAVES_USED 4

typedef float float4_t[4];
typedef float float3_t[3];
typedef float float4x4_t[4][4];

inline void l_float3_copy (volatile float3_t  dst , float3_t  src ) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

inline void l_float4_copy (volatile float4_t  dst , float4_t  src ) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

inline void l_float4x4_copy (volatile float4x4_t  dst , float4x4_t src ) {
    dst[0][0] = src[0][0];
    dst[0][1] = src[0][1];
    dst[0][2] = src[0][2];
    dst[0][3] = src[0][3];
    dst[1][0] = src[1][0];
    dst[1][1] = src[1][1];
    dst[1][2] = src[1][2];
    dst[1][3] = src[1][3];
    dst[2][0] = src[2][0];
    dst[2][1] = src[2][1];
    dst[2][2] = src[2][2];
    dst[2][3] = src[2][3];
    dst[3][0] = src[3][0];
    dst[3][1] = src[3][1];
    dst[3][2] = src[3][2];
    dst[3][3] = src[3][3];
}

struct ShavekpMatchingSettings {

    float4x4_t R1w_transpose;
    float4x4_t R2w_transpose;
    float4_t camera1_extrinsics_T_v;
    float4_t camera2_extrinsics_T_v;
    float3_t p_o1_transformed;
    float3_t p_o2_transformed;
    float EPS;
    int patch_stride;
    int patch_win_half_width;
    uint8_t* kp1;
    uint8_t* kp2;
    uint8_t** patches1;
    uint8_t** patches2;
    float*   depth1;
    float*   depth2;
    int*     matched_kp;

    ShavekpMatchingSettings(float4x4_t I_R1w_transpose, float4x4_t I_R2w_transpose,
                            float4_t I_camera1_extrinsics_T_v, float4_t I_camera2_extrinsics_T_v,
                            float3_t I_p_o1_transformed, float3_t I_p_o2_transformed,
                            float I_EPS, int half_patch_width, uint8_t* _kp1, uint8_t* _kp2,
                            uint8_t** _patches1, uint8_t** _patches2, float* _depth1, float* _depth2,
                            int* _matched_kp) :
        kp1(_kp1), kp2(_kp2), patches1(_patches1), patches2(_patches2),
        depth1(_depth1), depth2(_depth2), matched_kp(_matched_kp)
    {
        l_float4x4_copy (R1w_transpose , I_R1w_transpose );
        l_float4x4_copy (R2w_transpose , I_R2w_transpose );
        l_float4_copy (camera1_extrinsics_T_v , I_camera1_extrinsics_T_v );
        l_float4_copy (camera2_extrinsics_T_v , I_camera2_extrinsics_T_v );
        l_float3_copy (p_o1_transformed , I_p_o1_transformed );
        l_float3_copy (p_o2_transformed , I_p_o2_transformed );
        EPS = I_EPS;
        patch_stride=half_patch_width*2+1;
        patch_win_half_width=half_patch_width;
    }

    ShavekpMatchingSettings()
    {
        float4x4_t Z4x4 {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
        float4_t Z4 = {0,0,0,0};
        float3_t Z3 = {0,0,0};

        //l_float4x4
        l_float4x4_copy (R1w_transpose , Z4x4 );
        l_float4x4_copy (R2w_transpose  , Z4x4 );
        //l_float4;
        l_float4_copy ( camera1_extrinsics_T_v, Z4);
        l_float4_copy ( camera2_extrinsics_T_v ,Z4);
        //l_float3
        l_float3_copy (p_o1_transformed,Z3 );
        l_float3_copy (p_o2_transformed,Z3 );
        //float
        EPS = 0;
        patch_win_half_width= 0;
        patch_stride =0 ;
    }
};
