//
//  RCCameraManager.mm
//  RC3DK
//
//  Created by Brian on 1/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCDebugLog.h"
#import "RCCameraManager.h"
#import "camera_control_interface.h"

camera_control_interface::camera_control_interface(): platform_ptr(NULL)
{
    platform_ptr = (void *)CFBridgingRetain([[RCCameraManager alloc] init]);
}

void camera_control_interface::init(void *platform)
{
    [(__bridge RCCameraManager *)platform_ptr setVideoDevice:(__bridge id)platform];
}

void camera_control_interface::release_platform_specific_object()
{
    [(__bridge RCCameraManager *)platform_ptr releaseVideoDevice];
}

camera_control_interface::~camera_control_interface()
{
    CFBridgingRelease(platform_ptr);
    platform_ptr = nil;
}

void camera_control_interface::focus_lock_at_current_position(std::function<void (uint64_t, float)> callback)
{
    [(__bridge RCCameraManager *)platform_ptr lockFocusCurrentWithCallback:callback];
}

void camera_control_interface::focus_lock_at_position(float position, std::function<void (uint64_t, float)> callback)
{
    [(__bridge RCCameraManager *)platform_ptr lockFocusToPosition:position withCallback:callback];
}

void camera_control_interface::focus_once_and_lock(std::function<void (uint64_t, float)> callback)
{
    [(__bridge RCCameraManager *)platform_ptr focusOnceAndLockWithCallback:callback];
}

void camera_control_interface::focus_unlock()
{
    [(__bridge RCCameraManager *)platform_ptr unlockFocus];
}

typedef NS_ENUM(int, RCCameraManagerOperationType) {
    RCCameraManagerOperationNone = 0,
    RCCameraManagerOperationFocusOnce,
    RCCameraManagerOperationFocusLock,
    RCCameraManagerOperationFocusCurrent,
    RCCameraManagerOperationFocusPosition,
};

@implementation RCCameraManager
{
    AVCaptureDevice * videoDevice;
    BOOL isFocusCapable;
    BOOL hasLensPosition;
    AVCaptureFocusMode previousFocusMode;

    BOOL isFocusing;
    RCCameraManagerOperationType pendingOperation;
    std::function<void (uint64_t, float)> callback;

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

- (void)finishOperation
{
    uint64_t timestamp = 0;
    float focal_length = 1;
    if(hasLensPosition)
        focal_length = videoDevice.lensPosition;
    if(callback)
        callback(timestamp, focal_length);
    callback = nil;
    pendingOperation = RCCameraManagerOperationNone;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    // Don't track operations which are handled by iOS 8
    if(pendingOperation == RCCameraManagerOperationFocusCurrent || pendingOperation == RCCameraManagerOperationFocusPosition)
        return;

    if ([keyPath isEqualToString:@"adjustingFocus"]) {
        bool wasFocusing = isFocusing;
        isFocusing = [change[NSKeyValueChangeNewKey] isEqualToNumber:@1];
        if(isFocusing && !pendingOperation) {
            // TODO: Should we do something here, e.g. reset the filter or create an error?
            DLog(@"ERROR: Started a focus after we should be locked");
        }
        if(pendingOperation && wasFocusing && !isFocusing) {
            DLog(@"Locking the focus");
            if(timeoutTimer)
                [timeoutTimer invalidate];

            if ([videoDevice lockForConfiguration:nil]) {
                if([videoDevice isFocusModeSupported:AVCaptureFocusModeLocked]) {
                    [videoDevice setFocusMode:AVCaptureFocusModeLocked];
                    [videoDevice unlockForConfiguration];
                }
            }
            [self finishOperation];
        }
    }
}


- (void)focusTimeout:(NSTimer*)theTimer
{
    if (!isFocusing && pendingOperation) {
        // For some reason, even though we've requested it, the focus event isn't happening, give up and continue
        pendingOperation = RCCameraManagerOperationNone;
        DLog(@"Focus timed out, continuing");
        [self finishOperation];
    }
}

- (void) setVideoDevice:(AVCaptureDevice *)device
{
    if(videoDevice) {
        [self releaseVideoDevice];
    }

    videoDevice = device;
    pendingOperation = RCCameraManagerOperationNone;
    [videoDevice addObserver:self forKeyPath:@"adjustingFocus" options:NSKeyValueObservingOptionNew context:nil];
    isFocusCapable = [videoDevice isFocusModeSupported:AVCaptureFocusModeLocked] && [videoDevice isFocusModeSupported:AVCaptureFocusModeAutoFocus];
    hasLensPosition = [videoDevice respondsToSelector:@selector(setFocusModeLockedWithLensPosition:completionHandler:)];
    previousFocusMode = videoDevice.focusMode;
}

- (void) releaseVideoDevice
{
    if(videoDevice) {
        // If we were already watching adjustingFocus on a different device, we can stop now
        [videoDevice removeObserver:self forKeyPath:@"adjustingFocus"];
        pendingOperation = RCCameraManagerOperationNone;
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

- (void) focusOperation:(RCCameraManagerOperationType)operation withCallback:(std::function<void (uint64_t, float)>)focus_callback
{
    // Only one operation can be performed at a time
    if(pendingOperation)
        return;

    callback = focus_callback;

    if(!isFocusCapable) {
        DLog(@"INFO: Doesn't support focus, starting without");
        [self finishOperation];
    }
    else if(operation == RCCameraManagerOperationFocusLock &&
            videoDevice.focusMode == AVCaptureFocusModeLocked && !videoDevice.adjustingFocus) {
        // Focus is already locked and we requested a lock
        DLog(@"INFO: Focus is already locked, starting");
        [self finishOperation];
    }
    else {
        if ([videoDevice lockForConfiguration:nil]) {
            if([videoDevice isFocusModeSupported:AVCaptureFocusModeAutoFocus]) {
                DLog(@"Focusing once before starting");
                pendingOperation = operation;
                [videoDevice setFocusMode:AVCaptureFocusModeAutoFocus];
                timeoutTimer = [NSTimer scheduledTimerWithTimeInterval:3.0 target:self selector:@selector(focusTimeout:) userInfo:nil repeats:NO];
            }
            [videoDevice unlockForConfiguration];
        }
    }

}

- (void) focusOnceAndLockWithCallback:(std::function<void (uint64_t, float)>)focus_callback
{
    DLog(@"Focus once and lock requested");
    [self focusOperation:RCCameraManagerOperationFocusOnce withCallback:focus_callback];
}

- (void) lockFocusWithCallback:(std::function<void (uint64_t, float)>)focus_callback
{
    DLog(@"Focus lock requested");
    [self focusOperation:RCCameraManagerOperationFocusLock withCallback:focus_callback];
}

- (void) lockLensPosition:(float)position withCallback:(std::function<void (uint64_t, float)>)focus_callback
{
    if(!isFocusCapable) {
        DLog(@"INFO: Doesn't support focus, starting without");
        focus_callback(0, 1);
        return;
    }

    if(position == AVCaptureLensPositionCurrent)
        pendingOperation = RCCameraManagerOperationFocusCurrent;
    else
        pendingOperation = RCCameraManagerOperationFocusPosition;

    if([videoDevice lockForConfiguration:nil]) {
        __weak typeof(self) weakSelf = self;
        [videoDevice setFocusModeLockedWithLensPosition:AVCaptureLensPositionCurrent completionHandler:^(CMTime syncTime) {
            __strong typeof(self) strongSelf = weakSelf;
            // TODO: Convert time from device time to session time, as per wwdc 2014 508
            //
            // We don't currently pass in the session, so masterClock is not accessible
            // The device clock is tied to the input port not the device, apparently at AVCaptureInputPort.clock
            // something like [device.ports objectAtIndex:0] clock] which is also part of the session
            // CMTime converted = CMSyncConvertTime(syncTime, <#CMClockOrTimebaseRef fromClockOrTimebase#>, session.masterClock);
            uint64_t time_us = (uint64_t)(syncTime.value / (syncTime.timescale / 1000000.));
            focus_callback(time_us, strongSelf->videoDevice.lensPosition);
            strongSelf->pendingOperation = RCCameraManagerOperationNone;
        }];
        [videoDevice unlockForConfiguration];
    }
}

- (void) lockFocusToPosition:(float)position withCallback:(std::function<void (uint64_t, float)>)focus_callback
{
    DLog(@"Focus lock position %f requested", position);
    if(hasLensPosition)
        [self lockLensPosition:position withCallback:focus_callback];
    else
        // iOS 7 only
        [self lockFocusWithCallback:focus_callback];
}

- (void) lockFocusCurrentWithCallback:(std::function<void (uint64_t, float)>)focus_callback
{
    DLog(@"Focus lock current requested, hasLensPosition %d", hasLensPosition);
    if(hasLensPosition)
        [self lockLensPosition:AVCaptureLensPositionCurrent withCallback:focus_callback];
    else
        // iOS 7 only
        [self lockFocusWithCallback:focus_callback];
}

- (void) unlockFocus
{
    DLog(@"Unlock focus");
    callback = nil;
    pendingOperation = RCCameraManagerOperationNone;
    if (isFocusCapable && [videoDevice lockForConfiguration:nil]) {
        if([videoDevice isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus]) {
            [videoDevice setFocusMode:AVCaptureFocusModeContinuousAutoFocus];
        }
        [videoDevice unlockForConfiguration];
    }
}
@end
