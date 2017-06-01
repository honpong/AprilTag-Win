/*
 * shave_stereo.cpp
 *
 *  Created on: Apr 20, 2017
 *      Author: ntuser
 */
// 1: Includes
// ----------------------------------------------------------------------------
#include "shave_stereo.h"
//#include "vec4.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
#if 0 // Configure debug prints
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif
//#define KP_DEBUG_PRINTS
//#define FINAL_DEBUG

// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
//## Define Memory useage ##
//output
static u8 __attribute__((section(".ddr.bss"))) p_kp1[sizeof(float3_t)*MAX_KP1+sizeof(int)]; //l_float3
static u8 __attribute__((section(".ddr.bss"))) p_kp2[sizeof(float3_t)*MAX_KP2+sizeof(int)]; //l_float3
//input
static int   __attribute__((section(".ddr_direct.bss"))) kp_intersect[MAX_KP1][MAX_KP2_INTERSECT+ 1];
static float __attribute__((section(".ddr_direct.bss"))) kp_intersect_depth[MAX_KP1][MAX_KP2_INTERSECT];
volatile __attribute__((section(".cmx_direct.data")))  ShavekpMatchingSettings cvst5_kpMatchingParams;
// ----------------------------------------------------------------------------
// 4: Static Local Data
//## extern entery point Defined  in makefile ##
extern u32 cvst5_stereo_kp_matching_main;
u32 entryPoint_intersect = (u32)&cvst5_stereo_kp_matching_main;

//## set shave handler params ##
bool shave_stereo::s_shavesOpened_st = false;
osDrvSvuHandler_t shave_stereo::s_handler[12];

// ----------------------------------------------------------------------------
// 5: Static Function Prototypes
float inline compute_mean_or(const fast_tracker::feature & f);
float inline ncc_score_or(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float min_score, float mean1);
float keypoint_compare_or(const fast_tracker::feature & f1, const fast_tracker::feature &f2);
// ----------------------------------------------------------------------------
// 6: Functions Implementation
shave_stereo::shave_stereo()
{
    if (s_shavesOpened_st == false)
    {
        s_shavesOpened_st = true;
        s32 sc = OsDrvSvuInit();
        sc |=OsDrvSvuOpenShave(&s_handler[SHAVE_USED], SHAVE_USED, OS_MYR_PROTECTION_SEM);
        if (sc!=0)
        {
            printf("#ERROR: error init shaves sc:%d \n",(int)sc );
        }
    }
}
shave_stereo::~shave_stereo() {}

void shave_stereo::stereo_matching_shave (const std::vector<tracker::point> & kp1, std::vector<tracker::point> & kp2,const fast_tracker::feature * f1_group[MAX_KP1],const fast_tracker::feature * f2_group[MAX_KP2], state_camera & camera1, state_camera & camera2, std::vector<tracker::point> * new_keypoints_p)
{
    keypoint_intersect_shave(kp1,kp2,camera1,camera2);
    keypoint_compare_leon(kp1,kp2,f1_group,f2_group,*new_keypoints_p);
}
void shave_stereo::keypoint_intersect_shave(const std::vector<tracker::point> & kp1, std::vector<tracker::point> & kp2,state_camera & camera1, state_camera & camera2 )
{

    feature_t f1_n,f2_n;
    v3 p2_calibrated,p2_cal_transformed,p1_calibrated,p1_cal_transformed;
    v3 o1_transformed = camera1.extrinsics.T.v;
    v3 o2_transformed = camera2.extrinsics.T.v;
    float3_t p_o1_transformed = {o1_transformed[0],o1_transformed[1],o1_transformed[2]};
    float3_t p_o2_transformed = {o2_transformed[0],o2_transformed[1],o2_transformed[2]};
    m3 R1w = camera1.extrinsics.Q.v.toRotationMatrix();
    m3 R2w = camera2.extrinsics.Q.v.toRotationMatrix();


    float3_t* p_kp2_transformed = (float3_t*) (p_kp2+sizeof(int));
    float3_t* p_kp1_transformed = (float3_t*) (p_kp1+sizeof(int));
    R1w = camera1.extrinsics.Q.v.toRotationMatrix();
    R2w = camera2.extrinsics.Q.v.toRotationMatrix();

    //prepare p_kp2_transformed
    int nk2=0;
    for(auto & k2 : kp2) {
        feature_t f2(k2.x,k2.y);
        f2_n=camera2.intrinsics.undistort_feature(camera2.intrinsics.normalize_feature(f2));
        p2_calibrated << f2_n.x(), f2_n.y(), 1;
        p2_cal_transformed = R2w*p2_calibrated + camera2.extrinsics.T.v;

        p_kp2_transformed[nk2][0] = p2_cal_transformed(0); // todo : Amir : check if we can skip the v3;
        p_kp2_transformed[nk2][1] = p2_cal_transformed(1);
        p_kp2_transformed[nk2][2] = p2_cal_transformed(2);
        nk2++;
    }
    //prepare p_kp1_transformed
    int nk1=0;
    for(const tracker::point & k1 : kp1) {
        feature_t f1(k1.x,k1.y);
        f1_n=camera1.intrinsics.undistort_feature(camera1.intrinsics.normalize_feature(f1));
        p1_calibrated  << f1_n.x(),f1_n.y(),1;
        p1_cal_transformed = R1w*p1_calibrated + camera1.extrinsics.T.v;
        p_kp1_transformed[nk1][0] = p1_cal_transformed(0);
        p_kp1_transformed[nk1][1] = p1_cal_transformed(1);
        p_kp1_transformed[nk1][2] = p1_cal_transformed(2);
        nk1++;
    }

    *((int*)p_kp1)=nk1;
    *((int*)p_kp2)=nk2;

    DPRINTF("\t\t AS:nk1 : %d, nk2: %d, &nk1 %d,&nk2 %d\n ",nk1,nk2, &p_kp1, &p_kp2);
    u32 running = 0, sc = 0;
    //prepare local vars
    //Eigen types
    m3 E_R1w_t=R1w.transpose();
    m3 E_R2w_t=R2w.transpose();
    v3 E_camera1_extrinsics_T_v = camera1.extrinsics.T.v;
    v3 E_camera2_extrinsics_T_v = camera2.extrinsics.T.v;
    //local array vector type
    float4x4_t R1w_transpose { {E_R1w_t(0,0), E_R1w_t(0,1),E_R1w_t(0,2),0 },
                               {E_R1w_t(1,0), E_R1w_t(1,1),E_R1w_t(1,2),0 },
                               {E_R1w_t(2,0), E_R1w_t(2,1),E_R1w_t(2,2),0 },
                               { 0, 0, 0, 0 } };

    float4x4_t R2w_transpose {{ E_R2w_t(0,0), E_R2w_t(0,1),E_R2w_t(0,2),0 },
                               {E_R2w_t(1,0), E_R2w_t(1,1),E_R2w_t(1,2),0 },
                               {E_R1w_t(2,0), E_R2w_t(2,1),E_R2w_t(2,2),0 },
                               { 0, 0, 0, 0 }};
    float4_t camera1_extrinsics_T_v = { E_camera1_extrinsics_T_v[0],E_camera1_extrinsics_T_v[1],E_camera1_extrinsics_T_v[2] };
    float4_t camera2_extrinsics_T_v = { E_camera2_extrinsics_T_v[0],E_camera2_extrinsics_T_v[1],E_camera2_extrinsics_T_v[2] };

    //copy to  cvst; //todo : direcrt copy without local step;
    l_float4x4_copy (cvst5_kpMatchingParams.R1w_transpose,R1w_transpose );
    l_float4x4_copy (cvst5_kpMatchingParams.R2w_transpose,R2w_transpose );
    l_float4_copy   (cvst5_kpMatchingParams.camera1_extrinsics_T_v,camera1_extrinsics_T_v );
    l_float4_copy   (cvst5_kpMatchingParams.camera2_extrinsics_T_v,camera2_extrinsics_T_v );
    l_float3_copy   (cvst5_kpMatchingParams.p_o1_transformed,p_o1_transformed);
    l_float3_copy   (cvst5_kpMatchingParams.p_o2_transformed,p_o2_transformed);
    cvst5_kpMatchingParams.EPS=1e-14;


    int i = 5;
    sc |= OsDrvSvuResetShave(&s_handler[i]);
    sc |= OsDrvSvuSetAbsoluteDefaultStack(&s_handler[i]);
    sc |= OsDrvSvuStartShaveCC(&s_handler[i], entryPoint_intersect,"iiii",p_kp1, p_kp2,kp_intersect,kp_intersect_depth);
    sc |=OsDrvSvuWaitShaves(1, &s_handler[SHAVE_USED], OS_DRV_SVU_WAIT_FOREVER, &running);
    if (0 == sc && 0 == running) {
        DPRINTF("\t\t##shave_matching point ## shaves returned\n");
    }
    else {
        printf("#ERROR: error running shaves sc:%d running:%d \n", (int)sc,(int)running);
    }
}
void shave_stereo::keypoint_compare_leon  (const std::vector<tracker::point> & kp1, std::vector<tracker::point> & kp2,const fast_tracker::feature * f1_group[],const fast_tracker::feature * f2_group[], std::vector<tracker::point> &  new_keypoints )
{

#ifdef KP_DEBUG_PRINTS
    int* MA_kp_intersect;
    printf("\n kp_intersect\n");
    for(int i=0; i< *(int*) f1_group; i++)
    {
        MA_kp_intersect = kp_intersect[i];
        printf("\n line %d :: %d :",i,MA_kp_intersect[0]);
         for ( int j=0; j< MA_kp_intersect[0]; j++)
         {
                printf(" %d",MA_kp_intersect[j+1]);
         }
    }
    printf ("\n");
#endif


    int i=0;
    float second_best_score = fast_good_match;
    float best_score = fast_good_match;
    float best_depth = 0;
    int n_kp1 = *((int*)p_kp1);
    feature_t best_f2;
    static std::vector<tracker::point> other_keypoints;
    //fast_tracker::feature f1; //todo: amir define outside off the loop;
   // fast_tracker::feature  f2;

    new_keypoints.clear();
    other_keypoints.clear();

    DPRINTF("\t#INFO - keypointCompareShave n_kp1 %d,&kp1 %d, &kp2 %d \n",n_kp1,&kp1,&kp2);
    for(i=0; i<n_kp1; i++ )
    {
        const fast_tracker::feature & f1 = *f1_group[i];
        int KPfound = kp_intersect[i][0];
        tracker::point  k1 = kp1[i];
        DPRINTF("\n\t#INFO KP1:%d:%f  KPfound %d \n ",i,(double)k1.id,KPfound);
        if (KPfound == 0)
        {
               other_keypoints.push_back(k1);
            continue;
        }
        best_score = second_best_score = fast_good_match;
        best_depth = 0;
        int* pshave_kps = &kp_intersect[i][1];
        for (int j = 0; j < KPfound; j++)
        {
            const fast_tracker::feature & f2 = *f2_group[pshave_kps[j]];
            float score = keypoint_compare_or(f1, f2);
            DPRINTF("\t\t{%f,%f} kp_score %f, depth %f \n",kp2[pshave_kps[j]].x,kp2[pshave_kps[j]].y,score,kp_intersect_depth[i][j]);
            if(score > best_score)
            {
                 feature_t ff2  {kp2[pshave_kps[j]].x, kp2[pshave_kps[j]].y};
                 second_best_score = best_score;
                 best_score = score;
                 best_depth = kp_intersect_depth[i][j];
                 best_f2 = ff2;
            }
        }
        //float ratio = sqrt(second_best_score)/sqrt(best_score);
        // If we have two candidates, just give up
        if(best_depth && second_best_score == fast_good_match)
        {
            DPRINTF("\t\tkp1%f:{%f %f}, best kp2 {%f %f},score %f, no_second_best %d,best_depth:%f \n",(double)k1.id, k1.x, k1.y, best_f2.x(), best_f2.y(), best_score, second_best_score==fast_good_match,best_depth);
            k1.depth = best_depth;
            new_keypoints.push_back(k1);
        } else
        {
            other_keypoints.push_back(k1);
            DPRINTF("\t\tOther:tkp1%f:{%f %f}, best kp2 {%f %f},score %f, no_second_best %d, best_depth %f \n",(double)k1.id, k1.x, k1.y, best_f2.x(), best_f2.y(), best_score, second_best_score==fast_good_match, best_depth);
        }
    }
    // Add points which do not have depth after the points with depth
    for(tracker::point & k1 : other_keypoints)
    {
        new_keypoints.push_back(k1);
    }
#ifdef FINAL_DEBUG
    printf("keypointCompareShave \n");
    int ii=0;
    for(auto & k1 : new_keypoints) {
        printf("%d: ID:%f {%f,%f} score %f \n ",ii,(double)k1.id,k1.x,k1.y,k1.score);
    ii++;
    }
#endif
}

float inline ncc_score_or(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float min_score, float mean1)
{
    int patch_win_half_width = half_patch_width;
    int window = patch_win_half_width;
    int patch_stride = full_patch_width;
    int full = patch_win_half_width * 2 + 1;
    int area = full * full + 3 * 3;
    int xsize = full_patch_width;
    int ysize = full_patch_width;

    if(x1 < window || y1 < window || x2 < window || y2 < window || x1 >= xsize - window || x2 >= xsize - window || y1 >= ysize - window || y2 >= ysize - window) return -1;

    const unsigned char *p1 = im1 + patch_stride * (y1 - window) + x1;
    const unsigned char *p2 = im2 + patch_stride * (y2 - window) + x2;

    int sum2 = 0;
    for(int dy = -window; dy <= window; ++dy, p2+=patch_stride) {
        for(int dx = -window; dx <= window; ++dx) {
            sum2 += p2[dx];
        }
    }

    p2 = im2 + patch_stride * (y2 - 1) + x2;
    for (int dy = -1; dy <= 1; ++dy, p2 += patch_stride) {
        for (int dx = -1; dx <= 1; ++dx) {
            sum2 += p2[dx];
        }
    }

    float mean2 = sum2 / (float)area;

    p2 = im2 + patch_stride * (y2 - window) + x2;
    float top = 0, bottom1 = 0, bottom2 = 0;
    for(int dy = -window; dy <= window; ++dy, p1 += patch_stride, p2+=patch_stride) {
        for(int dx = -window; dx <= window; ++dx) {
            float t1 = (p1[dx] - mean1);
            float t2 = (p2[dx] - mean2);
            top += t1 * t2;
            bottom1 += (t1 * t1);
            bottom2 += (t2 * t2);
            if((dx >= -1) && (dx <= 1) && (dy >= -1) && (dy <= 1))
            {
                top += t1 * t2;
                bottom1 += (t1 * t1);
                bottom2 += (t2 * t2);
            }
        }
    }
    // constant patches can't be matched
    if(bottom1 < 1e-15 || bottom2 < 1e-15 || top < 0.f)
      return min_score;

    return top*top/(bottom1 * bottom2);
}
float inline compute_mean_or(const fast_tracker::feature & f)
{
    int patch_stride = full_patch_width;
    const int area = full_patch_width*full_patch_width + 3*3;
    int sum1 = 0;
    for(int i = 0; i < full_patch_width*full_patch_width; i++)
        sum1 += f.patch[i];

    // center weighting
    uint8_t * p1 = (uint8_t*)f.patch + patch_stride * (half_patch_width - 1) + half_patch_width;
    for (int dy = -1; dy <= 1; ++dy, p1 += patch_stride)
    {
        for (int dx = -1; dx <= 1; ++dx) {
            sum1 += p1[dx];
        }
    }
    float mean1 = sum1 / (float)area;
    return mean1;
}
float keypoint_compare_or(const fast_tracker::feature & f1, const fast_tracker::feature &f2)
{
	float mean1 = compute_mean_or(f1);
	float min_score = 0;
	return ncc_score_or(f1.patch, half_patch_width, half_patch_width, f2.patch, half_patch_width, half_patch_width, min_score, mean1);
}

