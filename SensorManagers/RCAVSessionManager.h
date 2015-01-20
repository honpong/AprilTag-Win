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
- (void) addInputToSession;
- (BOOL) startSession;
- (void) endSession;
- (BOOL) isRunning;
- (BOOL) addOutput:(AVCaptureOutput*)output;
- (void) removeOutput:(AVCaptureOutput*)output;
- (bool) isImageClean;

+ (RCAVSessionManager*) sharedInstance;

@end


