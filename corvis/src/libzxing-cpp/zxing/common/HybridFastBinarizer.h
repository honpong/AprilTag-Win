// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
#ifndef __HYBRIDFASTBINARIZER_H__
#define __HYBRIDFASTBINARIZER_H__
/*
 *  HybridFastBinarizer.h
 *  zxing
 *
 *  Created 2014 by Brian Fulkerson
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vector>
#include <zxing/Binarizer.h>
#include <zxing/LuminanceSource.h>
#include <zxing/common/GlobalHistogramBinarizer.h>
#include <zxing/common/BitArray.h>
#include <zxing/common/BitMatrix.h>

namespace zxing {

    class DummyLuminanceSource : public LuminanceSource {
    public:

        DummyLuminanceSource(int width, int height) : LuminanceSource(width, height) {}

        ArrayRef<char> getRow(int y, ArrayRef<char> row) const {
            throw IllegalArgumentException("getRow not supported.");
        }

        ArrayRef<char> getMatrix() const {
            throw IllegalArgumentException("getMatrix not supported");
        }
    };

    class HybridFastBinarizer : public GlobalHistogramBinarizer {
    private:
        const char * L;
        int width_, height_;
        Ref<BitMatrix> matrix_;
        Ref<BitArray> cached_row_;

    public:
        HybridFastBinarizer(const char * data, int width, int height, Ref<zxing::LuminanceSource> source);
        virtual ~HybridFastBinarizer();

        virtual Ref<BitMatrix> getBlackMatrix();

    private:
        // We'll be using one-D arrays because C++ can't dynamically allocate 2D
        // arrays
        ArrayRef<int> calculateBlackPoints(const char * luminances,
                                           int subWidth,
                                           int subHeight,
                                           int width,
                                           int height);
        void calculateThresholdForBlock(const char * luminances,
                                        int subWidth,
                                        int subHeight,
                                        int width,
                                        int height,
                                        ArrayRef<int> blackPoints,
                                        Ref<BitMatrix> const& matrix);
        void thresholdBlock(const char * luminances,
                            int xoffset,
                            int yoffset,
                            int threshold,
                            int stride,
                            Ref<BitMatrix> const& matrix);
    };
}

#endif
