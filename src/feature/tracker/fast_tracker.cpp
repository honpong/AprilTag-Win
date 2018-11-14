#include "fast_tracker.h"
#include "fast_constants.h"
#include "descriptor.h"
#include "fast_9.h"

#include <algorithm>

using namespace std;

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

void fast_tracker::init(const int x, const int y, const int s, const int ps)
{
    xsize = x;
    ysize = y;
    stride = s;
    patch_stride = ps;
    make_offsets(pixel, stride);
}

void fast_tracker::init_mask(const image &image, const std::vector<feature_position> &current)
{
    if (!mask)
        mask = std::make_unique<scaled_mask>(image.width_px, image.height_px);
    mask->initialize();
    for (auto &f : current)
        mask->clear((int)f.x, (int)f.y);
}

vector<tracker::feature_track> &fast_tracker::detect(const image &image, const std::vector<feature_position> &current, size_t number_desired)
{
    init_mask(image, current);

    init(image.width_px, image.height_px, image.stride_px, full_patch_width);

    size_t need = std::min(200.f, number_desired * 3.2f);
    features.clear();
    features.reserve(need+1);
    int x, y, mx, my, x1, y1, x2, y2;
    
    int bstart = fast_detect_threshold;
    x1 = 8;
    y1 = 8;
    x2 = image.width_px - 8;
    y2 = image.height_px - 8;
    
    for(my = y1; my < y2; my+=8) {
        for(mx = x1; mx < x2; mx+=8) {
            if(mask && !mask->test(mx, my)) continue;
            int bestx = -1, besty = -1;
            int save_start = bstart;
            bool found = false;
            for(y = my; y < my+8; ++y) {
                //This makes this a bit faster on x86, but leaving out for now as it's untested on other architectures and could slow us down:
                //if(mx % 64 == 0) _mm_prefetch((const char *)(im + (y+8)*stride + mx), 0);
                for(x = mx; x< mx+8; ++x) {
                    const uint8_t* p = image.image + y*stride + x;
                    uint8_t val = (uint8_t)(((uint16_t)p[0] + (((uint16_t)p[-stride] + (uint16_t)p[stride] + (uint16_t)p[-1] + (uint16_t)p[1]) >> 2)) >> 1);
                    
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
                            bestx = x;
                            besty = y;
                            bstart = bmin;
                            found = true;
                            break;
                        }
                        b = (bmin + bmax) / 2;
                    }
                }
            }
            if(found) {
                features.push_back({(float)bestx, (float)besty, (float)bstart, 0});
                push_heap(features.begin(), features.end(), xy_comp);
                if(features.size() > need) {
                    pop_heap(features.begin(), features.end(), xy_comp);
                    features.pop_back();
                    bstart = (int)(features[0].score + 1);
                } else bstart = save_start;
            }
        }
    }
    sort_heap(features.begin(), features.end(), xy_comp);
    return finalize_detect(features.data(), features.data() + features.size(), image, number_desired);
}

vector<tracker::feature_track> &fast_tracker::finalize_detect(xy *features_begin, xy *features_end, const image &image, size_t number_desired) {
    return non_maximum_suppression(features_begin, features_end, image, number_desired);
}

vector<tracker::feature_track> &fast_tracker::non_maximum_suppression(xy *features_begin, xy *features_end, const image &image, size_t number_desired)
{
    feature_points.clear();
    feature_points.reserve(number_desired);

    for (; features_begin != features_end; ++features_begin) {
        auto &d = *features_begin;
        if(!is_trackable<DESCRIPTOR::border_size>((int)d.x, (int)d.y, image.width_px, image.height_px) || !mask->test((int)d.x, (int)d.y))
            continue;
        mask->clear((int)d.x, (int)d.y);
        feature_points.emplace_back(make_shared<fast_feature<DESCRIPTOR>>(d.x, d.y, image), d.x, d.y, d.score);
        if (feature_points.size() == number_desired)
            break;
    }
    return feature_points;
}

void fast_tracker::track(const image &image, vector<feature_track *> &tracks)
{
    init(image.width_px, image.height_px, image.stride_px, full_patch_width);

    for(auto &tp : tracks) {
        auto &t = *tp;
        fast_feature<DESCRIPTOR> &f = *static_cast<fast_feature<DESCRIPTOR>*>(t.feature.get());
        xy preds[2];
        int pred_count = 0;
        if(t.x      != INFINITY) preds[pred_count++] = {t.x+t.dx, t.y+t.dy, 0, 0};
        if(t.pred_x != INFINITY) preds[pred_count++] = {t.pred_x, t.pred_y, 0, 0};

        xy bestkp {INFINITY, INFINITY, DESCRIPTOR::max_track_distance, 0};
        for(int i = 0; i < pred_count; ++i) {
            int x, y;
            int x1 = (int)ceilf(preds[i].x - fast_track_radius);
            int x2 = (int)floorf(preds[i].x + fast_track_radius);
            int y1 = (int)ceilf(preds[i].y - fast_track_radius);
            int y2 = (int)floorf(preds[i].y + fast_track_radius);
            
            int half = DESCRIPTOR::border_size;
            
            if(x1 >= half && x2 < xsize - half && y1 >= half && y2 < ysize - half) {
                for(y = y1; y <= y2; y++) {
                    for(x = x1; x <= x2; x++) {
                        const uint8_t* p = image.image + y*stride + x;
                        uint8_t val = (uint8_t)(((uint16_t)p[0] + (((uint16_t)p[-stride] + (uint16_t)p[stride] + (uint16_t)p[-1] + (uint16_t)p[1]) >> 2)) >> 1);
                        if(fast_9_kernel(p, pixel, val, fast_track_threshold)) {
                            auto score = f.descriptor.distance_track((float)x, (float)y, image);
                            if(score < bestkp.score) {
                                bestkp.x = (float)x;
                                bestkp.y = (float)y;
                                bestkp.score = score;
                            }
                        }
                    }
                }
            }
            if(bestkp.score < DESCRIPTOR::good_track_distance) break;
        }

        if(bestkp.x != INFINITY && t.x != INFINITY) {
            t.dx = bestkp.x - t.x;
            t.dy = bestkp.y - t.y;
        } else {
            t.dx = 0;
            t.dy = 0;
        }
        t.x = bestkp.x;
        t.y = bestkp.y;
        t.score = bestkp.score;
    }
}
