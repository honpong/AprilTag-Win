//
//  RCSensorFusion.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMedia/CoreMedia.h>
#import <CoreLocation/CoreLocation.h>
#import <CoreMotion/CoreMotion.h>
#import <AVFoundation/AVFoundation.h>
#import "RCSensorFusionData.h"
#import "RCSensorFusionStatus.h"
#import "RCLicenseError.h"
#import "RCSensorFusionError.h"


/** The delegate of RCSensorFusion must implement this protocol in order to receive sensor fusion updates. */
@protocol RCSensorFusionDelegate <NSObject>

/** Sent to the delegate to provide the latest output of sensor fusion.
 
 This is called after each video frame is processed by RCSensorFusion, typically 30 times per second.
 @param data An instance of RCSensorFusionData that contains the latest sensor fusion data.
 */
- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data;

/** Sent to the delegate whenever the status of RCSensorFusion changes, including when an error occurs.
 
 @param status An instance of RCSensorFusionStatus containing the current sensor fusion status.
 */
- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus*)status;

@end

/** This class is the business end of the library, and the only one that you really need to use in order to get data out of it.
 This class is a psuedo-singleton. You shouldn't instantiate this class directly, but rather get an instance of it via the
 sharedInstance class method.
 
 */
@interface RCSensorFusion : NSObject

/** Set this property to a delegate object that will receive the sensor fusion updates. The object must implement the RCSensorFusionDelegate protocol. */
@property (weak) id<RCSensorFusionDelegate> delegate;

/** Use this method to get a shared instance of this class */
+ (RCSensorFusion *) sharedInstance;

/** Sets the 3DK license key. Call this once before starting sensor fusion. In most cases, this should be done when your app starts.
 
 @param licenseKey A 30 character string. Obtain a license key by contacting RealityCap.
 */
- (void) setLicenseKey:(NSString*)licenseKey;

/** Sets the current location of the device.
 
 @param location The device's current location (including altitude) is used to account for differences in gravity across the earth. If location is unavailable, results may be less accurate. This should be called before starting sensor fusion or calibration.
*/
- (void) setLocation:(CLLocation*)location;

/** Determine if valid saved calibration data exists from a previous run.
 
 @return If false, it is strongly recommended to perform a calibration procedure, including calling startStaticCalibration, and running with video processing in both portrait and landscape for at least 5 seconds each.
 @note In some cases, calibration data may become invalid or go out of date, in which case this will return false even if it previously returned true. It is recommended to check hasCalibrationData before each use, even if calibration has previously been run successfully.
 */
- (bool) hasCalibrationData;

/** Starts a special one-time static calibration mode.
 
 This method may be called to estimate internal parameters; running it once on a particular device should improve the quality of output for that device. The device should be placed on a solid surface (not held in the hand), and left completely still for the duration of the static calibration. The camera is not used in this mode, so it is OK if the device is placed on its back. Check [RCSensorFusionStatus progress] to determine how well the parameters have been calibrated. When finished, RCSensorFusion will return to the inactive state and the resulting device parameters will be saved. You do not need to call startSensorFusion or stopSensorFusion when running static calibration.
 */
- (void) startStaticCalibration;

/** Prepares the object to receive video and inertial data, and starts sensor fusion updates.
 
 This method should be called when you are ready to begin receiving sensor fusion updates and your user is aware to point the camera at an appropriate visual scene. After you call this method you should immediately begin passing video, accelerometer, and gyro data using receiveVideoFrame, receiveAccelerometerData, and receiveGyroData respectively. Full processing will not begin until the user has held the device steady for a two second initialization period (this occurs concurrently with focusing the camera). The device does not need to be perfectly still; minor shake from the device being held in hand is acceptable. If the user moves during this time, the two second timer will start again. The progress of this timer is provided as a float between 0 and 1 in [RCSensorFusionStatus progress].
 
 @param device The camera device to be used for capture. This function will lock the focus on the camera device (if the device is capable of focusing) before starting video processing. No other modifications to the camera settings are made.
 */
- (void) startSensorFusionWithDevice:(AVCaptureDevice *)device;

/** Prepares the object to receive video and inertial data, and starts sensor fusion updates.
 
 This method may be called when you are ready to begin receiving sensor fusion updates and your user is aware to point the camera at an appropriate visual scene. After you call this method you should immediately begin passing video, accelerometer, and gyro data using receiveVideoFrame, receiveAccelerometerData, and receiveGyroData respectively. It is strongly recommended to call [startSensorFusionWithDevice:] rather than this function, unless it is absolutely impossible for the device to be held steady for two seconds while initializing (for example, in a moving vehicle). There will be a delay after calling this function before video processing begins, while the camera is focused and sensor fusion is initialized.
 
 @param device The camera device to be used for capture. This function will lock the focus on the camera device (if the device is capable of focusing) before starting video processing. No other modifications to the camera settings are made.
 @note It is strongly recommended to call [startSensorFusionWithDevice:] rather than this function
 */
- (void) startSensorFusionUnstableWithDevice:(AVCaptureDevice *)device;

/* Note: this has been switched to a regular comment since it does not work now.
 
 Request that sensor fusion attempt to track a user-selected feature.
 
 If you call this method, the sensor fusion algorithm will make its best effort to detect and track a visual feature near the specified image coordinates. There is no guarantee that such a feature may be identified or tracked for any length of time (for example, if you specify coordinates in the middle of a blank wall, no feature will be found. Any such feature is also not likely to be found at the exact pixel coordinates specified.
 @param x The requested horizontal location, in pixels relative to the image coordinate frame.
 @param x The requested vertical location, in pixels relative to the image coordinate frame.
 */
//- (void) selectUserFeatureWithX:(float)x withY:(float)Y;

/** Stops the processing of video and inertial data and releases all related resources. */
- (void) stopSensorFusion;

/** Once sensor fusion has started, video frames should be passed in as they are received from the camera. 
 @param sampleBuffer A CMSampleBufferRef representing a single video frame. You can obtain the sample buffer via the AVCaptureSession class, or you can use RCAVSessionManager to manage the session and pass the frames in for you. In either case, you can retrieve a sample buffer after it has been processed from [RCSensorFusionData sampleBuffer]. If you manage the AVCaptureSession yourself, you must use the 640x480 preset ([AVCaptureSession setSessionPreset:AVCaptureSessionPreset640x480]) and set the output format to 420f ([AVCaptureVideoDataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:'420f'] forKey:(id)kCVPixelBufferPixelFormatTypeKey]]).
 */
- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer;

/** Once sensor fusion has started, acceleration data should be passed in as it's received from the accelerometer.
 @param accelerometerData The CMAccelerometerData object. You can obtain the CMAccelerometerData object from CMMotionManager, or you can use RCMotionManager to handle the setup and passing of motion data for you. If you manage CMMotionManager yourself, you must set the accelerometer update interval to .01 ([CMMotionManager setAccelerometerUpdateInterval:.01]).
 */
- (void) receiveAccelerometerData:(CMAccelerometerData *)accelerometerData;

/** Once sensor fusion has started, angular velocity data should be passed in as it's received from the gyro.
 @param gyroData The CMGyroData object. You can obtain the CMGyroData object from CMMotionManager, or you can use RCMotionManager to handle the setup and passing of motion data for you. If you manage CMMotionManager yourself, you must set the gyro update interval to .01 ([CMMotionManager setAccelerometerUpdateInterval:.01]).
 */
- (void) receiveGyroData:(CMGyroData *)gyroData;

// deprecated
- (void) validateLicense:(NSString*)apiKey withCompletionBlock:(void (^)(int licenseType, int licenseStatus))completionBlock withErrorBlock:(void (^)(NSError*))errorBlock __attribute((deprecated("Call setLicenseKey: instead.")));
- (void) startInertialOnlyFusion __attribute((deprecated("No longer needed; does nothing.")));
- (void) startProcessingVideoWithDevice:(AVCaptureDevice *)device __attribute((deprecated("Use startSensorFusionWithDevice instead.")));
- (void) stopProcessingVideo __attribute((deprecated("Use stopSensorFusion instead.")));
- (void) stopStaticCalibration __attribute((deprecated("No longer needed; does nothing.")));

@end
