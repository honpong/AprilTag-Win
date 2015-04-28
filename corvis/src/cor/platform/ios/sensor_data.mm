//
//  sensor_data.mm
//  RC3DK
//
//  Created by Eagle Jones on 3/17/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "sensor_data.h"
#import <CoreMedia/CoreMedia.h>
#import <CoreMotion/CoreMotion.h>
#include <stdexcept>

static sensor_clock::time_point time_point_from_CMTime(const CMTime &time)
{
    uint64_t time_ns;
    if(time.timescale == 1000000000) time_ns = time.value;
    else time_ns = time.value / (time.timescale / 1000000000.);

    return sensor_clock::time_point(std::chrono::nanoseconds(time_ns));
}

static sensor_clock::time_point time_point_fromNSTimeInterval(const NSTimeInterval &time)
{
    uint64_t time_ns = time * 1000000000;
    return sensor_clock::time_point(std::chrono::nanoseconds(time_ns));
}

#include <iostream>

camera_data::camera_data(void *h): image_handle((void *)CFRetain(h), [](void *h) {CFRelease(h);})
{
    auto sampleBuffer = (CMSampleBufferRef)image_handle.get();
    if(!sampleBuffer) throw std::runtime_error("Null sample buffer");
    CMTime time = (CMTime)CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    if(!pixelBuffer) throw std::runtime_error("Null image buffer");
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
    
    //TODO: when we properly handle rolling shutter, propagate timestamps into camera_data class and timestamp at beginning of frame (pull exif metadata from RCSensorFusion into here)
    timestamp = time_point_from_CMTime(time);
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

accelerometer_data::accelerometer_data(void *handle)
{
    auto accelerationData = (__bridge CMAccelerometerData *)handle;
    timestamp = time_point_fromNSTimeInterval(accelerationData.timestamp);
    //ios gives acceleration in g-units, so multiply by standard gravity in m/s^2
    //it appears that accelerometer axes are flipped
    accel_m__s2[0] = -accelerationData.acceleration.x * 9.80665;
    accel_m__s2[1] = -accelerationData.acceleration.y * 9.80665;
    accel_m__s2[2] = -accelerationData.acceleration.z * 9.80665;
}

gyro_data::gyro_data(void *handle)
{
    auto gyroData = (__bridge CMGyroData *)handle;
    timestamp = time_point_fromNSTimeInterval(gyroData.timestamp);
    angvel_rad__s[0] = gyroData.rotationRate.x;
    angvel_rad__s[1] = gyroData.rotationRate.y;
    angvel_rad__s[2] = gyroData.rotationRate.z;
}

