//
//  sensor_data.mm
//  RC3DK
//
//  Created by Eagle Jones on 3/17/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "RCSensorData.h"

#import <ImageIO/ImageIO.h>

static sensor_clock::time_point time_point_from_CMTime(const CMTime &time)
{
    uint64_t time_us;
    if(time.timescale == 1000000) time_us = time.value;
    else time_us = (uint64_t)(time.value / (time.timescale / 1000000.));

    return sensor_clock::time_point(std::chrono::microseconds(time_us));
}

static sensor_clock::time_point time_point_fromNSTimeInterval(const NSTimeInterval &time)
{
    uint64_t time_us = (uint64_t)(time * 1000000);
    return sensor_clock::time_point(std::chrono::microseconds(time_us));
}

#include <iostream>

static void cleanupSampleBuffer(void *h)
{
    CMSampleBufferRef sampleBuffer = (CMSampleBufferRef)h;
    if(sampleBuffer)
    {
        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
        CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
        CVPixelBufferRelease(pixelBuffer);
    }
    CFRelease(h);
}

camera_data camera_data_from_CMSampleBufferRef(CMSampleBufferRef sampleBuffer)
{
    camera_data d;
    d.image_handle = std::unique_ptr<void, void(*)(void *)>((void *)CFRetain(sampleBuffer), cleanupSampleBuffer);
    if(!sampleBuffer) throw std::runtime_error("Null sample buffer");
    CMTime time = (CMTime)CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    if(!pixelBuffer) throw std::runtime_error("Null image buffer");
    pixelBuffer = (CVPixelBufferRef)CVPixelBufferRetain(pixelBuffer);
    
    d.width = (int)CVPixelBufferGetWidth(pixelBuffer);
    d.height = (int)CVPixelBufferGetHeight(pixelBuffer);
    CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
    
    if(CVPixelBufferIsPlanar(pixelBuffer))
    {
        d.stride = (int)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
        d.image = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
    }
    else
    {
        d.stride = (int)CVPixelBufferGetBytesPerRow(pixelBuffer);
        d.image = (unsigned char *)CVPixelBufferGetBaseAddress(pixelBuffer);
    }
    
    d.timestamp = time_point_from_CMTime(time);
    CFDictionaryRef metadataDict = (CFDictionaryRef)CMGetAttachment(sampleBuffer, kCGImagePropertyExifDictionary , NULL);
    float exposure = [(NSString *)CFDictionaryGetValue(metadataDict, kCGImagePropertyExifExposureTime) floatValue];
    auto duration = std::chrono::duration<float>(exposure);
    d.exposure_time = std::chrono::duration_cast<sensor_clock::duration>(duration);
    return std::move(d);
}

accelerometer_data accelerometer_data_from_CMAccelerometerData(CMAccelerometerData *accelerationData)
{
    accelerometer_data d;
    d.timestamp = time_point_fromNSTimeInterval(accelerationData.timestamp);
    //ios gives acceleration in g-units, so multiply by standard gravity in m/s^2
    //it appears that accelerometer axes are flipped
    d.accel_m__s2[0] = (float)(-accelerationData.acceleration.x * 9.80665);
    d.accel_m__s2[1] = (float)(-accelerationData.acceleration.y * 9.80665);
    d.accel_m__s2[2] = (float)(-accelerationData.acceleration.z * 9.80665);
    return d;
}

gyro_data gyro_data_from_CMGyroData(CMGyroData *gyroData)
{
    gyro_data d;
    d.timestamp = time_point_fromNSTimeInterval(gyroData.timestamp);
    d.angvel_rad__s[0] = (float)gyroData.rotationRate.x;
    d.angvel_rad__s[1] = (float)gyroData.rotationRate.y;
    d.angvel_rad__s[2] = (float)gyroData.rotationRate.z;
    return d;
}

