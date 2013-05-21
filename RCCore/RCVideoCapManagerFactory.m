//
//  TMVideoCapManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCVideoCapManagerFactory.h"

@interface RCVideoCapManagerImpl : NSObject <RCVideoCapManager, AVCaptureVideoDataOutputSampleBufferDelegate>
{
    AVCaptureSession *_session;
    id<RCCorvisManager> _corvisManager;
    AVCaptureVideoDataOutput *_avDataOutput;
    bool isCapturing;
    CMBufferQueueRef previewBufferQueue;
}

@end

@implementation RCVideoCapManagerImpl
@synthesize delegate, videoOrientation;

- (id)initWithSession:(AVCaptureSession*)session
{
    return [self initWithSession:session withOutput:[[AVCaptureVideoDataOutput alloc] init] withCorvisManager:[RCCorvisManagerFactory getCorvisManagerInstance]];
}

- (id)initWithSession:(AVCaptureSession*)session withOutput:(AVCaptureVideoDataOutput*)output withCorvisManager:(id<RCCorvisManager>)corvisManager
{
    if(self = [super init])
    {
        LOGME
        
        _session = session;
        _corvisManager = corvisManager;
        _avDataOutput = output;
        
        [_avDataOutput setAlwaysDiscardsLateVideoFrames:NO];
//        [_avDataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:'420f'] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
        [_avDataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCVPixelFormatType_32BGRA] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
        
        //causes lag
        [_session addOutput:_avDataOutput];
        
        AVCaptureConnection *videoConnection = [_avDataOutput connectionWithMediaType:AVMediaTypeVideo];
        videoOrientation = [videoConnection videoOrientation];
                
        isCapturing = NO;
    }
    
    return self;
}

/** @returns True if successfully started. False if setupVideoCapWithSession was not called first, 
 or av session not running. 
 */
- (bool)startVideoCap
{
	LOGME
    
    if(!_avDataOutput)
    {
        NSLog(@"Failed to start video capture. Video capture not setup yet.");
        return false;
    }
    
    if(![_session isRunning])
    {
        NSLog(@"Failed to start video capture. AV capture session not running.");
        return false;
    }
    
    if(![_corvisManager isPluginsStarted])
    {
        NSLog(@"Failed to start video capture. Corvis plugins not started yet.");
        return false;
    }
    
    if (!isCapturing)
    {
        dispatch_queue_t queue = dispatch_queue_create("MyQueue", DISPATCH_QUEUE_SERIAL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
        [_avDataOutput setSampleBufferDelegate:self queue:queue];
        dispatch_release(queue);
        
        // Create a shallow queue for buffers going to the display for preview.
        OSStatus err = CMBufferQueueCreate(kCFAllocatorDefault, 1, CMBufferQueueGetCallbacksForUnsortedSampleBuffers(), &previewBufferQueue);
        if (err) NSLog(@"ERROR creating CMBufferQueue");
        
        isCapturing = YES;
    }
    
    return true;
}

- (void)stopVideoCap
{
    LOGME
    
    if (isCapturing)
    {
        [_avDataOutput setSampleBufferDelegate:nil queue:NULL];
        isCapturing = NO; //turns off processing of frames
    }
    
    if (previewBufferQueue) {
		CFRelease(previewBufferQueue);
		previewBufferQueue = NULL;
	}
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
        if(!CMSampleBufferDataIsReady(sampleBuffer) )
        {
            NSLog( @"sample buffer is not ready. Skipping sample" );
            return;
        }
        
        CMTime timestamp = (CMTime)CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
        
        //capture image meta data
        //        CFDictionaryRef metadataDict = CMGetAttachment(sampleBuffer, kCGImagePropertyExifDictionary , NULL);
        //        NSLog(@"metadata: %@", metadataDict);
        
        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
                
        uint32_t width = CVPixelBufferGetWidth(pixelBuffer);
        uint32_t height = CVPixelBufferGetHeight(pixelBuffer);
        
        CVPixelBufferLockBaseAddress(pixelBuffer, 0);
        unsigned char *pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
        
        [_corvisManager receiveVideoFrame:pixel withWidth:width withHeight:height withTimestamp:timestamp];
    }
    
    // Enqueue it for preview.  This is a shallow queue, so if image processing is taking too long,
    // we'll drop this frame for preview (this keeps preview latency low).
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

@end

@implementation RCVideoCapManagerFactory

static id<RCVideoCapManager> instance;

+ (void)setupVideoCapWithSession:(AVCaptureSession*)session
{
    if (!instance)
    {
        instance = [[RCVideoCapManagerImpl alloc] initWithSession:session];
    }
}

+ (void)setupVideoCapWithSession:(AVCaptureSession*)session withOutput:(AVCaptureVideoDataOutput*)output withCorvisManager:(id<RCCorvisManager>)corvisManager
{
    if (!instance)
    {
        instance = [[RCVideoCapManagerImpl alloc] initWithSession:session withOutput:output withCorvisManager:corvisManager];
    }
}

/** @returns Returns nil if setupVideoCapWithSession hasn't been called yet. Otherwise returns single instance. */
+ (id<RCVideoCapManager>)getVideoCapManagerInstance
{
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void)setVideoCapManagerInstance:(id<RCVideoCapManager>)mockObject
{
    instance = mockObject;
}
@end
