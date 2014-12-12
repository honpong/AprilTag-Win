//
//  RCCameraManager.h
//  RC3DK
//
//  Created by Brian on 1/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <functional>

@interface RCCameraManager : NSObject

+ (id) sharedInstance;

- (void) setVideoDevice:(AVCaptureDevice *)device;
- (void) releaseVideoDevice;

- (void) focusOnceAndLockWithCallback:(std::function<void (uint64_t, float)>)callback;
- (void) lockFocusWithCallback:(std::function<void (uint64_t, float)>)callback;
- (void) lockFocusCurrentWithCallback:(std::function<void (uint64_t, float)>)callback;
- (void) lockFocusToPosition:(float)position withCallback:(std::function<void (uint64_t, float)>)callback;
- (void) unlockFocus;

@end
