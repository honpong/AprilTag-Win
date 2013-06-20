//
//  TMVideoCapManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#import <CoreImage/CoreImage.h>
#include <stdio.h>
#import <ImageIO/ImageIO.h>
#import "RCAVSessionManager.h"
#import "RCSensorFusion.h"

@protocol RCVideoFrameDelegate <NSObject>
@required
- (void)pixelBufferReadyForDisplay:(CVPixelBufferRef)pixelBuffer;	// This method is always called on the main thread.
@end

@interface RCVideoManager : NSObject

- (bool) startVideoCapture;
- (void) stopVideoCapture;
- (BOOL)isCapturing;

@property id<RCVideoFrameDelegate> delegate;
@property AVCaptureVideoOrientation videoOrientation;
@property AVCaptureSession *session;
@property AVCaptureVideoDataOutput *output;

+ (void)setupVideoCapWithSession:(AVCaptureSession*)session;
+ (void)setupVideoCapWithSession:(AVCaptureSession *)session withOutput:(AVCaptureVideoDataOutput *)output;
+ (RCVideoManager *) sharedInstance;

@end
