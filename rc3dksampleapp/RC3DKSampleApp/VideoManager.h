//
//  VideoManager.h
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#import <CoreImage/CoreImage.h>
#include <stdio.h>
#import <ImageIO/ImageIO.h>
#import "AVSessionManager.h"

@protocol VideoFrameDelegate <NSObject>
@required
- (void)pixelBufferReadyForDisplay:(CVPixelBufferRef)pixelBuffer;	// This method is always called on the main thread.
@end

/** Handles getting video frames from the AV session, and passes them directly to the RCSensorFusion shared instance.
 
 This class is identical to RCVideoManager, included in the 3DK framework. 
 */
@interface VideoManager : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate, RCVideoFrameProvider>

- (void) setupWithSession:(AVCaptureSession*)avSession;
- (bool) startVideoCapture;
- (void) stopVideoCapture;
- (BOOL) isCapturing;

@property id<RCVideoFrameDelegate> delegate;
@property (readonly) AVCaptureVideoOrientation videoOrientation;
@property (readonly) AVCaptureSession *session;
@property (readonly) AVCaptureVideoDataOutput *output;

+ (VideoManager *) sharedInstance;

@end
