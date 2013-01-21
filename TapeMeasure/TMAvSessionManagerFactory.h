//
//  TMAvSessionManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCorvisManagerFactory.h"
#import <AVFoundation/AVFoundation.h>

/** Manages the AV session. You must call createAndConfigAVSession before startSession. This class automatically handles an app pause and
 ends the AV session. It does not automatically resume the AV session on an app resume.
 */
@protocol TMAVSessionManager <NSObject>

@property AVCaptureSession *session;
@property AVCaptureVideoPreviewLayer *videoPreviewLayer;

- (void)startSession;
- (void)endSession;
- (bool)isRunning;
- (void)addOutput:(AVCaptureVideoDataOutput*)output;
- (void)createAndConfigAVSession;

@end

@interface TMAvSessionManagerFactory

+ (id<TMAVSessionManager>)getAVSessionManagerInstance;
+ (void)setAVSessionManagerInstance:(id<TMAVSessionManager>)mockObject;

@end


