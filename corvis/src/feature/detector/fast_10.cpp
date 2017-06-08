/*This is mechanically generated code*/
#include "fast.h"
#include <math.h>
#include <stdlib.h>
#include <algorithm>
using namespace std;

static bool xy_comp(const xy &first, const xy &second)
{
    return first.score > second.score;
}

vector<xy> &fast_detector_10::detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight)
{
    int need = number_wanted * 8;
    features.clear();
    features.reserve(need+1);
    int x, y, x1, y1, x2, y2;

    int bstart = bthresh;
    x1 = (winx < 8) ? 8: winx;
    y1 = (winy < 8) ? 8: winy;
    x2 = (winx + winwidth > xsize - 8) ? xsize - 8: winx + winwidth;
    y2 = (winy + winheight > ysize - 8) ? ysize - 8: winy + winheight;
    
    for(y = y1; y < y2; y++)
        for(x = x1; x < x2; x++) {
                if(mask && !mask->test(x, y)) { x += 7 - (x % 8); continue; }
                const byte* p = im + y*stride + x;
                byte val = (byte)(((uint16_t)p[0] + (((uint16_t)p[-stride] + (uint16_t)p[stride] + (uint16_t)p[-1] + (uint16_t)p[1]) >> 2)) >> 1);

                int bmin = bstart;
                int bmax = 255;
                int b = bstart;
    
                //Compute the score using binary search
	for(;;)
    {
		int cb = val + b;
		int c_b= val - b;


        if( p[pixel[0]] > cb)
         if( p[pixel[1]] > cb)
          if( p[pixel[2]] > cb)
           if( p[pixel[3]] > cb)
            if( p[pixel[4]] > cb)
             if( p[pixel[5]] > cb)
              if( p[pixel[6]] > cb)
               if( p[pixel[7]] > cb)
                if( p[pixel[8]] > cb)
                 if( p[pixel[9]] > cb)
                  goto is_a_corner;
                 else
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
              else if( p[pixel[6]] < c_b)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else if( p[pixel[12]] < c_b)
                if( p[pixel[7]] < c_b)
                 if( p[pixel[8]] < c_b)
                  if( p[pixel[9]] < c_b)
                   if( p[pixel[10]] < c_b)
                    if( p[pixel[11]] < c_b)
                     if( p[pixel[13]] < c_b)
                      if( p[pixel[14]] < c_b)
                       if( p[pixel[15]] < c_b)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
             else if( p[pixel[5]] < c_b)
              if( p[pixel[15]] > cb)
               if( p[pixel[11]] > cb)
                if( p[pixel[12]] > cb)
                 if( p[pixel[13]] > cb)
                  if( p[pixel[14]] > cb)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else if( p[pixel[11]] < c_b)
                if( p[pixel[6]] < c_b)
                 if( p[pixel[7]] < c_b)
                  if( p[pixel[8]] < c_b)
                   if( p[pixel[9]] < c_b)
                    if( p[pixel[10]] < c_b)
                     if( p[pixel[12]] < c_b)
                      if( p[pixel[13]] < c_b)
                       if( p[pixel[14]] < c_b)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               if( p[pixel[6]] < c_b)
                if( p[pixel[7]] < c_b)
                 if( p[pixel[8]] < c_b)
                  if( p[pixel[9]] < c_b)
                   if( p[pixel[10]] < c_b)
                    if( p[pixel[11]] < c_b)
                     if( p[pixel[12]] < c_b)
                      if( p[pixel[13]] < c_b)
                       if( p[pixel[14]] < c_b)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
             else
              if( p[pixel[11]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else if( p[pixel[11]] < c_b)
               if( p[pixel[6]] < c_b)
                if( p[pixel[7]] < c_b)
                 if( p[pixel[8]] < c_b)
                  if( p[pixel[9]] < c_b)
                   if( p[pixel[10]] < c_b)
                    if( p[pixel[12]] < c_b)
                     if( p[pixel[13]] < c_b)
                      if( p[pixel[14]] < c_b)
                       if( p[pixel[15]] < c_b)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
            else if( p[pixel[4]] < c_b)
             if( p[pixel[14]] > cb)
              if( p[pixel[10]] > cb)
               if( p[pixel[11]] > cb)
                if( p[pixel[12]] > cb)
                 if( p[pixel[13]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      if( p[pixel[8]] > cb)
                       if( p[pixel[9]] > cb)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else if( p[pixel[10]] < c_b)
               if( p[pixel[5]] < c_b)
                if( p[pixel[6]] < c_b)
                 if( p[pixel[7]] < c_b)
                  if( p[pixel[8]] < c_b)
                   if( p[pixel[9]] < c_b)
                    if( p[pixel[11]] < c_b)
                     if( p[pixel[12]] < c_b)
                      if( p[pixel[13]] < c_b)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else if( p[pixel[14]] < c_b)
              if( p[pixel[6]] < c_b)
               if( p[pixel[7]] < c_b)
                if( p[pixel[8]] < c_b)
                 if( p[pixel[9]] < c_b)
                  if( p[pixel[10]] < c_b)
                   if( p[pixel[11]] < c_b)
                    if( p[pixel[12]] < c_b)
                     if( p[pixel[13]] < c_b)
                      if( p[pixel[5]] < c_b)
                       goto is_a_corner;
                      else
                       if( p[pixel[15]] < c_b)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              if( p[pixel[5]] < c_b)
               if( p[pixel[6]] < c_b)
                if( p[pixel[7]] < c_b)
                 if( p[pixel[8]] < c_b)
                  if( p[pixel[9]] < c_b)
                   if( p[pixel[10]] < c_b)
                    if( p[pixel[11]] < c_b)
                     if( p[pixel[12]] < c_b)
                      if( p[pixel[13]] < c_b)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
            else
             if( p[pixel[10]] > cb)
              if( p[pixel[11]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      if( p[pixel[8]] > cb)
                       if( p[pixel[9]] > cb)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else if( p[pixel[10]] < c_b)
              if( p[pixel[6]] < c_b)
               if( p[pixel[7]] < c_b)
                if( p[pixel[8]] < c_b)
                 if( p[pixel[9]] < c_b)
                  if( p[pixel[11]] < c_b)
                   if( p[pixel[12]] < c_b)
                    if( p[pixel[13]] < c_b)
                     if( p[pixel[14]] < c_b)
                      if( p[pixel[5]] < c_b)
                       goto is_a_corner;
                      else
                       if( p[pixel[15]] < c_b)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
           else if( p[pixel[3]] < c_b)
            if( p[pixel[9]] > cb)
             if( p[pixel[10]] > cb)
              if( p[pixel[11]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      if( p[pixel[8]] > cb)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      if( p[pixel[8]] > cb)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else if( p[pixel[9]] < c_b)
             if( p[pixel[6]] < c_b)
              if( p[pixel[7]] < c_b)
               if( p[pixel[8]] < c_b)
                if( p[pixel[10]] < c_b)
                 if( p[pixel[11]] < c_b)
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[4]] < c_b)
                     goto is_a_corner;
                    else
                     if( p[pixel[13]] < c_b)
                      if( p[pixel[14]] < c_b)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                   else
                    if( p[pixel[13]] < c_b)
                     if( p[pixel[14]] < c_b)
                      if( p[pixel[15]] < c_b)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            if( p[pixel[9]] > cb)
             if( p[pixel[10]] > cb)
              if( p[pixel[11]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      if( p[pixel[8]] > cb)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      if( p[pixel[8]] > cb)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else if( p[pixel[9]] < c_b)
             if( p[pixel[6]] < c_b)
              if( p[pixel[7]] < c_b)
               if( p[pixel[8]] < c_b)
                if( p[pixel[10]] < c_b)
                 if( p[pixel[11]] < c_b)
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    if( p[pixel[5]] < c_b)
                     if( p[pixel[4]] < c_b)
                      goto is_a_corner;
                     else
                      if( p[pixel[14]] < c_b)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                    else
                     if( p[pixel[14]] < c_b)
                      if( p[pixel[15]] < c_b)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
          else if( p[pixel[2]] < c_b)
           if( p[pixel[8]] > cb)
            if( p[pixel[9]] > cb)
             if( p[pixel[10]] > cb)
              if( p[pixel[11]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[3]] > cb)
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else if( p[pixel[8]] < c_b)
            if( p[pixel[6]] < c_b)
             if( p[pixel[7]] < c_b)
              if( p[pixel[9]] < c_b)
               if( p[pixel[10]] < c_b)
                if( p[pixel[11]] < c_b)
                 if( p[pixel[5]] < c_b)
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[3]] < c_b)
                    goto is_a_corner;
                   else
                    if( p[pixel[12]] < c_b)
                     if( p[pixel[13]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                  else
                   if( p[pixel[12]] < c_b)
                    if( p[pixel[13]] < c_b)
                     if( p[pixel[14]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    if( p[pixel[14]] < c_b)
                     if( p[pixel[15]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else
           if( p[pixel[8]] > cb)
            if( p[pixel[9]] > cb)
             if( p[pixel[10]] > cb)
              if( p[pixel[11]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[3]] > cb)
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     if( p[pixel[7]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else if( p[pixel[8]] < c_b)
            if( p[pixel[6]] < c_b)
             if( p[pixel[7]] < c_b)
              if( p[pixel[9]] < c_b)
               if( p[pixel[10]] < c_b)
                if( p[pixel[11]] < c_b)
                 if( p[pixel[12]] < c_b)
                  if( p[pixel[5]] < c_b)
                   if( p[pixel[4]] < c_b)
                    if( p[pixel[3]] < c_b)
                     goto is_a_corner;
                    else
                     if( p[pixel[13]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                   else
                    if( p[pixel[13]] < c_b)
                     if( p[pixel[14]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                  else
                   if( p[pixel[13]] < c_b)
                    if( p[pixel[14]] < c_b)
                     if( p[pixel[15]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
         else if( p[pixel[1]] < c_b)
          if( p[pixel[7]] > cb)
           if( p[pixel[8]] > cb)
            if( p[pixel[9]] > cb)
             if( p[pixel[10]] > cb)
              if( p[pixel[11]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[3]] > cb)
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                if( p[pixel[2]] > cb)
                 if( p[pixel[3]] > cb)
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else if( p[pixel[7]] < c_b)
           if( p[pixel[6]] < c_b)
            if( p[pixel[8]] < c_b)
             if( p[pixel[9]] < c_b)
              if( p[pixel[10]] < c_b)
               if( p[pixel[5]] < c_b)
                if( p[pixel[4]] < c_b)
                 if( p[pixel[3]] < c_b)
                  if( p[pixel[2]] < c_b)
                   goto is_a_corner;
                  else
                   if( p[pixel[11]] < c_b)
                    if( p[pixel[12]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[11]] < c_b)
                   if( p[pixel[12]] < c_b)
                    if( p[pixel[13]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[11]] < c_b)
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    if( p[pixel[14]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                if( p[pixel[11]] < c_b)
                 if( p[pixel[12]] < c_b)
                  if( p[pixel[13]] < c_b)
                   if( p[pixel[14]] < c_b)
                    if( p[pixel[15]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else
           goto is_not_a_corner;
         else
          if( p[pixel[7]] > cb)
           if( p[pixel[8]] > cb)
            if( p[pixel[9]] > cb)
             if( p[pixel[10]] > cb)
              if( p[pixel[11]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[13]] > cb)
                 if( p[pixel[14]] > cb)
                  if( p[pixel[15]] > cb)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[3]] > cb)
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                if( p[pixel[2]] > cb)
                 if( p[pixel[3]] > cb)
                  if( p[pixel[4]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[6]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else if( p[pixel[7]] < c_b)
           if( p[pixel[6]] < c_b)
            if( p[pixel[8]] < c_b)
             if( p[pixel[9]] < c_b)
              if( p[pixel[10]] < c_b)
               if( p[pixel[11]] < c_b)
                if( p[pixel[5]] < c_b)
                 if( p[pixel[4]] < c_b)
                  if( p[pixel[3]] < c_b)
                   if( p[pixel[2]] < c_b)
                    goto is_a_corner;
                   else
                    if( p[pixel[12]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                  else
                   if( p[pixel[12]] < c_b)
                    if( p[pixel[13]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    if( p[pixel[14]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[12]] < c_b)
                  if( p[pixel[13]] < c_b)
                   if( p[pixel[14]] < c_b)
                    if( p[pixel[15]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else
           goto is_not_a_corner;
        else if( p[pixel[0]] < c_b)
         if( p[pixel[1]] > cb)
          if( p[pixel[7]] > cb)
           if( p[pixel[6]] > cb)
            if( p[pixel[8]] > cb)
             if( p[pixel[9]] > cb)
              if( p[pixel[10]] > cb)
               if( p[pixel[5]] > cb)
                if( p[pixel[4]] > cb)
                 if( p[pixel[3]] > cb)
                  if( p[pixel[2]] > cb)
                   goto is_a_corner;
                  else
                   if( p[pixel[11]] > cb)
                    if( p[pixel[12]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[11]] > cb)
                   if( p[pixel[12]] > cb)
                    if( p[pixel[13]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[11]] > cb)
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    if( p[pixel[14]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                if( p[pixel[11]] > cb)
                 if( p[pixel[12]] > cb)
                  if( p[pixel[13]] > cb)
                   if( p[pixel[14]] > cb)
                    if( p[pixel[15]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else if( p[pixel[7]] < c_b)
           if( p[pixel[8]] < c_b)
            if( p[pixel[9]] < c_b)
             if( p[pixel[10]] < c_b)
              if( p[pixel[11]] < c_b)
               if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[3]] < c_b)
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                if( p[pixel[2]] < c_b)
                 if( p[pixel[3]] < c_b)
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else
           goto is_not_a_corner;
         else if( p[pixel[1]] < c_b)
          if( p[pixel[2]] > cb)
           if( p[pixel[8]] > cb)
            if( p[pixel[6]] > cb)
             if( p[pixel[7]] > cb)
              if( p[pixel[9]] > cb)
               if( p[pixel[10]] > cb)
                if( p[pixel[11]] > cb)
                 if( p[pixel[5]] > cb)
                  if( p[pixel[4]] > cb)
                   if( p[pixel[3]] > cb)
                    goto is_a_corner;
                   else
                    if( p[pixel[12]] > cb)
                     if( p[pixel[13]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                  else
                   if( p[pixel[12]] > cb)
                    if( p[pixel[13]] > cb)
                     if( p[pixel[14]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    if( p[pixel[14]] > cb)
                     if( p[pixel[15]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else if( p[pixel[8]] < c_b)
            if( p[pixel[9]] < c_b)
             if( p[pixel[10]] < c_b)
              if( p[pixel[11]] < c_b)
               if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[3]] < c_b)
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else if( p[pixel[2]] < c_b)
           if( p[pixel[3]] > cb)
            if( p[pixel[9]] > cb)
             if( p[pixel[6]] > cb)
              if( p[pixel[7]] > cb)
               if( p[pixel[8]] > cb)
                if( p[pixel[10]] > cb)
                 if( p[pixel[11]] > cb)
                  if( p[pixel[12]] > cb)
                   if( p[pixel[5]] > cb)
                    if( p[pixel[4]] > cb)
                     goto is_a_corner;
                    else
                     if( p[pixel[13]] > cb)
                      if( p[pixel[14]] > cb)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                   else
                    if( p[pixel[13]] > cb)
                     if( p[pixel[14]] > cb)
                      if( p[pixel[15]] > cb)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else if( p[pixel[9]] < c_b)
             if( p[pixel[10]] < c_b)
              if( p[pixel[11]] < c_b)
               if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      if( p[pixel[8]] < c_b)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      if( p[pixel[8]] < c_b)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else if( p[pixel[3]] < c_b)
            if( p[pixel[4]] > cb)
             if( p[pixel[14]] > cb)
              if( p[pixel[6]] > cb)
               if( p[pixel[7]] > cb)
                if( p[pixel[8]] > cb)
                 if( p[pixel[9]] > cb)
                  if( p[pixel[10]] > cb)
                   if( p[pixel[11]] > cb)
                    if( p[pixel[12]] > cb)
                     if( p[pixel[13]] > cb)
                      if( p[pixel[5]] > cb)
                       goto is_a_corner;
                      else
                       if( p[pixel[15]] > cb)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else if( p[pixel[14]] < c_b)
              if( p[pixel[10]] > cb)
               if( p[pixel[5]] > cb)
                if( p[pixel[6]] > cb)
                 if( p[pixel[7]] > cb)
                  if( p[pixel[8]] > cb)
                   if( p[pixel[9]] > cb)
                    if( p[pixel[11]] > cb)
                     if( p[pixel[12]] > cb)
                      if( p[pixel[13]] > cb)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else if( p[pixel[10]] < c_b)
               if( p[pixel[11]] < c_b)
                if( p[pixel[12]] < c_b)
                 if( p[pixel[13]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      if( p[pixel[8]] < c_b)
                       if( p[pixel[9]] < c_b)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              if( p[pixel[5]] > cb)
               if( p[pixel[6]] > cb)
                if( p[pixel[7]] > cb)
                 if( p[pixel[8]] > cb)
                  if( p[pixel[9]] > cb)
                   if( p[pixel[10]] > cb)
                    if( p[pixel[11]] > cb)
                     if( p[pixel[12]] > cb)
                      if( p[pixel[13]] > cb)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
            else if( p[pixel[4]] < c_b)
             if( p[pixel[5]] > cb)
              if( p[pixel[15]] < c_b)
               if( p[pixel[11]] > cb)
                if( p[pixel[6]] > cb)
                 if( p[pixel[7]] > cb)
                  if( p[pixel[8]] > cb)
                   if( p[pixel[9]] > cb)
                    if( p[pixel[10]] > cb)
                     if( p[pixel[12]] > cb)
                      if( p[pixel[13]] > cb)
                       if( p[pixel[14]] > cb)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else if( p[pixel[11]] < c_b)
                if( p[pixel[12]] < c_b)
                 if( p[pixel[13]] < c_b)
                  if( p[pixel[14]] < c_b)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               if( p[pixel[6]] > cb)
                if( p[pixel[7]] > cb)
                 if( p[pixel[8]] > cb)
                  if( p[pixel[9]] > cb)
                   if( p[pixel[10]] > cb)
                    if( p[pixel[11]] > cb)
                     if( p[pixel[12]] > cb)
                      if( p[pixel[13]] > cb)
                       if( p[pixel[14]] > cb)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
             else if( p[pixel[5]] < c_b)
              if( p[pixel[6]] > cb)
               if( p[pixel[12]] > cb)
                if( p[pixel[7]] > cb)
                 if( p[pixel[8]] > cb)
                  if( p[pixel[9]] > cb)
                   if( p[pixel[10]] > cb)
                    if( p[pixel[11]] > cb)
                     if( p[pixel[13]] > cb)
                      if( p[pixel[14]] > cb)
                       if( p[pixel[15]] > cb)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else if( p[pixel[6]] < c_b)
               if( p[pixel[7]] < c_b)
                if( p[pixel[8]] < c_b)
                 if( p[pixel[9]] < c_b)
                  goto is_a_corner;
                 else
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
              else
               if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
             else
              if( p[pixel[11]] > cb)
               if( p[pixel[6]] > cb)
                if( p[pixel[7]] > cb)
                 if( p[pixel[8]] > cb)
                  if( p[pixel[9]] > cb)
                   if( p[pixel[10]] > cb)
                    if( p[pixel[12]] > cb)
                     if( p[pixel[13]] > cb)
                      if( p[pixel[14]] > cb)
                       if( p[pixel[15]] > cb)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else if( p[pixel[11]] < c_b)
               if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
            else
             if( p[pixel[10]] > cb)
              if( p[pixel[6]] > cb)
               if( p[pixel[7]] > cb)
                if( p[pixel[8]] > cb)
                 if( p[pixel[9]] > cb)
                  if( p[pixel[11]] > cb)
                   if( p[pixel[12]] > cb)
                    if( p[pixel[13]] > cb)
                     if( p[pixel[14]] > cb)
                      if( p[pixel[5]] > cb)
                       goto is_a_corner;
                      else
                       if( p[pixel[15]] > cb)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else if( p[pixel[10]] < c_b)
              if( p[pixel[11]] < c_b)
               if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      if( p[pixel[8]] < c_b)
                       if( p[pixel[9]] < c_b)
                        goto is_a_corner;
                       else
                        goto is_not_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
           else
            if( p[pixel[9]] > cb)
             if( p[pixel[6]] > cb)
              if( p[pixel[7]] > cb)
               if( p[pixel[8]] > cb)
                if( p[pixel[10]] > cb)
                 if( p[pixel[11]] > cb)
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    if( p[pixel[5]] > cb)
                     if( p[pixel[4]] > cb)
                      goto is_a_corner;
                     else
                      if( p[pixel[14]] > cb)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                    else
                     if( p[pixel[14]] > cb)
                      if( p[pixel[15]] > cb)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else if( p[pixel[9]] < c_b)
             if( p[pixel[10]] < c_b)
              if( p[pixel[11]] < c_b)
               if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      if( p[pixel[8]] < c_b)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      if( p[pixel[8]] < c_b)
                       goto is_a_corner;
                      else
                       goto is_not_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
          else
           if( p[pixel[8]] > cb)
            if( p[pixel[6]] > cb)
             if( p[pixel[7]] > cb)
              if( p[pixel[9]] > cb)
               if( p[pixel[10]] > cb)
                if( p[pixel[11]] > cb)
                 if( p[pixel[12]] > cb)
                  if( p[pixel[5]] > cb)
                   if( p[pixel[4]] > cb)
                    if( p[pixel[3]] > cb)
                     goto is_a_corner;
                    else
                     if( p[pixel[13]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                   else
                    if( p[pixel[13]] > cb)
                     if( p[pixel[14]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                  else
                   if( p[pixel[13]] > cb)
                    if( p[pixel[14]] > cb)
                     if( p[pixel[15]] > cb)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else if( p[pixel[8]] < c_b)
            if( p[pixel[9]] < c_b)
             if( p[pixel[10]] < c_b)
              if( p[pixel[11]] < c_b)
               if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[3]] < c_b)
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     if( p[pixel[7]] < c_b)
                      goto is_a_corner;
                     else
                      goto is_not_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
         else
          if( p[pixel[7]] > cb)
           if( p[pixel[6]] > cb)
            if( p[pixel[8]] > cb)
             if( p[pixel[9]] > cb)
              if( p[pixel[10]] > cb)
               if( p[pixel[11]] > cb)
                if( p[pixel[5]] > cb)
                 if( p[pixel[4]] > cb)
                  if( p[pixel[3]] > cb)
                   if( p[pixel[2]] > cb)
                    goto is_a_corner;
                   else
                    if( p[pixel[12]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                  else
                   if( p[pixel[12]] > cb)
                    if( p[pixel[13]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    if( p[pixel[14]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[12]] > cb)
                  if( p[pixel[13]] > cb)
                   if( p[pixel[14]] > cb)
                    if( p[pixel[15]] > cb)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else if( p[pixel[7]] < c_b)
           if( p[pixel[8]] < c_b)
            if( p[pixel[9]] < c_b)
             if( p[pixel[10]] < c_b)
              if( p[pixel[11]] < c_b)
               if( p[pixel[12]] < c_b)
                if( p[pixel[13]] < c_b)
                 if( p[pixel[14]] < c_b)
                  if( p[pixel[15]] < c_b)
                   goto is_a_corner;
                  else
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[3]] < c_b)
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                if( p[pixel[2]] < c_b)
                 if( p[pixel[3]] < c_b)
                  if( p[pixel[4]] < c_b)
                   if( p[pixel[5]] < c_b)
                    if( p[pixel[6]] < c_b)
                     goto is_a_corner;
                    else
                     goto is_not_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
              else
               goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else
           goto is_not_a_corner;
        else
         if( p[pixel[6]] > cb)
          if( p[pixel[7]] > cb)
           if( p[pixel[8]] > cb)
            if( p[pixel[9]] > cb)
             if( p[pixel[10]] > cb)
              if( p[pixel[5]] > cb)
               if( p[pixel[4]] > cb)
                if( p[pixel[3]] > cb)
                 if( p[pixel[2]] > cb)
                  if( p[pixel[1]] > cb)
                   goto is_a_corner;
                  else
                   if( p[pixel[11]] > cb)
                    goto is_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[11]] > cb)
                   if( p[pixel[12]] > cb)
                    goto is_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[11]] > cb)
                  if( p[pixel[12]] > cb)
                   if( p[pixel[13]] > cb)
                    goto is_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                if( p[pixel[11]] > cb)
                 if( p[pixel[12]] > cb)
                  if( p[pixel[13]] > cb)
                   if( p[pixel[14]] > cb)
                    goto is_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
              else
               if( p[pixel[11]] > cb)
                if( p[pixel[12]] > cb)
                 if( p[pixel[13]] > cb)
                  if( p[pixel[14]] > cb)
                   if( p[pixel[15]] > cb)
                    goto is_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else
           goto is_not_a_corner;
         else if( p[pixel[6]] < c_b)
          if( p[pixel[7]] < c_b)
           if( p[pixel[8]] < c_b)
            if( p[pixel[9]] < c_b)
             if( p[pixel[10]] < c_b)
              if( p[pixel[5]] < c_b)
               if( p[pixel[4]] < c_b)
                if( p[pixel[3]] < c_b)
                 if( p[pixel[2]] < c_b)
                  if( p[pixel[1]] < c_b)
                   goto is_a_corner;
                  else
                   if( p[pixel[11]] < c_b)
                    goto is_a_corner;
                   else
                    goto is_not_a_corner;
                 else
                  if( p[pixel[11]] < c_b)
                   if( p[pixel[12]] < c_b)
                    goto is_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                else
                 if( p[pixel[11]] < c_b)
                  if( p[pixel[12]] < c_b)
                   if( p[pixel[13]] < c_b)
                    goto is_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
               else
                if( p[pixel[11]] < c_b)
                 if( p[pixel[12]] < c_b)
                  if( p[pixel[13]] < c_b)
                   if( p[pixel[14]] < c_b)
                    goto is_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
              else
               if( p[pixel[11]] < c_b)
                if( p[pixel[12]] < c_b)
                 if( p[pixel[13]] < c_b)
                  if( p[pixel[14]] < c_b)
                   if( p[pixel[15]] < c_b)
                    goto is_a_corner;
                   else
                    goto is_not_a_corner;
                  else
                   goto is_not_a_corner;
                 else
                  goto is_not_a_corner;
                else
                 goto is_not_a_corner;
               else
                goto is_not_a_corner;
             else
              goto is_not_a_corner;
            else
             goto is_not_a_corner;
           else
            goto is_not_a_corner;
          else
           goto is_not_a_corner;
         else
          goto is_not_a_corner;

		is_a_corner:
			bmin=b;
			goto end_if;

		is_not_a_corner:
                        if(b == bstart) break;
			bmax=b;
			goto end_if;

		end_if:

                        if(bmin == bmax - 1 || bmin == bmax) {
                            features.push_back({(float)x, (float)y, (float)bmin, 0});
                            push_heap(features.begin(), features.end(), xy_comp);
                            if(features.size() > need) {
                                pop_heap(features.begin(), features.end(), xy_comp);
                                features.pop_back();
                                bstart = (int)(features[0].score + 1);
                            }
                            break;
                        }
                        b = (bmin + bmax) / 2;
                }
            }
    sort_heap(features.begin(), features.end(), xy_comp);
    return features;
}

static void make_offsets(int pixel[], int row_stride)
{
        pixel[0] = 0 + row_stride * 3;
        pixel[1] = 1 + row_stride * 3;
        pixel[2] = 2 + row_stride * 2;
        pixel[3] = 3 + row_stride * 1;
        pixel[4] = 3 + row_stride * 0;
        pixel[5] = 3 + row_stride * -1;
        pixel[6] = 2 + row_stride * -2;
        pixel[7] = 1 + row_stride * -3;
        pixel[8] = 0 + row_stride * -3;
        pixel[9] = -1 + row_stride * -3;
        pixel[10] = -2 + row_stride * -2;
        pixel[11] = -3 + row_stride * -1;
        pixel[12] = -3 + row_stride * 0;
        pixel[13] = -3 + row_stride * 1;
        pixel[14] = -2 + row_stride * 2;
        pixel[15] = -1 + row_stride * 3;
}

void fast_detector_10::init(const int x, const int y, const int s, const int ps, const int phw)
{
    xsize = x;
    ysize = y;
    stride = s;
    patch_stride = ps;
    patch_win_half_width = phw;
    make_offsets(pixel, stride);
}

template<typename Descriptor>
xy fast_detector_10::track(const Descriptor& descriptor, const tracker::image& image, float predx, float predy, float radius, int b)
{
    int x, y;
    
    xy best = {INFINITY, INFINITY, Descriptor::min_score, 0.f};
    
    int x1 = (int)ceilf(predx - radius);
    int x2 = (int)floorf(predx + radius);
    int y1 = (int)ceilf(predy - radius);
    int y2 = (int)floorf(predy + radius);
    
    int half = Descriptor::border_size;
    
    if(x1 < half || x2 >= xsize - half || y1 < half || y2 >= ysize - half)
        return best;
 
    for(y = y1; y <= y2; y++) {
        for(x = x1; x <= x2; x++) {
            const byte* p = image.image + y*stride + x;
            byte val = (byte)(((uint16_t)p[0] + (((uint16_t)p[-stride] + (uint16_t)p[stride] + (uint16_t)p[-1] + (uint16_t)p[1]) >> 2)) >> 1);
		
            int cb = val + b;
            int c_b= val - b;

        if(p[pixel[0]] > cb)
         if(p[pixel[1]] > cb)
          if(p[pixel[2]] > cb)
           if(p[pixel[3]] > cb)
            if(p[pixel[4]] > cb)
             if(p[pixel[5]] > cb)
              if(p[pixel[6]] > cb)
               if(p[pixel[7]] > cb)
                if(p[pixel[8]] > cb)
                 if(p[pixel[9]] > cb)
                  {}
                 else
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   continue;
                else
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   continue;
                 else
                  continue;
               else
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else if(p[pixel[6]] < c_b)
               if(p[pixel[12]] > cb)
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else if(p[pixel[12]] < c_b)
                if(p[pixel[7]] < c_b)
                 if(p[pixel[8]] < c_b)
                  if(p[pixel[9]] < c_b)
                   if(p[pixel[10]] < c_b)
                    if(p[pixel[11]] < c_b)
                     if(p[pixel[13]] < c_b)
                      if(p[pixel[14]] < c_b)
                       if(p[pixel[15]] < c_b)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               if(p[pixel[12]] > cb)
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
             else if(p[pixel[5]] < c_b)
              if(p[pixel[15]] > cb)
               if(p[pixel[11]] > cb)
                if(p[pixel[12]] > cb)
                 if(p[pixel[13]] > cb)
                  if(p[pixel[14]] > cb)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else if(p[pixel[11]] < c_b)
                if(p[pixel[6]] < c_b)
                 if(p[pixel[7]] < c_b)
                  if(p[pixel[8]] < c_b)
                   if(p[pixel[9]] < c_b)
                    if(p[pixel[10]] < c_b)
                     if(p[pixel[12]] < c_b)
                      if(p[pixel[13]] < c_b)
                       if(p[pixel[14]] < c_b)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               if(p[pixel[6]] < c_b)
                if(p[pixel[7]] < c_b)
                 if(p[pixel[8]] < c_b)
                  if(p[pixel[9]] < c_b)
                   if(p[pixel[10]] < c_b)
                    if(p[pixel[11]] < c_b)
                     if(p[pixel[12]] < c_b)
                      if(p[pixel[13]] < c_b)
                       if(p[pixel[14]] < c_b)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
             else
              if(p[pixel[11]] > cb)
               if(p[pixel[12]] > cb)
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else if(p[pixel[11]] < c_b)
               if(p[pixel[6]] < c_b)
                if(p[pixel[7]] < c_b)
                 if(p[pixel[8]] < c_b)
                  if(p[pixel[9]] < c_b)
                   if(p[pixel[10]] < c_b)
                    if(p[pixel[12]] < c_b)
                     if(p[pixel[13]] < c_b)
                      if(p[pixel[14]] < c_b)
                       if(p[pixel[15]] < c_b)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
            else if(p[pixel[4]] < c_b)
             if(p[pixel[14]] > cb)
              if(p[pixel[10]] > cb)
               if(p[pixel[11]] > cb)
                if(p[pixel[12]] > cb)
                 if(p[pixel[13]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      if(p[pixel[8]] > cb)
                       if(p[pixel[9]] > cb)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else if(p[pixel[10]] < c_b)
               if(p[pixel[5]] < c_b)
                if(p[pixel[6]] < c_b)
                 if(p[pixel[7]] < c_b)
                  if(p[pixel[8]] < c_b)
                   if(p[pixel[9]] < c_b)
                    if(p[pixel[11]] < c_b)
                     if(p[pixel[12]] < c_b)
                      if(p[pixel[13]] < c_b)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else if(p[pixel[14]] < c_b)
              if(p[pixel[6]] < c_b)
               if(p[pixel[7]] < c_b)
                if(p[pixel[8]] < c_b)
                 if(p[pixel[9]] < c_b)
                  if(p[pixel[10]] < c_b)
                   if(p[pixel[11]] < c_b)
                    if(p[pixel[12]] < c_b)
                     if(p[pixel[13]] < c_b)
                      if(p[pixel[5]] < c_b)
                       {}
                      else
                       if(p[pixel[15]] < c_b)
                        {}
                       else
                        continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              if(p[pixel[5]] < c_b)
               if(p[pixel[6]] < c_b)
                if(p[pixel[7]] < c_b)
                 if(p[pixel[8]] < c_b)
                  if(p[pixel[9]] < c_b)
                   if(p[pixel[10]] < c_b)
                    if(p[pixel[11]] < c_b)
                     if(p[pixel[12]] < c_b)
                      if(p[pixel[13]] < c_b)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
            else
             if(p[pixel[10]] > cb)
              if(p[pixel[11]] > cb)
               if(p[pixel[12]] > cb)
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      if(p[pixel[8]] > cb)
                       if(p[pixel[9]] > cb)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else if(p[pixel[10]] < c_b)
              if(p[pixel[6]] < c_b)
               if(p[pixel[7]] < c_b)
                if(p[pixel[8]] < c_b)
                 if(p[pixel[9]] < c_b)
                  if(p[pixel[11]] < c_b)
                   if(p[pixel[12]] < c_b)
                    if(p[pixel[13]] < c_b)
                     if(p[pixel[14]] < c_b)
                      if(p[pixel[5]] < c_b)
                       {}
                      else
                       if(p[pixel[15]] < c_b)
                        {}
                       else
                        continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
           else if(p[pixel[3]] < c_b)
            if(p[pixel[9]] > cb)
             if(p[pixel[10]] > cb)
              if(p[pixel[11]] > cb)
               if(p[pixel[12]] > cb)
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      if(p[pixel[8]] > cb)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      if(p[pixel[8]] > cb)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else if(p[pixel[9]] < c_b)
             if(p[pixel[6]] < c_b)
              if(p[pixel[7]] < c_b)
               if(p[pixel[8]] < c_b)
                if(p[pixel[10]] < c_b)
                 if(p[pixel[11]] < c_b)
                  if(p[pixel[12]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[4]] < c_b)
                     {}
                    else
                     if(p[pixel[13]] < c_b)
                      if(p[pixel[14]] < c_b)
                       {}
                      else
                       continue;
                     else
                      continue;
                   else
                    if(p[pixel[13]] < c_b)
                     if(p[pixel[14]] < c_b)
                      if(p[pixel[15]] < c_b)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            if(p[pixel[9]] > cb)
             if(p[pixel[10]] > cb)
              if(p[pixel[11]] > cb)
               if(p[pixel[12]] > cb)
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      if(p[pixel[8]] > cb)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      if(p[pixel[8]] > cb)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else if(p[pixel[9]] < c_b)
             if(p[pixel[6]] < c_b)
              if(p[pixel[7]] < c_b)
               if(p[pixel[8]] < c_b)
                if(p[pixel[10]] < c_b)
                 if(p[pixel[11]] < c_b)
                  if(p[pixel[12]] < c_b)
                   if(p[pixel[13]] < c_b)
                    if(p[pixel[5]] < c_b)
                     if(p[pixel[4]] < c_b)
                      {}
                     else
                      if(p[pixel[14]] < c_b)
                       {}
                      else
                       continue;
                    else
                     if(p[pixel[14]] < c_b)
                      if(p[pixel[15]] < c_b)
                       {}
                      else
                       continue;
                     else
                      continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
          else if(p[pixel[2]] < c_b)
           if(p[pixel[8]] > cb)
            if(p[pixel[9]] > cb)
             if(p[pixel[10]] > cb)
              if(p[pixel[11]] > cb)
               if(p[pixel[12]] > cb)
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[3]] > cb)
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else if(p[pixel[8]] < c_b)
            if(p[pixel[6]] < c_b)
             if(p[pixel[7]] < c_b)
              if(p[pixel[9]] < c_b)
               if(p[pixel[10]] < c_b)
                if(p[pixel[11]] < c_b)
                 if(p[pixel[5]] < c_b)
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[3]] < c_b)
                    {}
                   else
                    if(p[pixel[12]] < c_b)
                     if(p[pixel[13]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                  else
                   if(p[pixel[12]] < c_b)
                    if(p[pixel[13]] < c_b)
                     if(p[pixel[14]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[12]] < c_b)
                   if(p[pixel[13]] < c_b)
                    if(p[pixel[14]] < c_b)
                     if(p[pixel[15]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else
           if(p[pixel[8]] > cb)
            if(p[pixel[9]] > cb)
             if(p[pixel[10]] > cb)
              if(p[pixel[11]] > cb)
               if(p[pixel[12]] > cb)
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[3]] > cb)
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     if(p[pixel[7]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else if(p[pixel[8]] < c_b)
            if(p[pixel[6]] < c_b)
             if(p[pixel[7]] < c_b)
              if(p[pixel[9]] < c_b)
               if(p[pixel[10]] < c_b)
                if(p[pixel[11]] < c_b)
                 if(p[pixel[12]] < c_b)
                  if(p[pixel[5]] < c_b)
                   if(p[pixel[4]] < c_b)
                    if(p[pixel[3]] < c_b)
                     {}
                    else
                     if(p[pixel[13]] < c_b)
                      {}
                     else
                      continue;
                   else
                    if(p[pixel[13]] < c_b)
                     if(p[pixel[14]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                  else
                   if(p[pixel[13]] < c_b)
                    if(p[pixel[14]] < c_b)
                     if(p[pixel[15]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
         else if(p[pixel[1]] < c_b)
          if(p[pixel[7]] > cb)
           if(p[pixel[8]] > cb)
            if(p[pixel[9]] > cb)
             if(p[pixel[10]] > cb)
              if(p[pixel[11]] > cb)
               if(p[pixel[12]] > cb)
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[3]] > cb)
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(p[pixel[2]] > cb)
                 if(p[pixel[3]] > cb)
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else if(p[pixel[7]] < c_b)
           if(p[pixel[6]] < c_b)
            if(p[pixel[8]] < c_b)
             if(p[pixel[9]] < c_b)
              if(p[pixel[10]] < c_b)
               if(p[pixel[5]] < c_b)
                if(p[pixel[4]] < c_b)
                 if(p[pixel[3]] < c_b)
                  if(p[pixel[2]] < c_b)
                   {}
                  else
                   if(p[pixel[11]] < c_b)
                    if(p[pixel[12]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[11]] < c_b)
                   if(p[pixel[12]] < c_b)
                    if(p[pixel[13]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[11]] < c_b)
                  if(p[pixel[12]] < c_b)
                   if(p[pixel[13]] < c_b)
                    if(p[pixel[14]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(p[pixel[11]] < c_b)
                 if(p[pixel[12]] < c_b)
                  if(p[pixel[13]] < c_b)
                   if(p[pixel[14]] < c_b)
                    if(p[pixel[15]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else
           continue;
         else
          if(p[pixel[7]] > cb)
           if(p[pixel[8]] > cb)
            if(p[pixel[9]] > cb)
             if(p[pixel[10]] > cb)
              if(p[pixel[11]] > cb)
               if(p[pixel[12]] > cb)
                if(p[pixel[13]] > cb)
                 if(p[pixel[14]] > cb)
                  if(p[pixel[15]] > cb)
                   {}
                  else
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[3]] > cb)
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(p[pixel[2]] > cb)
                 if(p[pixel[3]] > cb)
                  if(p[pixel[4]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[6]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else if(p[pixel[7]] < c_b)
           if(p[pixel[6]] < c_b)
            if(p[pixel[8]] < c_b)
             if(p[pixel[9]] < c_b)
              if(p[pixel[10]] < c_b)
               if(p[pixel[11]] < c_b)
                if(p[pixel[5]] < c_b)
                 if(p[pixel[4]] < c_b)
                  if(p[pixel[3]] < c_b)
                   if(p[pixel[2]] < c_b)
                    {}
                   else
                    if(p[pixel[12]] < c_b)
                     {}
                    else
                     continue;
                  else
                   if(p[pixel[12]] < c_b)
                    if(p[pixel[13]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[12]] < c_b)
                   if(p[pixel[13]] < c_b)
                    if(p[pixel[14]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[12]] < c_b)
                  if(p[pixel[13]] < c_b)
                   if(p[pixel[14]] < c_b)
                    if(p[pixel[15]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else
           continue;
        else if(p[pixel[0]] < c_b)
         if(p[pixel[1]] > cb)
          if(p[pixel[7]] > cb)
           if(p[pixel[6]] > cb)
            if(p[pixel[8]] > cb)
             if(p[pixel[9]] > cb)
              if(p[pixel[10]] > cb)
               if(p[pixel[5]] > cb)
                if(p[pixel[4]] > cb)
                 if(p[pixel[3]] > cb)
                  if(p[pixel[2]] > cb)
                   {}
                  else
                   if(p[pixel[11]] > cb)
                    if(p[pixel[12]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[11]] > cb)
                   if(p[pixel[12]] > cb)
                    if(p[pixel[13]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[11]] > cb)
                  if(p[pixel[12]] > cb)
                   if(p[pixel[13]] > cb)
                    if(p[pixel[14]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(p[pixel[11]] > cb)
                 if(p[pixel[12]] > cb)
                  if(p[pixel[13]] > cb)
                   if(p[pixel[14]] > cb)
                    if(p[pixel[15]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else if(p[pixel[7]] < c_b)
           if(p[pixel[8]] < c_b)
            if(p[pixel[9]] < c_b)
             if(p[pixel[10]] < c_b)
              if(p[pixel[11]] < c_b)
               if(p[pixel[12]] < c_b)
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[3]] < c_b)
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(p[pixel[2]] < c_b)
                 if(p[pixel[3]] < c_b)
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else
           continue;
         else if(p[pixel[1]] < c_b)
          if(p[pixel[2]] > cb)
           if(p[pixel[8]] > cb)
            if(p[pixel[6]] > cb)
             if(p[pixel[7]] > cb)
              if(p[pixel[9]] > cb)
               if(p[pixel[10]] > cb)
                if(p[pixel[11]] > cb)
                 if(p[pixel[5]] > cb)
                  if(p[pixel[4]] > cb)
                   if(p[pixel[3]] > cb)
                    {}
                   else
                    if(p[pixel[12]] > cb)
                     if(p[pixel[13]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                  else
                   if(p[pixel[12]] > cb)
                    if(p[pixel[13]] > cb)
                     if(p[pixel[14]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[12]] > cb)
                   if(p[pixel[13]] > cb)
                    if(p[pixel[14]] > cb)
                     if(p[pixel[15]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else if(p[pixel[8]] < c_b)
            if(p[pixel[9]] < c_b)
             if(p[pixel[10]] < c_b)
              if(p[pixel[11]] < c_b)
               if(p[pixel[12]] < c_b)
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[3]] < c_b)
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else if(p[pixel[2]] < c_b)
           if(p[pixel[3]] > cb)
            if(p[pixel[9]] > cb)
             if(p[pixel[6]] > cb)
              if(p[pixel[7]] > cb)
               if(p[pixel[8]] > cb)
                if(p[pixel[10]] > cb)
                 if(p[pixel[11]] > cb)
                  if(p[pixel[12]] > cb)
                   if(p[pixel[5]] > cb)
                    if(p[pixel[4]] > cb)
                     {}
                    else
                     if(p[pixel[13]] > cb)
                      if(p[pixel[14]] > cb)
                       {}
                      else
                       continue;
                     else
                      continue;
                   else
                    if(p[pixel[13]] > cb)
                     if(p[pixel[14]] > cb)
                      if(p[pixel[15]] > cb)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else if(p[pixel[9]] < c_b)
             if(p[pixel[10]] < c_b)
              if(p[pixel[11]] < c_b)
               if(p[pixel[12]] < c_b)
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      if(p[pixel[8]] < c_b)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      if(p[pixel[8]] < c_b)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else if(p[pixel[3]] < c_b)
            if(p[pixel[4]] > cb)
             if(p[pixel[14]] > cb)
              if(p[pixel[6]] > cb)
               if(p[pixel[7]] > cb)
                if(p[pixel[8]] > cb)
                 if(p[pixel[9]] > cb)
                  if(p[pixel[10]] > cb)
                   if(p[pixel[11]] > cb)
                    if(p[pixel[12]] > cb)
                     if(p[pixel[13]] > cb)
                      if(p[pixel[5]] > cb)
                       {}
                      else
                       if(p[pixel[15]] > cb)
                        {}
                       else
                        continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else if(p[pixel[14]] < c_b)
              if(p[pixel[10]] > cb)
               if(p[pixel[5]] > cb)
                if(p[pixel[6]] > cb)
                 if(p[pixel[7]] > cb)
                  if(p[pixel[8]] > cb)
                   if(p[pixel[9]] > cb)
                    if(p[pixel[11]] > cb)
                     if(p[pixel[12]] > cb)
                      if(p[pixel[13]] > cb)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else if(p[pixel[10]] < c_b)
               if(p[pixel[11]] < c_b)
                if(p[pixel[12]] < c_b)
                 if(p[pixel[13]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      if(p[pixel[8]] < c_b)
                       if(p[pixel[9]] < c_b)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              if(p[pixel[5]] > cb)
               if(p[pixel[6]] > cb)
                if(p[pixel[7]] > cb)
                 if(p[pixel[8]] > cb)
                  if(p[pixel[9]] > cb)
                   if(p[pixel[10]] > cb)
                    if(p[pixel[11]] > cb)
                     if(p[pixel[12]] > cb)
                      if(p[pixel[13]] > cb)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
            else if(p[pixel[4]] < c_b)
             if(p[pixel[5]] > cb)
              if(p[pixel[15]] < c_b)
               if(p[pixel[11]] > cb)
                if(p[pixel[6]] > cb)
                 if(p[pixel[7]] > cb)
                  if(p[pixel[8]] > cb)
                   if(p[pixel[9]] > cb)
                    if(p[pixel[10]] > cb)
                     if(p[pixel[12]] > cb)
                      if(p[pixel[13]] > cb)
                       if(p[pixel[14]] > cb)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else if(p[pixel[11]] < c_b)
                if(p[pixel[12]] < c_b)
                 if(p[pixel[13]] < c_b)
                  if(p[pixel[14]] < c_b)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               if(p[pixel[6]] > cb)
                if(p[pixel[7]] > cb)
                 if(p[pixel[8]] > cb)
                  if(p[pixel[9]] > cb)
                   if(p[pixel[10]] > cb)
                    if(p[pixel[11]] > cb)
                     if(p[pixel[12]] > cb)
                      if(p[pixel[13]] > cb)
                       if(p[pixel[14]] > cb)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
             else if(p[pixel[5]] < c_b)
              if(p[pixel[6]] > cb)
               if(p[pixel[12]] > cb)
                if(p[pixel[7]] > cb)
                 if(p[pixel[8]] > cb)
                  if(p[pixel[9]] > cb)
                   if(p[pixel[10]] > cb)
                    if(p[pixel[11]] > cb)
                     if(p[pixel[13]] > cb)
                      if(p[pixel[14]] > cb)
                       if(p[pixel[15]] > cb)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else if(p[pixel[12]] < c_b)
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else if(p[pixel[6]] < c_b)
               if(p[pixel[7]] < c_b)
                if(p[pixel[8]] < c_b)
                 if(p[pixel[9]] < c_b)
                  {}
                 else
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   continue;
                else
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   continue;
                 else
                  continue;
               else
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               if(p[pixel[12]] < c_b)
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
             else
              if(p[pixel[11]] > cb)
               if(p[pixel[6]] > cb)
                if(p[pixel[7]] > cb)
                 if(p[pixel[8]] > cb)
                  if(p[pixel[9]] > cb)
                   if(p[pixel[10]] > cb)
                    if(p[pixel[12]] > cb)
                     if(p[pixel[13]] > cb)
                      if(p[pixel[14]] > cb)
                       if(p[pixel[15]] > cb)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else if(p[pixel[11]] < c_b)
               if(p[pixel[12]] < c_b)
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
            else
             if(p[pixel[10]] > cb)
              if(p[pixel[6]] > cb)
               if(p[pixel[7]] > cb)
                if(p[pixel[8]] > cb)
                 if(p[pixel[9]] > cb)
                  if(p[pixel[11]] > cb)
                   if(p[pixel[12]] > cb)
                    if(p[pixel[13]] > cb)
                     if(p[pixel[14]] > cb)
                      if(p[pixel[5]] > cb)
                       {}
                      else
                       if(p[pixel[15]] > cb)
                        {}
                       else
                        continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else if(p[pixel[10]] < c_b)
              if(p[pixel[11]] < c_b)
               if(p[pixel[12]] < c_b)
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      if(p[pixel[8]] < c_b)
                       if(p[pixel[9]] < c_b)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
           else
            if(p[pixel[9]] > cb)
             if(p[pixel[6]] > cb)
              if(p[pixel[7]] > cb)
               if(p[pixel[8]] > cb)
                if(p[pixel[10]] > cb)
                 if(p[pixel[11]] > cb)
                  if(p[pixel[12]] > cb)
                   if(p[pixel[13]] > cb)
                    if(p[pixel[5]] > cb)
                     if(p[pixel[4]] > cb)
                      {}
                     else
                      if(p[pixel[14]] > cb)
                       {}
                      else
                       continue;
                    else
                     if(p[pixel[14]] > cb)
                      if(p[pixel[15]] > cb)
                       {}
                      else
                       continue;
                     else
                      continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else if(p[pixel[9]] < c_b)
             if(p[pixel[10]] < c_b)
              if(p[pixel[11]] < c_b)
               if(p[pixel[12]] < c_b)
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      if(p[pixel[8]] < c_b)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      if(p[pixel[8]] < c_b)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
          else
           if(p[pixel[8]] > cb)
            if(p[pixel[6]] > cb)
             if(p[pixel[7]] > cb)
              if(p[pixel[9]] > cb)
               if(p[pixel[10]] > cb)
                if(p[pixel[11]] > cb)
                 if(p[pixel[12]] > cb)
                  if(p[pixel[5]] > cb)
                   if(p[pixel[4]] > cb)
                    if(p[pixel[3]] > cb)
                     {}
                    else
                     if(p[pixel[13]] > cb)
                      {}
                     else
                      continue;
                   else
                    if(p[pixel[13]] > cb)
                     if(p[pixel[14]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                  else
                   if(p[pixel[13]] > cb)
                    if(p[pixel[14]] > cb)
                     if(p[pixel[15]] > cb)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else if(p[pixel[8]] < c_b)
            if(p[pixel[9]] < c_b)
             if(p[pixel[10]] < c_b)
              if(p[pixel[11]] < c_b)
               if(p[pixel[12]] < c_b)
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[3]] < c_b)
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     if(p[pixel[7]] < c_b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
         else
          if(p[pixel[7]] > cb)
           if(p[pixel[6]] > cb)
            if(p[pixel[8]] > cb)
             if(p[pixel[9]] > cb)
              if(p[pixel[10]] > cb)
               if(p[pixel[11]] > cb)
                if(p[pixel[5]] > cb)
                 if(p[pixel[4]] > cb)
                  if(p[pixel[3]] > cb)
                   if(p[pixel[2]] > cb)
                    {}
                   else
                    if(p[pixel[12]] > cb)
                     {}
                    else
                     continue;
                  else
                   if(p[pixel[12]] > cb)
                    if(p[pixel[13]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[12]] > cb)
                   if(p[pixel[13]] > cb)
                    if(p[pixel[14]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[12]] > cb)
                  if(p[pixel[13]] > cb)
                   if(p[pixel[14]] > cb)
                    if(p[pixel[15]] > cb)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else if(p[pixel[7]] < c_b)
           if(p[pixel[8]] < c_b)
            if(p[pixel[9]] < c_b)
             if(p[pixel[10]] < c_b)
              if(p[pixel[11]] < c_b)
               if(p[pixel[12]] < c_b)
                if(p[pixel[13]] < c_b)
                 if(p[pixel[14]] < c_b)
                  if(p[pixel[15]] < c_b)
                   {}
                  else
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                 else
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[3]] < c_b)
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(p[pixel[2]] < c_b)
                 if(p[pixel[3]] < c_b)
                  if(p[pixel[4]] < c_b)
                   if(p[pixel[5]] < c_b)
                    if(p[pixel[6]] < c_b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else
           continue;
        else
         if(p[pixel[6]] > cb)
          if(p[pixel[7]] > cb)
           if(p[pixel[8]] > cb)
            if(p[pixel[9]] > cb)
             if(p[pixel[10]] > cb)
              if(p[pixel[5]] > cb)
               if(p[pixel[4]] > cb)
                if(p[pixel[3]] > cb)
                 if(p[pixel[2]] > cb)
                  if(p[pixel[1]] > cb)
                   {}
                  else
                   if(p[pixel[11]] > cb)
                    {}
                   else
                    continue;
                 else
                  if(p[pixel[11]] > cb)
                   if(p[pixel[12]] > cb)
                    {}
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[11]] > cb)
                  if(p[pixel[12]] > cb)
                   if(p[pixel[13]] > cb)
                    {}
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(p[pixel[11]] > cb)
                 if(p[pixel[12]] > cb)
                  if(p[pixel[13]] > cb)
                   if(p[pixel[14]] > cb)
                    {}
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               if(p[pixel[11]] > cb)
                if(p[pixel[12]] > cb)
                 if(p[pixel[13]] > cb)
                  if(p[pixel[14]] > cb)
                   if(p[pixel[15]] > cb)
                    {}
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else
           continue;
         else if(p[pixel[6]] < c_b)
          if(p[pixel[7]] < c_b)
           if(p[pixel[8]] < c_b)
            if(p[pixel[9]] < c_b)
             if(p[pixel[10]] < c_b)
              if(p[pixel[5]] < c_b)
               if(p[pixel[4]] < c_b)
                if(p[pixel[3]] < c_b)
                 if(p[pixel[2]] < c_b)
                  if(p[pixel[1]] < c_b)
                   {}
                  else
                   if(p[pixel[11]] < c_b)
                    {}
                   else
                    continue;
                 else
                  if(p[pixel[11]] < c_b)
                   if(p[pixel[12]] < c_b)
                    {}
                   else
                    continue;
                  else
                   continue;
                else
                 if(p[pixel[11]] < c_b)
                  if(p[pixel[12]] < c_b)
                   if(p[pixel[13]] < c_b)
                    {}
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(p[pixel[11]] < c_b)
                 if(p[pixel[12]] < c_b)
                  if(p[pixel[13]] < c_b)
                   if(p[pixel[14]] < c_b)
                    {}
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               if(p[pixel[11]] < c_b)
                if(p[pixel[12]] < c_b)
                 if(p[pixel[13]] < c_b)
                  if(p[pixel[14]] < c_b)
                   if(p[pixel[15]] < c_b)
                    {}
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
             else
              continue;
            else
             continue;
           else
            continue;
          else
           continue;
         else
          continue;

        double score = descriptor.distance(x, y, image);
        if(Descriptor::is_better(score,best.score)) {
            best.x = (float)x;
            best.y = (float)y;
            best.score = score;
        }
        }
    }
    return best;
}

#include "descriptor.h"
template xy fast_detector_10::track<DESCRIPTOR>(const DESCRIPTOR& descriptor, const tracker::image& image, float predx, float predy, float radius, int b);
