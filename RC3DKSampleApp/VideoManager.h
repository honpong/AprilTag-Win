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
#import "AVSessionManager.h"
#import <RC3DK/RC3DK.h>

@protocol VideoFrameDelegate <NSObject>
@required
- (void)pixelBufferReadyForDisplay:(CVPixelBufferRef)pixelBuffer;	// This method is always called on the main thread.
@end

@interface VideoManager : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

- (void) setupWithSession:(AVCaptureSession*)avSession;
- (bool) startVideoCapture;
- (void) stopVideoCapture;
- (BOOL) isCapturing;

#ifdef DEBUG
- (void) setupWithSession:(AVCaptureSession *)avSession withOutput:(AVCaptureVideoDataOutput *)avOutput;
#endif

@property id<VideoFrameDelegate> delegate;
@property (readonly) AVCaptureVideoOrientation videoOrientation;
@property (readonly) AVCaptureSession *session;
@property (readonly) AVCaptureVideoDataOutput *output;

+ (VideoManager *) sharedInstance;

@end
