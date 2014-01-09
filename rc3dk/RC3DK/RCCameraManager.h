//
//  RCCameraManager.h
//  RC3DK
//
//  Created by Brian on 1/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface RCCameraManager : NSObject

+ (id) sharedInstance;

- (void) setVideoDevice:(AVCaptureDevice *)device;

- (void) saveFocus;
- (void) restoreFocus;
- (BOOL) isFocusing;

- (void) focusOnceAndLockWithTarget:(id)target action:(SEL)callback;
- (void) lockFocusWithTarget:(id)target action:(SEL)callback;

@end
