//
//  RCStereo.m
//  RCCore
//
//  Created by Eagle Jones on 5/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCStereo.h"

#import <CoreImage/CoreImage.h>
#import <UIKit/UIKit.h>

#include "vec4.h"
#include "filter.h"
#include "stereo.h"

@implementation RCStereo
{
    stereo mystereo;
    NSString * texture_path;
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
        [self resetBasename];
    }
    
    return self;
}

- (NSString *) timeStampedFilenameWithSuffix:(NSString *)suffix
{
    NSURL * documentURL = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
    
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd_HH-mm-ss"];
    
    NSDate *date = [NSDate date];
    NSString * formattedDateString = [dateFormatter stringFromDate:date];
    NSMutableString * filename = [[NSMutableString alloc] initWithString:formattedDateString];
    [filename appendString:suffix];
    NSURL *fileUrl = [documentURL URLByAppendingPathComponent:filename];
    
    return [fileUrl path];
}

- (void) writeSampleBuffer:(CMSampleBufferRef)sampleBuffer withFilename:(NSString *)filename
{
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    CVPixelBufferLockBaseAddress(imageBuffer, 0);
    CIImage * image = [CIImage imageWithCVPixelBuffer:imageBuffer];
    CIContext *context = [CIContext contextWithOptions:nil];
    
    UIImage * lastImage = [UIImage imageWithCGImage:[context createCGImage:image fromRect:image.extent]];
    
    DLog(@"Writing to %@", filename);
    
    if(![UIImageJPEGRepresentation(lastImage, .8) writeToFile:filename atomically:YES])
    {
        NSLog(@"FAILED");
    }
    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
}

- (void) processFrame:(RCSensorFusionData *)data withFinal:(bool)final
{
    struct stereo_global s;
    
    CMSampleBufferRef sampleBuffer = data.sampleBuffer;
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
    unsigned char *pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
    
    s.width = (int)CVPixelBufferGetWidth(pixelBuffer);
    s.height = (int)CVPixelBufferGetHeight(pixelBuffer);
    s.T = data.cameraTransformation.translation.vector;
    s.W = rotation_vector(data.cameraTransformation.rotation.x, data.cameraTransformation.rotation.y, data.cameraTransformation.rotation.z);
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
    
    if(final) {
        [self writeSampleBuffer:data.sampleBuffer withFilename:texture_path];
    }
    
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


- (RCFeaturePoint *) triangulatePointWithMesh:(CGPoint)point
{
    v4 world;
    bool success = mystereo.triangulate_mesh(point.x, point.y, world);
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

static void sensor_fusion_stereo_progress(float progress)
{
    static RCStereo * stereo;
    if(!stereo)
        stereo = [RCStereo sharedInstance];

    if(stereo.delegate && [stereo.delegate respondsToSelector:@selector(stereoDidUpdateProgress:)]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [stereo.delegate stereoDidUpdateProgress:progress];
        });
    }
}

// Preprocesses the stereo data with the current frame
- (bool) preprocess
{
    bool success;
    success = mystereo.preprocess(false);
    if(success) {
        success = mystereo.preprocess_mesh(sensor_fusion_stereo_progress);
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        if(_delegate && [_delegate respondsToSelector:@selector(stereoDidFinish)])
            [_delegate stereoDidFinish];
    });
    return success;

}

- (void) resetBasename
{
    NSString * debug_basename = [self timeStampedFilenameWithSuffix:@"-stereo"];
    texture_path = [debug_basename stringByAppendingString:@".jpg"];
    NSString * texture_filename = [[[NSURL URLWithString:texture_path] pathComponents] lastObject];

    mystereo.set_debug_basename([debug_basename UTF8String]);
    mystereo.set_debug_texture_filename([texture_filename UTF8String]);
}

-(void) reset
{
    mystereo.reset();
    [self resetBasename];
}
@end
