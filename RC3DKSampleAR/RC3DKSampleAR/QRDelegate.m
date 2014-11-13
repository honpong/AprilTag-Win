//
//  QRDelegate.m
//  RC3DKSampleAR
//
//  Created by Brian on 11/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "QRDelegate.h"

@interface QRDetection : NSObject

@property NSString * code;
@property NSArray * corners;
@property uint64_t time;

@property RCTransformation* transformation;

@end

@implementation QRDetection

@end

@implementation QRDelegate
{
    NSMutableArray * savedTransformations;
    NSMutableArray * savedTimes;
    NSMutableArray * detections;
    RCCameraParameters * camera;
    size_t imageWidth, imageHeight;
}

-(id)init
{
    if (self = [super init])
    {
        savedTransformations = [NSMutableArray array];
        savedTimes = [NSMutableArray array];
        detections = [NSMutableArray array];
    }
    return self;
}

- (void) handleSyncedDetection:(QRDetection *)detection
{
    CGPoint topleft, bottomleft, bottomright, topright;
    CGPointMakeWithDictionaryRepresentation((CFDictionaryRef)[detection.corners objectAtIndex:0], &topleft);
    CGPointMakeWithDictionaryRepresentation((CFDictionaryRef)[detection.corners objectAtIndex:1], &bottomleft);
    CGPointMakeWithDictionaryRepresentation((CFDictionaryRef)[detection.corners objectAtIndex:2], &bottomright);
    CGPointMakeWithDictionaryRepresentation((CFDictionaryRef)[detection.corners objectAtIndex:3], &topright);
    
    NSLog(@"Code detected %@ at %llu, corners (%f, %f), (%f, %f), (%f, %f), (%f, %f)", detection.code, detection.time, topleft.x * imageWidth, topleft.y * imageHeight, bottomleft.x * imageWidth, bottomleft.y * imageHeight, bottomright.x * imageWidth, bottomright.y * imageHeight, topright.x * imageWidth, topright.y * imageHeight);
    
    
    /*
     What needs to be done. Derivation follows: http://vision.ucla.edu//MASKS/MASKS-ch5.pdf section 5.3
     
     X1 = 3D position of corner in QR-centered coordinates
     R,T = transformation bringing camera coordinates to QR-centered coordinates
     p = unknown projective scale factor (z2)
     X2 = (x2, y2, 1) - normalized image coordinates of projected point
     
     X1 = R p X2 + T

     N = normal to plane where the 4 features lie (in camera coordinates)
     d = distance from camera to the plane (not to the point)
     N^t p X2 = d follows from above two definitions, so
     1/d N^t p X2 = 1
     
     Now we multiply T by 1, and subtitute in the above.
     
     X1 = R p X2 + T / d N^t p X2
     X1 = H p X2, where H = R + T / d N^t
     
     Take cross product, X1 x X1 = 0; express as X1^ X1, where X1^ is (X1 hat) the skew-symmetric matrix for cross product.
     
     X1^ = [0   -z1  y1]
           [x1    0 -x1]
           [-y1  x1   0]
     
     X1^ X1 = X1^ H p X2
     0 = X1^ H p X2; p is a non-zero scalar, so we can divide it out
     0 = X1^ H X2
     
     Now work out the coefficients
     
     H X2 = [ H0 H1 H2 ] [x2]   [H0x2+H1y2+H2]
            [ H3 H4 H5 ]*[y2] = [H3x2+H4y2+H5]
            [ H6 H7 H8 ] [ 1]   [H6x2+H7y2+H8]
     
     (each col multiplied by)
                H0      H1      H2      H3      H4      H5      H6      H7      H8
                0       +0      +0    - z1 x2 - z1 y2 - z1    + y1 x2 + y1 y2 + y
     X1^ H X2 = z1 x2 + z1 y2 + z1      +0      +0      +0    - x1 x2 - x1 y2 - x1
               -y1 x2 - y1 y2 - y1    + x1 x2 + x1 y2 + x1      +0      +0      +0
     
     This only has rank 2, so we just use the first two constraints.
     
     Decomposition of planar homography matrix should follow Masks derivation.
     */
    
    [self.delegate codeDetected:detection.code withCorners:detection.corners withTransformation:detection.transformation withTimestamp:detection.time];
}

- (void) syncDetections
{
    if(![savedTimes count] || ![detections count]) return;

    NSMutableArray * emitObjects = [NSMutableArray array];
    NSMutableArray * deleteObjects = [NSMutableArray array];
    for(QRDetection * detection in detections)
    {
        NSNumber * time = [NSNumber numberWithUnsignedLongLong:detection.time];
        if(time > [savedTimes lastObject]) // haven't received sensor fusion data yet
            continue;

        if(time < [savedTimes firstObject]) {
            NSLog(@"Warning: dropping a detection, didn't associate it in time");
            [deleteObjects addObject:detection];
            continue;
        }

        NSRange searchRange = NSMakeRange(0, [savedTimes count]);
        NSUInteger findIndex = [savedTimes indexOfObject:time
                                           inSortedRange:searchRange
                                                 options:NSBinarySearchingFirstEqual
                                         usingComparator:^(id obj1, id obj2)
                                {
                                    return [obj1 compare:obj2];
                                }];

        if(findIndex != NSNotFound) {
            detection.transformation = [savedTransformations objectAtIndex:findIndex];
            [emitObjects addObject:detection];
        }
    }

    if(self.delegate) {
        for(QRDetection * detection in emitObjects) {
            [self handleSyncedDetection:detection];
        }
    }
    [detections removeObjectsInArray:emitObjects];
    [detections removeObjectsInArray:deleteObjects];
}

- (BOOL) detectionIsLate:(uint64_t)timestamp {
    if ([savedTimes count]) {
        NSNumber * startNum = [savedTimes objectAtIndex:0];
        uint64_t start = [startNum unsignedLongLongValue];
        if(timestamp < start)
            return true;
    }
    return false;
}

- (void) addSensorFusionData:(RCSensorFusionData *)data
{
    @synchronized(savedTimes) {
        camera = data.cameraParameters;
        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(data.sampleBuffer);
        
        imageWidth = CVPixelBufferGetWidth(pixelBuffer);
        imageHeight = CVPixelBufferGetHeight(pixelBuffer);
        
        CMTime time = CMSampleBufferGetPresentationTimeStamp(data.sampleBuffer);
        uint64_t time_us = time.value / (time.timescale / 1000000.);
        [savedTransformations addObject:data.cameraTransformation];
        [savedTimes addObject:[NSNumber numberWithUnsignedLongLong:time_us]];
        if([savedTransformations count] > 100)
            [savedTransformations removeObjectAtIndex:0];
        if([savedTimes count] > 100)
            [savedTimes removeObjectAtIndex:0];

        [self syncDetections];
    }
}

- (void) observeQR:(NSString *)code withCorners:(NSArray *)corners withTimestamp:(uint64_t)timestamp
{
    @synchronized(savedTimes) {
        if([self detectionIsLate:timestamp]) {
            NSLog(@"Warning: Detection came too late");
            return;
        }

        QRDetection * detection = [[QRDetection alloc] init];
        detection.code = code;
        detection.time = timestamp;
        detection.corners = corners;
        [detections addObject:detection];

        [self syncDetections];
    }
}

#pragma mark - AVCaptureMetadataOutputObjectsDelegate methods

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputMetadataObjects:(NSArray *)metadataObjects fromConnection:(AVCaptureConnection *)connection
{
    for(AVMetadataMachineReadableCodeObject *metadata in metadataObjects)
    {
        uint64_t time_us = metadata.time.value / (metadata.time.timescale / 1000000.);
        [self observeQR:metadata.stringValue withCorners:metadata.corners withTimestamp:time_us];
        break; //We only use the first one. Might want to implement something more advanced in the future
    }
}

@end
