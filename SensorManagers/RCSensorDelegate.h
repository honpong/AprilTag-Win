//
//  RCSensorDelegate.h
//
//  Created by Eagle Jones on 6/13/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

/** Provides a generic sensor interface.
 
 A delegate implementing this protocol is required for the provided RCCalibration views, and can provide a simplified sensor interface for use with RCSensorFusion.
 */
@protocol RCSensorDelegate <CLLocationManagerDelegate>

@required
/**
 Starts the AV session, video capture, and motion capture
 */
- (void) startAllSensors;
/**
 Stops the AV session, video capture, and motion capture
 */
- (void) stopAllSensors;
/**
 Starts motion capture only
 */
- (void) startMotionSensors;
/**
 Attempts to start location updates (authorizing if necessary)
 */
- (void) startLocationUpdatesIfAllowed;
/**
 Returns the most recently updated location
 */
- (CLLocation *) getStoredLocation;
/**
 @returns The AVCaptureDevice used to capture video.
 */
- (AVCaptureDevice*) getVideoDevice;
/**
 @returns Gets a object that sends video frames to it's delegate. This is where the video frames for the video preview views come from.
 */
- (id<RCVideoFrameProvider>) getVideoProvider;
/**
 Starts the AVCaptureSession that we're using to capture video.
 */
- (void) startVideoSession;
/**
 Stops the AVCaptureSession.
 */
- (void) stopVideoSession;
@end

/** A standard implementation of RCSensorDelegate.
 
 This implementation uses RCAVSessionManager, RCVideoManager, RCLocationManager, and RCMotionManager.
 */
@interface SensorDelegate : NSObject <RCSensorDelegate>

+ (SensorDelegate*) sharedInstance;

@end
