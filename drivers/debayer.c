// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include "debayer.h"

typedef enum {
  DC1394_COLOR_FILTER_RGGB = 512,
  DC1394_COLOR_FILTER_GBRG,
  DC1394_COLOR_FILTER_GRBG,
  DC1394_COLOR_FILTER_BGGR
} dc1394color_filter_t;

void bayer_to_gray(unsigned char *in, int iwidth, int iheight, unsigned char *out, int owidth, int oheight)
{
  /*
    we generate the pixel between each quad, eg:
    R G
     * <-We generate this pixel. could be R * .299 + G1 * .294 + G2 * .294 + B * .114
    G B  Let's just simplify and make it (R+G+G+B)/4, as in downsample. but we do it between every set.
  */
  for(int y = 0; y < oheight-1; y++) {
    for(int x = 0; x < owidth-1; x++) {
      uint16_t sum = in[0] + in[1] + in[iwidth] + in[iwidth+1];
      *out = sum >> 2;
      ++out;
      ++in;
    }
    *out = (in[0] + in[iwidth]) >> 1;
    ++out;
    in += (iwidth - owidth + 1);
  } 

  for(int x = 0; x < owidth-1; x++) {
    uint16_t sum = in[0] + in[1];
    *out = sum >> 1;
    ++out;
    ++in;
  }
  *out = *in;
}

void bayer_to_gray_sucky(unsigned char *input, int iwidth, int iheight, unsigned char *out, int owidth, int oheight, int bayer_pattern)
{
  /*
      We employ the following seperable filter:
      horizontal: apply color scaling (G is scaled by 1/2), then
                    1/2 1 /2
        vertical:   1/2 1 1/2
        (we multiply first by two and second by 1/2 for intermediate precision)
        *note that this treats the corner G's different than middle G (desirable)
      Where G=.587, C = {R:.299, B:.114}

      intermediates are stored as uint16

      second step is performed on the previous line as we go
      to keep everything in the L1 and minimize buffer space
      keep four lines of temp space so that we can mask index
    */

#define R .299*256.
#define G .587*256.
#define B .114*256.
  //max width 2048.
    static uint16_t temp[2048 * 4];
    bzero(temp, 2048*4*2);
    unsigned char bayer[4];
    switch(bayer_pattern) {
    case DC1394_COLOR_FILTER_RGGB:
        bayer[0] = R;
        bayer[1] = G/2;
        bayer[2] = G/2;
        bayer[3] = B;
        break;
    case DC1394_COLOR_FILTER_BGGR:
        bayer[0] = B;
        bayer[1] = G/2;
        bayer[2] = G/2;
        bayer[3] = R;
        break;
    case DC1394_COLOR_FILTER_GRBG:
        bayer[0] = G/2;
        bayer[1] = R;
        bayer[2] = B;
        bayer[3] = G/2;
        break;
    case DC1394_COLOR_FILTER_GBRG:
        bayer[0] = G/2;
        bayer[1] = B;
        bayer[2] = R;
        bayer[3] = G/2;
        break;
    default:
        fprintf(stderr, "bayer pattern is %d\n", bayer_pattern);
        exit(1);
    }

    bzero(out, owidth); 
    out+=owidth;
    //at each y index we're actually filling the previous row.
    for(int y = 2; y < oheight; ++y) {
      //fprintf(stderr, "line %d\n", y);
        unsigned char *src = input + y * iwidth;
	int oline = (y&3) * owidth;
        int offset = (y&3) * iwidth;
        int back1 = ((y-1)&3) * iwidth - offset;
        int back2 = ((y-2)&3) * iwidth - offset;
   
        uint16_t *ot = temp + oline;
        unsigned char *br = bayer + 2 *(y&1); //which bayer  row are we on?
        
        unsigned char bayer1, bayer2;
	
        //now do the previous row
        *out = 0;
        ++out;
        ++ot;
        for(int x = 1; x < owidth-1; ++x) {
            bayer1 = br[(uint64_t)src&1];
            bayer2 = br[((uint64_t)src+1)&1];
            *ot = (((src[0] + src[2]) * bayer1) >> 1) + src[1] * bayer2;
            src+=1;
            //now do the previous row
            *out = (((ot[back2] + *ot) >> 1) + ot[back1]) >> 8;
            ++out;
            ++ot;
        }
        *out = 0;
        ++out;
        ++ot;
    }
#undef R
#undef G
#undef B
}
