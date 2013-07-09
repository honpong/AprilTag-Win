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

/** Contains information about problems encountered by RCSensorFusion.
 
 Not all errors indicate failure, but some may. In that case, you can call [RCSensorFusion resetSensorFusion]. */
@interface RCSensorFusionError : NSError

/** The device moved much faster than expected.
 
 It is possible to proceed normally without addressing this error, but it may also indicate that the output is no longer valid. */
@property (readonly, nonatomic) bool speed;

/** No visual features were detected in the most recent image.
 
 This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall. However, if this is received repeatedly, it may indicate that the camera is covered or it is too dark. */
@property (readonly, nonatomic) bool vision;

/** A fatal internal error has occured.
 
 Please contact RealityCap and provide the code property of the RCSensorFusionError object. */
@property (readonly, nonatomic) bool other;

@end

/** The delegate of RCSensorFusion must implement this protocol in order to receive sensor fusion updates. */
@protocol RCSensorFusionDelegate <NSObject>

/** Sent to the delegate to provide the latest output of sensor fusion.
 
 This is called after each video frame is processed by RCSensorFusion, typically 30 times per second. 
 @param data An instance of RCSensorFusionData that contains the latest sensor fusion data.
 */
- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data;

/** Sent to the delegate if RCSensorFusion encounters a problem.
 @param error An instance of RCSensorFusionError indicating the problem encountered. Some conditions indicate a fatal error, meaning that the delegate must take action to continue (typically by calling [RCSensorFusion resetSensorFusion]).
 */
- (void) sensorFusionError:(RCSensorFusionError*)error;

@end

/** This class is the business end of the library, and the only one that you really need to use in order to get data out of it.
 This class is a psuedo-singleton. You shouldn't instantiate this class directly, but rather get an instance of it via the
 sharedInstance class method.
 
 Typical usage of this class would go something like this:

    // Prepare for sensor fusion.
    RCSensorFusion* sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self;
 
    // Start sensor fusion. Pass in a CLLocation object that represents the device's current location.
    [sensorFusion startSensorFusion:location];

    // Call these methods to repeatedly pass in the video frames and inertial data.
    [sensorFusion receiveVideoFrame:sampleBuffer];
    [sensorFusion receiveAccelerometerData:timestamp withX:x withY:y withZ:z];
    [sensorFusion receiveGyroData:timestamp withX:x withY:y withZ:z];

    // Implement the RCSensorFusionDelegate protocol methods to receive sensor fusion data.
    - (void) sensorFusionDidUpdate:(RCSensorFusionData*)data {}
    - (void) sensorFusionError:(NSError*)error {}

    // When you no longer want to receive sensor fusion data, stop it.
    // This releases a significant amount of resources, so be sure to call it.
    [sensorFusion stopSensorFusion];
 
 */
@interface RCSensorFusion : NSObject

/** Set this property to a delegate object that will receive the sensor fusion updates. The object must implement the RCSensorFusionDelegate protocol. */
@property (weak) id<RCSensorFusionDelegate> delegate;

/** Prepares the object to receive video and inertial data. 
 @param location The device's current location (including alititude) is used to account for differences in gravity across the earth. If set to nil because location is unavailable, results may be less accurate.
 */
- (void) startSensorFusion:(CLLocation*)location;

/** Stops the processing of video and inertial data and releases related resources. */
- (void) stopSensorFusion;

/** Fully resets the object to the state it would be in after calling startSensorFusion:. This could be
 called after receiving an error in [RCSensorFusionDelegate sensorFusionError:].*/
- (void) resetSensorFusion;

/** @returns True if sensor fusion is running. */
- (BOOL) isSensorFusionRunning;

/** Sets the physical origin of the coordinate system to the current location.
 
 Immediately after calling this method, the translation returned to the delegate will be (0, 0, 0).
 */
- (void) resetOrigin;

- (bool) saveCalibration; // TODO: should this be exposed externally?

/** Once sensor fusion has started, video frames should be passed in as they are received from the camera. 
 @param sampleBuffer A CMSampleBufferRef representing a single video frame. You can obtain the sample buffer via the AVCaptureSession class, or you can use RCAVSessionManager to manage the session and pass the frames in for you. In either case, you can retrieve a sample buffer after it has been processed from [RCSensorFusionData sampleBuffer].
 */
- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer;

/** Once sensor fusion has started, acceleration data should be passed in as it's received from the accelerometer 
 @param timestamp The timestamp from a CMAccelerometerData object
 @param x The x property from a CMAccelerometerData object
 @param y The y property from a CMAccelerometerData object
 @param z The z property from a CMAccelerometerData object
 */
- (void) receiveAccelerometerData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;

/** Once sensor fusion has started, angular velocity data should be passed in as it's received from the gyro 
 @param timestamp The timestamp from a CMGyroData object
 @param x The x property from a CMGyroData object
 @param y The y property from a CMGyroData object
 @param z The z property from a CMGyroData object
 */
- (void) receiveGyroData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;

/** Use this method to get a shared instance of this class */
+ (RCSensorFusion *) sharedInstance;

@end
