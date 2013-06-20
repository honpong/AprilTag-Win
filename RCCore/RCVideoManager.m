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
    RCSensorFusion* _sensorFusion;
    bool isCapturing;
    CMBufferQueueRef previewBufferQueue;
}
@synthesize delegate, videoOrientation, session, output;

static RCVideoManager * instance = nil;

+ (void)setupVideoCapWithSession:(AVCaptureSession*)session
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] initWithSession:session];
    });
}

+ (void) setupVideoCapWithSession:(AVCaptureSession *)session withOutput:(AVCaptureVideoDataOutput *)output withSensorFusion:(RCSensorFusion *)sensorFusion
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] initWithSession:session withOutput:output withSensorFusion:sensorFusion];
    });
}

/** @returns Returns nil if setupVideoCapWithSession hasn't been called yet. Otherwise returns single instance. */
+ (RCVideoManager *) sharedInstance
{
    return instance;
}

- (id)initWithSession:(AVCaptureSession*)session
{
    LOGME
    return [self initWithSession:session withOutput:[[AVCaptureVideoDataOutput alloc] init] withSensorFusion:[RCSensorFusion sharedInstance]];
}

- (id) initWithSession:(AVCaptureSession *)session withOutput:(AVCaptureVideoDataOutput *)output withSensorFusion:(RCSensorFusion*)sensorFusion
{
    if(self = [super init])
    {
        self.session = session;
        _sensorFusion = sensorFusion;
        self.output = output;
        
        [output setAlwaysDiscardsLateVideoFrames:YES];
        [output setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:'420f'] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
        
        //causes lag
        [self.session addOutput:self.output];
        
        AVCaptureConnection *videoConnection = [self.output connectionWithMediaType:AVMediaTypeVideo];
        videoOrientation = [videoConnection videoOrientation];
                
        isCapturing = NO;

        dispatch_queue_t queue = dispatch_queue_create("MyQueue", DISPATCH_QUEUE_SERIAL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
        [self.output setSampleBufferDelegate:self queue:queue];
        dispatch_release(queue);
        
        // Create a shallow queue for buffers going to the display for preview.
        OSStatus err = CMBufferQueueCreate(kCFAllocatorDefault, 1, CMBufferQueueGetCallbacksForUnsortedSampleBuffers(), &previewBufferQueue);
        if (err) NSLog(@"ERROR creating CMBufferQueue");
    }
    
    return self;
}

- (void)dealloc {
    [self.output setSampleBufferDelegate:nil queue:NULL];
}

/** @returns True if successfully started. False if setupVideoCapWithSession was not called first,
 or av session not running. 
 */
- (bool) startVideoCapture
{
	LOGME
    
    if(![self.session isRunning])
    {
        NSLog(@"Failed to start video capture. AV capture session not running.");
        return false;
    }
    
    if(![_sensorFusion isPluginsStarted])
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
        [_sensorFusion receiveVideoFrame:sampleBuffer];
    }
    
    // Enqueue it for preview.  This is a shallow queue, so if image processing is taking too long,
    // we'll drop this frame for preview (this keeps preview latency low).
    if(self.delegate) {
        OSStatus err = CMBufferQueueEnqueue(previewBufferQueue, sampleBuffer);
        if ( !err ) {
            dispatch_async(dispatch_get_main_queue(), ^{
                CMSampleBufferRef sbuf = (CMSampleBufferRef)CMBufferQueueDequeueAndRetain(previewBufferQueue);
                if (sbuf) {
                    CVImageBufferRef pixBuf = CMSampleBufferGetImageBuffer(sbuf); //TODO: redunant with code below
                    [self.delegate pixelBufferReadyForDisplay:pixBuf];
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
