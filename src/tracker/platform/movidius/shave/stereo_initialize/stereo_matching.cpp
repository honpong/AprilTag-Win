/// @file

// 1: Includes
// ----------------------------------------------------------------------------
#include "stereo_matching.h"
#include "swcCdma.h"
#include <mv_types.h>
#include <swcFrameTypes.h>
#include <svuCommonShave.h>
#include <swcSIMDUtils.h>
#include <string.h>
#include <cmath>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
#include "stereo_commonDefs.hpp"
#include "common_shave.h"
#include "patch_constants.h"

// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
static u8 p_kp1_Buffer[sizeof(float3_t)*MAX_KP1+sizeof(int)]; //l_float3
static u8 p_kp2_Buffer[sizeof(float3_t)*MAX_KP2+sizeof(int)]; //l_float3
static u8 f1_FeatureBuffer[128];
static u8 f2_FeatureBuffer[128];

// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------
void stereo_matching::init(ShavekpMatchingSettings kpMatchingParams)
{
    //kp intersect
    p_o1_transformed= { kpMatchingParams.p_o1_transformed[0], kpMatchingParams.p_o1_transformed[1], kpMatchingParams.p_o1_transformed[2] ,0};
    p_o2_transformed= { kpMatchingParams.p_o2_transformed[0], kpMatchingParams.p_o2_transformed[1], kpMatchingParams.p_o2_transformed[2] ,0};
    EPS= kpMatchingParams.EPS;
   //todo: Amir - castinig works,consider convert all other also!!!
    //compare
    patch_stride = kpMatchingParams.patch_stride;
    patch_win_half_width = kpMatchingParams.patch_win_half_width;
}

bool stereo_matching::l_l_intersect_shave(int i , int j,float4 &P1,float4 &P2, float &s1, float &s2, float &det)
{
    float3_t* kp1=(float3_t*)(p_kp1_Buffer+sizeof(int));
    float3_t* kp2=(float3_t*)(p_kp2_Buffer+sizeof(int));
    float4 v1 = {kp1[i][0],kp1[i][1],kp1[i][2],0};
    float4 v2 = {kp2[j][0],kp2[j][1],kp2[j][2],0};
    float4 p1 = p_o1_transformed;
    float4 p2 = p_o2_transformed;

    // Minimize D(s1,s2) = ||P1(s1)-P2(s2)|| by solving D' = 0
    auto p21_1 = mvuDot(p2-p1,v1), v11 = mvuDot(v1,v1), v12 = mvuDot(v1,v2);
    auto p21_2 = mvuDot(p2-p1,v2), v22 = mvuDot(v2,v2);
    det = v11 * v22 - v12 * v12;
    s1 = (p21_1 * v22 - p21_2 * v12);
    s2 = (p21_1 * v12 - p21_2 * v11);
    P1 = det * p1 + s1 * v1;
    P2 = det * p2 + s2 * v2;
    return std::fabs(det) > EPS;
}

void stereo_matching::stereo_kp_matching_and_compare(u8* p_kp1, u8* p_kp2, u8 * patches1[] , u8 * patches2[], float * depths1, float* depths2, int* matched_kp)
{
  //kp intersect vars
    float4 pa,pb;
  //kp compare vars
    float second_best_distance = INFINITY;
    float best_distance = INFINITY;
    float best_depth1 = 0;
    float best_depth2 = 0;
    float distance  = INFINITY ;
    unsigned short mean1 , mean2 ;
    //DMA - bring KP1 - KP2
    dmaTransactionList_t* dmaRef[2];
    dmaTransactionList_t dmaImportKeypoint[2];
    u32 dmaRequsterId= dmaInitRequester(1);
    dmaRef[0]= dmaCreateTransaction(dmaRequsterId, &dmaImportKeypoint[0], p_kp1, p_kp1_Buffer, sizeof(float3_t)*MAX_KP1+sizeof(int));
    dmaRef[1]= dmaCreateTransaction(dmaRequsterId, &dmaImportKeypoint[1], p_kp2, p_kp2_Buffer, sizeof(float3_t)*MAX_KP2+sizeof(int));
    dmaLinkTasks(dmaRef[0], 1, dmaRef[1]);
    dmaStartListTask(dmaRef[0]);
    dmaWaitTask(dmaRef[0]);

    int n_kp1=*((int*)(p_kp1_Buffer));
    int n_kp2=*((int*)(p_kp2_Buffer));
    int patch_buffer_size = patch_stride *patch_stride;
    int shaveNum = scGetShaveNumber() ;
    DPRINTF("\t#AS- START SHAVE %d\n",shaveNum);

    //prepare patch pointer array to buffers
    u8 *patch1_pa[7], *patch2_pa[7];
    for(int i = 0; i < 7; ++i){
        patch1_pa[i] = f1_FeatureBuffer + i * patch_stride;
    }

    for(int i = 0; i < 7; ++i){
        patch2_pa[i] = f2_FeatureBuffer + i *patch_stride;
    }

    //start kp1 loop
    int start_index = shaveNum    *n_kp1/STEREO_SHAVES_USED;
    int end_index   = (shaveNum+1)*n_kp1/STEREO_SHAVES_USED;

    for (int i=start_index ; i < end_index ; i++)
    {

        best_distance = second_best_distance = INFINITY;
        best_depth1 = best_depth2 = 0;
        int best_kp = -1;

        //bring f1 feature
        u8* patch_source =  (u8*) (patches1[i]);
        memcpy(f1_FeatureBuffer,patch_source,patch_buffer_size); //memcpy gave better results than DMA.
        mean1 = compute_mean7x7_from_pointer_array(patch_win_half_width,patch_win_half_width ,patch1_pa) ;

        for ( int j=0; j< n_kp2; j++)
        {
            float d1, d2, det;
            if (!l_l_intersect_shave(i,j, pa, pb, d1, d2, det))
            {
                DPRINTF( "Failed intersect\n");
                continue;
            }
            if(d1*det <= 0 || d2*det <= 0)
            {
                DPRINTF("Lines were %.2fcm from intersecting at a depth of %.2fcm\n", sqrt(mvuDot(pa-pb,pa-pb))*100, cam1_intersect[2]*100);
                continue;        // TODO: set minz and maxz or at least bound error when close to / far away from camera
            }
            // intersection_error_percent = error / mean(depth1,depth2)
            if(mvuDot(pa-pb,pa-pb) < (0.02f*0.02f) * (((d1+d2)/2)*((d1+d2)/2)))
            {
                //bring f2 feature
                u8* patch_source_2 = (u8*) (patches2[j]) ;
                memcpy(f2_FeatureBuffer,patch_source_2,patch_buffer_size);

                mean2 = compute_mean7x7_from_pointer_array(patch_win_half_width,patch_win_half_width ,patch2_pa) ;
                distance = score_match_from_pointer_array( patch1_pa, patch2_pa,patch_win_half_width,patch_win_half_width,patch_win_half_width,mean1, mean2) ;
                if(distance < best_distance)
                {
                    DPRINTF("\t\t\t After score:kp1 %d, kp2 %d ,shave %d , after mean2:%d, distance %f , depth %f , Error %f\n",i,j, mean1,mean2,distance, d1/det ,sqrt(error)_2);
                    second_best_distance = best_distance;
                    best_distance = distance;
                    best_depth1 = d1 / det;
                    best_depth2 = d2 / det;
                    best_kp = j;
                }
                else if(distance < second_best_distance)
                    second_best_distance = distance;
            }
        } //end kp2 loop

        if(best_distance < patch_good_track_distance && second_best_distance > patch_good_track_distance)
        {
            depths1[i] = best_depth1;
            depths2[i] = best_depth2;
            matched_kp[i] = best_kp;
        }
    }//end kp1 loop
}


