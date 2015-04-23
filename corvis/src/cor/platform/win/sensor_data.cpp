//
//  sensor_data.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/17/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "../sensor_data.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"
#include <stdexcept>
#include <iostream>

typedef std::pair<PXCImage *, PXCImage::ImageData> handle_type;

camera_data::camera_data(void *h) : image_handle(h, [](void *h) {})
{
    PXCImage *pxci = (PXCImage *)h;
    PXCImage::ImageData data;
    auto result = pxci->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_Y8, &data);
    if(result != PXC_STATUS_NO_ERROR || !data.planes[0]) throw std::runtime_error("PXCImage->AcquireAccess failed!");
    image_handle.reset(new handle_type(pxci, data)); //TODO avoid allocation here?
 
    image = data.planes[0];
    stride = data.pitches[0];
    auto info = pxci->QueryInfo();
    width = info.width;
    height = info.height;
    auto time = pxci->QueryTimeStamp();
    std::cerr << "camera timestamp: " << time << "\n";
    //TODO: timestamp
    timestamp = sensor_clock::now();
}

camera_data::~camera_data()
{
    auto h = (handle_type *)image_handle.get();
    if (h)
    {
        h->first->ReleaseAccess(&(h->second));
        delete h;
    }
}
