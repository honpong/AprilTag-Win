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
}
@end

@implementation RCVideoCapManagerImpl

- (id)init
{
    if(self = [super init])
    {
        NSLog(@"Init video capture");
    }
    
    return self;
}

- (void)setupVideoCapWithSession:(AVCaptureSession*)session withCorvisManager:(id<RCCorvisManager>)corvisManager
{
    if (!_avDataOutput)
    {
        NSLog(@"Setting up video capture");
        
        _session = session;
        _corvisManager = corvisManager;
        
        _avDataOutput = [[AVCaptureVideoDataOutput alloc] init];
        [_avDataOutput setAlwaysDiscardsLateVideoFrames:NO];
        [_avDataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:'420f'] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
        
        //causes lag
        [_session addOutput:_avDataOutput];
        
        isCapturing = NO;
    }
}

/** @returns True if successfully started. False if setupVideoCapWithSession was not called first, 
 or av session not running. 
 */
- (bool)startVideoCap
{
	NSLog(@"Starting video capture");
    
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
        dispatch_queue_t queue = dispatch_queue_create("MyQueue", NULL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
        [_avDataOutput setSampleBufferDelegate:self queue:queue];
        dispatch_release(queue);
        
        isCapturing = YES;
    }
    
    return true;
}

- (void)stopVideoCap
{
    if (isCapturing)
    {
        NSLog(@"Stopping video capture");
        
        [_avDataOutput setSampleBufferDelegate:nil queue:NULL];
        isCapturing = NO; //turns off processing of frames
    }
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
        
        NSLog(@"%f video frame received", (double)timestamp.value / (double)timestamp.timescale);
        
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
}

@end

@implementation RCVideoCapManagerFactory

static id<RCVideoCapManager> instance;

+ (id<RCVideoCapManager>)getVideoCapManagerInstance
{
    if (instance == nil)
    {
        instance = [[RCVideoCapManagerImpl alloc] init];
    }
    
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void)setVideoCapManagerInstance:(id<RCVideoCapManager>)mockObject
{
    instance = mockObject;
}
@end
