
#include "common_shave.h"
#include <math.h>

/*
 * compute the mean of a 7x7 patch
 * double the weight of the inner 3x3
 * neglects the value after the decimal point
 */

unsigned short compute_mean7x7_from_pointer_array(int x_offset_center_from_pointer,int patch_win_half_width ,u8 **pPatch_pa)
{
  	//int shaveNum = scGetShaveNumber();
    //if(shaveNum== 0)
    int full = patch_win_half_width * 2 + 1;
    int area = full * full + 3 * 3;
    ushort8 sum = 0;
    for (int y = 0; y < full; ++y) {
        u8* pLine = pPatch_pa[y] + x_offset_center_from_pointer - patch_win_half_width;
        ushort8 line = {pLine[0], pLine[1], pLine[2], pLine[3], pLine[4], pLine[5], pLine[6], 0};
        sum += line;
        if(y > 1 && y < 5){
            sum.s234 += line.s234;
        }
    }
    //TODO: reduction function
    unsigned short sum1 = sum[0] + sum[1] + sum [2]+ sum[3] + sum[4] + sum [5] + sum[6];
    unsigned short mean = sum1 / area;
    return mean;
}

float score_match_from_pointer_array(u8 **pPatch1_pa, u8 **pPatch2_pa,int x1_offset_center_from_pointer , int x2_offset_center_from_pointer,int patch_win_half_width, unsigned short mean1, unsigned short mean2)
{
    const unsigned char *p1 ;
    const unsigned char *p2 ;
    int8 top = 0, bottom1 = 0, bottom2 = 0;
    int full = patch_win_half_width * 2 + 1 ;
    for(int dy = 0; dy < full; ++dy) {
        p1 = pPatch1_pa[dy] + x1_offset_center_from_pointer - patch_win_half_width;
        p2 = pPatch2_pa[dy] + x2_offset_center_from_pointer - patch_win_half_width;
        int8 t1 = {p1[0], p1[1], p1[2], p1[3], p1[4], p1[5], p1[6], 0};
        t1 = t1 - (int)mean1;
        int8 t2 = {p2[0], p2[1], p2[2], p2[3], p2[4], p2[5], p2[6], 0};
        t2 = t2 - (int)mean2;
        int8 t12 = t1 *t2;
        top += t12;
        int8 t1square = t1 * t1;
        int8 t2square = t2 * t2;
        bottom1 += t1square;
        bottom2 += t2square;
        if( (dy > 1) && (dy < 5))
        {
            top.s234 += t1.s234 * t2.s234;
            bottom1.s234 += (t1.s234 * t1.s234);
            bottom2.s234 += (t2.s234 * t2.s234);
        }
    }
    float sumTop = (float)top[0] + (float)top[1] + (float)top[2]+ (float)top[3] +(float)top[4] +
                   (float)top[5] + (float)top[6];
    float sumBottom1 = (float)bottom1[0] + (float)bottom1[1] + (float)bottom1[2]+ (float)bottom1[3] +
                       (float)bottom1[4] + (float)bottom1[5] + (float)bottom1[6];
    float sumBottom2 = (float)bottom2[0] + (float)bottom2[1] + (float)bottom2[2]+ (float)bottom2[3] +
                       (float)bottom2[4] + (float)bottom2[5] + (float)bottom2[6];
    // constant patches can't be matched
    if(sumBottom1 < 1e-15 || sumBottom2 < 1e-15 || sumTop < 0.f)
    {
        return INFINITY;
    }
    return (float) 1 - (sumTop * sumTop)/(sumBottom1 * sumBottom2);
}
