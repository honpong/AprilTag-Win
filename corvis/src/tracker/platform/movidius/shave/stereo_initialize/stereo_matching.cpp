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

// 2:  Source Specific #defines and types  (typedef,enum,struct)
//#define DEBUG_PRINTS
#include "stereo_commonDefs.hpp"
#include "common_shave.h"

static const float fast_good_match = 0.65f*0.65f;

// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
static u8 __attribute__((section(".cmx.bss"))) p_kp1_Buffer[sizeof(float3_t)*MAX_KP1+sizeof(int)]; //l_float3
static u8 __attribute__((section(".cmx.bss"))) p_kp2_Buffer[sizeof(float3_t)*MAX_KP2+sizeof(int)]; //l_float3
static u8 __attribute__((section(".cmx.data")))  f1_FeatureBuffer[128];
static u8 __attribute__((section(".cmx.data")))  f2_FeatureBuffer[128];

dmaTransactionList_t /*__attribute__((section(".cmx.cdmaDescriptors")))*/ dmaImportKeypoint[2];

// ----------------------------------------------------------------------------
// 5: Static Function Prototypes
static float4 m_mult ( float4x4 A, float4 B)
{
    float4 res= {mvuDot(A.rows[0],B), mvuDot(A.rows[1],B), mvuDot(A.rows[2],B), mvuDot(A.rows[3],B) };
    return (res);
}

// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------
void stereo_matching::init(ShavekpMatchingSettings kpMatchingParams)
{
    //kp intersect
    camera1_extrinsics_T_v=  { kpMatchingParams.camera1_extrinsics_T_v[0], kpMatchingParams.camera1_extrinsics_T_v[1], kpMatchingParams.camera1_extrinsics_T_v[2], kpMatchingParams.camera1_extrinsics_T_v[3] };
    camera2_extrinsics_T_v=  { kpMatchingParams.camera2_extrinsics_T_v[0], kpMatchingParams.camera2_extrinsics_T_v[1], kpMatchingParams.camera2_extrinsics_T_v[2], kpMatchingParams.camera2_extrinsics_T_v[3] };
    p_o1_transformed= { kpMatchingParams.p_o1_transformed[0], kpMatchingParams.p_o1_transformed[1], kpMatchingParams.p_o1_transformed[2] ,0};
    p_o2_transformed= { kpMatchingParams.p_o2_transformed[0], kpMatchingParams.p_o2_transformed[1], kpMatchingParams.p_o2_transformed[2] ,0};
    EPS= kpMatchingParams.EPS;
   //todo: Amir - castinig works,consider convert all other also!!!
    R1w_transpose= * (float4x4*) kpMatchingParams.R1w_transpose;
    R2w_transpose= * (float4x4*) kpMatchingParams.R2w_transpose;
    //compare
    patch_stride = kpMatchingParams.patch_stride;
    patch_win_half_width = kpMatchingParams.patch_win_half_width;
}

bool stereo_matching::l_l_intersect_shave(int i , int j,float4 *pa,float4 *pb)
{
// From http://paulbourke.net/geometry/pointlineplane/lineline.c
// line 1 is p1 to p2, line 2 is p3 to p4
        /*
           Calculate the line segment PaPb that is the shortest route between
           two lines P1P2 and P3P4. Calculate also the values of mua and mub where
              Pa= P1 + mua (P2 - P1)
              Pb= P3 + mub (P4 - P3)
           Return FALSE if no solution exists.
        */
    float3_t* kp1=(float3_t*)(p_kp1_Buffer+sizeof(int));
    float3_t* kp2=(float3_t*)(p_kp2_Buffer+sizeof(int));
    float4 p1_cal_transformed= {kp1[i][0],kp1[i][1],kp1[i][2],0};
    float4 p2_cal_transformed= {kp2[j][0],kp2[j][1],kp2[j][2],0};

    float4 p1=p_o1_transformed;
    float4 p2=p1_cal_transformed;
    float4 p3=p_o2_transformed;
    float4 p4=p2_cal_transformed;

    float4 p43=p4 -p3;
    if (fabs(p43[0]) < EPS &fabs(p43[1]) < EPS &fabs(p43[2]) < EPS) //todo: Amir convert to SIMD
        return (false);
    float4 p21=p2 -p1 ;
    if (fabs(p21[0]) < EPS &fabs(p21[1]) < EPS &fabs(p21[2]) < EPS)
      return (false);
    float4 p13= p1 -p3;


    double d4321= mvuDot( p43, p21 );
    double d4343= mvuDot( p43, p43 );
    double d2121= mvuDot( p21, p21 );

    double denom= d2121 * d4343 - d4321 * d4321;
    if (fabs(denom) < EPS)
        return(false);
    double d1343= mvuDot( p13, p43 );
    double d1321= mvuDot( p13, p21 );

    double numer= d1343 * d4321 - d1321 * d4343;
    float mua= numer / denom;
    float mub= (d1343 + d4321 * mua) / d4343;

    *pa= p1 + mua * p21;
    *pb= p3 + mub * p43;
    pa[3]= 1 ;
    pb[3]= 1 ;
    return(true);
}

void stereo_matching::stereo_kp_matching_and_compare(u8* p_kp1, u8* p_kp2, u8 * patches1[] , u8 * patches2[], float * depths1, float * errors1)
{
  //kp intersect vars
    float4 pa,pb;
    bool success= 0;
    float depth,error,error_2;
    float4 cam1_intersect,cam2_intersect;
  //kp compare vars
    float second_best_score = fast_good_match;
    float best_score = fast_good_match;
    float best_depth = 0;
    float best_error = 0;
    float score  = 0 ;
    float min_score = 0;
    unsigned short mean1 , mean2 ;
    //DMA - bring KP1 - KP2
    dmaTransactionList_t* dmaRef[2];

    u32 dmaRequsterId= dmaInitRequester(1);
    dmaRef[0]= dmaCreateTransaction(dmaRequsterId, &dmaImportKeypoint[0], p_kp1, p_kp1_Buffer, sizeof(float3_t)*MAX_KP1+sizeof(int));
    dmaRef[1]= dmaCreateTransaction(dmaRequsterId, &dmaImportKeypoint[1], p_kp2, p_kp2_Buffer, sizeof(float3_t)*MAX_KP2+sizeof(int));
    dmaLinkTasks(dmaRef[0], 1, dmaRef[1]);
    dmaStartListTask(dmaRef[0]);
    dmaWaitTask(dmaRef[0]);

    int n_kp1=*((int*)(p_kp1_Buffer));
    int n_kp2=*((int*)(p_kp2_Buffer));
    int patch_buffer_size = patch_stride *patch_stride;
    float3_t* kp1=(float3_t*)(p_kp1_Buffer+sizeof(int));
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
    int start_index = (int) ( shaveNum*(n_kp1)/(SHAVES_USED));
    int end_index ;
    if (shaveNum == 3)
       end_index = n_kp1 ;
    else
       end_index = (int)( (shaveNum+1)*(n_kp1)/(SHAVES_USED)) ;

    for (int i=start_index ; i < end_index ; i++)
    {

        best_score = second_best_score = fast_good_match;
        best_depth = 0;

        //bring f1 feature
        u8* patch_source =  (u8*) (patches1[i]);
        memcpy(f1_FeatureBuffer,patch_source,patch_buffer_size); //memcpy gave better results than DMA.
        mean1 = compute_mean7x7_from_pointer_array(patch_win_half_width,patch_win_half_width ,patch1_pa) ;

        for ( int j=0; j< n_kp2; j++)
        {
            success= l_l_intersect_shave(i,j, &pa, &pb);
            if(!success)
            {
                DPRINTF( "Failed intersect\n");
                continue;
            }
            //ORIGINAL error= fabs(pa[0]-pb[0])+fabs(pa[1]-pb[1])+fabs(pa[2]-pb[2]);
            error_2= mvuDot(pa-pb,pa-pb);
            cam1_intersect= m_mult(R1w_transpose,pa - camera1_extrinsics_T_v);
            cam2_intersect= m_mult(R2w_transpose,pa - camera2_extrinsics_T_v);

            if(cam1_intersect[2] <= 0 || cam2_intersect[2] <= 0)
            {
                DPRINTF("Lines were %.2fcm from intersecting at a depth of %.2fcm\n", error*100, cam1_intersect[2]*100);
                continue;        // TODO: set minz and maxz or at least bound error when close to / far away from camera
            }
            error = sqrt(error_2);
            float intersection_error_percent = error/cam1_intersect[2];
            if(intersection_error_percent > 0.05)
            {
                DPRINTF("intersection_error_percent too large %f, failing\n",float (error/cam1_intersect[2]));
                continue;
            }
            depth = cam1_intersect[2];
//START COMPARE
            DPRINTF("\t\tkp1 %d, kp2 %d, depth %f, error %f \n",i,j,depth,error);
            if(depth && intersection_error_percent < 0.02 )
            {
                //bring f2 feature
                u8* patch_source_2 = (u8*) (patches2[j]) ;
                memcpy(f2_FeatureBuffer,patch_source_2,patch_buffer_size);

                mean2 = compute_mean7x7_from_pointer_array(patch_win_half_width,patch_win_half_width ,patch2_pa) ;
                score = score_match_from_pointer_array( patch1_pa, patch2_pa,patch_win_half_width,patch_win_half_width,patch_win_half_width,min_score,mean1, mean2) ;
                if(score > best_score)
                {
                    DPRINTF("\t\t\t After score:kp1 %d, kp2 %d ,shave %d , after mean2:%d, score %f , depth %f , Error %f\n",i,j, mean1,mean2,score, depth ,error);
                    second_best_score = best_score;
                    best_score = score;
                    best_depth = depth;
                    best_error = intersection_error_percent;
                }
            }
        } //end kp2 loop

        if(best_depth && second_best_score == fast_good_match)
        {
            depths1[i] = best_depth;
            errors1[i] = best_error;
        }
    }//end kp1 loop
}


