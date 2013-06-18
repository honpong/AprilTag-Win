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
#import "RCAVSessionManagerFactory.h"
#import "RCPimManagerFactory.h"

@protocol RCVideoFrameDelegate <NSObject>
@required
- (void)pixelBufferReadyForDisplay:(CVPixelBufferRef)pixelBuffer;	// This method is always called on the main thread.
@end

@protocol RCVideoCapManager <NSObject>
- (bool)startVideoCap;
- (void)stopVideoCap;
- (BOOL)isCapturing;
@property id<RCVideoFrameDelegate> delegate;
@property AVCaptureVideoOrientation videoOrientation;
@end

@interface RCVideoCapManagerFactory : NSObject

+ (void)setupVideoCapWithSession:(AVCaptureSession*)session;
+ (id<RCVideoCapManager>) getInstance;

#ifdef DEBUG
+ (void)setupVideoCapWithSession:(AVCaptureSession*)session withOutput:(AVCaptureVideoDataOutput*)output withCorvisManager:(id<RCPimManager>)corvisManager;
+ (void)setInstance:(id<RCVideoCapManager>)mockObject;
#endif

@end
