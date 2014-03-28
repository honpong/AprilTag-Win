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

@protocol RCVideoFrameDelegate <NSObject>
@required
- (void) displaySampleBuffer:(CMSampleBufferRef)sampleBuffer;	// This method is always called on the main thread.
@end

@interface RCVideoManager : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

- (void) setupWithSession:(AVCaptureSession*)avSession;
- (bool) startVideoCapture;
- (void) stopVideoCapture;
- (BOOL) isCapturing;

#ifdef DEBUG
- (void) setupWithSession:(AVCaptureSession *)avSession withOutput:(AVCaptureVideoDataOutput *)avOutput;
#endif

@property id<RCVideoFrameDelegate> delegate;
@property (readonly) AVCaptureVideoOrientation videoOrientation;
@property (readonly) AVCaptureSession *session;
@property (readonly) AVCaptureVideoDataOutput *output;

+ (RCVideoManager *) sharedInstance;

@end
