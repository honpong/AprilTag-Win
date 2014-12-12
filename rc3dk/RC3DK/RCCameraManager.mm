//
//  RCCameraManager.mm
//  RC3DK
//
//  Created by Brian on 1/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

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
    [(__bridge RCCameraManager *)platform_ptr lockFocusWithCallback:callback];
}

void camera_control_interface::focus_lock_at_position(float position, std::function<void (uint64_t)> callback)
{
    //TODO: implement
}

void camera_control_interface::focus_once_and_lock(std::function<void (uint64_t, float)> callback)
{
    [(__bridge RCCameraManager *)platform_ptr focusOnceAndLockWithCallback:callback];
}

void camera_control_interface::focus_unlock()
{
    //TODO: implement
}

typedef NS_ENUM(int, RCCameraManagerOperationType) {
    RCCameraManagerOperationNone = 0, // will never be passed to a delegate
    RCCameraManagerOperationFocusOnce,
    RCCameraManagerOperationFocusLock,
};

@implementation RCCameraManager
{
    AVCaptureDevice * videoDevice;
    BOOL isFocusCapable;
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

- (void)performCallbackWithTime:(uint64_t)timestamp withFocalLength:(float)focal_length
{
    if(callback)
        callback(timestamp, focal_length);
    callback = nil;
    pendingOperation = RCCameraManagerOperationNone;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
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
            [self performCallbackWithTime:0 withFocalLength:0];
        }
    }
}


- (void)focusTimeout:(NSTimer*)theTimer
{
    if (!isFocusing && pendingOperation) {
        // For some reason, even though we've requested it, the focus event isn't happening, give up and continue
        pendingOperation = RCCameraManagerOperationNone;
        DLog(@"Focus timed out, continuing");
        [self performCallbackWithTime:0 withFocalLength:0];
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
        [self performCallbackWithTime:0 withFocalLength:0];
    }
    else if(operation == RCCameraManagerOperationFocusLock &&
            videoDevice.focusMode == AVCaptureFocusModeLocked && !videoDevice.adjustingFocus) {
        // Focus is already locked and we requested a lock
        DLog(@"INFO: Focus is already locked, starting");
        [self performCallbackWithTime:0 withFocalLength:0];
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

@end
