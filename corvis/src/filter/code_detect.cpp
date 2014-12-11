//  code_detect.cpp
//
//  Created by Brian on 12/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//
//  Adapted from /cpp/cli/src/main.cpp

#include "code_detect.h"
#include <stdio.h>

#include <zxing/common/Counted.h>
#include <zxing/Binarizer.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/Result.h>
#include <zxing/ReaderException.h>
//#include <zxing/common/GlobalHistogramBinarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/common/GreyscaleLuminanceSource.h>
#include <exception>
#include <zxing/Exception.h>
#include <zxing/common/IllegalArgumentException.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/DecodeHints.h>

#include <zxing/qrcode/QRCodeReader.h>
#include <zxing/multi/qrcode/QRCodeMultiReader.h>
#include <zxing/multi/ByQuadrantReader.h>
#include <zxing/multi/MultipleBarcodeReader.h>
#include <zxing/multi/GenericMultipleBarcodeReader.h>

using namespace std;
using namespace zxing;
using namespace zxing::multi;
using namespace zxing::qrcode;

vector<Ref<Result> > decode(Ref<BinaryBitmap> image, DecodeHints hints) {
    Ref<Reader> reader(new MultiFormatReader);
    return vector<Ref<Result> >(1, reader->decode(image, hints));
}

vector<Ref<Result> > decode_multi(Ref<BinaryBitmap> image, DecodeHints hints) {
    MultiFormatReader delegate;
    GenericMultipleBarcodeReader reader(delegate);
    return reader.decodeMultiple(image, hints);
}

vector<Ref<Result> > detect_qr(Ref<LuminanceSource> source, bool hybrid) {
    vector<Ref<Result> > results;
    string cell_result;
    int res = -1;
    bool search_multi = false;

    try {
        Ref<Binarizer> binarizer;
        binarizer = new HybridBinarizer(source);
        // binarizer = new GlobalHistogramBinarizer(source);
        DecodeHints hints(DecodeHints::DEFAULT_HINT);
        hints.setTryHarder(false);
        Ref<BinaryBitmap> binary(new BinaryBitmap(binarizer));
        if (search_multi) {
            results = decode_multi(binary, hints);
        } else {
            results = decode(binary, hints);
        }
        res = 0;
    } catch (const ReaderException& e) {
        cell_result = "zxing::ReaderException: " + string(e.what());
        res = -2;
    } catch (const zxing::IllegalArgumentException& e) {
        cell_result = "zxing::IllegalArgumentException: " + string(e.what());
        res = -3;
    } catch (const zxing::Exception& e) {
        cell_result = "zxing::Exception: " + string(e.what());
        res = -4;
    } catch (const std::exception& e) {
        cell_result = "std::exception: " + string(e.what());
        res = -5;
    }

    for (size_t i = 0; i < results.size(); i++) {
        cout << "  Format: " << BarcodeFormat::barcodeFormatNames[results[i]->getBarcodeFormat()];
        for (int j = 0; j < results[i]->getResultPoints()->size(); j++) {
            cout << "  Point[" << j <<  "]: "
            << results[i]->getResultPoints()[j]->getX() << " "
            << results[i]->getResultPoints()[j]->getY() << endl;
        }
        cout << results[i]->getText()->getText() << endl;
    }

    return results;
}

vector<struct qr_detection> code_detect_qr(const uint8_t * image, int width, int height)
{
    vector<struct qr_detection> results;
    fprintf(stderr, "starting detection");
    ArrayRef<char> image_ref = ArrayRef<char>((char *)image, width*height);
    Ref<GreyscaleLuminanceSource> source = Ref<GreyscaleLuminanceSource>(new GreyscaleLuminanceSource(GreyscaleLuminanceSource(image_ref, width, height, 0, 0, width, height)));
    vector<Ref<Result> > result = detect_qr(source, true);

    return results;
}