//
//  RCVideoManager.h
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#import <CoreImage/CoreImage.h>
#include <stdio.h>
#import <ImageIO/ImageIO.h>
#import "RCAVSessionManager.h"
#import "RC3DK.h"
#import "RCVideoFrameProvider.h"

/** 
 Handles getting video frames from the AV session, and passes them to its delegates. Its delegates typically include instances of RCSensorFusion and/or RCVideoPreview.
 */
@interface RCVideoManager : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate, RCVideoFrameProvider>

- (void) setupWithSession:(AVCaptureSession*)avSession;
- (bool) startVideoCapture;
- (void) stopVideoCapture;
- (BOOL) isCapturing;

@property (readonly) AVCaptureVideoOrientation videoOrientation;
@property (readonly) AVCaptureSession *session;
@property (readonly) AVCaptureVideoDataOutput *output;

+ (RCVideoManager *) sharedInstance;

@end
