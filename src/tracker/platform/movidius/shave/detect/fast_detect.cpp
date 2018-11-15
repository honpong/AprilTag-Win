#include "fast9ScoreCv.h"
#include "swcCdma.h"
#include "svuCommonShave.h"
#include "fast_tracker.h"
#include "image_defines.h"
#include <algorithm>

constexpr int features_mutex_id = 14;

extern "C" __attribute__((dllexport))
void fast_detect(const u8 *image, int2 size, int2 stride_, int2 win, int2 win_size,
                 size_t need, fast_tracker::xy *features, size_t &features_size,
                 unsigned &threshold, u8 *mask, int mask_stride, int &workers) {
    int stride = stride_.x;
    assert(stride >= size.x && size.x <= MAX_WIDTH && size.y <= MAX_HEIGHT && need >= 0);

    int x1 = (win.x < PADDING) ? PADDING : win.x;
    int y1 = (win.y < PADDING) ? PADDING : win.y;
    int x2 = (win.x + win_size.x > size.x - PADDING) ? size.x - PADDING : win.x + win_size.x;
    int y2 = (win.y + win_size.y > size.y - PADDING) ? size.y - PADDING : win.y + win_size.y;

    int width = x2 - x1;
    int padded_width = x2 - x1 + 2 * PADDING;
    u8 *top_left = const_cast<u8*>(image) - 3 * stride + x1 - PADDING;
    __attribute__((aligned(16))) uint8_t line_buffer[8 * MAX_WIDTH];
    u32 dma_id = dmaInitRequester(1);
    {
        dmaTransactionList_t dma_rows;
        dmaCreateTransactionSrcStride(dma_id, &dma_rows, top_left + y1 * stride, line_buffer, (3+1+3) * padded_width, padded_width, stride);
        dmaStartListTask(&dma_rows);
        dmaWaitTask(&dma_rows);
    }

    for (int by = y1; by < y2; by += 8) {
        struct { int32_t size; uint8_t  data[MAX_WIDTH]; }  scores[8];
        struct { int32_t size; uint16_t data[MAX_WIDTH]; } offsets[8];
        for (int y = by, dy = 0; y < by+8 && y < y2; y++, dy++) {
            dmaTransactionList_t dma_next_row;
            dmaStartListTask(dmaCreateTransaction(dma_id, &dma_next_row, top_left + (y + 3+1+3) * stride, line_buffer + (y-y1 + 3+1+3) % 8 * padded_width, padded_width));
            u8 *lines[7];
            for (int i=0; i<7; i++)
                lines[i] = line_buffer + (y-y1 + i) % 8 * padded_width + PADDING;

            uint32_t work[((4 + 16) * MAX_WIDTH + 8)/sizeof(uint32_t)]; // struct work { u32 valid_indices[width]; uint8_t _[8]; score[width][16]; } work;
            mvcvfast9ScoreCv_asm(lines, (u8*)&scores[dy], (u16*)&offsets[dy], threshold, width, (void*)work);

            dmaWaitTask(&dma_next_row);
        }

        int j[8] = {};
        for(int bx = x1; bx < x2; bx += 8) {
            if (!mask[by/8 * mask_stride + bx/8]) {
                for (int dy = 0; dy < 8; dy++)
                    while (j[dy] < offsets[dy].size && x1+offsets[dy].data[j[dy]] < bx+8)
                        j[dy]++;
            } else {
                fast_tracker::xy best = {-1, -1, 0, 0};
                for (int y = by, dy = 0; y < by+8 && y < y2; y++, dy++)
                    for (int x,s; j[dy] < offsets[dy].size && (x = x1 + offsets[dy].data[j[dy]]) < bx+8; j[dy]++)
                        if ((s = scores[dy].data[j[dy]]) >= best.score)
                            best = { (float)x, (float)y, (float)s, 0 };
                if (best.score > 0) {
                    scMutexRequest(features_mutex_id);

                    features[features_size++] = best;
                    std::push_heap(features, features+features_size, fast_tracker::xy_comp);
                    if(features_size > need) {
                        std::pop_heap(features, features+features_size--, fast_tracker::xy_comp);
                        threshold = (unsigned)(features[0].score + 1);
                    }

                    scMutexRelease(features_mutex_id);
                }
            }
        }
    }
    scMutexRequest(features_mutex_id);
    bool last = --workers == 0;
    scMutexRelease(features_mutex_id);
    if (last)
        std::sort_heap(features, features+features_size, fast_tracker::xy_comp);
}
