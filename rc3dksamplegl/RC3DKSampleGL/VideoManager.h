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

/** 
 Handles getting video frames from the AV session, and passes them directly to the RCSensorFusion shared instance.
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
