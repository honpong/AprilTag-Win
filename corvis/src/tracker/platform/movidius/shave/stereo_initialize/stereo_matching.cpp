/// @file

// 1: Includes
// ----------------------------------------------------------------------------
#include "stereo_matching.h"
#include "swcCdma.h"
#include <mv_types.h>
#include <swcFrameTypes.h>
#include <svuCommonShave.h>
#include <swcSIMDUtils.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
//#define DEBUG_PRINTS
#include "stereo_commonDefs.hpp"

#if 0 // Configure debug prints
#include <stdio.h>
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
extern ShavekpMatchingSettings kpMatchingParams;
// ----------------------------------------------------------------------------
// 4: Static Local Data
static u8 __attribute__((section(".cmx.bss"))) p_kp1_Buffer[sizeof(float3_t)*MAX_KP1+sizeof(int)]; //l_float3
static u8 __attribute__((section(".cmx.bss"))) p_kp2_Buffer[sizeof(float3_t)*MAX_KP2+sizeof(int)]; //l_float3
static int   __attribute__((section(".cmx.bss"))) kp_intersect_Buffer[MAX_KP2_INTERSECT+ 1];    //reduce 2'nd dim to 64
static int   __attribute__((section(".cmx.bss"))) kp_intersect_Buffer_1[MAX_KP2_INTERSECT+ 1];    //reduce 2'nd dim to 64
static float __attribute__((section(".cmx.bss"))) kp_intersect_depth_Buffer[MAX_KP2_INTERSECT]; //reduce 2'nd dim to 64
static float __attribute__((section(".cmx.bss"))) kp_intersect_depth_Buffer_1[MAX_KP2_INTERSECT]; //reduce 2'nd dim to 64

dmaTransactionList_t /*__attribute__((section(".cmx.cdmaDescriptors")))*/ dmaImportKeypoint[2];
dmaTransactionList_t /*__attribute__((section(".cmx.cdmaDescriptors")))*/ dmaOutIntersect[2];


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
void stereo_matching::init()
{
    camera1_extrinsics_T_v=  { kpMatchingParams.camera1_extrinsics_T_v[0], kpMatchingParams.camera1_extrinsics_T_v[1], kpMatchingParams.camera1_extrinsics_T_v[2], kpMatchingParams.camera1_extrinsics_T_v[3] };
    camera2_extrinsics_T_v=  { kpMatchingParams.camera2_extrinsics_T_v[0], kpMatchingParams.camera2_extrinsics_T_v[1], kpMatchingParams.camera2_extrinsics_T_v[2], kpMatchingParams.camera2_extrinsics_T_v[3] };
    p_o1_transformed= { kpMatchingParams.p_o1_transformed[0], kpMatchingParams.p_o1_transformed[1], kpMatchingParams.p_o1_transformed[2] ,0};
    p_o2_transformed= { kpMatchingParams.p_o2_transformed[0], kpMatchingParams.p_o2_transformed[1], kpMatchingParams.p_o2_transformed[2] ,0};
    EPS= kpMatchingParams.EPS;
   //todo: Amir - castinig works,consider convert all other also!!!
    R1w_transpose= * (float4x4*) kpMatchingParams.R1w_transpose;
    R2w_transpose= * (float4x4*) kpMatchingParams.R2w_transpose;
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

void stereo_matching::stereo_kp_matching(u8* p_kp1, u8* p_kp2, int* kp_intersect, float* kp_intersect_depth)
{
    float4 pa,pb;
    bool success= 0;
    float depth,error,error_2;
    float4 cam1_intersect,cam2_intersect;
    int MaKPfound;
    int* MA_kp_intersect;
    float* MA_kp_intersect_depth;
    bool dma_flag =0 ;
    //DMA - bring KP1 - KP2
    dmaTransactionList_t* dmaRef[2];
    dmaTransactionList_t* dmaOut[2];
    u32 dmaRequsterId= dmaInitRequester(1);
    dmaRef[0]= dmaCreateTransaction(dmaRequsterId, &dmaImportKeypoint[0], p_kp1, p_kp1_Buffer, sizeof(float3_t)*MAX_KP1+sizeof(int));
    dmaRef[1]= dmaCreateTransaction(dmaRequsterId, &dmaImportKeypoint[1], p_kp2, p_kp2_Buffer, sizeof(float3_t)*MAX_KP2+sizeof(int));
    dmaLinkTasks(dmaRef[0], 1, dmaRef[1]);
    dmaStartListTask(dmaRef[0]);
    dmaWaitTask(dmaRef[1]);
    int n_kp1=*((int*)(p_kp1_Buffer));
    int n_kp2=*((int*)(p_kp2_Buffer));

    for(int i=0; i< n_kp1; i++)
    {
        if (dma_flag==0)
        {
            MA_kp_intersect= kp_intersect_Buffer;
            MA_kp_intersect_depth= kp_intersect_depth_Buffer;
            dma_flag = 1 ;
        } else  {
            MA_kp_intersect= kp_intersect_Buffer_1;
            MA_kp_intersect_depth= kp_intersect_depth_Buffer_1;
            dma_flag = 0 ;
        }

        MaKPfound=0;
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
            //intersection_error_percent= error/cam1_intersect[2];
            if(float (error_2/(cam1_intersect[2]*cam1_intersect[2])) > (0.02*0.02))
            {
                error= sqrt(error_2);
                DPRINTF("intersection_error_percent too large %f, failing\n",float (error/cam1_intersect[2] ));
                continue;
            }
            //float3 intersection= pa;
            depth= cam1_intersect[2];
            MA_kp_intersect[MaKPfound+1]= j;
            MA_kp_intersect_depth[MaKPfound]= depth;
            MaKPfound++;
            DPRINTF("%d:%d depth %f,error %f,intersection %f \t\n ",i,j, cam1_intersect[2],error,error/cam1_intersect[2]);
        }
        MA_kp_intersect[0]= MaKPfound;

//DMA_TRANSACTION_ON
            if (i!=0)
                 dmaWaitTask(dmaOut[0]);
            dmaOut[0]= dmaCreateTransaction(dmaRequsterId, &dmaOutIntersect[0], (u8*)MA_kp_intersect, (u8*) &kp_intersect[i*(MAX_KP2_INTERSECT+1)], sizeof (int)*(MaKPfound+ 1));
            dmaOut[1]= dmaCreateTransaction(dmaRequsterId, &dmaOutIntersect[1], (u8*)MA_kp_intersect_depth, (u8*) &kp_intersect_depth[i*(MAX_KP2_INTERSECT)], sizeof (float)*MaKPfound+1);
            dmaLinkTasks(dmaOut[0], 1, dmaOut[1]);
            dmaStartListTask(dmaOut[0]);
    }
    dmaWaitTask(dmaOut[0]);

#ifdef DEBUG_PRINTS
    if(shaveNum== 5)
    {
        printf("\n kp_intersect\n");
        for(int i=0; i< n_kp1; i++)
        {
            MA_kp_intersect= &kp_intersect[i*(MAX_KP2_INTERSECT+1)];
            printf("\n line %d :: %d :",i,MA_kp_intersect[0]);
             for ( int j=0; j< MA_kp_intersect[0]; j++)
             {
                    printf(" %d",MA_kp_intersect[j+1]);
             }
        }
        printf ("\n");
    }
#endif

}



