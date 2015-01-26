//  code_detect.h
//
//  Created by Brian on 12/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef __CODE_DETECT_H__
#define __CODE_DETECT_H__

#include "../cor/cor_types.h"

#include <vector>

using namespace std;

struct qr_detection {
    feature_t upper_left;
    feature_t upper_right;
    feature_t lower_right;
    feature_t lower_left;
    int modules;
    char data[1024];
};

vector<struct qr_detection> code_detect_qr(const uint8_t * image, int width, int height);

#endif
