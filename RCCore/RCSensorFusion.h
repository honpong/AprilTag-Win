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

/** This class is the business end of the library, and the only one that you really need to use in order to get data out of it.
 This class is a psuedo-singleton. You shouldn't instantiate this class directly, but rather get an instance of it via the
 sharedInstance class method.
 
 Typical usage of this class would go something like this:

    // prepare for sensor fusion
    RCSensorFusion* sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self;
 
    // start sensor fusion. pass in a CLLocation object that represents the device's current location.
    [sensorFusion startSensorFusion:location];

    // begin repeatedly passing in the video frames and motion data
    [sensorFusion receiveVideoFrame:sampleBuffer];
    [sensorFusion receiveAccelerometerData:timestamp withX:x withY:y withZ:z];
    [sensorFusion receiveGyroData:timestamp withX:x withY:y withZ:z];

    // implement the RCSensorFusionDelegate protocol methods to receive sensor fusion data
    - (void) sensorFusionDidUpdate:(RCSensorFusionData*)data {}
    - (void) sensorFusionError:(NSError*)error {}

    // when you no longer want to receive sensor fusion data, stop it
    [sensorFusion stopSensorFusion];
 
 */
@interface RCSensorFusion : NSObject

@property (weak) id<RCSensorFusionDelegate> delegate;

- (void) startSensorFusion:(CLLocation*)location;
- (void) stopSensorFusion;
- (void) resetSensorFusion;
- (BOOL) isSensorFusionRunning;
- (void) markStart;
- (bool) saveCalibration;
- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer;
- (void) receiveAccelerometerData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (void) receiveGyroData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (void) getCurrentCameraMatrix:(float [16])matrix withFocalCenterRadial:(float [5])focalCenterRadial withVirtualTapeStart:(float[3])start;

/** Use this method to get a shared instance of this class */
+ (RCSensorFusion *) sharedInstance;

@end
