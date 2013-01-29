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
#import "RCCorvisManagerFactory.h"

@protocol RCVideoCapManager <NSObject>
- (bool)startVideoCap;
- (void)stopVideoCap;
- (BOOL)isCapturing;
@end

@interface RCVideoCapManagerFactory
+ (void)setupVideoCapWithSession:(AVCaptureSession*)session;
+ (void)setupVideoCapWithSession:(AVCaptureSession*)session withOutput:(AVCaptureVideoDataOutput*)output withCorvisManager:(id<RCCorvisManager>)corvisManager;
+ (id<RCVideoCapManager>)getVideoCapManagerInstance;
+ (void)setVideoCapManagerInstance:(id<RCVideoCapManager>)mockObject;
@end
