//
//  RCSensorFusion.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMedia/CoreMedia.h>
#import <CoreLocation/CoreLocation.h>
#import "RCMotionManager.h"
#import "RCVideoManager.h"
#import "RCSensorFusionData.h"
#import "RCSensorFusionStatus.h"

@protocol RCSensorFusionDelegate <NSObject>

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data;
- (void) sensorFusionWarning:(int)code;
- (void) sensorFusionError:(NSError*)error;

@end

@interface RCSensorFusion : NSObject

@property (weak) id<RCSensorFusionDelegate> delegate;

- (void) startSensorFusion:(AVCaptureSession*)session withLocation:(CLLocation*)location;
- (void) stopSensorFusion;
- (void) resetSensorFusion;
- (BOOL) isSensorFusionRunning;
- (void) markStart;
- (bool) saveCalibration;
- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer;
- (void) receiveAccelerometerData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (void) receiveGyroData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (void) getCurrentCameraMatrix:(float [16])matrix withFocalCenterRadial:(float [5])focalCenterRadial withVirtualTapeStart:(float[3])start;

+ (RCSensorFusion *) sharedInstance;

@end
