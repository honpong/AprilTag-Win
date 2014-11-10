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
            [self.delegate codeDetected:detection.code withCorners:detection.corners withTransformation:detection.transformation withTimestamp:detection.time];
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
    for (AVMetadataMachineReadableCodeObject *metadata in metadataObjects) {
        uint64_t time_us = metadata.time.value / (metadata.time.timescale / 1000000.);
        [self observeQR:metadata.stringValue withCorners:metadata.corners withTimestamp:time_us];
        /*
         barCodeObject = (AVMetadataMachineReadableCodeObject *)[_prevLayer transformedMetadataObjectForMetadataObject:(AVMetadataMachineReadableCodeObject *)metadata];
         highlightViewRect = barCodeObject.bounds;
         */
    }
}

@end
