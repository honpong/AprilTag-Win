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
- (void)setupVideoCapWithSession:(AVCaptureSession*)session withCorvisManager:(id<RCCorvisManager>)corvisManager;
- (bool)startVideoCap;
- (void)stopVideoCap;
@end

@interface RCVideoCapManagerFactory
+ (id<RCVideoCapManager>)getVideoCapManagerInstance;
+ (void)setVideoCapManagerInstance:(id<RCVideoCapManager>)mockObject;
@end
