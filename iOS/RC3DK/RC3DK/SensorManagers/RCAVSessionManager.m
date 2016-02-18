//
//  RCAVSessionManager.m
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCAVSessionManager.h"
#import "RCDebugLog.h"

@implementation RCAVSessionManager
{
    bool hasInput;
}

@synthesize session, videoDevice;

+ (id) sharedInstance
{
    static RCAVSessionManager *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [RCAVSessionManager new];
    });
    return instance;
}

+(void)requestCameraAccessWithCompletion:(void (^)(BOOL))handler
{
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:handler];
}

- (void) configureCameraWithFrameRate:(int)rate withWidth:(int)width withHeight:(int)height
{
    CMTime minFrameDuration = CMTimeMake(1, rate);
    AVCaptureDeviceFormat *bestFormat = nil;
    AVFrameRateRange *bestFrameRateRange = nil;

    NSMutableArray *possibleFormats = [[NSMutableArray alloc] init];
    AVCaptureDeviceFormat *vga = nil;

    //Identify VGA mode and get resolution / 420f matches
    for ( AVCaptureDeviceFormat *format in [videoDevice formats] ) {
        CMVideoDimensions sz = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
        FourCharCode fcc = CMVideoFormatDescriptionGetCodecType(format.formatDescription);
        if(sz.width == width && sz.height == height && fcc == '420f') {
            //Find the highest FOV vga format
            if(vga == nil)
                vga = format;

            if(format.videoFieldOfView > vga.videoFieldOfView)
                vga = format;

            [possibleFormats addObject:format];
        }
    }
    
    NSMutableArray *fovFormats = [[NSMutableArray alloc] init];
    for ( AVCaptureDeviceFormat *format in possibleFormats ) {
        if(format.videoFieldOfView == vga.videoFieldOfView) [fovFormats addObject:format];
    }
    possibleFormats = fovFormats;
    
    NSMutableArray *binnedFormats = [[NSMutableArray alloc] init];
    for ( AVCaptureDeviceFormat *format in possibleFormats ) {
        if(!format.isVideoBinned)
            continue;
        [binnedFormats addObject:format];
    }
    if(binnedFormats.count > 0) possibleFormats = binnedFormats;
    
    NSMutableArray *rateFormats = [[NSMutableArray alloc] init];
    for ( AVCaptureDeviceFormat *format in possibleFormats ) {
        for ( AVFrameRateRange *range in format.videoSupportedFrameRateRanges ) {
            if(range.maxFrameRate >= rate && range.minFrameRate <= rate)
            {
                [rateFormats addObject:format];
                break;
            }
        }
    }
    if(rateFormats.count > 0) possibleFormats = rateFormats;

    for ( AVCaptureDeviceFormat *format in possibleFormats ) {
        for ( AVFrameRateRange *range in format.videoSupportedFrameRateRanges ) {
            if ( range.maxFrameRate > bestFrameRateRange.maxFrameRate ) {
                bestFormat = format;
                bestFrameRateRange = range;
            }
        }
    }
    
    if ( bestFormat ) {
        DLog(@"Best format found:\n%@", bestFormat.description);
        if ( [videoDevice lockForConfiguration:NULL] == YES ) {
            // If our desired rate is faster than the fastest rate we can set then
            // use the fastest one we can set instead
            if(CMTimeCompare(minFrameDuration, bestFrameRateRange.minFrameDuration) < 0)
                minFrameDuration = bestFrameRateRange.minFrameDuration;
            videoDevice.activeFormat = bestFormat;
            videoDevice.activeVideoMinFrameDuration = minFrameDuration;
            videoDevice.activeVideoMaxFrameDuration = minFrameDuration;
            [videoDevice unlockForConfiguration];
        }
    }
    else
    {
        DLog(@"Format not found, defaulting to 640x480 preset.");
        [session setSessionPreset:AVCaptureSessionPreset640x480];
    }
}

- (id) init
{
    self = [super init];
    
    if (self)
    {
        LOGME
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handlePause)
                                                     name:UIApplicationWillResignActiveNotification
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleTerminate)
                                                     name:UIApplicationWillTerminateNotification
                                                   object:nil];
        
        session = [[AVCaptureSession alloc] init];
        
        [session setSessionPreset:AVCaptureSessionPresetInputPriority]; // 640x480 required
        
        hasInput = false;
    }
    
    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self endSession];
}

- (void) addInputToSessionWithFrameRate:(int)frameRate withWidth:(int)width withHeight:(int)height
{
    videoDevice = [self cameraWithPosition:AVCaptureDevicePositionBack];

    if (videoDevice)
    {
        NSError *error;
        AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:&error];
        if (error)
        {
            DLog(@"Error getting AVCaptureDeviceInput object: %@", error);
            return;
        }
        
        [session addInput:input];
        hasInput = true;
    }
    else
    {
        DLog(@"Couldn't get video device");
        return;
    }

    if ([videoDevice lockForConfiguration:nil]) {
        if([videoDevice isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus])
            [videoDevice setFocusMode:AVCaptureFocusModeContinuousAutoFocus];
        if([videoDevice isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure])
            [videoDevice setExposureMode:AVCaptureExposureModeContinuousAutoExposure];
        if([videoDevice isWhiteBalanceModeSupported:AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance])
            [videoDevice setWhiteBalanceMode:AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance];
        [videoDevice unlockForConfiguration];
        [self configureCameraWithFrameRate:frameRate withWidth:width withHeight:height];
    } else {
        DLog(@"error while configuring camera");
        return;
    }
}

- (BOOL) startSessionWithFrameRate:(int)frameRate withWidth:(int)width withHeight:(int)height
{
    LOGME
    
    if (!session)
    {
        DLog(@"Session is nil");
        return false;
    }
    
    if(!hasInput) [self addInputToSessionWithFrameRate:frameRate withWidth:width withHeight:height];
    
    if (![session isRunning]) [session startRunning];
    
    return true;
}

- (BOOL) startSession
{
    return [self startSessionWithFrameRate:30 withWidth:640 withHeight:480];
}

- (void) endSession
{
    LOGME
    if ([session isRunning]) [session stopRunning];
}

- (BOOL) isRunning
{
    return session ? session.isRunning : false;
}

- (BOOL) addOutput:(AVCaptureOutput*)output
{
    if (!session)
    {
        DLog(@"Session is nil");
        return false;
    }
    
    if ([session canAddOutput:output])
    {
        [session addOutput:output];
    }
    else
    {
        DLog(@"Can't add output to session");
        return false;
    }
    
    return true;
}

- (void) removeOutput:(AVCaptureOutput *)output
{
    if (session)
        [session removeOutput:output];
}

- (bool) isImageClean
{
    if(videoDevice.adjustingFocus) {
        //DLog(@"Adjusting focus");
        return false;
    }
    /*if(videoDevice.adjustingWhiteBalance) {
        DLog(@"Adjusting white balance");
        return false;
    }
    if(videoDevice.adjustingExposure) {
        DLog(@"Adjusting exposure");
        return false;
    }*/
    return true;
}

- (void) handlePause
{
    LOGME
    [self endSession];    
}

- (void) handleTerminate
{
    LOGME
    [self endSession];
}

- (AVCaptureDevice *)cameraWithPosition:(AVCaptureDevicePosition) position
{
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    
    for (AVCaptureDevice *device in devices)
    {
        if ([device position] == position) return device;
    }
    
    return nil;
}

@end
