//
//  RCCameraManager.m
//  RC3DK
//
//  Created by Brian on 1/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCCameraManager.h"

@implementation RCCameraManager
{
    AVCaptureDevice * videoDevice;
    BOOL isFocusCapable;
    AVCaptureFocusMode previousFocusMode;

    BOOL isFocusing;
    BOOL waitingForFocus;

    id finishedFocusAndLockTarget;
    SEL finishedFocusAndLockCallback;

    NSTimer * timeoutTimer;
}

+ (id) sharedInstance
{
    static RCCameraManager *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (BOOL) isFocusing
{
    return isFocusing;
}

- (id) init
{
    self = [super init];

    if (self)
    {
        finishedFocusAndLockTarget = nil;
        finishedFocusAndLockCallback = nil;
        waitingForFocus = false;
        LOGME
    }

    return self;
}

- (void)performFinishedAction
{
    if(finishedFocusAndLockTarget && finishedFocusAndLockCallback)
        [finishedFocusAndLockTarget performSelector:finishedFocusAndLockCallback];
    finishedFocusAndLockCallback = nil;
    finishedFocusAndLockTarget = nil;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if ([keyPath isEqualToString:@"adjustingFocus"]) {
        bool wasFocusing = isFocusing;
        isFocusing = [[change objectForKey:NSKeyValueChangeNewKey] isEqualToNumber:[NSNumber numberWithInt:1]];
        if(isFocusing && !waitingForFocus) {
            // TODO: Should we do something here, e.g. reset the filter or create an error?
            NSLog(@"ERROR: Started a focus after we should be locked");
        }
        if(waitingForFocus && wasFocusing && !isFocusing) {
            waitingForFocus = false;
            NSLog(@"Locking the focus");
            if ([videoDevice lockForConfiguration:nil]) {
                if([videoDevice isFocusModeSupported:AVCaptureFocusModeLocked]) {
                    [videoDevice setFocusMode:AVCaptureFocusModeLocked];
                    [videoDevice unlockForConfiguration];
                }
            }
            [self performFinishedAction];
        }
    }
}


- (void)focusTimeout:(NSTimer*)theTimer
{
    if (!isFocusing && waitingForFocus) {
        // For some reason, even though we've requested it, the focus event isn't happening, give up and continue
        waitingForFocus = false;
        NSLog(@"INFO: Focus timed out, calling selector");
        [self performFinishedAction];
    }
}

- (void) setVideoDevice:(AVCaptureDevice *)device
{
    if(videoDevice) {
        [self releaseVideoDevice];
    }

    videoDevice = device;
    waitingForFocus = false;
    [videoDevice addObserver:self forKeyPath:@"adjustingFocus" options:NSKeyValueObservingOptionNew context:nil];
    isFocusCapable = [videoDevice isFocusModeSupported:AVCaptureFocusModeLocked] && [videoDevice isFocusModeSupported:AVCaptureFocusModeAutoFocus];
    previousFocusMode = videoDevice.focusMode;
}

- (void) releaseVideoDevice
{
    if(videoDevice) {
        // If we were already watching adjustingFocus on a different device, we can stop now
        [videoDevice removeObserver:self forKeyPath:@"adjustingFocus"];
        finishedFocusAndLockTarget = nil;
        finishedFocusAndLockCallback = nil;
        waitingForFocus = false;
        if(isFocusCapable) {
            if ([videoDevice lockForConfiguration:nil]) {
                if([videoDevice isFocusModeSupported:previousFocusMode])
                    [videoDevice setFocusMode:previousFocusMode];
                [videoDevice unlockForConfiguration];
            }
        }
        videoDevice = nil;

        // There might be a timer hanging around that was started but never fired if we tap the start / stop button rapidly
        if(timeoutTimer)
            [timeoutTimer invalidate];
    }
}

- (void) focusOnceAndLockWithTarget:(id)target action:(SEL)callback;
{
    // Only one focusandlock can be performed at a time
    if(waitingForFocus)
        return;
    finishedFocusAndLockTarget = target;
    finishedFocusAndLockCallback = callback;
    if(!isFocusCapable) {
        // Device doesn't support focus, so we are already ready to start
        NSLog(@"INFO: Doesn't support focus, starting without");
        [self performFinishedAction];
    }
    else {
        if ([videoDevice lockForConfiguration:nil]) {
            if([videoDevice isFocusModeSupported:AVCaptureFocusModeAutoFocus]) {
                NSLog(@"Focusing once before starting");
                waitingForFocus = true;
                [videoDevice setFocusMode:AVCaptureFocusModeAutoFocus];
                timeoutTimer = [NSTimer scheduledTimerWithTimeInterval:3.0 target:self selector:@selector(focusTimeout:) userInfo:nil repeats:NO];
            }
            [videoDevice unlockForConfiguration];
        }
    }

}

- (void) lockFocusWithTarget:(id)target action:(SEL)callback
{
    if(!isFocusCapable) {
        NSLog(@"INFO: Doesn't support focus, starting without");
        finishedFocusAndLockTarget = target;
        finishedFocusAndLockCallback = callback;
        [self performFinishedAction];
    }
    else if(videoDevice.focusMode == AVCaptureFocusModeLocked && !videoDevice.adjustingFocus) {
        // Device doesn't support focus or is already locked, so we are already ready to start
        NSLog(@"INFO: Focus is already locked, starting");
        [self performFinishedAction];
    }
    else
        [self focusOnceAndLockWithTarget:target action:callback];
}

@end
