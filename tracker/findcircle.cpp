// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include <iostream>
#include <opencv2/calib3d/calib3d.hpp>

extern "C" int findCircles(IplImage *img, CvSize size, CvPoint2D32f *features);

extern "C" int findCircles(IplImage *img, CvSize size, CvPoint2D32f *features)
{
    std::vector <cv::Point2f> pointBuf;
    int res = cv::findCirclesGrid(cv::Mat(img), size, pointBuf, cv::CALIB_CB_ASYMMETRIC_GRID);
    if(res) {
        for(int i = 0; i < size.width * size.height; ++i) {
            features[i] = pointBuf[i];
        }
    }
    return res;
}
