//
//  TMVideoCapManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMVideoCapManagerFactory.h"

@interface TMVideoCapManagerImpl : NSObject <TMVideoCapManager, AVCaptureVideoDataOutputSampleBufferDelegate>
{
    AVCaptureVideoDataOutput *_avDataOutput;
    bool isCapturing;
}
@end

@implementation TMVideoCapManagerImpl

- (id)init
{
    if(self = [super init])
    {
        NSLog(@"Init video capture");
        
        if(![SESSION_MANAGER isRunning])
        {
            NSLog(@"Failed to init video capture. Session not running.");
            return self;
        }
        
        _avDataOutput = [[AVCaptureVideoDataOutput alloc] init];
        [_avDataOutput setAlwaysDiscardsLateVideoFrames:NO];
        [_avDataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:'420f'] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
         
        [SESSION_MANAGER addOutput:_avDataOutput];
        
        isCapturing = YES;
    }
    
    return self;
}

//- (void)dealloc
//{
//    [SESSION_MANAGER.session removeOutput:_avDataOutput];
//}
//
//- (void)handlePause
//{
//    [self stopVideoCap];
//}
//
//- (void)handleTerminate
//{
//    [self stopVideoCap];
//}

- (void)startVideoCap
{
	NSLog(@"Starting video capture");
    
    dispatch_queue_t queue = dispatch_queue_create("MyQueue", NULL); //docs "You use the queue to modify the priority given to delivering and processing the video frames."
    [_avDataOutput setSampleBufferDelegate:self queue:queue];
    dispatch_release(queue);
    
    isCapturing = YES;
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
	if(isCapturing)
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
        
        [CORVIS_MANAGER receiveVideoFrame:pixel withWidth:width withHeight:height withTimestamp:timestamp];
    }
}

@end

@implementation TMVideoCapManagerFactory

static id<TMVideoCapManager> instance;

/** A convenience method for simply instatiating the object, since it's somewhat expensive. */
+ (void)setupVideoCapManager
{
    if (instance == nil)
    {
        instance = [[TMVideoCapManagerImpl alloc] init];
    }
}

+ (id<TMVideoCapManager>)getVideoCapManagerInstance
{
    if (instance == nil)
    {
        instance = [[TMVideoCapManagerImpl alloc] init];
    }
    
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void)setVideoCapManagerInstance:(id<TMVideoCapManager>)mockObject
{
    instance = mockObject;
}
@end
