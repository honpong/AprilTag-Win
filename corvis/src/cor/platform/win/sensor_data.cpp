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

static void cleanup_handle(void *h)
{
    auto handle = (handle_type *)h;
    if (handle)
    {
        auto res = handle->first->ReleaseAccess(&handle->second);
        handle->first->Release();
    }
    delete handle;
}

camera_data::camera_data(PXCImage *h) : image_handle(new handle_type, cleanup_handle)
{
    auto handle = (handle_type *)image_handle.get();
    handle->first = (PXCImage *)h;
    auto result = handle->first->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_Y8, &handle->second);
    if (result != PXC_STATUS_NO_ERROR || !handle->second.planes[0]) throw std::runtime_error("PXCImage->AcquireAccess failed!"); 
    image = handle->second.planes[0];
    stride = handle->second.pitches[0];
    auto info = handle->first->QueryInfo();
    width = info.width;
    height = info.height;
    //Subtract off 6370 (blank interval) and half of frame time. Todo: get actual exposure time
    timestamp = sensor_clock::time_point(sensor_clock::duration(handle->first->QueryTimeStamp() - 6370 - 166670));
    handle->first->AddRef();
}

