//
//  RCSensorFusion.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMedia/CoreMedia.h>
#include "feature_info.h"
#import <CoreLocation/CoreLocation.h>
#import "RCMotionManager.h"
#import "RCVideoManager.h"
#import "RCSensorFusionData.h"
#import "RCSensorFusionStatus.h"

typedef enum
{
    WARNING_SPEED, WARNING_VISION
} SensorFusionWarningCode;

typedef enum
{
    ERROR_SPEED, ERROR_VISION, ERROR_OTHER
} SensorFusionErrorCode;

@protocol RCSensorFusionDelegate <NSObject>

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data;
- (void) sensorFusionWarning:(int)code;
- (void) sensorFusionError:(NSError*)error;

@end

@interface RCSensorFusion : NSObject

@property (weak) id<RCSensorFusionDelegate> delegate;

- (void) startSensorFusion:(AVCaptureSession*)session withLocation:(CLLocation*)location;
- (void) stopSensorFusion;
- (BOOL) isPluginsStarted;
- (void) markStart;
- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer;
- (void) receiveAccelerometerData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (void) receiveGyroData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (int) getCurrentFeatures:(struct corvis_feature_info *)features withMax:(int)max;
- (void) getCurrentCameraMatrix:(float [16])matrix withFocalCenterRadial:(float [5])focalCenterRadial withVirtualTapeStart:(float[3])start;

+ (RCSensorFusion *) sharedInstance;

@end
