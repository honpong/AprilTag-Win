//
//  RCCameraManager.h
//  RC3DK
//
//  Created by Brian on 1/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@protocol RCCameraManagerDelegate <NSObject>

- (void) focusOperationFinished:(bool)timedOut;

@end

@interface RCCameraManager : NSObject

+ (id) sharedInstance;

- (void) setVideoDevice:(AVCaptureDevice *)device;
- (void) releaseVideoDevice;

- (void) focusOnceAndLock;
- (void) lockFocus;

@property (weak, nonatomic) id<RCCameraManagerDelegate> delegate;

@end
