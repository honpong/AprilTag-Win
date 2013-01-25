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

/** Manages the AV session. You must call createAndConfigAVSession before startSession. This class automatically handles an app pause and
 ends the AV session. It does not automatically resume the AV session on an app resume.
 */
@protocol RCAVSessionManager <NSObject>

@property AVCaptureSession *session;
@property AVCaptureVideoPreviewLayer *videoPreviewLayer;

- (BOOL)startSession;
- (void)endSession;
- (BOOL)isRunning;
- (BOOL)addOutput:(AVCaptureVideoDataOutput*)output;
- (void)setupAVSession;

@end

@interface RCAVSessionManagerFactory

+ (id<RCAVSessionManager>)getAVSessionManagerInstance;
+ (void)setAVSessionManagerInstance:(id<RCAVSessionManager>)mockObject;

@end


