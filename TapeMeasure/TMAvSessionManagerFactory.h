//
//  TMAvSessionManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@class AVCaptureSession;
@class AVCaptureVideoPreviewLayer;

@protocol TMAVSessionManager <NSObject>

@property AVCaptureSession *session;
@property AVCaptureVideoPreviewLayer *videoPreviewLayer;

- (void)startSession;
- (void)endSession;
- (BOOL)isRunning;

@end

@interface TMAvSessionManagerFactory : NSObject

+ (id<TMAVSessionManager>)getAVSessionManagerInstance;

@end


