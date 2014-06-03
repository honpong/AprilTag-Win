//
//  RCStereo.m
//  RCCore
//
//  Created by Eagle Jones on 5/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCStereo.h"
#include "vec4.h"
#include "filter.h"
#include "stereo.h"

@implementation RCStereo
{
    stereo mystereo;
}

+ (id) sharedInstance
{
    static RCStereo *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (id) init
{
    self = [super init];
    
    if (self)
    {
        LOGME
    }
    
    return self;
}

- (void) processFrame:(RCSensorFusionData *)data withFinal:(bool)final
{
    struct stereo_global s;
    
    CMSampleBufferRef sampleBuffer = data.sampleBuffer;
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
    unsigned char *pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
    
    s.width = CVPixelBufferGetWidth(pixelBuffer);
    s.height = CVPixelBufferGetHeight(pixelBuffer);
    s.T = data.cameraTransformation.translation.vector;
    s.W = rotation_vector(data.cameraTransformation.rotation.x, data.cameraTransformation.rotation.y, data.cameraTransformation.rotation.z);
    RCTransformation *camCal = [[data.transformation getInverse] composeWithTransformation:data.cameraTransformation];
    s.focal_length = data.cameraParameters.focalLength;
    s.center_x = data.cameraParameters.opticalCenterX;
    s.center_y = data.cameraParameters.opticalCenterY;
    s.k1 = data.cameraParameters.radialSecondDegree;
    s.k2 = data.cameraParameters.radialFourthDegree;
    s.k3 = 0.;

    list<stereo_feature> features;
    for (RCFeaturePoint *feature in data.featurePoints) {
        features.push_back(stereo_feature(feature.id, v4(feature.x, feature.y, 0., 0.)));
    }
    mystereo.process_frame(s, pixel, features, final);
    
    CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
}

- (RCFeaturePoint *) triangulatePoint:(CGPoint)point
{
    v4 world;
    bool success = mystereo.triangulate(point.x, point.y, world);
    if(!success)
        return NULL;
    
    // TODO: The feature is initialized with a completely invalid OriginalDepth
    // but this is not used in drawing or calculating the distance
    RCFeaturePoint* feature = [[RCFeaturePoint alloc]
                               initWithId:0
                               withX:point.x
                               withY:point.y
                               withOriginalDepth:[
                                                  [RCScalar alloc]
                                                  initWithScalar:1
                                                  withStdDev:100]
                               withWorldPoint:[
                                               [RCPoint alloc]
                                               initWithX:world[0]
                                               withY:world[1]
                                               withZ:world[2]]
                               withInitialized:YES
                               ];
    return feature;
}

// Preprocesses the stereo data with the current frame
- (bool) preprocess
{
    return mystereo.preprocess();
}

-(void) reset
{
    mystereo.reset();
}

@end
