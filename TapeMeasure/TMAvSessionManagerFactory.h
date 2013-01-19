//
//  TMAvSessionManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCorvisManagerFactory.h"
#import <AVFoundation/AVFoundation.h>

@protocol TMAVSessionManager <NSObject>

@property AVCaptureSession *session;
@property AVCaptureVideoPreviewLayer *videoPreviewLayer;

- (void)startSession;
- (void)endSession;
- (bool)isRunning;
- (void)addOutput:(AVCaptureVideoDataOutput*)output;

@end

@interface TMAvSessionManagerFactory

+ (id<TMAVSessionManager>)getAVSessionManagerInstance;
+ (void)setAVSessionManagerInstance:(id<TMAVSessionManager>)mockObject;

@end


