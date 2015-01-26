//  code_detect.cpp
//
//  Created by Brian on 12/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//
//  Adapted from /cpp/cli/src/main.cpp

#include "code_detect.h"

#include <zxing/Result.h>
#include <zxing/ReaderException.h>
#include <zxing/common/IllegalArgumentException.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/DecodeHints.h>
#include <zxing/qrcode/QRCodeReader.h>
#include <zxing/multi/qrcode/QRCodeMultiReader.h>
#include <zxing/common/HybridFastBinarizer.h>

using namespace std;
using namespace zxing;
using namespace zxing::multi;
using namespace zxing::qrcode;

vector<Ref<Result> > decode(Ref<BinaryBitmap> image, DecodeHints hints) {
    Ref<Reader> reader(new QRCodeReader);
    return vector<Ref<Result> >(1, reader->decode(image, hints));
}

vector<Ref<Result> > decode_multi(Ref<BinaryBitmap> image, DecodeHints hints) {
    QRCodeMultiReader reader;
    return reader.decodeMultiple(image, hints);
}

vector<Ref<Result> > detect_qr(const char * image, int width, int height) {
    vector<Ref<Result> > results;
    string cell_result;
    int res = -1;
    bool search_multi = false;

    try {
        Ref<Binarizer> binarizer;
        Ref<LuminanceSource> source(new DummyLuminanceSource(width, height));
        binarizer = new HybridFastBinarizer(image, width, height, source);
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

    /*
    for (size_t i = 0; i < results.size(); i++) {
        cout << "  Format: " << BarcodeFormat::barcodeFormatNames[results[i]->getBarcodeFormat()];
        for (int j = 0; j < results[i]->getResultPoints()->size(); j++) {
            cout << "  Point[" << j <<  "]: "
            << results[i]->getResultPoints()[j]->getX() << " "
            << results[i]->getResultPoints()[j]->getY() << endl;
        }
        cout << results[i]->getText()->getText() << endl;
    }
    */

    return results;
}

vector<struct qr_detection> code_detect_qr(const uint8_t * image, int width, int height)
{
    vector<struct qr_detection> results;

    vector<Ref<Result> > result = detect_qr((const char *)image, width, height);
    for(int i = 0; i < result.size(); i++) {
        struct qr_detection d;
        ArrayRef<Ref<zxing::ResultPoint> > res = result[i]->getResultPoints();
        if(res->size() == 4) {
            d.lower_left.x = res[0]->getX();
            d.lower_left.y = res[0]->getY();
            d.upper_left.x = res[1]->getX();
            d.upper_left.y = res[1]->getY();
            d.upper_right.x = res[2]->getX();
            d.upper_right.y = res[2]->getY();
            d.lower_right.x = res[3]->getX();
            d.lower_right.y = res[3]->getY();
            Ref<zxing::String> data = result[i]->getText();
            strncpy(d.data, data->getText().c_str(), 1024);
            
            results.push_back(d);
        }
        else {
            fprintf(stderr, "Warning: qr detected, but with %d points\n", res->size());
        }
    }

    return results;
}
