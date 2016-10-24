#include <zxing/common/HybridFastBinarizer.h>

using namespace std;
using namespace zxing;

namespace {
    const int BLOCK_SIZE_POWER = 3;
    const int BLOCK_SIZE = 1 << BLOCK_SIZE_POWER; // ...0100...00
    const int BLOCK_SIZE_MASK = BLOCK_SIZE - 1;   // ...0011...11
    const int MINIMUM_DIMENSION = BLOCK_SIZE * 5;
}

HybridFastBinarizer::HybridFastBinarizer(const char * data, int width, int height, Ref<LuminanceSource> source) :
GlobalHistogramBinarizer(source), L(data), width_(width), height_(height), matrix_(NULL), cached_row_(NULL) {
}

HybridFastBinarizer::~HybridFastBinarizer() {
}

/**
 * Calculates the final BitMatrix once for all requests. This could be called once from the
 * constructor instead, but there are some advantages to doing it lazily, such as making
 * profiling easier, and not doing heavy lifting when callers don't expect it.
 */
Ref<BitMatrix> HybridFastBinarizer::getBlackMatrix() {
    if (matrix_) {
        return matrix_;
    }
    int width = width_;
    int height = height_;
    if (width >= MINIMUM_DIMENSION && height >= MINIMUM_DIMENSION) {
        const char * luminances = L;
        int subWidth = width >> BLOCK_SIZE_POWER;
        if ((width & BLOCK_SIZE_MASK) != 0) {
            subWidth++;
        }
        int subHeight = height >> BLOCK_SIZE_POWER;
        if ((height & BLOCK_SIZE_MASK) != 0) {
            subHeight++;
        }
        ArrayRef<int> blackPoints =
        calculateBlackPoints(luminances, subWidth, subHeight, width, height);

        Ref<BitMatrix> newMatrix (new BitMatrix(width, height));
        calculateThresholdForBlock(luminances,
                                   subWidth,
                                   subHeight,
                                   width,
                                   height,
                                   blackPoints,
                                   newMatrix);
        matrix_ = newMatrix;
    } else {
        // If the image is too small, fall back to the global histogram approach.
        throw IllegalArgumentException("Image is too small for hybrid approach");
    }
    return matrix_;
}

namespace {
    inline int cap(int value, int min, int max) {
        return value < min ? min : value > max ? max : value;
    }
}

void
HybridFastBinarizer::calculateThresholdForBlock(const char * luminances,
                                            int subWidth,
                                            int subHeight,
                                            int width,
                                            int height,
                                            ArrayRef<int> blackPoints,
                                            Ref<BitMatrix> const& matrix) {
    for (int y = 0; y < subHeight; y++) {
        int yoffset = y << BLOCK_SIZE_POWER;
        int maxYOffset = height - BLOCK_SIZE;
        if (yoffset > maxYOffset) {
            yoffset = maxYOffset;
        }
        for (int x = 0; x < subWidth; x++) {
            int xoffset = x << BLOCK_SIZE_POWER;
            int maxXOffset = width - BLOCK_SIZE;
            if (xoffset > maxXOffset) {
                xoffset = maxXOffset;
            }
            int left = cap(x, 2, subWidth - 3);
            int top = cap(y, 2, subHeight - 3);
            int sum = 0;
            for (int z = -2; z <= 2; z++) {
                int *blackRow = &blackPoints[(top + z) * subWidth];
                sum += blackRow[left - 2];
                sum += blackRow[left - 1];
                sum += blackRow[left];
                sum += blackRow[left + 1];
                sum += blackRow[left + 2];
            }
            int average = sum / 25;
            thresholdBlock(luminances, xoffset, yoffset, average, width, matrix);
        }
    }
}

void HybridFastBinarizer::thresholdBlock(const char * luminances,
                                     int xoffset,
                                     int yoffset,
                                     int threshold,
                                     int stride,
                                     Ref<BitMatrix> const& matrix) {
    for (int y = 0, offset = yoffset * stride + xoffset;
         y < BLOCK_SIZE;
         y++,  offset += stride) {
        for (int x = 0; x < BLOCK_SIZE; x++) {
            int pixel = luminances[offset + x] & 0xff;
            if (pixel <= threshold) {
                matrix->set(xoffset + x, yoffset + y);
            }
        }
    }
}

namespace {
    inline int getBlackPointFromNeighbors(ArrayRef<int> blackPoints, int subWidth, int x, int y) {
        return (blackPoints[(y-1)*subWidth+x] +
                2*blackPoints[y*subWidth+x-1] +
                blackPoints[(y-1)*subWidth+x-1]) >> 2;
    }
}


ArrayRef<int> HybridFastBinarizer::calculateBlackPoints(const char * luminances,
                                                    int subWidth,
                                                    int subHeight,
                                                    int width,
                                                    int height) {
    const int minDynamicRange = 24;

    ArrayRef<int> blackPoints (subHeight * subWidth);
    for (int y = 0; y < subHeight; y++) {
        int yoffset = y << BLOCK_SIZE_POWER;
        int maxYOffset = height - BLOCK_SIZE;
        if (yoffset > maxYOffset) {
            yoffset = maxYOffset;
        }
        for (int x = 0; x < subWidth; x++) {
            int xoffset = x << BLOCK_SIZE_POWER;
            int maxXOffset = width - BLOCK_SIZE;
            if (xoffset > maxXOffset) {
                xoffset = maxXOffset;
            }
            int sum = 0;
            int min = 0xFF;
            int max = 0;
            for (int yy = 0, offset = yoffset * width + xoffset;
                 yy < BLOCK_SIZE;
                 yy++, offset += width) {
                for (int xx = 0; xx < BLOCK_SIZE; xx++) {
                    int pixel = luminances[offset + xx] & 0xFF;
                    sum += pixel;
                    // still looking for good contrast
                    if (pixel < min) {
                        min = pixel;
                    }
                    if (pixel > max) {
                        max = pixel;
                    }
                }

                // short-circuit min/max tests once dynamic range is met
                if (max - min > minDynamicRange) {
                    // finish the rest of the rows quickly
                    for (yy++, offset += width; yy < BLOCK_SIZE; yy++, offset += width) {
                        for (int xx = 0; xx < BLOCK_SIZE; xx += 2) {
                            sum += luminances[offset + xx] & 0xFF;
                            sum += luminances[offset + xx + 1] & 0xFF;
                        }
                    }
                }
            }
            // See
            // http://groups.google.com/group/zxing/browse_thread/thread/d06efa2c35a7ddc0
            int average = sum >> (BLOCK_SIZE_POWER * 2);
            if (max - min <= minDynamicRange) {
                average = min >> 1;
                if (y > 0 && x > 0) {
                    int bp = getBlackPointFromNeighbors(blackPoints, subWidth, x, y);
                    if (min < bp) {
                        average = bp;
                    }
                }
            }
            blackPoints[y * subWidth + x] = average;
        }
    }
    return blackPoints;
}
