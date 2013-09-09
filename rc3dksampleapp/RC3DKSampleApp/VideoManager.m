//
//  VideoManager.m
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "VideoManager.h"

@implementation VideoManager
{
    RCSensorFusion* sensorFusion;
    bool isCapturing;
    CMBufferQueueRef previewBufferQueue;
}
@synthesize delegate, videoOrientation, session, output;

+ (VideoManager *) sharedInstance
{
    static VideoManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [VideoManager new];
    });
    return instance;
}

/** Invocations after the first have no effect */
- (void) setupWithSession:(AVCaptureSession*)avSession
{
    LOGME
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
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

    dispatch_queue_t queue = dispatch_queue_create("MyQueue", DISPATCH_QUEUE_SERIAL);
    [output setSampleBufferDelegate:self queue:queue];
    
    // Create a shallow queue for buffers going to the display for preview.
    OSStatus err = CMBufferQueueCreate(kCFAllocatorDefault, 1, CMBufferQueueGetCallbacksForUnsortedSampleBuffers(), &previewBufferQueue);
    if (err) NSLog(@"ERROR creating CMBufferQueue");
}

/** @returns True if successfully started. False if setupWithSession: was not called first, or AV session not running. */
- (bool) startVideoCapture
{
	LOGME
    
    if(session == nil || ![session isRunning])
    {
        NSLog(@"Failed to start video capture. AV capture session not running.");
        return false;
    }
    
    if(sensorFusion == nil || ![sensorFusion isSensorFusionRunning])
    {
        NSLog(@"Failed to start video capture. Plugins not started yet.");
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
	if (isCapturing) 
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
                    CVImageBufferRef pixBuf = CMSampleBufferGetImageBuffer(sbuf); //TODO: redunant with code below
                    [delegate pixelBufferReadyForDisplay:pixBuf];
                    CFRelease(sbuf);
                }
            });
        }
        else
        {
            NSLog(@"ERROR dispatching video frame to delegate for preview");
        }
    }
}

@end
