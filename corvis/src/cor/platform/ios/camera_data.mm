//
//  camera_data.mm
//  RC3DK
//
//  Created by Eagle Jones on 3/17/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "camera_data.h"
#import <CoreMedia/CoreMedia.h>



camera_data::camera_data(): image_handle(nullptr, nullptr), image(nullptr), timestamp(0), width(0), height(0), stride(0)
{
}

camera_data::camera_data(void *h): image_handle((void *)CFRetain(h), [](void *h) {CFRelease(h);})
{
    auto sampleBuffer = (CMSampleBufferRef)image_handle.get();
    CMTime time = (CMTime)CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    
    //capture image meta data
    //        CFDictionaryRef metadataDict = CMGetAttachment(sampleBuffer, kCGImagePropertyExifDictionary , NULL);
    //        DLog(@"metadata: %@", metadataDict);
    
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    pixelBuffer = (CVPixelBufferRef)CVPixelBufferRetain(pixelBuffer);
    
    width = CVPixelBufferGetWidth(pixelBuffer);
    height = CVPixelBufferGetHeight(pixelBuffer);
    CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
    
    if(CVPixelBufferIsPlanar(pixelBuffer))
    {
        stride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
        image = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
    }
    else
    {
        stride = CVPixelBufferGetBytesPerRow(pixelBuffer);
        image = (unsigned char *)CVPixelBufferGetBaseAddress(pixelBuffer);
    }
    
    if(width != 640 || height != 480 || stride != 640) {
        NSLog(@"Image dimensions are incorrect! Make sure you're using the right video preset and not changing the orientation on the capture connection.\n");
        abort();
    }
    uint64_t time_us = time.value / (time.timescale / 1000000.);
    
    timestamp = time_us + 16667;
}

camera_data::~camera_data()
{
    CMSampleBufferRef sampleBuffer = (CMSampleBufferRef)image_handle.get();
    if(sampleBuffer)
    {
        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
        CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
        CVPixelBufferRelease(pixelBuffer);
    }
}
