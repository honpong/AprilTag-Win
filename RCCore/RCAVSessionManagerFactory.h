//
//  TMAvSessionManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCorvisManagerFactory.h"
#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

/** Manages the AV session. You must call setupAVSession before getInstance or startSession.
 This class automatically handles an app pause and ends the AV session. It does not automatically resume the 
 AV session on an app resume.
 */
@protocol RCAVSessionManager <NSObject>

@property AVCaptureSession *session;
@property AVCaptureDevice *videoDevice;

- (BOOL)startSession;
- (void)endSession;
- (BOOL)isRunning;
- (BOOL)addOutput:(AVCaptureVideoDataOutput*)output;
- (void)lockFocus;
- (void)unlockFocus;
- (bool)isImageClean;

@end

@interface RCAVSessionManagerFactory : NSObject

+ (void)setupAVSession;
+ (id<RCAVSessionManager>) getInstance;
+ (void) setInstance:(id<RCAVSessionManager>)mockObject;

@end


