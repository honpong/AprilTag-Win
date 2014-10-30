//
//  RCVideoManager.m
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCVideoManager.h"
#import "RCDebugLog.h"

@implementation RCVideoManager
{
    RCSensorFusion* sensorFusion;
    bool isCapturing;
}
@synthesize delegate, videoOrientation, session, output;

+ (RCVideoManager *) sharedInstance
{
    static RCVideoManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [RCVideoManager new];
    });
    return instance;
}

/** Invocations after the first have no effect */
- (void) setupWithSession:(AVCaptureSession*)avSession
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        LOGME
        
        AVCaptureVideoDataOutput* avOutput = [[AVCaptureVideoDataOutput alloc] init];
        [output setAlwaysDiscardsLateVideoFrames:YES];
        [output setVideoSettings:@{(id)kCVPixelBufferPixelFormatTypeKey: [NSNumber numberWithInt:'420f']}];
        
        [self setupWithSession:avSession withOutput:avOutput];
    });
}

// not externally visible in release build. here for testing purposes.
- (void) setupWithSession:(AVCaptureSession *)avSession withOutput:(AVCaptureVideoDataOutput *)avOutput
{
    session = avSession;
    output = avOutput;
    sensorFusion = [RCSensorFusion sharedInstance];

    //causes lag
    [self.session addOutput:self.output];
    
    AVCaptureConnection *videoConnection = [self.output connectionWithMediaType:AVMediaTypeVideo];
    videoOrientation = [videoConnection videoOrientation];
            
    isCapturing = NO;

    dispatch_queue_t queue = dispatch_queue_create("MyQueue", DISPATCH_QUEUE_SERIAL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
    [output setSampleBufferDelegate:self queue:queue];
//    dispatch_release(queue); // illegal on iOS 6+
}

/** @returns True if successfully started. False if setupWithSession: was not called first, or AV session not running. */
- (bool) startVideoCapture
{
	LOGME
    
    if(session == nil || ![session isRunning])
    {
        DLog(@"Failed to start video capture. AV capture session not running.");
        return false;
    }
    
    isCapturing = YES;
    
    return true;
}

- (void) stopVideoCapture
{
    LOGME
    isCapturing = NO;
}

- (BOOL)isCapturing
{
    return isCapturing;
}

//called on each video frame
-(void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    
    // send video frames to the 3DK sensor fusion engine
    if(sensorFusion != nil && isCapturing)
    {
        [sensorFusion receiveVideoFrame:sampleBuffer];
    }
    
    // send video frames to the video preview view that is set as this object's delegate
    if (delegate && [delegate respondsToSelector:@selector(displaySampleBuffer:)])
    {
        [delegate displaySampleBuffer:sampleBuffer];
    }
    
    CFRelease(sampleBuffer);
}

@end
