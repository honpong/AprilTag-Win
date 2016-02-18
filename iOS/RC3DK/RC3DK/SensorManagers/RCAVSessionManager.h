//
//  RCAVSessionManager.h
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

/** Manages the AV session.
 
 This class automatically handles an app pause and ends the AV session. It does not automatically resume the
 AV session on an app resume.
 */

@interface RCAVSessionManager : NSObject

@property (retain, nonatomic) AVCaptureSession *session;
@property (retain, nonatomic) AVCaptureDevice *videoDevice;

+ (void) requestCameraAccessWithCompletion:(void (^)(BOOL granted))handler;
/** Configures the device and adds the input to the session. This will trigger the caemra access dialog if it wasn't already triggered. */
- (BOOL) startSession;
- (BOOL) startSessionWithFrameRate:(int)frameRate withWidth:(int)width withHeight:(int)height;
- (void) endSession;
- (BOOL) isRunning;
- (BOOL) addOutput:(AVCaptureOutput*)output;
- (void) removeOutput:(AVCaptureOutput*)output;
- (bool) isImageClean;
/** Sets the camera to a fixed frame rate which is either the fastest rate supported by the camera, or the rate parameter. */
- (void) configureCameraWithFrameRate:(int)rate withWidth:(int)width withHeight:(int)height;

+ (RCAVSessionManager*) sharedInstance;

@end


