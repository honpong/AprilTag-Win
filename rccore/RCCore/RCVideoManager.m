//
//  TMVideoCapManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCVideoManager.h"

@implementation RCVideoManager
{
    RCSensorFusion* sensorFusion;
    bool isCapturing;
    CMBufferQueueRef previewBufferQueue;
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
        [output setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:'420f'] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
        
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
    
    // Create a shallow queue for buffers going to the display for preview.
    OSStatus err = CMBufferQueueCreate(kCFAllocatorDefault, 1, CMBufferQueueGetCallbacksForUnsortedSampleBuffers(), &previewBufferQueue);
    if (err) DLog(@"ERROR creating CMBufferQueue");
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
    
    if(sensorFusion == nil || ![sensorFusion isSensorFusionRunning])
    {
        DLog(@"Failed to start video capture. Plugins not started yet.");
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
	if (isCapturing) //TODO: a better way to determine if plugins are started
    {
        [sensorFusion receiveVideoFrame:sampleBuffer];
    }
    
    // Enqueue it for preview.  This is a shallow queue, so if image processing is taking too long,
    // we'll drop this frame for preview (this keeps preview latency low).
    if (delegate)
    {
        OSStatus err = CMBufferQueueEnqueue(previewBufferQueue, sampleBuffer);
        if ( !err )
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                CMSampleBufferRef sbuf = (CMSampleBufferRef)CMBufferQueueDequeueAndRetain(previewBufferQueue);
                if (sbuf) {
                    CVImageBufferRef pixBuf = CMSampleBufferGetImageBuffer(sbuf);
                    [delegate pixelBufferReadyForDisplay:pixBuf];
                    CFRelease(sbuf);
                }
            });
        }
        else
        {
            DLog(@"ERROR dispatching video frame to delegate for preview");
        }
    }
}

@end
