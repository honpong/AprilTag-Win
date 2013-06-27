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

static RCVideoManager * instance = nil;

+ (void)setupVideoCapWithSession:(AVCaptureSession*)session
{
    instance = [[RCVideoManager alloc] initWithSession:session];
}

+ (void) setupVideoCapWithSession:(AVCaptureSession *)session withOutput:(AVCaptureVideoDataOutput *)output
{
    instance = [[RCVideoManager alloc] initWithSession:session withOutput:output];
}

/** @returns Returns nil if setupVideoCapWithSession hasn't been called yet. Otherwise returns single instance. */
+ (RCVideoManager *) sharedInstance
{
    return instance;
}

- (id)initWithSession:(AVCaptureSession*)session_
{
    LOGME
    return [self initWithSession:session_ withOutput:[[AVCaptureVideoDataOutput alloc] init]];
}

- (id) initWithSession:(AVCaptureSession *)session_ withOutput:(AVCaptureVideoDataOutput *)output_
{
    if(self = [super init])
    {
        session = session_;
        output = output_;
        sensorFusion = [RCSensorFusion sharedInstance];
        
        [output setAlwaysDiscardsLateVideoFrames:YES];
        [output setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:'420f'] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
        
        //causes lag
        [self.session addOutput:self.output];
        
        AVCaptureConnection *videoConnection = [self.output connectionWithMediaType:AVMediaTypeVideo];
        videoOrientation = [videoConnection videoOrientation];
                
        isCapturing = NO;

        dispatch_queue_t queue = dispatch_queue_create("MyQueue", DISPATCH_QUEUE_SERIAL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
        [output setSampleBufferDelegate:self queue:queue];
        dispatch_release(queue);
        
        // Create a shallow queue for buffers going to the display for preview.
        OSStatus err = CMBufferQueueCreate(kCFAllocatorDefault, 1, CMBufferQueueGetCallbacksForUnsortedSampleBuffers(), &previewBufferQueue);
        if (err) NSLog(@"ERROR creating CMBufferQueue");
    }
    
    return self;
}

- (void)dealloc {
//    [output setSampleBufferDelegate:nil queue:NULL];
}

/** @returns True if successfully started. False if setupVideoCapWithSession was not called first,
 or av session not running. 
 */
- (bool) startVideoCapture
{
	LOGME
    
    if(![session isRunning])
    {
        NSLog(@"Failed to start video capture. AV capture session not running.");
        return false;
    }
    
    if(![sensorFusion isSensorFusionRunning])
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
	if(isCapturing) //TODO: a better way to determine if plugins are started
    {
        [sensorFusion receiveVideoFrame:sampleBuffer];
    }
    
    // Enqueue it for preview.  This is a shallow queue, so if image processing is taking too long,
    // we'll drop this frame for preview (this keeps preview latency low).
    if(delegate)
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
