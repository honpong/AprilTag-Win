#include "fast_9.h"
#include "fast.h"
#include <math.h>
#include <stdlib.h>
#include <algorithm>
using namespace std;

static bool xy_comp(const xy &first, const xy &second)
{
    return first.score > second.score;
}

vector<xy> &fast_detector_9::detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight)
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
                if(fast_9_kernel(p, pixel, val, b))
                {
                    bmin=b;
                } else {
                    if(b == bstart) break;
                    bmax=b;
                }
                
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

void fast_detector_9::init(const int x, const int y, const int s, const int ps, const int phw)
{
    xsize = x;
    ysize = y;
    stride = s;
    patch_stride = ps;
    patch_win_half_width = phw;
    make_offsets(pixel, stride);
}

template<typename Descriptor>
xy fast_detector_9::track(const Descriptor& descriptor, const tracker::image& image, float predx, float predy, float radius, int b)
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
            if(fast_9_kernel(p, pixel, val, b)) {
                auto score = descriptor.distance(x, y, image);
                if(Descriptor::is_better(score,best.score)) {
                    best.x = (float)x;
                    best.y = (float)y;
                    best.score = score;
                }
            }
        }
    }
    return best;
}

#include "descriptor.h"
template xy fast_detector_9::track<DESCRIPTOR>(const DESCRIPTOR& descriptor, const tracker::image& image, float predx, float predy, float radius, int b);
